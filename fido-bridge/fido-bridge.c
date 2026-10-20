/*
 * fido-bridge: expose a USB FIDO2/U2F security key to Termux programs.
 *
 * Usage (from Termux):
 *   termux-usb -r -e fido-bridge /dev/bus/usb/001/002
 *
 * termux-usb asks Android for permission, opens the device and runs this
 * program with the opened file descriptor (as the last argument and, on newer
 * termux-api versions, in $TERMUX_USB_FD).
 *
 * The bridge then:
 *   1. wraps that fd with libusb_wrap_sys_device(),
 *   2. finds the HID interface whose report descriptor declares usage page
 *      0xF1D0 (FIDO Alliance), detaches the kernel HID driver and claims it,
 *   3. locates the interface's interrupt IN and OUT endpoints,
 *   4. listens on $PREFIX/tmp/fido.sock (SOCK_STREAM) and shuttles raw
 *      64-byte HID reports between connected clients and the key.
 *
 * Socket protocol: every message in either direction is exactly 64 bytes, a
 * raw CTAPHID packet with no report-ID prefix. Like hidraw, every report the
 * key sends is delivered to every connected client; CTAPHID clients already
 * filter by channel ID (and by nonce for INIT replies).
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <libusb.h>

#define REPORT_LEN       64
#define FIDO_USAGE_PAGE  0xF1D0
#define MAX_CLIENTS      16
#define USB_IN_TIMEOUT   250   /* ms; lets the reader thread notice shutdown */
#define USB_OUT_TIMEOUT  2000  /* ms */
#define HID_DT_HID       0x21
#define HID_DT_REPORT    0x22
#define DEFAULT_PREFIX   "/data/data/com.termux/files/usr"

struct client {
	int fd;                    /* -1 when the slot is free */
	uint8_t buf[REPORT_LEN];   /* partial report being assembled */
	size_t fill;
};

static volatile sig_atomic_t g_stop;
static int g_verbose;
static int g_wake[2] = { -1, -1 };     /* reader thread -> main loop */

static libusb_context *g_ctx;
static libusb_device_handle *g_dev;
static int g_ifnum = -1;
static int g_driver_detached;
static uint8_t g_ep_in, g_ep_out;

static pthread_mutex_t g_clients_lock = PTHREAD_MUTEX_INITIALIZER;
static struct client g_clients[MAX_CLIENTS];

