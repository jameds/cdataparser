/*
Copyright 2021 James R.
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
#include <string.h>
#include <ctype.h>

#define TOSTR2(x) #x
#define TOSTR(x) TOSTR2(x)

#define MAXSOURCELINE 4095
#define MAXINTID 63
#define MAXEXTID 31
#define MAXFIELD 1023
#define MAXNEST 63

enum {
	SEEK,/* looking for first annotation */
	PACKID,/* first annotation paren */
	TAG,/* struct/union/typedef */
	STRUCTNAME,/* stop if anonymous */
	FIELD,/* field type or closing brace */
	FIELDNAME,/* field name or semicolon */
	PACKVAR,/* PACKVAR parameter */
	NEST,/* PACKNEST parameter */
	TYPEDEF,/* after closing brace, use new type name */
} mode1 = SEEK;

enum {
	ALIGN_SCALAR = -1,
	ALIGN_ARRAY = -2,
};

#define NONE 0
int mode2 = NONE;

int inlinetag;
int nest;

char keyword[sizeof "struct"];
char tag[MAXINTID + 1];

char fields[MAXFIELD][MAXINTID + 1];
char extrafield[MAXFIELD][MAXEXTID + 1];
char fieldflag[MAXFIELD];
int alignment[MAXFIELD];
int numfields;
int numlinks[3];

int
iswordstart (int c)
{
	return c == '_' || isalpha(c);
}

int
iswordpart (int c)
{
	return iswordstart(c) || isdigit(c);
}

char *
skip_word (char *p)
{
	if (iswordstart(*p))
	{
		while (iswordpart(*++p))
			;
		return p;
	}
	else
		return &p[1];/* skip first character of nonword */
}

char *
skip_white (char *p)
{
	return p + strspn(p, "\t\n\v\f\r ");
}

int
streq
(		const char * p1,
		const char * p2,
		int n)
{
	return n == (int)strlen(p2) ? !memcmp(p1, p2, n) : 0;
}

int
iskeyword
(		char * p,
		int n)
{
	return streq(p, "struct", n) || streq(p, "union", n);
}

void
wordcpy
(		char * p,
		const char * s,
		int ns,
		int np)
{
	if (ns > np)
	{
		fprintf(stderr, "too long (>%d): %.*s\n",
				np, ns, s);
		exit(1);
	}

	memcpy(p, s, ns);
	p[ns] = '\0';
}

void
make_link
(		char * p,
		int n,
		int flag)
{
	fieldflag[numfields] = flag;
	wordcpy(extrafield[numfields], p, n, MAXEXTID);
	numlinks[flag]++;
}

int
check_nest (char *p)
{
	return *p == '{' ? (++nest, 0) : 1;
}

int
is_eol_mode (void)
{
	switch (mode1)
	{
#ifndef DEFAULT_NOPACK
		case FIELDNAME:
#endif
		case PACKVAR:
		case NEST:
			return 1;

		default:
			return 0;
	}
}

void
print_link
(		char * field,
		char * def)
{
	printf(
			"  {"
			" offsetof (%s %s, %s),"
			" &%s"
			" },\n",
			keyword, tag, field,
			def);
}

void
print_sizeof
(		const char * prefix,
		char * field)
{
	printf("%s((%s %s*)NULL)->%s",
			prefix, keyword, tag, field);
}

void
print_field
(		char * field,
		int align)
{
	printf("  { offsetof (%s %s, %s)",
			keyword, tag, field);

	if (align == 0)
	{
		puts(" 0, 0 },");
	}
	else
	{
		print_sizeof(", sizeof ", field);

		if (align < 0)
		{
			if (align == ALIGN_SCALAR)
			{
				print_sizeof(", sizeof ", field);
			}
			else if (align == ALIGN_ARRAY)
			{
				print_sizeof(", sizeof *", field);
			}
		}
		else
			printf(", %d", align);

		puts(" },");
	}
}

void
print_link_list
(		int flag,
		const char * type)
{
	int n = numlinks[flag];
	int i = 0;

	if (n)
	{
		printf(" (struct %s[]){\n", type);

		do
		{
			if (fieldflag[i] == flag)
			{
				print_link(fields[i], extrafield[i]);
				n--;
			}
			i++;
		}
		while (n);

		puts(" },");
	}
	else
		puts(" NULL,");
}

