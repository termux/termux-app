[termux-services](https://github.com/termux/termux-services) contains a
set of scripts for controlling services. Instead of putting commands in
\~/.bashrc or \~/.bash_profile, they can be started and stopped with
termux-services.

| Package      | Daemon       | Port | Description                                                                                                                    |
|--------------|--------------|------|--------------------------------------------------------------------------------------------------------------------------------|
| apache2      | httpd        | 8080 | Apache Web Server [HTTP Web Server](https://en.wikipedia.org/wiki/Web_server)                                                  |
| at           | atd          |      | AT and batch delayed command scheduling utility and daemon [at](https://en.wikipedia.org/wiki/At_(command))                    |
| bitcoin      | bitcoind     |      | Bitcoin Core                                                                                                                   |
| busybox      | telnetd      | 8023 | Remote terminal service [Telnet](https://en.wikipedia.org/wiki/Telnet)                                                         |
| busybox      | ftpd         | 8021 | FTP (File Transfer Protocol) [FTP](https://en.wikipedia.org/wiki/File_Transfer_Protocol)                                       |
| cronie       | crond        |      | Daemon that runs specified programs at scheduled times [cron](https://en.wikipedia.org/wiki/Cron)                              |
| emacs        | emacsd       |      | Extensible, customizable text editor-and more [EMACS](https://www.emacswiki.org/emacs/EmacsAsDaemon)                           |
| ipfs         | ipfs         |      | A peer-to-peer hypermedia distribution protocol [IPFS](https://en.wikipedia.org/wiki/InterPlanetary_File_System)               |
| libmosquitto | mosquitto    | 1883 | MQTT is a lightweight, publish-subscribe network protocol [MQTT](https://en.wikipedia.org/wiki/MQTT)                           |
| lighttpd     | lighttpd     | 8080 | Lighttpd Small webserver HTTP Web Server                                                                                       |
|              |              |      |                                                                                                                                |
| lnd          | lnd          |      | Lightning Network Daemon                                                                                                       |
| mariadb      | mysqld       | 3306 | [MariaDB](https://en.wikipedia.org/wiki/MariaDB) Community-developed fork of the MySQL from its original authors               |
| mpd          | mpd          |      | [Music Player Daemon (MPD)](https://en.wikipedia.org/wiki/Music_Player_Daemon)                                                 |
| mpdscribble  | mpdscribble  |      | MPD client which submits tracks being played                                                                                   |
| nginx        | nginx        | 8080 | NGINX Web server HTTP Web Server                                                                                               |
| openssh      | sshd         | 8022 | OpenSSH [SSH](https://en.wikipedia.org/wiki/Secure_Shell_Protocol)                                                             |
| postgresql   | postgres     | 5432 | PostgreSQL database [PostgreSQL](https://en.wikipedia.org/wiki/PostgreSQL)                                                     |
| privoxy      | privoxy      |      | Privoxy is non-caching web proxy with advanced filtering capabilities [HTTP Proxy](https://en.wikipedia.org/wiki/Proxy_server) |
| tor          | tor          |      | The Onion Router anonymizing overlay network [Tor](https://en.wikipedia.org/wiki/Tor_(network))                                |
| transmission | transmission |      | BitTorrent client [BitTorrent](https://en.wikipedia.org/wiki/BitTorrent)                                                       |

Supported services

To install termux-services, run

`pkg install termux-services`

and then restart termux so that the service-daemon is started.

To then enable and run a service, run

`sv-enable `<service>

If you only want to run it once, run

`sv up `<service>

To later stop a service, run:

`sv down `<service>

Or to disable it

`sv-disable `<service>

A service is disabled if \`\$PREFIX/var/service/<service>/down\` exists,
so the \`sv-enable\` and \`sv-disable\` scripts touches, or removes,
this file.

termux-services uses the programs from
[runit](http://smarden.org/runit/) to control the services. A bunch of
example scripts are available from the [same
site](http://smarden.org/runit/runscripts.html). If you find a script
you want to use, or if you write your own, you can use set it up by
running:

`mkdir -p $PREFIX/var/service/`<PKG>`/log`
`ln -sf $PREFIX/share/termux-services/svlogger $PREFIX/var/service/`<PKG>`/log/run`

and then put your run script for the package at
\$PREFIX/var/service/<PKG>/run and make sure that it is runnable.

You can then run

`sv up `<PKG>

to start it.

Log files for services are situated in \$PREFIX/var/log/sv/<PKG>/ with
the active log file named "current".

### Starting termux-services on boot

If you want your services to start on device boot, see the example in
[Termux:Boot](Termux:Boot)