static void logmsg(const char *fmt, ...)
{
	va_list ap;
	fputs("fido-bridge: ", stderr);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

static void on_signal(int sig)
{
	(void)sig;
	g_stop = 1;
}

static void wake_main(void)
{
	if (g_wake[1] >= 0) {
		ssize_t r = write(g_wake[1], "x", 1);
		(void)r;
	}
}

static const char *ctaphid_cmd_name(uint8_t cmd)
{
	switch (cmd & 0x7f) {
	case 0x01: return "PING";
	case 0x03: return "MSG";
	case 0x04: return "LOCK";
	case 0x06: return "INIT";
	case 0x08: return "WINK";
	case 0x10: return "CBOR";
	case 0x11: return "CANCEL";
	case 0x3b: return "KEEPALIVE";
	case 0x3f: return "ERROR";
	default:   return "?";
	}
}

/* With -v, print a one-line summary of each CTAPHID packet. */
static void trace(const char *dir, const uint8_t *p)
{
	uint32_t cid;

	if (!g_verbose)
		return;
	cid = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
	if (p[4] & 0x80)
		fprintf(stderr, "fido-bridge: %s cid=%08x %s len=%u\n", dir, cid,
		        ctaphid_cmd_name(p[4]), (unsigned)(p[5] << 8 | p[6]));
	else
		fprintf(stderr, "fido-bridge: %s cid=%08x cont seq=%u\n", dir, cid, p[4]);
}

/* ---------------------------------------------------------------- USB --- */

/* Walk the short items of a HID report descriptor looking for a global
 * Usage Page item equal to 0xF1D0. */
static int report_desc_is_fido(const uint8_t *d, int len)
{
	int i = 0;

	while (i < len) {
		uint8_t prefix = d[i];
		int size;
		uint32_t val = 0;

		if (prefix == 0xFE) {          /* long item: FE, size, tag, data */
			if (i + 1 >= len)
				break;
			i += 3 + d[i + 1];
			continue;
		}
		size = prefix & 0x03;
		if (size == 3)
			size = 4;
		if (i + 1 + size > len)
			break;
		for (int k = 0; k < size; k++)
			val |= (uint32_t)d[i + 1 + k] << (8 * k);
		if ((prefix & 0xFC) == 0x04 && val == FIDO_USAGE_PAGE)
			return 1;
		i += 1 + size;
	}
	return 0;
}

/* Length of the report descriptor, taken from the HID class descriptor that
 * sits in the interface descriptor's "extra" bytes. */
static int report_desc_len(const struct libusb_interface_descriptor *alt)
{
	const uint8_t *e = alt->extra;
	int n = alt->extra_length;

	while (n >= 2 && e[0] >= 2 && e[0] <= n) {
		if (e[1] == HID_DT_HID && e[0] >= 9) {
			int count = e[5];
			for (int j = 0; j < count && 6 + 3 * j + 2 < e[0]; j++) {
				const uint8_t *c = e + 6 + 3 * j;
				if (c[0] == HID_DT_REPORT)
					return c[1] | c[2] << 8;
			}
		}
		n -= e[0];
		e += e[0];
	}
	return 0;
}

static void detach_driver(int ifnum)
{
	int r = libusb_kernel_driver_active(g_dev, ifnum);

	if (r == 1) {
		r = libusb_detach_kernel_driver(g_dev, ifnum);
		if (r == 0) {
			g_driver_detached = 1;
			logmsg("detached kernel driver from interface %d", ifnum);
		} else {
			logmsg("could not detach kernel driver from interface %d: %s",
			       ifnum, libusb_strerror(r));
		}
	} else if (r < 0 && r != LIBUSB_ERROR_NOT_SUPPORTED) {
		logmsg("kernel driver query on interface %d: %s", ifnum, libusb_strerror(r));
	}
}

static void release_interface(int ifnum)
{
	libusb_release_interface(g_dev, ifnum);
	if (g_driver_detached) {
		int r = libusb_attach_kernel_driver(g_dev, ifnum);
		if (r < 0 && r != LIBUSB_ERROR_NOT_FOUND && r != LIBUSB_ERROR_NO_DEVICE)
			logmsg("could not reattach kernel driver: %s", libusb_strerror(r));
		g_driver_detached = 0;
	}
}

/* Is this (now claimed) interface the FIDO one? */
static int interface_is_fido(const struct libusb_interface_descriptor *alt)
{
	uint8_t buf[4096];
	int want = report_desc_len(alt);
	int r;

	if (want <= 0 || want > (int)sizeof(buf))
		want = sizeof(buf);
	r = libusb_control_transfer(g_dev,
	        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_INTERFACE,
	        LIBUSB_REQUEST_GET_DESCRIPTOR, HID_DT_REPORT << 8,
	        alt->bInterfaceNumber, buf, (uint16_t)want, 1000);
	if (r < 0) {
		logmsg("reading report descriptor of interface %d: %s",
		       alt->bInterfaceNumber, libusb_strerror(r));
		return 0;
	}
	return report_desc_is_fido(buf, r);
}

static int find_endpoints(const struct libusb_interface_descriptor *alt)
{
	g_ep_in = g_ep_out = 0;
	for (int i = 0; i < alt->bNumEndpoints; i++) {
		const struct libusb_endpoint_descriptor *ep = &alt->endpoint[i];

		if ((ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_INTERRUPT)
			continue;
		if (ep->wMaxPacketSize != REPORT_LEN)
			logmsg("note: endpoint 0x%02x max packet size is %u, expected %d",
			       ep->bEndpointAddress, ep->wMaxPacketSize, REPORT_LEN);
		if ((ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
			if (!g_ep_in)
				g_ep_in = ep->bEndpointAddress;
		} else if (!g_ep_out) {
			g_ep_out = ep->bEndpointAddress;
		}
	}
	return g_ep_in && g_ep_out;
}

/* Try every HID interface: detach, claim, check its usage page; keep the
 * first FIDO one and give the rest back to the kernel. */
static int claim_fido_interface(void)
{
	struct libusb_config_descriptor *cfg;
	int r = libusb_get_active_config_descriptor(libusb_get_device(g_dev), &cfg);

	if (r < 0) {
		logmsg("reading config descriptor: %s", libusb_strerror(r));
		return -1;
	}
	for (int i = 0; i < cfg->bNumInterfaces; i++) {
		const struct libusb_interface_descriptor *alt;

		if (cfg->interface[i].num_altsetting < 1)
			continue;
		alt = &cfg->interface[i].altsetting[0];
		if (alt->bInterfaceClass != LIBUSB_CLASS_HID)
			continue;

		detach_driver(alt->bInterfaceNumber);
		r = libusb_claim_interface(g_dev, alt->bInterfaceNumber);
		if (r < 0) {
			logmsg("claiming interface %d: %s", alt->bInterfaceNumber,
			       libusb_strerror(r));
			release_interface(alt->bInterfaceNumber);
			continue;
		}
		if (interface_is_fido(alt) && find_endpoints(alt)) {
			g_ifnum = alt->bInterfaceNumber;
			logmsg("FIDO interface %d, IN 0x%02x, OUT 0x%02x",
			       g_ifnum, g_ep_in, g_ep_out);
			libusb_free_config_descriptor(cfg);
			return 0;
		}
		release_interface(alt->bInterfaceNumber);
	}
	libusb_free_config_descriptor(cfg);
	logmsg("no HID interface with usage page 0x%04X and interrupt IN/OUT endpoints",
	       FIDO_USAGE_PAGE);
	return -1;
}

/* ------------------------------------------------------------ clients --- */

static void broadcast(const uint8_t *report)
{
	pthread_mutex_lock(&g_clients_lock);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		struct client *c = &g_clients[i];
		ssize_t n;

		if (c->fd < 0)
			continue;
		n = send(c->fd, report, REPORT_LEN, MSG_NOSIGNAL | MSG_DONTWAIT);
		if (n != REPORT_LEN) {
			/* Slow or vanished reader: a torn report would desync the
			 * stream, so hang up and let the main loop reap it. */
			logmsg("client %d not keeping up, disconnecting", c->fd);
			shutdown(c->fd, SHUT_RDWR);
		}
	}
	pthread_mutex_unlock(&g_clients_lock);
}

static void *usb_reader(void *arg)
{
	uint8_t buf[REPORT_LEN];

	(void)arg;
	while (!g_stop) {
		int got = 0;
		int r = libusb_interrupt_transfer(g_dev, g_ep_in, buf, sizeof(buf), &got,
		                                  USB_IN_TIMEOUT);

		if (r == LIBUSB_ERROR_TIMEOUT || r == LIBUSB_ERROR_INTERRUPTED) {
			if (got == 0)
				continue;
		} else if (r == LIBUSB_ERROR_OVERFLOW) {
			logmsg("IN transfer overflow, dropping report");
			continue;
		} else if (r < 0) {
			logmsg("IN transfer failed: %s", libusb_strerror(r));
			g_stop = 1;
			wake_main();
			break;
		}
		if (got <= 0)
			continue;
		if (got < REPORT_LEN)
			memset(buf + got, 0, REPORT_LEN - got);
		trace("key->", buf);
		broadcast(buf);
	}
	return NULL;
}

static void client_close(int i)
{
	pthread_mutex_lock(&g_clients_lock);
	if (g_clients[i].fd >= 0) {
		if (g_verbose)
			logmsg("client %d disconnected", g_clients[i].fd);
		close(g_clients[i].fd);
		g_clients[i].fd = -1;
		g_clients[i].fill = 0;
	}
	pthread_mutex_unlock(&g_clients_lock);
}

static void client_accept(int lfd)
{
	int fd = accept4(lfd, NULL, NULL, SOCK_CLOEXEC);

	if (fd < 0)
		return;
	pthread_mutex_lock(&g_clients_lock);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (g_clients[i].fd < 0) {
			g_clients[i].fd = fd;
			g_clients[i].fill = 0;
			pthread_mutex_unlock(&g_clients_lock);
			if (g_verbose)
				logmsg("client %d connected", fd);
			return;
		}
	}
	pthread_mutex_unlock(&g_clients_lock);
	logmsg("too many clients, refusing connection");
	close(fd);
}

/* Read what the client has sent; forward each completed 64-byte report. */
static void client_read(int i)
{
	struct client *c = &g_clients[i];
	ssize_t n = recv(c->fd, c->buf + c->fill, REPORT_LEN - c->fill, MSG_DONTWAIT);

	if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR)) {
		client_close(i);
		return;
	}
	if (n < 0)
		return;
	c->fill += (size_t)n;
	if (c->fill < REPORT_LEN)
		return;
	c->fill = 0;

	trace("->key", c->buf);
	int sent = 0;
	int r = libusb_interrupt_transfer(g_dev, g_ep_out, c->buf, REPORT_LEN, &sent,
	                                  USB_OUT_TIMEOUT);
	if (r < 0 || sent != REPORT_LEN) {
		logmsg("OUT transfer failed: %s", r < 0 ? libusb_strerror(r) : "short write");
		if (r == LIBUSB_ERROR_NO_DEVICE)
			g_stop = 1;
	}
}

