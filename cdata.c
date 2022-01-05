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
(		struct pack_state * P,
		size_t k,
		int align,
		struct packdef_part const * part)
{
	char *p = &P->wire[k];
	char *var = &P->object[P->ofs + part->ofs + k];

	if (align == 2)
		*(int16_t*)var = cdata_decode16(p);
	else
		*(int32_t*)var = cdata_decode32(p);
}

static void
encode_align
(		struct pack_state * P,
		size_t k,
		int align,
		struct packdef_part const * part)
{
	char *p = &P->wire[k];
	char *var = &P->object[P->ofs + part->ofs + k];

	if (align == 2)
		cdata_encode16(p, *(int16_t*)var);
	else
		cdata_encode32(p, *(int32_t*)var);
}

static void
pack_array
(		struct pack_state * P,
		void(*pk)(struct pack_state*,
			size_t,int,struct packdef_part const*),
		struct packdef_part const * part)
{
	size_t k = 0;

	while (k < part->siz)
	{
		pk(P, k, part->align, part);
		k += part->align;
	}
}

static int
call_part
(		struct packdef_part const * part,
		size_t ofs,
		void * udata)
{
	struct pack_state *P = udata;

	if (P->wire)
	{
		if (part->align == 2 || part->align == 4)
		{
			if (part->siz % part->align)
				return fuckoff(), 1;

			pack_array(P, P->mode == CDATA_READ ?
					decode_align : encode_align, part);
		}
		else if (part->siz == 2 || part->siz == 4)
		{
			(P->mode == CDATA_READ ?
					decode_align :
					encode_align)(P, 0, part->siz, part);
		}
		else
		{
			memcpy(P->wire, &P->object[P->ofs + ofs],
					part->siz);
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
