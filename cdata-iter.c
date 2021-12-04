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

struct cdata_iterate {
	cdata_iterator cb;
	void * udata;
};

static int
iter_part
(		struct packdef const * def,
		size_t ofs,
		void * udata)
{
	struct cdata_iterate const * it = udata;

	struct packdef_part const * part = def->parts;

	int i;

	for (i = 0; i < def->num_parts; ++i)
	{
		if (it->cb(&part[i], ofs + part[i].ofs,
					it->udata))
			return 1;
	}

	return 0;
}

static int
iter_from
(		struct packdef const * def,
		cdata_nest_iterator cb,
		void * udata,
		size_t ofs)
{
	struct packdef_nest const * nest = def->nest;

	int i;

	if (cb(def, ofs, udata))
		return 1;

	for (i = 0; i < def->num_nest; ++i)
	{
		if (iter_from(nest[i].def, cb, udata,
					ofs + nest[i].ofs))
			return 1;
	}

	return 0;
}

int
cdata_iterate
(		struct packdef const * def,
		cdata_iterator cb,
		void * udata)
{
	struct cdata_iterate it = { cb, udata };

	return cdata_iterate_nest(def, iter_part, &it);
}

int
cdata_iterate_shallow
(		struct packdef const * def,
		cdata_iterator cb,
		void * udata)
{
	struct cdata_iterate it = { cb, udata };

	return iter_part(def, 0, &it);
}

int
cdata_iterate_nest
(		struct packdef const * def,
		cdata_nest_iterator cb,
		void * udata)
{
	return iter_from(def, cb, udata, 0);
}
