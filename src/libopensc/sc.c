/*
 * sc.c: General functions
 *
 * Copyright (C) 2001, 2002  Juha Yrjölä <juha.yrjola@iki.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef ENABLE_OPENSSL
#include <openssl/crypto.h>     /* for OPENSSL_cleanse */
#endif


#include "internal.h"

#ifdef PACKAGE_VERSION
static const char *sc_version = PACKAGE_VERSION;
#else
static const char *sc_version = "(undef)";
#endif

#ifdef _WIN32
#include <windows.h>
#define PAGESIZE 0
#else
#include <sys/mman.h>
#include <limits.h>
#include <unistd.h>
#ifndef PAGESIZE
#define PAGESIZE 0
#endif
#endif

const char *sc_get_version(void)
{
    return sc_version;
}

int sc_hex_to_bin(const char *in, u8 *out, size_t *outlen)
{
	const char *sc_hex_to_bin_separators = " :";
	if (in == NULL || out == NULL || outlen == NULL) {
		return SC_ERROR_INVALID_ARGUMENTS;
	}

	int byte_needs_nibble = 0;
	int r = SC_SUCCESS;
	size_t left = *outlen;
	u8 byte = 0;
	while (*in != '\0' && 0 != left) {
		char c = *in++;
		u8 nibble;
		if      ('0' <= c && c <= '9')
			nibble = c - '0';
		else if ('a' <= c && c <= 'f')
			nibble = c - 'a' + 10;
		else if ('A' <= c && c <= 'F')
			nibble = c - 'A' + 10;
		else {
			if (strchr(sc_hex_to_bin_separators, (int) c)) {
				if (byte_needs_nibble) {
					r = SC_ERROR_INVALID_ARGUMENTS;
					goto err;
				}
				continue;
			}
			r = SC_ERROR_INVALID_ARGUMENTS;
			goto err;
		}

		if (byte_needs_nibble) {
			byte |= nibble;
			*out++ = (u8) byte;
			left--;
			byte_needs_nibble = 0;
		} else {
			byte  = nibble << 4;
			byte_needs_nibble = 1;
		}
	}

	if (left == *outlen && 1 == byte_needs_nibble && 0 != left) {
		/* no output written so far, but we have a valid nibble in the upper
		 * bits. Allow this special case. */
		*out = (u8) byte>>4;
		left--;
		byte_needs_nibble = 0;
	}

	/* for ease of implementation we only accept completely hexed bytes. */
	if (byte_needs_nibble) {
		r = SC_ERROR_INVALID_ARGUMENTS;
		goto err;
	}

	/* skip all trailing separators to see if we missed something */
	while (*in != '\0') {
		if (NULL == strchr(sc_hex_to_bin_separators, (int) *in))
			break;
		in++;
	}
	if (*in != '\0') {
		r = SC_ERROR_BUFFER_TOO_SMALL;
		goto err;
	}

err:
	*outlen -= left;
	return r;
}

int sc_bin_to_hex(const u8 *in, size_t in_len, char *out, size_t out_len,
				  int in_sep)
{
	if (in == NULL || out == NULL) {
		return SC_ERROR_INVALID_ARGUMENTS;
	}

	if (in_sep > 0) {
		if (out_len < in_len*3 || out_len < 1)
			return SC_ERROR_BUFFER_TOO_SMALL;
	} else {
		if (out_len < in_len*2 + 1)
			return SC_ERROR_BUFFER_TOO_SMALL;
	}

	const char hex[] = "0123456789abcdef";
	while (in_len) {
		unsigned char value = *in++;
		*out++ = hex[(value >> 4) & 0xF];
		*out++ = hex[ value       & 0xF];
		in_len--;
		if (in_len && in_sep > 0)
			*out++ = (char)in_sep;
	}
	*out = '\0';

	return SC_SUCCESS;
}

/*
 * Right trim all non-printable characters
 */
size_t sc_right_trim(u8 *buf, size_t len) {

	size_t i;

	if (!buf)
		return 0;

	if (len > 0) {
		for(i = len-1; i > 0; i--) {
			if(!isprint(buf[i])) {
				buf[i] = '\0';
				len--;
				continue;
			}
			break;
		}
	}
	return len;
}

