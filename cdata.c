/*
Copyright 2022 James R.
All rights reserved.

Redistribution and use in source and binary forms, with or
without modification, is permitted provided that the
following condition is met:

Redistributions of source code must retain the above
copyright notice, this condition and the following
disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "cdata.h"

static void default_error_handler (const char*, int);

static cdata_error_handler
error_handler = default_error_handler;

#define fuckoff() \
	(*error_handler)(__FILE__, __LINE__)

struct pack_state {
	struct packdef const * def;
	char * object;
	char * wire;
	size_t size;
	size_t ofs;/* object offset; misc use */
	int mode;
};

static void
default_error_handler
(		const char * file,
		int line)
{
	int d = errno;

	fprintf(stderr, "%s:%d: ", file, line);

	errno = d;
	perror("");

	abort();
}

void
cdata_set_error_handler (cdata_error_handler cb)
{
	error_handler = cb ? cb : default_error_handler;
}

static void
decode_align
(		void const * p,
		int align,
		void * var)
{
	if (align == 2)
		*(int16_t*)var = cdata_decode16(p);
	else if (align == 4)
		*(int32_t*)var = cdata_decode32(p);
}

static void
encode_align
(		void * p,
		int align,
		void const * var)
{
	if (align == 2)
		cdata_encode16(p, *(int16_t const*)var);
	else if (align == 4)
		cdata_encode32(p, *(int32_t const*)var);
}

static int
align_copy
(		void * p,
		size_t size,
		int align,
		void const * s)
{
	if (align == 2 || align == 4)
		return 1;
	else
		return memcpy(p, s, size), 0;
}

void
cdata_encode_array
(		void * wire,
		void const * var,
		size_t size,
		int align)
{
	size_t k = 0;

	if (align_copy(wire, size, align, var))
	{
		while (k < size)
		{
			encode_align(&((char*)wire)[k], align,
					&((char const*)var)[k]);
			k += align;
		}
	}
}

void
cdata_decode_array
(		void const * wire,
		void * var,
		size_t size,
		int align)
{
	size_t k = 0;

	if (align_copy(var, size, align, wire))
	{
		while (k < size)
		{
			decode_align(&((char const*)wire)[k],
					align, &((char*)var)[k]);
			k += align;
		}
	}
}

static int
call_part
(		struct packdef_part const * part,
		size_t ofs,
		void * udata)
{
	struct pack_state *P = udata;
	char *var = &P->object[P->ofs + ofs];

	if (P->wire)
	{
		if (part->siz % part->align)
			return fuckoff(), 1;

		if (P->mode == CDATA_READ)
		{
			cdata_decode_array(P->wire, var, part->siz,
					part->align);
		}
		else
		{
			cdata_encode_array(P->wire, var, part->siz,
					part->align);
		}

		P->wire += part->siz;
	}

	P->size += part->siz;

	return 0;
}

static int
call_custom
(		struct packdef const * def,
		size_t ofs,
		void * udata)
{
	struct pack_state *P = udata;

	struct packdef_custom const *custom = def->custom;

	int i;

	char *var = &P->object[ofs];

	size_t out;

	for (i = 0; i < def->num_custom; ++i)
	{
		out = custom[i].cb(P->mode, P->wire,
				&var[custom[i].ofs]);

		if (P->wire)
			P->wire += out;

		P->size += out;
	}

	P->ofs = ofs;

	return cdata_iterate_shallow(def, call_part, P);
}

static size_t
iter_custom
(		struct pack_state * P,
		int mode,
		char * buffer)
{
	if (!P->object)
		return fuckoff(), 0;

	P->wire = buffer;
	P->mode = mode;
	P->size = 0;

	if (cdata_iterate_nest(P->def, call_custom, P))
		return 0;

	return P->size;
}

struct pack_state *
cdata_new (struct packdef const *def)
{
	struct pack_state *P = malloc(sizeof *P);

	if (!P)
		return fuckoff(), NULL;

	P->def = def;

	return P;
}

void
cdata_load
(		struct pack_state * P,
		void const * object)
{
	/* Force remove the const so this can operate
		read/write later; amazingly safe and proper code. */
	P->object = memchr(object, *(const char*)object, 1);
}

void
cdata_close (struct pack_state *P)
{
	free(P);
}

size_t
cdata_get_pack_size (struct pack_state *P)
{
	return iter_custom(P, CDATA_WRITE, NULL);
}

size_t
cdata_pack
(		struct pack_state * P,
		void * buffer)
{
	return iter_custom(P, CDATA_WRITE, buffer);
}

size_t
cdata_unpack
(		struct pack_state * P,
		void * buffer)
{
	return iter_custom(P, CDATA_READ, buffer);
}
