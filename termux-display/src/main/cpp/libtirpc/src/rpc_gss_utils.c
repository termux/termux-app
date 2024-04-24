/*
 * Copyright (c) 2013, Oracle America, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of "Oracle America, Inc." nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <errno.h>
#include <reentrant.h>

#include <rpc/auth_gss.h>
#include <rpc/rpcsec_gss.h>

#include <gssapi/gssapi_krb5.h>

/* Internal only */
void rpc_gss_set_error(int);
void rpc_gss_clear_error(void);
bool_t rpc_gss_oid_to_mech(rpc_gss_OID, char **);

/*
 * Return the static "_rpc_gss_error" if we are the main thread
 * (including non-threaded programs), or if an allocation fails.
 */
static rpc_gss_error_t *
__rpc_gss_error(void)
{
	static rpc_gss_error_t _rpc_gss_error = { 0, 0 };
	extern thread_key_t rg_key;
	rpc_gss_error_t *result;

	if (rg_key == KEY_INITIALIZER) {
		static mutex_t _rpc_gss_error_lock = MUTEX_INITIALIZER;
		int err = 0;

		mutex_lock(&_rpc_gss_error_lock);
		if (rg_key == KEY_INITIALIZER)
			err = thr_keycreate(&rg_key, free);
		mutex_unlock(&_rpc_gss_error_lock);

		if (err != 0)
			return &_rpc_gss_error;
	}

	result = thr_getspecific(rg_key);
	if (result == NULL) {
		result = calloc(1, sizeof(*result));
		if (result == NULL)
			return &_rpc_gss_error;

		if (thr_setspecific(rg_key, result) != 0) {
			free(result);
			return &_rpc_gss_error;
		}
	}

	return result;
}

/*
 * External API: Retrieve thread-specific error values
 *
 * error: address of rpc_gss_error_t structure to fill in
 */
void
rpc_gss_get_error(rpc_gss_error_t *result)
{
	rpc_gss_error_t *error = __rpc_gss_error();

	result->rpc_gss_error = error->rpc_gss_error;
	result->system_error = error->system_error;
}

/*
 * Internal only: Set thread-specific error value
 *
 * system_error: new value for thread's .system_error field.
 *		 .rpc_gss_error is always set to RPC_GSS_ER_SYSTEMERROR.
 */
void
rpc_gss_set_error(int system_error)
{
	rpc_gss_error_t *error = __rpc_gss_error();

	error->rpc_gss_error = RPC_GSS_ER_SYSTEMERROR;
	error->system_error = system_error;
}

/*
 * Internal only: Clear thread-specific error values
 */
void
rpc_gss_clear_error(void)
{
	rpc_gss_error_t *error = __rpc_gss_error();

	error->rpc_gss_error = RPC_GSS_ER_SUCCESS;
	error->system_error = 0;
}

/*
 * On Solaris, the GSS-API implementation consults the files
 * /etc/gss/mech and /etc/gss/qop for a mapping of mechanism
 * names to OIDs, and QOP names to numbers.
 *
 * GNU's GSS-API has no such database.  We emulate it here
 * with a built-in table of supported mechanisms and qops,
 * since the set of supported mechanisms is unlikely to
 * change often.
 */

struct _rpc_gss_qop {
	char			*qi_name;
	unsigned int		 qi_num;
};

struct _rpc_gss_mechanism {
	char			*mi_name;
	rpc_gss_OID_desc	 mi_oid;
	char			**mi_qop_names;
	struct _rpc_gss_qop	**mi_qops;
};


static struct _rpc_gss_qop _rpc_gss_qop_default = {
	.qi_name		= "GSS_C_QOP_DEFAULT",
	.qi_num			= GSS_C_QOP_DEFAULT,
};

static struct _rpc_gss_qop *_rpc_gss_krb5_qops[] = {
	&_rpc_gss_qop_default,
	NULL,
};

static char *_rpc_gss_krb5_qop_names[] = {
	"GSS_C_QOP_DEFAULT",
	NULL,
};

/* GSS_MECH_KRB5_OID: Defined by RFC 1964 */
static struct _rpc_gss_mechanism _rpc_gss_mech_kerberos_v5 = {
	.mi_name		= "kerberos_v5",
	.mi_oid			= { 9, "\052\206\110\206\367\022\001\002\002" },
	.mi_qop_names		= _rpc_gss_krb5_qop_names,
	.mi_qops		= _rpc_gss_krb5_qops,
};