u8 *ulong2bebytes(u8 *buf, unsigned long x)
{
	if (buf != NULL) {
		buf[3] = (u8) (x & 0xff);
		buf[2] = (u8) ((x >> 8) & 0xff);
		buf[1] = (u8) ((x >> 16) & 0xff);
		buf[0] = (u8) ((x >> 24) & 0xff);
	}
	return buf;
}

u8 *ushort2bebytes(u8 *buf, unsigned short x)
{
	if (buf != NULL) {
		buf[1] = (u8) (x & 0xff);
		buf[0] = (u8) ((x >> 8) & 0xff);
	}
	return buf;
}

unsigned long bebytes2ulong(const u8 *buf)
{
	if (buf == NULL)
		return 0UL;
	return (unsigned long)buf[0] << 24
		| (unsigned long)buf[1] << 16
		| (unsigned long)buf[2] << 8
		| (unsigned long)buf[3];
}

unsigned short bebytes2ushort(const u8 *buf)
{
	if (buf == NULL)
		return 0U;
	return (unsigned short)buf[0] << 8
		| (unsigned short)buf[1];
}

unsigned short lebytes2ushort(const u8 *buf)
{
	if (buf == NULL)
		return 0U;
	return (unsigned short)buf[1] << 8
		| (unsigned short)buf[0];
}

unsigned long lebytes2ulong(const u8 *buf)
{
	if (buf == NULL)
		return 0UL;
	return (unsigned long)buf[3] << 24
		| (unsigned long)buf[2] << 16
		| (unsigned long)buf[1] << 8
		| (unsigned long)buf[0];
}

void set_string(char **strp, const char *value)
{
	if (strp == NULL) {
		return;
	}

	free(*strp);
	*strp = value ? strdup(value) : NULL;
}

void sc_init_oid(struct sc_object_id *oid)
{
	int ii;

	if (!oid)
		return;
	for (ii=0; ii<SC_MAX_OBJECT_ID_OCTETS; ii++)
		oid->value[ii] = -1;
}

int sc_format_oid(struct sc_object_id *oid, const char *in)
{
	int        ii, ret = SC_ERROR_INVALID_ARGUMENTS;
	const char *p;
	char       *q;

	if (oid == NULL || in == NULL)
		return SC_ERROR_INVALID_ARGUMENTS;

	sc_init_oid(oid);

	p = in;
	for (ii=0; ii < SC_MAX_OBJECT_ID_OCTETS; ii++)   {
		oid->value[ii] = (int)strtol(p, &q, 10);
		if (!*q)
			break;

		if (!(q[0] == '.' && isdigit((unsigned char)q[1])))
			goto out;

		p = q + 1;
	}

	if (!sc_valid_oid(oid))
		goto out;

	ret = SC_SUCCESS;
out:
	if (ret)
		sc_init_oid(oid);

	return ret;
}

int sc_compare_oid(const struct sc_object_id *oid1, const struct sc_object_id *oid2)
{
	int i;

	if (oid1 == NULL || oid2 == NULL) {
		return SC_ERROR_INVALID_ARGUMENTS;
	}

	for (i = 0; i < SC_MAX_OBJECT_ID_OCTETS; i++)   {
		if (oid1->value[i] != oid2->value[i])
			return 0;
		if (oid1->value[i] == -1)
			break;
	}

	return 1;
}


int sc_valid_oid(const struct sc_object_id *oid)
{
	int ii;

	if (!oid)
		return 0;
	if (oid->value[0] == -1 || oid->value[1] == -1)
		return 0;
	if (oid->value[0] > 2 || oid->value[1] > 39)
		return 0;
	for (ii=0;ii<SC_MAX_OBJECT_ID_OCTETS;ii++)
		if (oid->value[ii])
			break;
	if (ii==SC_MAX_OBJECT_ID_OCTETS)
		return 0;
	return 1;
}


int sc_detect_card_presence(sc_reader_t *reader)
{
	int r;
	LOG_FUNC_CALLED(reader->ctx);
	if (reader->ops->detect_card_presence == NULL)
		LOG_FUNC_RETURN(reader->ctx, SC_ERROR_NOT_SUPPORTED);

	r = reader->ops->detect_card_presence(reader);

	// Check that we get sane return value from backend
	// detect_card_presence should return 0 if no card is present.
	if (r && !(r & SC_READER_CARD_PRESENT))
		LOG_FUNC_RETURN(reader->ctx, SC_ERROR_INTERNAL);

	LOG_FUNC_RETURN(reader->ctx, r);
}

