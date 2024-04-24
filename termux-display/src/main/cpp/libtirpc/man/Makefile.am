LOOKUP_MANS		= getrpcent.3t getrpcport.3t
NETCONFIG_MANS		= getnetconfig.3t getnetpath.3t
BIND_MANS		= bindresvport.3t
COMPAT_MANS		= des_crypt.3t rpc_soc.3t rpc_secure.3t rtime.3t
CLIENT_MANS		= rpc_clnt_auth.3t rpc_clnt_calls.3t rpc_clnt_create.3t \
			  rpcbind.3t
SERVER_MANS		= rpc_svc_calls.3t rpc_svc_create.3t rpc_svc_err.3t \
			  rpc_svc_reg.3t
GENERIC_MANS		= rpc.3t rpc_xdr.3t
RPCSEC_MANS		= rpcsec_gss.3t rpc_gss_get_error.3t \
			  rpc_gss_get_mechanisms.3t rpc_gss_get_mech_info.3t \
			  rpc_gss_get_versions.3t rpc_gss_is_installed.3t \
			  rpc_gss_mech_to_oid.3t rpc_gss_qop_to_num.3t \
			  rpc_gss_max_data_length.3t rpc_gss_seccreate.3t \
			  rpc_gss_set_defaults.3t rpc_gss_getcred.3t \
			  rpc_gss_get_principal_name.3t rpc_gss_set_callback.3t \
			  rpc_gss_set_svc_name.3t rpc_gss_svc_max_data_length.3t

dist_man5_MANS		= netconfig.5
dist_man3_MANS		= $(LOOKUP_MANS) $(NETCONFIG_MANS) \
			  $(BIND_MANS) $(GENERIC_MANS) $(COMPAT_MANS) \
			  $(CLIENT_MANS) $(SERVER_MANS)
if GSS
dist_man3_MANS		+= $(RPCSEC_MANS)
endif

EXTRA_DIST		= publickey.3t publickey.5 rpc.5

CLEANFILES		= cscope.* *~
DISTCLEANFILES		= Makefile.in
