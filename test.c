#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdata.h"

static size_t
pack_B_b
(		int mode,
		char * buffer,
		char * var)
{
	void **n = (void**)var;

	if (mode == CDATA_READ)
	{
		*n = (void*)(cdata_decode16(buffer) / 2);
	}
	else if (mode == CDATA_WRITE)
	{
		if (buffer)
		{
			cdata_encode16(buffer, (long)*n * 2);
		}
	}

	return 2;
}

PACKDEF (A_def) struct A {
	char a[4];
	int b DONOTPACK;
	int c[2];
};

PACKDEF (B_def) typedef struct {
	struct A a PACKNEST (A_def);
	int *b PACK (pack_B_b);
} B;

#ifndef cdp_stage
#include "test-def.c"
#endif

static void
enpack
(		struct pack_state * P,
		B * bruh)
{
	char *buffer = malloc(cdata_get_pack_size(P));
	size_t n = cdata_pack(P, buffer);
	fprintf(stderr, "%ld\n", n);
	fwrite(buffer, 1, n, stdout);
}

static void
unpack
(		struct pack_state * P,
		B * bruh,
		size_t n)
{
	char *buffer = malloc(n);
	fread(buffer, 1, n, stdin);
	cdata_unpack(P, buffer);
}

static int
help (void)
{
	fputs(
			"./test c > file # encode test\n"
			"./test d size < file # decode test\n",
			stderr);
	return 1;
}

static struct pack_state *
load (B *bruh)
{
	struct pack_state *P = cdata_new(&B_def);
	return cdata_load(P, bruh), P;
}

int
main
(		int ac,
		char ** av)
{
	B bruh;

	if (ac == 1)
		return help();

	if (!strcmp(av[1], "c"))
	{
		if (ac != 2)
			return help();

		strncpy(bruh.a.a, "Shit", 4);
		bruh.a.c[0] = 127;
		bruh.a.c[1] = 4096;
		bruh.b = (void*)33;
		enpack(load(&bruh), &bruh);
	}
	else if (!strcmp(av[1], "d"))
	{
		if (ac != 3)
			return help();

		unpack(load(&bruh), &bruh, atoi(av[2]));
		printf(
				"%.4s\n"
				"%d, %d\n"
				"%p\n",
				bruh.a.a,
				bruh.a.c[0],
				bruh.a.c[1],
				bruh.b);
	}

	return 0;
}