int sc_path_set(sc_path_t *path, int type, const u8 *id, size_t id_len,
	int idx, int count)
{
	if (path == NULL || id == NULL || id_len == 0 || id_len > SC_MAX_PATH_SIZE)
		return SC_ERROR_INVALID_ARGUMENTS;

	memset(path, 0, sizeof(*path));
	memcpy(path->value, id, id_len);
	path->len   = id_len;
	path->type  = type;
	path->index = idx;
	path->count = count;

	return SC_SUCCESS;
}

void sc_format_path(const char *str, sc_path_t *path)
{
	int type = SC_PATH_TYPE_PATH;

	if (path) {
		memset(path, 0, sizeof(*path));
		if (*str == 'i' || *str == 'I') {
			type = SC_PATH_TYPE_FILE_ID;
			str++;
		}
		path->len = sizeof(path->value);
		if (sc_hex_to_bin(str, path->value, &path->len) >= 0) {
			path->type = type;
		}
		path->count = -1;
	}
}

int sc_append_path(sc_path_t *dest, const sc_path_t *src)
{
	return sc_concatenate_path(dest, dest, src);
}

int sc_append_path_id(sc_path_t *dest, const u8 *id, size_t idlen)
{
	if (dest->len + idlen > SC_MAX_PATH_SIZE)
		return SC_ERROR_INVALID_ARGUMENTS;
	memcpy(dest->value + dest->len, id, idlen);
	dest->len += idlen;
	return SC_SUCCESS;
}

int sc_append_file_id(sc_path_t *dest, unsigned int fid)
{
	u8 id[2] = { fid >> 8, fid & 0xff };

	return sc_append_path_id(dest, id, 2);
}

int sc_concatenate_path(sc_path_t *d, const sc_path_t *p1, const sc_path_t *p2)
{
	sc_path_t tpath;

	if (d == NULL || p1 == NULL || p2 == NULL)
		return SC_ERROR_INVALID_ARGUMENTS;

	if (p1->type == SC_PATH_TYPE_DF_NAME || p2->type == SC_PATH_TYPE_DF_NAME)
		/* we do not support concatenation of AIDs at the moment */
		return SC_ERROR_NOT_SUPPORTED;

	if (p1->len + p2->len > SC_MAX_PATH_SIZE)
		return SC_ERROR_INVALID_ARGUMENTS;

	memset(&tpath, 0, sizeof(sc_path_t));
	memcpy(tpath.value, p1->value, p1->len);
	memcpy(tpath.value + p1->len, p2->value, p2->len);
	tpath.len  = p1->len + p2->len;
	tpath.type = SC_PATH_TYPE_PATH;
	/* use 'index' and 'count' entry of the second path object */
	tpath.index = p2->index;
	tpath.count = p2->count;
	/* the result is currently always as path */
	tpath.type  = SC_PATH_TYPE_PATH;

	*d = tpath;

	return SC_SUCCESS;
}

const char *sc_print_path(const sc_path_t *path)
{
	static char buffer[SC_MAX_PATH_STRING_SIZE + SC_MAX_AID_STRING_SIZE];

	if (sc_path_print(buffer, sizeof(buffer), path) != SC_SUCCESS)
		buffer[0] = '\0';

	return buffer;
}

int sc_path_print(char *buf, size_t buflen, const sc_path_t *path)
{
	size_t i;

	if (buf == NULL || path == NULL)
		return SC_ERROR_INVALID_ARGUMENTS;

	if (buflen < path->len * 2 + path->aid.len * 2 + 3)
		return SC_ERROR_BUFFER_TOO_SMALL;

	buf[0] = '\0';
	if (path->aid.len)   {
		for (i = 0; i < path->aid.len; i++)
			snprintf(buf + strlen(buf), buflen - strlen(buf), "%02x", path->aid.value[i]);
		snprintf(buf + strlen(buf), buflen - strlen(buf), "::");
	}

	for (i = 0; i < path->len; i++)
		snprintf(buf + strlen(buf), buflen - strlen(buf), "%02x", path->value[i]);
	if (!path->aid.len && path->type == SC_PATH_TYPE_DF_NAME)
		snprintf(buf + strlen(buf), buflen - strlen(buf), "::");

	return SC_SUCCESS;
}