/* GSS_KRB5_NT_PRINCIPAL_NAME: Defined by RFC 1964 */
static struct _rpc_gss_mechanism _rpc_gss_mech_kerberos_v5_princname = {
	.mi_name		= "kerberos_v5",
	.mi_oid			= { 10, "\052\206\110\206\367\022\001\002\002\001" },
	.mi_qop_names		= _rpc_gss_krb5_qop_names,
	.mi_qops		= _rpc_gss_krb5_qops,
};

static struct _rpc_gss_mechanism *_rpc_gss_mechanisms[] = {
	&_rpc_gss_mech_kerberos_v5,
	&_rpc_gss_mech_kerberos_v5_princname,
	NULL,
};

static char *_rpc_gss_mechanism_names[] = {
	"kerberos_v5",
	NULL,
};

static struct _rpc_gss_mechanism *
_rpc_gss_find_mechanism(char *mechanism)
{
	unsigned int i;

	for (i = 0; _rpc_gss_mechanisms[i] != NULL; i++)
		if (strcmp(mechanism, _rpc_gss_mechanisms[i]->mi_name) == 0)
			return _rpc_gss_mechanisms[i];
	return NULL;
}

static bool_t
_rpc_gss_OID_equal(rpc_gss_OID o1, rpc_gss_OID o2)
{
	return (o1->length == o2->length) &&
		(memcmp(o1->elements, o2->elements, o1->length) == 0);
}

static struct _rpc_gss_mechanism *
_rpc_gss_find_oid(rpc_gss_OID oid)
{
	unsigned int i;

	for (i = 0; _rpc_gss_mechanisms[i] != NULL; i++)
		if (_rpc_gss_OID_equal(oid, &_rpc_gss_mechanisms[i]->mi_oid))
			return _rpc_gss_mechanisms[i];
	return NULL;
}

static struct _rpc_gss_qop *
_rpc_gss_find_qop_by_name(struct _rpc_gss_mechanism *m, char *qop)
{
	unsigned int i;

	for (i = 0; m->mi_qops[i] != NULL; i++)
		if (strcmp(qop, m->mi_qops[i]->qi_name) == 0)
			return m->mi_qops[i];
	return NULL;
}

static struct _rpc_gss_qop *
_rpc_gss_find_qop_by_num(struct _rpc_gss_mechanism *m, u_int num)
{
	unsigned int i;

	for (i = 0; m->mi_qops[i] != NULL; i++)
		if (num == m->mi_qops[i]->qi_num)
			return m->mi_qops[i];
	return NULL;
}

/*
 * External API: Return array of security mechanism names
 *
 * Returns NULL-terminated array of pointers to NUL-terminated C
 * strings, each containing a mechanism name.  This is statically
 * allocated memory.  Caller must not free this array.
 */
char **
rpc_gss_get_mechanisms(void)
{
	rpc_gss_clear_error();
	return (char **)_rpc_gss_mechanism_names;
}

/*
 * External API: Return array of qop names for a security mechanism
 *
 * mechanism: NUL-terminated C string containing mechanism name
 * dummy: address of a service enum
 *
 * Returns NULL-terminated array of pointers to NUL-terminated C
 * strings, each containing a qop name.  This is statically
 * allocated memory.  Caller must not free this array.
 */
char **
rpc_gss_get_mech_info(char *mechanism, rpc_gss_service_t *dummy)
{
	struct _rpc_gss_mechanism *m;

	if (mechanism == NULL || dummy == NULL) {
		rpc_gss_set_error(EINVAL);
		return NULL;
	}

	m = _rpc_gss_find_mechanism(mechanism);
	if (m == NULL) {
		rpc_gss_set_error(ENOENT);
		return NULL;
	}

	rpc_gss_clear_error();
	*dummy = rpcsec_gss_svc_privacy;
	return (char **)m->mi_qop_names;
}

/*
 * External API: Return range of supported RPCSEC_GSS versions
 *
 * high: address of highest supported version to fill in
 * low: address of lowest supported version to fill in
 *
 * Returns TRUE if successful, or FALSE if an error occurs.
 */
bool_t
rpc_gss_get_versions(u_int *high, u_int *low)
{
	if (high == NULL || low == NULL) {
		rpc_gss_set_error(EINVAL);
		return FALSE;
	}

	rpc_gss_clear_error();
	*high = *low = RPCSEC_GSS_VERSION;
	return TRUE;
}