char *
parse_token (char *p)
{
	char *t = skip_word(p);
	int n = (t - p);

#ifdef DEBUG
	fprintf(stderr, "%d %1s %.*s\n",
			mode1, (char*)&mode2, n, p);
#endif

	if (nest)
	{
		if (*p == '{')
			nest++;
		else if (*p == '}')
			nest--;
	}
	else if (mode2)
	{
		switch (mode1)
		{
			case SEEK:
			case PACKID:
				if (*p != mode2)
				{
					fprintf(stderr, "expected '%c': %.*s\n",
							mode2, n, p);
					exit(1);
				}
		}

		if (is_eol_mode() && (*p == ',' || *p == ';'))
		{
			mode1 = *p == ',' ? FIELDNAME : FIELD;
			mode2 = NONE;

			if (fieldflag[numfields] == 0)
				numlinks[0]++;

			numfields++;
		}
		else if (*p == mode2)
		{
			mode2 = NONE;

			switch (mode1)
			{
				case SEEK:
					mode1 = PACKID;
					break;

				case PACKID:
					mode1 = TAG;
					break;

				case FIELDNAME:
					mode1 = FIELD;
					break;

				case PACKVAR:
				case NEST:
					if (*p == ')')
						mode2 = ';';
					break;
			}
		}
		else
		{
			/* skip nested structs */
			if (mode1 == FIELDNAME && check_nest(p))
			{
				if (*p == '[')
				{
					/* if this is an array get the sizeof each
						index */
					alignment[numfields] = ALIGN_ARRAY;
				}
				else if (streq(p, "PACK", n))
				{
					mode1 = PACKVAR;
					mode2 = '(';
				}
				else if (streq(p, "PACKTHIS", n))
				{
					mode1 = PACKVAR;
					mode2 = ';';
				}
				else if (streq(p, "PACKNEST", n))
				{
					mode1 = NEST;
					mode2 = '(';
				}
#ifndef DEFAULT_NOPACK
				else if (streq(p, "DONOTPACK", n))
				{
					mode1 = FIELD;
					mode2 = ';';
				}
#endif
			}
		}
	}
	else
	{
		switch (mode1)
		{
			case SEEK:
				if (streq(p, "PACKDEF", n))
				{
					mode2 = '(';
					numfields = 0;
					numlinks[0] = 0;
					numlinks[1] = 0;
					numlinks[2] = 0;
				}
				break;

			case PACKID:
				if (iswordstart(*p))
				{
					printf("struct packdef const %.*s = {\n",
							n, p);
					mode2 = ')';
				}
				else
				{
					fprintf(stderr, "not a word: %.*s\n",
							n, p);
					exit(1);
				}
				break;

			case TAG:
				if (streq(p, "typedef", n))
				{
					mode1 = FIELD;
					mode2 = '{';
					strcpy(keyword, "");
				}
				else if (iskeyword(p, n))
				{
					mode1 = STRUCTNAME;
					wordcpy(keyword, p, n, sizeof keyword);
				}
				break;

			case STRUCTNAME:
				wordcpy(tag, p, n, MAXINTID);
				mode1 = FIELD;
				mode2 = '{';
				break;

			case FIELD:
				if (*p == '}')
					mode1 = TYPEDEF;
				else if (!iskeyword(p, n))
					mode1 = FIELDNAME;
				break;

			case FIELDNAME:
				if (numfields == MAXFIELD)
				{
					fprintf(stderr,
							"too many fields (>%d): %.*s\n",
							MAXFIELD, n, p);
					exit(1);
				}

				if (check_nest(p))
				{
					wordcpy(fields[numfields], p, n, MAXINTID);
					fieldflag[numfields] = 0;
					alignment[numfields] = ALIGN_SCALAR;
					mode2 = ';';
				}
				break;

			case PACKVAR:
				if (iswordstart(*p))
					make_link(p, n, 2);
				else
					alignment[numfields] = atoi(p);

				mode2 = ')';
				break;

			case NEST:
				if (iswordstart(*p))
				{
					make_link(p, n, 1);
					mode2 = ')';
				}
				break;

			case TYPEDEF:
				/* for inline typedef, use the last token to
					skip any attributes */
				if (*p == ';')
				{
					int n = numlinks[0];
					int i = 0;

					print_link_list(1, "packdef_nest");
					print_link_list(2, "packdef_custom");

					if (n)
					{
						puts(" (struct packdef_part[]){");

						do
						{
							if (fieldflag[i] == 0)
							{
								print_field(fields[i],
										alignment[i]);
								n--;
							}
							i++;
						}
						while (n);

						puts(" },");
					}
					else
						puts(" NULL,");

					printf(" %d,%d,%d",
							numlinks[0],
							numlinks[1],
							numlinks[2]);

					puts("};");

					mode1 = SEEK;
				}
				else if (!strcmp(keyword, ""))
					wordcpy(tag, p, n, MAXINTID);
				break;
		}
	}

	return t;
}

int
main
(		int ac,
		char ** av)
{
	char buffer[MAXSOURCELINE + 1];

	puts("// This code was generated by cdataparser"
			" " TOSTR(COMMIT));

	while (fgets(buffer, sizeof buffer, stdin))
	{
		/* ignore preprocessor lines */
		if (buffer[0] != '#')
		{
			char *p = buffer;

			while (*(p = skip_white(p)))
				p = parse_token(p);
		}
	}

	if (feof(stdin))
		return 0;
	else
	{
		perror("");
		return 1;
	}
}
