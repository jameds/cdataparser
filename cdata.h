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

/*
How to use these macros in your source code:

PACKDEF(id)
Marks a structure definition to generate a packdef. All
struct members, including those of nested anonymous
structs, are included by default. Pointers are excluded
except when using a custom packing function or PACKNEST.

	PACKDEF (my_def) struct my_struct {
	};

PACK(alignment)
Manually set the alignment of a struct member, integer >1.

	// a now has 2-byte alignment, regardless of type.
	int a[8] PACK (2);

PACK(custom packing function)
Use a custom callback to pack this struct member. This
allows for packing variable length data. See the typedef
for cdata_packer.

	int b PACK (pack_b_member);

PACKNEST(packdef id)
Reuse a packdef for this struct member.

	struct my_other_struct c PACKNEST (my_other_def);

DONOTPACK
Exclude this struct member from the packdef.

	int d DONOTPACK;

PACKTHIS
Explicitly include a struct member in the packdef. This is
only required if cdp was compiled with DEFAULT_NOPACK
defined.

	int d PACKTHIS;
*/

#ifndef cdataparser_cdata_H
#define cdataparser_cdata_H

#include <stddef.h>/* offsetof */

/* The cdp annotations expand to nothing during normal
   compilation. Define CDATAPARSER_STAGE during
   preprocessing, before passing to cdp. */
#ifndef CDATAPARSER_STAGE
#define PACKDEF(x)
#define PACK(x)
#define PACKNEST(x)
#define PACKTHIS
#define DONOTPACK
#endif

#define CDATA_MAX_CUSTOM_SIZE (0xFFFFFFFC)/* 30 bits */

struct packdef;
struct packdef_custom;

struct packdef_part {
	size_t ofs;
	size_t siz;
	unsigned int align;
};

struct packdef_nest {
	size_t ofs;
	struct packdef const * def;
};

struct packdef {
	struct packdef_nest const * nest;
	struct packdef_custom const * custom;
	struct packdef_part const * parts;

	int num_parts;
	int num_nest;
	int num_custom;
};

/* return nonzero to stop iterating */
typedef int(*cdata_iterator)(struct packdef_part const*,
		size_t offset, void *userdata);

typedef int(*cdata_nest_iterator)(struct packdef const*,
		size_t offset, void *userdata);

/* returns nonzero if the iterator ever returns nonzero */
int cdata_iterate (struct packdef const*, cdata_iterator,
		void *userdata);

/* do not descend into nest */
int cdata_iterate_shallow (struct packdef const*,
		cdata_iterator, void *userdata);

/* the first iteration is the root packdef */
int cdata_iterate_nest (struct packdef const*,
		cdata_nest_iterator, void *userdata);

/*
HIGH LEVEL PACKING API
*/

/* cdata_packer modes */
enum {
	CDATA_READ,
	CDATA_WRITE,
};

/*
machine points to the struct member being (un)packed.

The return value is the number of bytes r/w.

In CDATA_READ mode, wire points to already encoded data.

In CDATA_WRITE mode, wire may be NULL or point to
a buffer, into which encoded data must be written.
*/
typedef size_t(*cdata_packer)(int mode,
		char *wire, char *machine);

struct packdef_custom {
	size_t ofs;
	cdata_packer cb;
};

typedef void(*cdata_error_handler)(
		const char *file, int line);

/* Set NULL to use the default, which aborts. */
void cdata_set_error_handler (cdata_error_handler);

/* May return NULL. */
struct pack_state * cdata_new (struct packdef const*,
		void const *object);

void cdata_close (struct pack_state*);

/* Returns the actual/expected size of a buffer. */
size_t cdata_get_pack_size (struct pack_state*);

/* These functions return the number of bytes written
   to/read from the buffer. */
size_t cdata_pack (struct pack_state*, void *buffer);
size_t cdata_unpack (struct pack_state*, void *buffer);

void cdata_encode16 (void *buffer, long val);
void cdata_encode32 (void *buffer, long val);

long cdata_decode16 (const void *buffer);
long cdata_decode32 (const void *buffer);

/*
VARIABLE LENGTH DATA
*/

/* Returns the number of bytes used to encode a size
   value. */
int cdata_size_bytes (size_t);

/* Ditto return value as cdata_size_bytes. */
int cdata_encode_size (void *buffer, size_t val);

size_t cdata_decode_size (const void *buffer);

#endif/*cdataparser_cdata_H*/