/*
 * External API: Check if a security mechanism is supported
 *
 * mechanism: NUL-terminated C string containing mechanism name
 *
 * Returns TRUE if the mechanism name is recognized and supported,
 * otherwise FALSE is returned.
 */
bool_t
rpc_gss_is_installed(char *mechanism)
{
	struct _rpc_gss_mechanism *m;

	if (mechanism == NULL) {
		rpc_gss_set_error(EINVAL);
		return FALSE;
	}

	rpc_gss_clear_error();
	m = _rpc_gss_find_mechanism(mechanism);
	if (m == NULL)
		return FALSE;
	return TRUE;
}

/*
 * External API: Return the OID for a given security mechanism
 *
 * mechanism: NUL-terminated C string containing mechanism name
 * oid: address of OID buffer to fill in
 *
 * Returns TRUE if the mechanism name is recognized and supported,
 * otherwise FALSE is returned.
 */
bool_t
rpc_gss_mech_to_oid(char *mechanism, rpc_gss_OID *result)
{
	struct _rpc_gss_mechanism *m;

	if (mechanism == NULL || result == NULL) {
		rpc_gss_set_error(EINVAL);
		return FALSE;
	}

	m = _rpc_gss_find_mechanism(mechanism);
	if (m == NULL) {
		rpc_gss_set_error(ENOENT);
		return FALSE;
	}

	*result = &m->mi_oid;
	rpc_gss_clear_error();
	return TRUE;
}

/*
 * Internal only: Return the mechanism name for a given OID
 *
 * oid: GSS mechanism OID
 * mechanism: address of a char * to fill in.  This is statically
 *	      allocated memory.  Caller must not free this memory.
 *
 * Returns TRUE if the OID is a recognized and supported mechanism,
 * otherwise FALSE is returned.
 */
bool_t
rpc_gss_oid_to_mech(rpc_gss_OID oid, char **mechanism)
{
	struct _rpc_gss_mechanism *m;

	if (oid == NULL || mechanism == NULL) {
		rpc_gss_set_error(EINVAL);
		return FALSE;
	}

	m = _rpc_gss_find_oid(oid);
	if (m == NULL) {
		rpc_gss_set_error(ENOENT);
		return FALSE;
	}

	*mechanism = m->mi_name;
	rpc_gss_clear_error();
	return TRUE;
}

/*
 * External API: Return the QOP number for a given QOP and mechanism name
 *
 * qop: NUL-terminated C string containing qop name
 * mechanism: NUL-terminated C string containing mechanism name
 * num: address of QOP buffer to fill in
 *
 * Returns TRUE if the qop and mechanism name are recognized and
 * supported, otherwise FALSE is returned.
 */
bool_t
rpc_gss_qop_to_num(char *qop, char *mechanism, u_int *num)
{
	struct _rpc_gss_mechanism *m;
	struct _rpc_gss_qop *q;

	if (qop == NULL || mechanism == NULL || num == NULL) {
		rpc_gss_set_error(EINVAL);
		return FALSE;
	}

	m = _rpc_gss_find_mechanism(mechanism);
	if (m == NULL)
		goto out_err;

	q = _rpc_gss_find_qop_by_name(m, qop);
	if (q == NULL)
		goto out_err;

	*num = q->qi_num;
	rpc_gss_clear_error();
	return TRUE;

out_err:
	rpc_gss_set_error(ENOENT);
	return FALSE;
}

/*
 * Internal only: Return the QOP name for a given mechanism and QOP number
 *
 * mechanism: NUL-terminated C string containing security mechanism name
 * num: QOP number
 * qop: address of a char * to fill in.  This is statically
 *	allocated memory.  Caller must not free this memory.
 *
 * Returns TRUE if the QOP and mechanism are recognized and supported,
 * otherwise FALSE is returned.
 */
bool_t
rpc_gss_num_to_qop(char *mechanism, u_int num, char **qop)
{
	struct _rpc_gss_mechanism *m;
	struct _rpc_gss_qop *q;

	if (mechanism == NULL || qop == NULL) {
		rpc_gss_set_error(EINVAL);
		return FALSE;
	}

	m = _rpc_gss_find_mechanism(mechanism);
	if (m == NULL)
		goto out_err;

	q = _rpc_gss_find_qop_by_num(m, num);
	if (q == NULL)
		goto out_err;

	*qop = q->qi_name;
	rpc_gss_clear_error();
	return TRUE;

out_err:
	rpc_gss_set_error(ENOENT);
	return FALSE;
}
