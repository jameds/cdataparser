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

#include "cdata.h"

void
cdata_encode16
(		void * buffer,
		long val)
{
	unsigned char *p = buffer;

	p[0] = (val & 0xFF);
	p[1] = ((val >> 8) & 0xFF);
}

void
cdata_encode32
(		void * buffer,
		long val)
{
	unsigned char *p = buffer;

	cdata_encode16(p, val);
	cdata_encode16(&p[2], (val >> 16));
}

long
cdata_decode16 (const void *buffer)
{
	const unsigned char *p = buffer;

	return (p[0] | (p[1] << 8));
}

long
cdata_decode32 (const void *buffer)
{
	const unsigned char *p = buffer;

	return (cdata_decode16(p) |
			(cdata_decode16(&p[2]) << 16));
}

int
cdata_size_bytes (size_t n)
{
	n <<= 2;
	return (1 << ((n > 0xFC) + (n > 0xFFFC)));
}

void
cdata_encode_size
(		void * buffer,
		size_t val)
{
	const size_t enc = (val << 2);

	switch (cdata_size_bytes(val))
	{
		case 4:
			cdata_encode32(buffer, ((enc & 0xFFFFFFFF) | 2));
			break;

		case 2:
			cdata_encode16(buffer, ((enc & 0xFFFF) | 1));
			break;

		default:
			*(unsigned char*)buffer = (enc & 0xFF);
	}
}

size_t
cdata_decode_size (const void *buffer)
{
	const int enc = *(const unsigned char*)buffer;

	switch (enc & 3)
	{
		case 2:
			return (cdata_decode32(buffer) >> 2);
		case 1:
			return (cdata_decode16(buffer) >> 2);
		default:
			return (enc >> 2);
	}
}
