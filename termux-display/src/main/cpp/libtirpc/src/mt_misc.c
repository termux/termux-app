
#include <pthread.h>
#include <reentrant.h>
#include <rpc/rpc.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

/* protects the services list (svc.c) */
pthread_rwlock_t	svc_lock = PTHREAD_RWLOCK_INITIALIZER;

/* protects svc_fdset and the xports[] array */
pthread_rwlock_t	svc_fd_lock = PTHREAD_RWLOCK_INITIALIZER;

/* protects the RPCBIND address cache */
pthread_rwlock_t	rpcbaddr_cache_lock = PTHREAD_RWLOCK_INITIALIZER;

/* protects authdes cache (svcauth_des.c) */
pthread_mutex_t	authdes_lock = PTHREAD_MUTEX_INITIALIZER;

/* serializes authdes ops initializations */
pthread_mutex_t authdes_ops_lock = PTHREAD_MUTEX_INITIALIZER;

/* protects des stats list */
pthread_mutex_t svcauthdesstats_lock = PTHREAD_MUTEX_INITIALIZER;

/* auth_none.c serialization */
pthread_mutex_t	authnone_lock = PTHREAD_MUTEX_INITIALIZER;

/* protects the Auths list (svc_auth.c) */
pthread_mutex_t	authsvc_lock = PTHREAD_MUTEX_INITIALIZER;

/* protects client-side fd lock array */
pthread_mutex_t	clnt_fd_lock = PTHREAD_MUTEX_INITIALIZER;

/* clnt_raw.c serialization */
pthread_mutex_t	clntraw_lock = PTHREAD_MUTEX_INITIALIZER;

/* domainname and domain_fd (getdname.c) and default_domain (rpcdname.c) */
pthread_mutex_t	dname_lock = PTHREAD_MUTEX_INITIALIZER;

/* dupreq variables (svc_dg.c) */
pthread_mutex_t	dupreq_lock = PTHREAD_MUTEX_INITIALIZER;

/* protects first_time and hostname (key_call.c) */
pthread_mutex_t	keyserv_lock = PTHREAD_MUTEX_INITIALIZER;

/* serializes rpc_trace() (rpc_trace.c) */
pthread_mutex_t	libnsl_trace_lock = PTHREAD_MUTEX_INITIALIZER;

/* loopnconf (rpcb_clnt.c) */
pthread_mutex_t	loopnconf_lock = PTHREAD_MUTEX_INITIALIZER;

/* serializes ops initializations */
pthread_mutex_t	ops_lock = PTHREAD_MUTEX_INITIALIZER;

/* protects ``port'' static in bindresvport() */
pthread_mutex_t	portnum_lock = PTHREAD_MUTEX_INITIALIZER;

/* protects proglst list (svc_simple.c) */
pthread_mutex_t	proglst_lock = PTHREAD_MUTEX_INITIALIZER;

/* serializes clnt_com_create() (rpc_soc.c) */
pthread_mutex_t	rpcsoc_lock = PTHREAD_MUTEX_INITIALIZER;

/* svc_raw.c serialization */
pthread_mutex_t	svcraw_lock = PTHREAD_MUTEX_INITIALIZER;

/* protects TSD key creation */
pthread_mutex_t	tsd_lock = PTHREAD_MUTEX_INITIALIZER;

/* protects RPCSEC GSS callback list */
pthread_mutex_t svcauth_cb_lock = PTHREAD_MUTEX_INITIALIZER;

/* serialize updates to AUTH ref count */
pthread_mutex_t auth_ref_lock = PTHREAD_MUTEX_INITIALIZER;

/* protects RPCSEC GSS cache */
pthread_mutex_t svcauth_gss_cache_lock = PTHREAD_MUTEX_INITIALIZER;

/* Library global tsd keys */
thread_key_t clnt_broadcast_key = KEY_INITIALIZER;
thread_key_t rpc_call_key = KEY_INITIALIZER;
thread_key_t tcp_key = KEY_INITIALIZER;
thread_key_t udp_key = KEY_INITIALIZER;
thread_key_t nc_key = KEY_INITIALIZER;
thread_key_t rce_key = KEY_INITIALIZER;
thread_key_t rg_key = KEY_INITIALIZER;
thread_key_t key_call_key = KEY_INITIALIZER;

/* xprtlist (svc_generic.c) */
pthread_mutex_t	xprtlist_lock = PTHREAD_MUTEX_INITIALIZER;

/* serializes calls to public key routines */
pthread_mutex_t serialize_pkey = PTHREAD_MUTEX_INITIALIZER;

/* protects global variables ni and nc_file (getnetconfig.c) */
pthread_mutex_t nc_db_lock = PTHREAD_MUTEX_INITIALIZER;

/* protects static port and startport (bindresvport.c) */
pthread_mutex_t port_lock = PTHREAD_MUTEX_INITIALIZER;

/* protects static disrupt (clnt_vc.c) */
pthread_mutex_t disrupt_lock = PTHREAD_MUTEX_INITIALIZER;

#undef	rpc_createerr

struct rpc_createerr rpc_createerr;

struct rpc_createerr *
__rpc_createerr()
{
	struct rpc_createerr *rce_addr;

	mutex_lock(&tsd_lock);
	if (rce_key == KEY_INITIALIZER)
		thr_keycreate(&rce_key, free);
	mutex_unlock(&tsd_lock);

	rce_addr = (struct rpc_createerr *)thr_getspecific(rce_key);
	if (!rce_addr) {
		rce_addr = (struct rpc_createerr *)
			malloc(sizeof (struct rpc_createerr));
		if (!rce_addr ||
		    thr_setspecific(rce_key, (void *) rce_addr) != 0) {
			if (rce_addr)
				free(rce_addr);
			return (&rpc_createerr);
		}
		memset(rce_addr, 0, sizeof (struct rpc_createerr));
	}
	return (rce_addr);
}

void tsd_key_delete(void)
{
	if (clnt_broadcast_key != KEY_INITIALIZER)
		pthread_key_delete(clnt_broadcast_key);
	if (rpc_call_key != KEY_INITIALIZER)
		pthread_key_delete(rpc_call_key);
	if (tcp_key != KEY_INITIALIZER)
		pthread_key_delete(tcp_key);
	if (udp_key != KEY_INITIALIZER)
		pthread_key_delete(udp_key);
	if (nc_key != KEY_INITIALIZER)
		pthread_key_delete(nc_key);
	if (rce_key != KEY_INITIALIZER)
		pthread_key_delete(rce_key);
	if (rg_key != KEY_INITIALIZER)
		pthread_key_delete(rce_key);
	return;
}