/* ------------------------------------------------------------- socket --- */

static int open_listener(const char *path)
{
	struct sockaddr_un sa = { .sun_family = AF_UNIX };
	int fd;

	if (strlen(path) >= sizeof(sa.sun_path)) {
		logmsg("socket path too long: %s", path);
		return -1;
	}
	strcpy(sa.sun_path, path);

	/* A leftover socket file is fine to remove; a live bridge is not. */
	fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd >= 0 && connect(fd, (struct sockaddr *)&sa, sizeof(sa)) == 0) {
		logmsg("another bridge is already listening on %s", path);
		close(fd);
		return -1;
	}
	if (fd >= 0)
		close(fd);
	unlink(path);

	fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0) {
		logmsg("socket: %s", strerror(errno));
		return -1;
	}
	mode_t old = umask(0077);
	int r = bind(fd, (struct sockaddr *)&sa, sizeof(sa));
	umask(old);
	if (r < 0 || listen(fd, 8) < 0) {
		logmsg("binding %s: %s", path, strerror(errno));
		close(fd);
		return -1;
	}
	chmod(path, 0600);
	return fd;
}

/* --------------------------------------------------------------- main --- */

static void usage(void)
{
	fprintf(stderr,
	        "usage: termux-usb -r -e fido-bridge /dev/bus/usb/BBB/DDD\n"
	        "       fido-bridge [-v] [-s socket] FD\n"
	        "\n"
	        "  -s PATH  listen here instead of $PREFIX/tmp/fido.sock\n"
	        "  -v       log every CTAPHID packet\n");
}