int sc_compare_path(const sc_path_t *path1, const sc_path_t *path2)
{
	return path1->len == path2->len
		&& !memcmp(path1->value, path2->value, path1->len);
}

int sc_compare_path_prefix(const sc_path_t *prefix, const sc_path_t *path)
{
	sc_path_t tpath;

	if (prefix->len > path->len)
		return 0;

	tpath     = *path;
	tpath.len = prefix->len;

	return sc_compare_path(&tpath, prefix);
}

const sc_path_t *sc_get_mf_path(void)
{
	static const sc_path_t mf_path = {
		{0x3f, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 2,
		0,
		0,
		SC_PATH_TYPE_PATH,
		{{0},0}
	};
	return &mf_path;
}

int sc_file_add_acl_entry(sc_file_t *file, unsigned int operation,
                          unsigned int method, unsigned long key_ref)
{
	sc_acl_entry_t *p, *_new;

	if (file == NULL || operation >= SC_MAX_AC_OPS) {
		return SC_ERROR_INVALID_ARGUMENTS;
	}

	switch (method) {
	case SC_AC_NEVER:
		sc_file_clear_acl_entries(file, operation);
		file->acl[operation] = (sc_acl_entry_t *) 1;
		return SC_SUCCESS;
	case SC_AC_NONE:
		sc_file_clear_acl_entries(file, operation);
		file->acl[operation] = (sc_acl_entry_t *) 2;
		return SC_SUCCESS;
	case SC_AC_UNKNOWN:
		sc_file_clear_acl_entries(file, operation);
		file->acl[operation] = (sc_acl_entry_t *) 3;
		return SC_SUCCESS;
	default:
		/* NONE and UNKNOWN get zapped when a new AC is added.
		 * If the ACL is NEVER, additional entries will be
		 * dropped silently. */
		if (file->acl[operation] == (sc_acl_entry_t *) 1)
			return SC_SUCCESS;
		if (file->acl[operation] == (sc_acl_entry_t *) 2
		 || file->acl[operation] == (sc_acl_entry_t *) 3)
			file->acl[operation] = NULL;
	}

	/* If the entry is already present (e.g. due to the mapping)
	 * of the card's AC with OpenSC's), don't add it again. */
	for (p = file->acl[operation]; p != NULL; p = p->next) {
		if ((p->method == method) && (p->key_ref == key_ref))
			return SC_SUCCESS;
	}

	_new = malloc(sizeof(sc_acl_entry_t));
	if (_new == NULL)
		return SC_ERROR_OUT_OF_MEMORY;
	_new->method = method;
	_new->key_ref = (unsigned)key_ref;
	_new->next = NULL;

	p = file->acl[operation];
	if (p == NULL) {
		file->acl[operation] = _new;
		return SC_SUCCESS;
	}
	while (p->next != NULL)
		p = p->next;
	p->next = _new;

	return SC_SUCCESS;
}

const sc_acl_entry_t * sc_file_get_acl_entry(const sc_file_t *file,
						  unsigned int operation)
{
	sc_acl_entry_t *p;
	static const sc_acl_entry_t e_never = {
		SC_AC_NEVER, SC_AC_KEY_REF_NONE, NULL
	};
	static const sc_acl_entry_t e_none = {
		SC_AC_NONE, SC_AC_KEY_REF_NONE, NULL
	};
	static const sc_acl_entry_t e_unknown = {
		SC_AC_UNKNOWN, SC_AC_KEY_REF_NONE, NULL
	};

	if (file == NULL || operation >= SC_MAX_AC_OPS) {
		return NULL;
	}

	p = file->acl[operation];
	if (p == (sc_acl_entry_t *) 1)
		return &e_never;
	if (p == (sc_acl_entry_t *) 2)
		return &e_none;
	if (p == (sc_acl_entry_t *) 3)
		return &e_unknown;

	return file->acl[operation];
}

void sc_file_clear_acl_entries(sc_file_t *file, unsigned int operation)
{
	sc_acl_entry_t *e;

	if (file == NULL || operation >= SC_MAX_AC_OPS) {
		return;
	}

	e = file->acl[operation];
	if (e == (sc_acl_entry_t *) 1 ||
	    e == (sc_acl_entry_t *) 2 ||
	    e == (sc_acl_entry_t *) 3) {
		file->acl[operation] = NULL;
		return;
	}

	while (e != NULL) {
		sc_acl_entry_t *tmp = e->next;
		free(e);
		e = tmp;
	}
	file->acl[operation] = NULL;
}

sc_file_t * sc_file_new(void)
{
	sc_file_t *file = (sc_file_t *)calloc(1, sizeof(sc_file_t));
	if (file == NULL)
		return NULL;

	file->magic = SC_FILE_MAGIC;
	return file;
}

void sc_file_free(sc_file_t *file)
{
	unsigned int i;
	if (file == NULL || !sc_file_valid(file))
		return;
	file->magic = 0;
	for (i = 0; i < SC_MAX_AC_OPS; i++)
		sc_file_clear_acl_entries(file, i);
	if (file->sec_attr)
		free(file->sec_attr);
	if (file->prop_attr)
		free(file->prop_attr);
	if (file->type_attr)
		free(file->type_attr);
	if (file->encoded_content)
		free(file->encoded_content);
	free(file);
}

void sc_file_dup(sc_file_t **dest, const sc_file_t *src)
{
	sc_file_t *newf;
	const sc_acl_entry_t *e;
	unsigned int op;

	*dest = NULL;
	if (!sc_file_valid(src))
		return;
	newf = sc_file_new();
	if (newf == NULL)
		return;
	*dest = newf;

	memcpy(&newf->path, &src->path, sizeof(struct sc_path));
	memcpy(&newf->name, &src->name, sizeof(src->name));
	newf->namelen = src->namelen;
	newf->type    = src->type;
	newf->shareable    = src->shareable;
	newf->ef_structure = src->ef_structure;
	newf->size    = src->size;
	newf->id      = src->id;
	newf->status  = src->status;
	for (op = 0; op < SC_MAX_AC_OPS; op++) {
		newf->acl[op] = NULL;
		e = sc_file_get_acl_entry(src, op);
		if (e != NULL) {
			if (sc_file_add_acl_entry(newf, op, e->method, e->key_ref) < 0)
				goto err;
		}
	}
	newf->record_length = src->record_length;
	newf->record_count  = src->record_count;

	if (sc_file_set_sec_attr(newf, src->sec_attr, src->sec_attr_len) < 0)
		goto err;
	if (sc_file_set_prop_attr(newf, src->prop_attr, src->prop_attr_len) < 0)
		goto err;
	if (sc_file_set_type_attr(newf, src->type_attr, src->type_attr_len) < 0)
		goto err;
	if (sc_file_set_content(newf, src->encoded_content, src->encoded_content_len) < 0)
		goto err;
	return;
err:
	sc_file_free(newf);
	*dest = NULL;
}

int sc_file_set_sec_attr(sc_file_t *file, const u8 *sec_attr,
			 size_t sec_attr_len)
{
	u8 *tmp;
	if (!sc_file_valid(file)) {
		return SC_ERROR_INVALID_ARGUMENTS;
	}

	if (sec_attr == NULL || sec_attr_len == 0) {
		if (file->sec_attr != NULL)
			free(file->sec_attr);
		file->sec_attr = NULL;
		file->sec_attr_len = 0;
		return 0;
	 }
	tmp = (u8 *) realloc(file->sec_attr, sec_attr_len);
	if (!tmp) {
		if (file->sec_attr)
			free(file->sec_attr);
		file->sec_attr     = NULL;
		file->sec_attr_len = 0;
		return SC_ERROR_OUT_OF_MEMORY;
	}
	file->sec_attr = tmp;
	memcpy(file->sec_attr, sec_attr, sec_attr_len);
	file->sec_attr_len = sec_attr_len;

	return 0;
}

int sc_file_set_prop_attr(sc_file_t *file, const u8 *prop_attr,
			 size_t prop_attr_len)
{
	u8 *tmp;
	if (!sc_file_valid(file)) {
		return SC_ERROR_INVALID_ARGUMENTS;
	}

	if (prop_attr == NULL || prop_attr_len == 0) {
		if (file->prop_attr != NULL)
			free(file->prop_attr);
		file->prop_attr = NULL;
		file->prop_attr_len = 0;
		return SC_SUCCESS;
	 }
	tmp = (u8 *) realloc(file->prop_attr, prop_attr_len);
	if (!tmp) {
		if (file->prop_attr)
			free(file->prop_attr);
		file->prop_attr = NULL;
		file->prop_attr_len = 0;
		return SC_ERROR_OUT_OF_MEMORY;
	}
	file->prop_attr = tmp;
	memcpy(file->prop_attr, prop_attr, prop_attr_len);
	file->prop_attr_len = prop_attr_len;

	return SC_SUCCESS;
}

int sc_file_set_type_attr(sc_file_t *file, const u8 *type_attr,
			 size_t type_attr_len)
{
	u8 *tmp;
	if (!sc_file_valid(file)) {
		return SC_ERROR_INVALID_ARGUMENTS;
	}

	if (type_attr == NULL || type_attr_len == 0) {
		if (file->type_attr != NULL)
			free(file->type_attr);
		file->type_attr = NULL;
		file->type_attr_len = 0;
		return SC_SUCCESS;
	 }
	tmp = (u8 *) realloc(file->type_attr, type_attr_len);
	if (!tmp) {
		if (file->type_attr)
			free(file->type_attr);
		file->type_attr = NULL;
		file->type_attr_len = 0;
		return SC_ERROR_OUT_OF_MEMORY;
	}
	file->type_attr = tmp;
	memcpy(file->type_attr, type_attr, type_attr_len);
	file->type_attr_len = type_attr_len;

	return SC_SUCCESS;
}


int sc_file_set_content(sc_file_t *file, const u8 *content,
			 size_t content_len)
{
	u8 *tmp;
	if (!sc_file_valid(file)) {
		return SC_ERROR_INVALID_ARGUMENTS;
	}

	if (content == NULL || content_len == 0) {
		if (file->encoded_content != NULL)
			free(file->encoded_content);
		file->encoded_content = NULL;
		file->encoded_content_len = 0;
		return SC_SUCCESS;
	}

	tmp = (u8 *) realloc(file->encoded_content, content_len);
	if (!tmp) {
		if (file->encoded_content)
			free(file->encoded_content);
		file->encoded_content = NULL;
		file->encoded_content_len = 0;
		return SC_ERROR_OUT_OF_MEMORY;
	}

	file->encoded_content = tmp;
	memcpy(file->encoded_content, content, content_len);
	file->encoded_content_len = content_len;

	return SC_SUCCESS;
}


int sc_file_valid(const sc_file_t *file) {
	if (file == NULL)
		return 0;
	return file->magic == SC_FILE_MAGIC;
}

int _sc_parse_atr(sc_reader_t *reader)
{
	u8 *p = reader->atr.value;
	int atr_len = (int) reader->atr.len;
	int n_hist, x;
	int tx[4] = {-1, -1, -1, -1};
	int i, FI, DI;
	const int Fi_table[] = {
		372, 372, 558, 744, 1116, 1488, 1860, -1,
		-1, 512, 768, 1024, 1536, 2048, -1, -1 };
	const int f_table[] = {
		40, 50, 60, 80, 120, 160, 200, -1,
		-1, 50, 75, 100, 150, 200, -1, -1 };
	const int Di_table[] = {
		-1, 1, 2, 4, 8, 16, 32, -1,
		12, 20, -1, -1, -1, -1, -1, -1 };

	reader->atr_info.hist_bytes_len = 0;
	reader->atr_info.hist_bytes = NULL;

	if (atr_len == 0) {
		sc_log(reader->ctx, "empty ATR - card not present?\n");
		return SC_ERROR_INTERNAL;
	}

	if (p[0] != 0x3B && p[0] != 0x3F) {
		sc_log(reader->ctx, "invalid sync byte in ATR: 0x%02X\n", p[0]);
		return SC_ERROR_INTERNAL;
	}
	n_hist = p[1] & 0x0F;
	x = p[1] >> 4;
	p += 2;
	atr_len -= 2;
	for (i = 0; i < 4 && atr_len > 0; i++) {
                if (x & (1 << i)) {
                        tx[i] = *p;
                        p++;
                        atr_len--;
                } else
                        tx[i] = -1;
        }
	if (tx[0] >= 0) {
		reader->atr_info.FI = FI = tx[0] >> 4;
		reader->atr_info.DI = DI = tx[0] & 0x0F;
		reader->atr_info.Fi = Fi_table[FI];
		reader->atr_info.f = f_table[FI];
		reader->atr_info.Di = Di_table[DI];
	} else {
		reader->atr_info.Fi = -1;
		reader->atr_info.f = -1;
		reader->atr_info.Di = -1;
	}
	if (tx[2] >= 0)
		reader->atr_info.N = tx[3];
	else
		reader->atr_info.N = -1;
	while (tx[3] > 0 && tx[3] & 0xF0 && atr_len > 0) {
		x = tx[3] >> 4;
		for (i = 0; i < 4 && atr_len > 0; i++) {
	                if (x & (1 << i)) {
	                        tx[i] = *p;
	                        p++;
	                        atr_len--;
	                } else
	                        tx[i] = -1;
		}
	}
	if (atr_len <= 0)
		return SC_SUCCESS;
	if (n_hist > atr_len)
		n_hist = atr_len;
	reader->atr_info.hist_bytes_len = n_hist;
	reader->atr_info.hist_bytes = p;
	return SC_SUCCESS;
}

void *sc_mem_secure_alloc(size_t len)
{
	void *p;

#ifdef _WIN32
	p = VirtualAlloc(NULL, len, MEM_COMMIT, PAGE_READWRITE);
	if (p != NULL) {
		VirtualLock(p, len);
	}
#else
	p = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (p != NULL) {
		mlock(p, len);
	}
#endif

	return p;
}

void sc_mem_secure_free(void *ptr, size_t len)
{
#ifdef _WIN32
	VirtualUnlock(ptr, len);
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	munlock(ptr, len);
	munmap(ptr, len);
#endif
}

void sc_mem_clear(void *ptr, size_t len)
{
	if (len > 0)   {
#ifdef HAVE_MEMSET_S
		memset_s(ptr, len, 0, len);
#elif _WIN32
		SecureZeroMemory(ptr, len);
#elif HAVE_EXPLICIT_BZERO
		explicit_bzero(ptr, len);
#elif ENABLE_OPENSSL
		OPENSSL_cleanse(ptr, len);
#else
		memset(ptr, 0, len);
#endif
	}
}

int sc_mem_reverse(unsigned char *buf, size_t len)
{
	unsigned char ch;
	size_t ii;

	if (!buf || !len)
		return SC_ERROR_INVALID_ARGUMENTS;

	for (ii = 0; ii < len / 2; ii++)   {
		ch = *(buf + ii);
		*(buf + ii) = *(buf + len - 1 - ii);
		*(buf + len - 1 - ii) = ch;
	}

	return SC_SUCCESS;
}

static int
sc_remote_apdu_allocate(struct sc_remote_data *rdata,
		struct sc_remote_apdu **new_rapdu)
{
	struct sc_remote_apdu *rapdu = NULL, *rr;

	if (!rdata)
		return SC_ERROR_INVALID_ARGUMENTS;

	rapdu = calloc(1, sizeof(struct sc_remote_apdu));
	if (rapdu == NULL)
		return SC_ERROR_OUT_OF_MEMORY;

	rapdu->apdu.data = &rapdu->sbuf[0];
	rapdu->apdu.resp = &rapdu->rbuf[0];
	rapdu->apdu.resplen = sizeof(rapdu->rbuf);

	if (new_rapdu)
		*new_rapdu = rapdu;

	if (rdata->data == NULL)   {
		rdata->data = rapdu;
		rdata->length = 1;
		return SC_SUCCESS;
	}

	for (rr = rdata->data; rr->next; rr = rr->next)
		;
	rr->next = rapdu;
	rdata->length++;

	return SC_SUCCESS;
}

static void
sc_remote_apdu_free (struct sc_remote_data *rdata)
{
	struct sc_remote_apdu *rapdu = NULL;

	if (!rdata)
		return;

	rapdu = rdata->data;
	while(rapdu)   {
		struct sc_remote_apdu *rr = rapdu->next;

		free(rapdu);
		rapdu = rr;
	}
}

void sc_remote_data_init(struct sc_remote_data *rdata)
{
	if (!rdata)
		return;
	memset(rdata, 0, sizeof(struct sc_remote_data));

	rdata->alloc = sc_remote_apdu_allocate;
	rdata->free = sc_remote_apdu_free;
}

static unsigned long  sc_CRC_tab32[256];
static int sc_CRC_tab32_initialized = 0;
unsigned sc_crc32(const unsigned char *value, size_t len)
{
	size_t ii, jj;
	unsigned long crc;
	unsigned long index, long_c;

	if (!sc_CRC_tab32_initialized)   {
		for (ii=0; ii<256; ii++) {
			crc = (unsigned long) ii;
			for (jj=0; jj<8; jj++) {
				if ( crc & 0x00000001L )
					crc = ( crc >> 1 ) ^ 0xEDB88320l;
				else
					crc =   crc >> 1;
			}
			sc_CRC_tab32[ii] = crc;
		}
		sc_CRC_tab32_initialized = 1;
	}

	crc = 0xffffffffL;
	for (ii=0; ii<len; ii++)   {
		long_c = 0x000000ffL & (unsigned long) (*(value + ii));
		index = crc ^ long_c;
		crc = (crc >> 8) ^ sc_CRC_tab32[ index & 0xff ];
	}

	crc ^= 0xffffffff;
	return  crc%0xffff;
}

const u8 *sc_compacttlv_find_tag(const u8 *buf, size_t len, u8 tag, size_t *outlen)
{
	if (buf != NULL) {
		size_t idx;
		u8 plain_tag = tag & 0xF0;
		size_t expected_len = tag & 0x0F;

	        for (idx = 0; idx < len; idx++) {
			if ((buf[idx] & 0xF0) == plain_tag && idx + expected_len < len &&
			    (expected_len == 0 || expected_len == (buf[idx] & 0x0F))) {
				if (outlen != NULL)
					*outlen = buf[idx] & 0x0F;
				return buf + (idx + 1);
			}
			idx += (buf[idx] & 0x0F);
                }
        }
	return NULL;
}

/**************************** mutex functions ************************/

int sc_mutex_create(const sc_context_t *ctx, void **mutex)
{
	if (ctx == NULL)
		return SC_ERROR_INVALID_ARGUMENTS;
	if (ctx->thread_ctx != NULL && ctx->thread_ctx->create_mutex != NULL)
		return ctx->thread_ctx->create_mutex(mutex);
	else
		return SC_SUCCESS;
}

int sc_mutex_lock(const sc_context_t *ctx, void *mutex)
{
	if (ctx == NULL)
		return SC_ERROR_INVALID_ARGUMENTS;
	if (ctx->thread_ctx != NULL && ctx->thread_ctx->lock_mutex != NULL)
		return ctx->thread_ctx->lock_mutex(mutex);
	else
		return SC_SUCCESS;
}

int sc_mutex_unlock(const sc_context_t *ctx, void *mutex)
{
	if (ctx == NULL)
		return SC_ERROR_INVALID_ARGUMENTS;
	if (ctx->thread_ctx != NULL && ctx->thread_ctx->unlock_mutex != NULL)
		return ctx->thread_ctx->unlock_mutex(mutex);
	else
		return SC_SUCCESS;
}

int sc_mutex_destroy(const sc_context_t *ctx, void *mutex)
{
	if (ctx == NULL)
		return SC_ERROR_INVALID_ARGUMENTS;
	if (ctx->thread_ctx != NULL && ctx->thread_ctx->destroy_mutex != NULL)
		return ctx->thread_ctx->destroy_mutex(mutex);
	else
		return SC_SUCCESS;
}

unsigned long sc_thread_id(const sc_context_t *ctx)
{
	if (ctx == NULL || ctx->thread_ctx == NULL ||
	    ctx->thread_ctx->thread_id == NULL)
		return 0UL;
	else
		return ctx->thread_ctx->thread_id();
}

void sc_free(void *p)
{
	free(p);
}