static int parse_fd(const char *s)
{
	char *end;
	long v;

	if (!s || !*s)
		return -1;
	v = strtol(s, &end, 10);
	return (*end == '\0' && v >= 0 && v < 65536) ? (int)v : -1;
}

int main(int argc, char **argv)
{
	char sock_path[sizeof(((struct sockaddr_un *)0)->sun_path) + 64];
	const char *sock_opt = NULL;
	int usb_fd = -1, lfd = -1, opt, rc = 1;
	pthread_t reader;
	int reader_started = 0;

	while ((opt = getopt(argc, argv, "s:vh")) != -1) {
		switch (opt) {
		case 's': sock_opt = optarg; break;
		case 'v': g_verbose = 1; break;
		default:  usage(); return opt == 'h' ? 0 : 2;
		}
	}
	/* termux-usb appends the fd as the last argument; newer versions also
	 * export it as TERMUX_USB_FD. */
	if (optind < argc)
		usb_fd = parse_fd(argv[argc - 1]);
	if (usb_fd < 0)
		usb_fd = parse_fd(getenv("TERMUX_USB_FD"));
	if (usb_fd < 0) {
		usage();
		return 2;
	}

	if (sock_opt) {
		snprintf(sock_path, sizeof(sock_path), "%s", sock_opt);
	} else {
		const char *prefix = getenv("PREFIX");
		snprintf(sock_path, sizeof(sock_path), "%s/tmp/fido.sock",
		         prefix && *prefix ? prefix : DEFAULT_PREFIX);
	}

	for (int i = 0; i < MAX_CLIENTS; i++)
		g_clients[i].fd = -1;

	struct sigaction sa = { .sa_handler = on_signal };
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	signal(SIGPIPE, SIG_IGN);

	/* Android apps may not enumerate /dev/bus/usb; work only on our fd. */
	libusb_set_option(NULL, LIBUSB_OPTION_NO_DEVICE_DISCOVERY);
	int r = libusb_init(&g_ctx);
	if (r < 0) {
		logmsg("libusb_init: %s", libusb_strerror(r));
		return 1;
	}
	r = libusb_wrap_sys_device(g_ctx, (intptr_t)usb_fd, &g_dev);
	if (r < 0) {
		logmsg("libusb_wrap_sys_device(fd %d): %s", usb_fd, libusb_strerror(r));
		goto out;
	}
	if (claim_fido_interface() < 0)
		goto out;
	if (pipe2(g_wake, O_CLOEXEC) < 0) {
		logmsg("pipe: %s", strerror(errno));
		goto out;
	}
	lfd = open_listener(sock_path);
	if (lfd < 0)
		goto out;
	if (pthread_create(&reader, NULL, usb_reader, NULL) != 0) {
		logmsg("cannot start USB reader thread");
		goto out;
	}
	reader_started = 1;
	logmsg("listening on %s", sock_path);

	while (!g_stop) {
		struct pollfd pfd[2 + MAX_CLIENTS];
		int slot[2 + MAX_CLIENTS];
		int n = 0;

		pfd[n].fd = lfd;       pfd[n].events = POLLIN; slot[n++] = -1;
		pfd[n].fd = g_wake[0]; pfd[n].events = POLLIN; slot[n++] = -1;
		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (g_clients[i].fd >= 0) {
				pfd[n].fd = g_clients[i].fd;
				pfd[n].events = POLLIN;
				slot[n++] = i;
			}
		}
		if (poll(pfd, n, 500) < 0) {
			if (errno == EINTR)
				continue;
			logmsg("poll: %s", strerror(errno));
			break;
		}
		if (pfd[0].revents & POLLIN)
			client_accept(lfd);
		for (int k = 2; k < n; k++) {
			if (pfd[k].revents & POLLIN)
				client_read(slot[k]);
			else if (pfd[k].revents & (POLLHUP | POLLERR | POLLNVAL))
				client_close(slot[k]);
		}
	}
	rc = 0;
	logmsg("shutting down");

out:
	g_stop = 1;
	if (reader_started)
		pthread_join(reader, NULL);
	for (int i = 0; i < MAX_CLIENTS; i++)
		client_close(i);
	if (lfd >= 0) {
		close(lfd);
		unlink(sock_path);
	}
	if (g_dev) {
		if (g_ifnum >= 0)
			release_interface(g_ifnum);
		libusb_close(g_dev);
	}
	libusb_exit(g_ctx);
	return rc;
}
