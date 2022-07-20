
/*
 * $Id: cf_gen.c,v 1.10.6.2 2002/08/22 12:30:59 squidadm Exp $
 *
 * DEBUG: none          Generate squid.conf.default and cf_parser.h
 * AUTHOR: Max Okumoto
 *
 * SQUID Web Proxy Cache          http://www.squid-cache.org/
 * ----------------------------------------------------------
 *
 *  Squid is the result of efforts by numerous individuals from
 *  the Internet community; see the CONTRIBUTORS file for full
 *  details.   Many organizations have provided support for Squid's
 *  development; see the SPONSORS file for full details.  Squid is
 *  Copyrighted (C) 2001 by the Regents of the University of
 *  California; see the COPYRIGHT file for full details.  Squid
 *  incorporates software developed and/or copyrighted by other
 *  sources; see the CREDITS file for full details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */

/*****************************************************************************
 * Abstract:	This program parses the input file and generates code and
 *		files used to configure the variables in squid.
 *		(ie it creates the squid.conf.default file from the cf.data file)
 *
 *		The output files are as follows:
 *		cf_parser.h - this file contains, default_all() which
 *			  initializes variables with the default
 *			  values, parse_line() that parses line from
 *			  squid.conf.default, dump_config that dumps the
 *			  current the values of the variables.
 *		squid.conf.default - default configuration file given to the server
 *			 administrator.
 *****************************************************************************/

#include "config.h"
#include "cf_gen_defines.h"

#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if defined(_SQUID_CYGWIN_)
#include <io.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "util.h"
#include "../lib/util.c"

#define MAX_LINE	1024	/* longest configuration line */
#define _PATH_PARSER		"cf_parser.h"
#define _PATH_SQUID_CONF	"squid.conf.default"

enum State {
    sSTART,
    s1,
    sDOC,
    sNOCOMMENT,
    sEXIT
};

typedef struct Line {
    char *data;
    struct Line *next;
} Line;

typedef struct EntryAlias {
    struct EntryAlias *next;
    char *name;
} EntryAlias;

typedef struct Entry {
    char *name;
    EntryAlias *alias;
    char *type;
    char *loc;
    char *default_value;
    Line *default_if_none;
    char *comment;
    char *ifdef;
    Line *doc;
    Line *nocomment;
    int array_flag;
    struct Entry *next;
} Entry;


static const char WS[] = " \t";
static int gen_default(Entry *, FILE *);
static void gen_parse(Entry *, FILE *);
static void gen_dump(Entry *, FILE *);
static void gen_free(Entry *, FILE *);
static void gen_conf(Entry *, FILE *);
static void gen_default_if_none(Entry *, FILE *);

static void
lineAdd(Line ** L, const char *str)
{
    while (*L)
	L = &(*L)->next;
    *L = xcalloc(1, sizeof(Line));
    (*L)->data = xstrdup(str);
}

int
main(int argc, char *argv[])
{
    FILE *fp;
    char *input_filename = argv[1];
    const char *output_filename = _PATH_PARSER;
    const char *conf_filename = _PATH_SQUID_CONF;
    int linenum = 0;
    Entry *entries = NULL;
    Entry *curr = NULL;
    enum State state;
    int rc = 0;
    char *ptr = NULL;
#ifdef _SQUID_OS2_
    const char *rmode = "rt";
#else
    const char *rmode = "r";
#endif

    /*-------------------------------------------------------------------*
     * Parse input file
     *-------------------------------------------------------------------*/

    /* Open input file */
    if ((fp = fopen(input_filename, rmode)) == NULL) {
	perror(input_filename);
	exit(1);
    }
#if defined(_SQUID_CYGWIN_)
    setmode(fileno(fp), O_TEXT);
#endif
    state = sSTART;
    while (feof(fp) == 0 && state != sEXIT) {
	char buff[MAX_LINE];
	char *t;
	if (NULL == fgets(buff, MAX_LINE, fp))
	    break;
	linenum++;
	if ((t = strchr(buff, '\n')))
	    *t = '\0';
	switch (state) {
	case sSTART:
	    if ((strlen(buff) == 0) || (!strncmp(buff, "#", 1))) {
		/* ignore empty and comment lines */
		(void) 0;
	    } else if (!strncmp(buff, "NAME:", 5)) {
		char *name, *aliasname;
		if ((name = strtok(buff + 5, WS)) == NULL) {
		    printf("Error in input file\n");
		    exit(1);
		}
		curr = calloc(1, sizeof(Entry));
		curr->name = xstrdup(name);
		while ((aliasname = strtok(NULL, WS)) != NULL) {
		    EntryAlias *alias = calloc(1, sizeof(EntryAlias));
		    alias->next = curr->alias;
		    alias->name = xstrdup(aliasname);
		    curr->alias = alias;
		}
		state = s1;
	    } else if (!strcmp(buff, "EOF")) {
		state = sEXIT;
	    } else if (!strcmp(buff, "COMMENT_START")) {
		curr = calloc(1, sizeof(Entry));
		curr->name = xstrdup("comment");
		curr->loc = xstrdup("none");
		state = sDOC;
	    } else {
		printf("Error on line %d\n", linenum);
		printf("--> %s\n", buff);
		exit(1);
	    }
	    break;

	case s1:
	    if ((strlen(buff) == 0) || (!strncmp(buff, "#", 1))) {
		/* ignore empty and comment lines */
		(void) 0;
	    } else if (!strncmp(buff, "COMMENT:", 8)) {
		ptr = buff + 8;
		while (xisspace(*ptr))
		    ptr++;
		curr->comment = xstrdup(ptr);
	    } else if (!strncmp(buff, "DEFAULT:", 8)) {
		ptr = buff + 8;
		while (xisspace(*ptr))
		    ptr++;
		curr->default_value = xstrdup(ptr);
	    } else if (!strncmp(buff, "DEFAULT_IF_NONE:", 16)) {
		ptr = buff + 16;
		while (xisspace(*ptr))
		    ptr++;
		lineAdd(&curr->default_if_none, ptr);
	    } else if (!strncmp(buff, "LOC:", 4)) {
		if ((ptr = strtok(buff + 4, WS)) == NULL) {
		    printf("Error on line %d\n", linenum);
		    exit(1);
		}
		curr->loc = xstrdup(ptr);
	    } else if (!strncmp(buff, "TYPE:", 5)) {
		if ((ptr = strtok(buff + 5, WS)) == NULL) {
		    printf("Error on line %d\n", linenum);
		    exit(1);
		}
		/* hack to support arrays, rather than pointers */
		if (0 == strcmp(ptr + strlen(ptr) - 2, "[]")) {
		    curr->array_flag = 1;
		    *(ptr + strlen(ptr) - 2) = '\0';
		}
		curr->type = xstrdup(ptr);
	    } else if (!strncmp(buff, "IFDEF:", 6)) {
		if ((ptr = strtok(buff + 6, WS)) == NULL) {
		    printf("Error on line %d\n", linenum);
		    exit(1);
		}
		curr->ifdef = xstrdup(ptr);
	    } else if (!strcmp(buff, "DOC_START")) {
		state = sDOC;
	    } else if (!strcmp(buff, "DOC_NONE")) {
		/* add to list of entries */
		curr->next = entries;
		entries = curr;
		state = sSTART;
	    } else {
		printf("Error on line %d\n", linenum);
		exit(1);
	    }
	    break;

	case sDOC:
	    if (!strcmp(buff, "DOC_END") || !strcmp(buff, "COMMENT_END")) {
		Line *head = NULL;
		Line *line = curr->doc;
		/* reverse order of doc lines */
		while (line != NULL) {
		    Line *tmp;
		    tmp = line->next;
		    line->next = head;
		    head = line;
		    line = tmp;
		}
		curr->doc = head;
		/* add to list of entries */
		curr->next = entries;
		entries = curr;
		state = sSTART;
	    } else if (!strcmp(buff, "NOCOMMENT_START")) {
		state = sNOCOMMENT;
	    } else {
		Line *line = calloc(1, sizeof(Line));
		line->data = xstrdup(buff);
		line->next = curr->doc;
		curr->doc = line;
	    }
	    break;

	case sNOCOMMENT:
	    if (!strcmp(buff, "NOCOMMENT_END")) {
		Line *head = NULL;
		Line *line = curr->nocomment;
		/* reverse order of lines */
		while (line != NULL) {
		    Line *tmp;
		    tmp = line->next;
		    line->next = head;
		    head = line;
		    line = tmp;
		}
		curr->nocomment = head;
		state = sDOC;
	    } else {
		Line *line = calloc(1, sizeof(Line));
		line->data = xstrdup(buff);
		line->next = curr->nocomment;
		curr->nocomment = line;
	    }
	    break;

	case sEXIT:
	    assert(0);		/* should never get here */
	    break;
	}
    }
    if (state != sEXIT) {
	printf("Error unexpected EOF\n");
	exit(1);
    } else {
	/* reverse order of entries */
	Entry *head = NULL;

	while (entries != NULL) {
	    Entry *tmp;

	    tmp = entries->next;
	    entries->next = head;
	    head = entries;
	    entries = tmp;
	}
	entries = head;
    }
    fclose(fp);

    /*-------------------------------------------------------------------*
     * Generate default_all()
     * Generate parse_line()
     * Generate dump_config()
     * Generate free_all()
     * Generate example squid.conf.default file
     *-------------------------------------------------------------------*/

    /* Open output x.c file */
    if ((fp = fopen(output_filename, "w")) == NULL) {
	perror(output_filename);
	exit(1);
    }
#if defined(_SQUID_CYGWIN_)
    setmode(fileno(fp), O_TEXT);
#endif
    fprintf(fp,
	"/*\n"
	" * Generated automatically from %s by %s\n"
	" *\n"
	" * Abstract: This file contains routines used to configure the\n"
	" *           variables in the squid server.\n"
	" */\n"
	"\n",
	input_filename, argv[0]
	);
    rc = gen_default(entries, fp);
    gen_default_if_none(entries, fp);
    gen_parse(entries, fp);
    gen_dump(entries, fp);
    gen_free(entries, fp);
    fclose(fp);

    /* Open output x.conf file */
    if ((fp = fopen(conf_filename, "w")) == NULL) {
	perror(conf_filename);
	exit(1);
    }
#if defined(_SQUID_CYGWIN_)
    setmode(fileno(fp), O_TEXT);
#endif
    gen_conf(entries, fp);
    fclose(fp);

    return (rc);
}

static int
gen_default(Entry * head, FILE * fp)
{
    Entry *entry;
    int rc = 0;
    fprintf(fp,
	"static void\n"
	"default_line(const char *s)\n"
	"{\n"
	"\tLOCAL_ARRAY(char, tmp_line, BUFSIZ);\n"
	"\txstrncpy(tmp_line, s, BUFSIZ);\n"
	"\txstrncpy(config_input_line, s, BUFSIZ);\n"
	"\tconfig_lineno++;\n"
	"\tparse_line(tmp_line);\n"
	"}\n"
	);
    fprintf(fp,
	"static void\n"
	"default_all(void)\n"
	"{\n"
	"\tcfg_filename = \"Default Configuration\";\n"
	"\tconfig_lineno = 0;\n"
	);
    for (entry = head; entry != NULL; entry = entry->next) {
	assert(entry->name);
	assert(entry != entry->next);

	if (!strcmp(entry->name, "comment"))
	    continue;
	if (entry->loc == NULL) {
	    fprintf(stderr, "NO LOCATION FOR %s\n", entry->name);
	    rc |= 1;
	    continue;
	}
	if (entry->default_value == NULL) {
	    fprintf(stderr, "NO DEFAULT FOR %s\n", entry->name);
	    rc |= 1;
	    continue;
	}
	assert(entry->default_value);
	if (entry->ifdef)
	    fprintf(fp, "#if %s\n", entry->ifdef);
	if (strcmp(entry->default_value, "none") == 0) {
	    fprintf(fp, "\t/* No default for %s */\n", entry->name);
	} else {
	    fprintf(fp, "\tdefault_line(\"%s %s\");\n",
		entry->name,
		entry->default_value);
	}
	if (entry->ifdef)
	    fprintf(fp, "#endif\n");
    }
    fprintf(fp, "\tcfg_filename = NULL;\n");
    fprintf(fp, "}\n\n");
    return rc;
}

static void
gen_default_if_none(Entry * head, FILE * fp)
{
    Entry *entry;
    Line *line;
    fprintf(fp,
	"static void\n"
	"defaults_if_none(void)\n"
	"{\n"
	);
    for (entry = head; entry != NULL; entry = entry->next) {
	assert(entry->name);
	assert(entry->loc);
	if (entry->default_if_none == NULL)
	    continue;
	if (entry->ifdef)
	    fprintf(fp, "#if %s\n", entry->ifdef);
	if (entry->default_if_none) {
	    fprintf(fp,
		"\tif (check_null_%s(%s)) {\n",
		entry->type,
		entry->loc);
	    for (line = entry->default_if_none; line; line = line->next)
		fprintf(fp,
		    "\t\tdefault_line(\"%s %s\");\n",
		    entry->name,
		    line->data);
	    fprintf(fp, "\t}\n");
	}
	if (entry->ifdef)
	    fprintf(fp, "#endif\n");
    }
    fprintf(fp, "}\n\n");
}

static void
gen_parse(Entry * head, FILE * fp)
{
    Entry *entry;
    char *name;
    EntryAlias *alias;

    fprintf(fp,
	"static int\n"
	"parse_line(char *buff)\n"
	"{\n"
	"\tint\tresult = 1;\n"
	"\tchar\t*token;\n"
	"\tdebug(0,10)(\"parse_line: %%s\\n\", buff);\n"
	"\tif ((token = strtok(buff, w_space)) == NULL)\n"
	"\t\t(void) 0;\t/* ignore empty lines */\n"
	);

    for (entry = head; entry != NULL; entry = entry->next) {
	if (strcmp(entry->name, "comment") == 0)
	    continue;
	if (entry->ifdef)
	    fprintf(fp, "#if %s\n", entry->ifdef);
	name = entry->name;
	alias = entry->alias;
      next_alias:
	fprintf(fp, "\telse if (!strcmp(token, \"%s\"))\n", name);
	assert(entry->loc);
	if (strcmp(entry->loc, "none") == 0) {
	    fprintf(fp,
		"\t\tparse_%s();\n",
		entry->type
		);
	} else {
	    fprintf(fp,
		"\t\tparse_%s(&%s%s);\n",
		entry->type, entry->loc,
		entry->array_flag ? "[0]" : ""
		);
	}
	if (alias) {
	    name = alias->name;
	    alias = alias->next;
	    goto next_alias;
	}
	if (entry->ifdef)
	    fprintf(fp, "#endif\n");
    }

    fprintf(fp,
	"\telse\n"
	"\t\tresult = 0; /* failure */\n"
	"\treturn(result);\n"
	"}\n\n"
	);
}

static void
gen_dump(Entry * head, FILE * fp)
{
    Entry *entry;
    fprintf(fp,
	"static void\n"
	"dump_config(StoreEntry *entry)\n"
	"{\n"
	);
    for (entry = head; entry != NULL; entry = entry->next) {
	assert(entry->loc);
	if (strcmp(entry->loc, "none") == 0)
	    continue;
	if (strcmp(entry->name, "comment") == 0)
	    continue;
	if (entry->ifdef)
	    fprintf(fp, "#if %s\n", entry->ifdef);
	fprintf(fp, "\tdump_%s(entry, \"%s\", %s);\n",
	    entry->type,
	    entry->name,
	    entry->loc);
	if (entry->ifdef)
	    fprintf(fp, "#endif\n");
    }
    fprintf(fp, "}\n\n");
}

static void
gen_free(Entry * head, FILE * fp)
{
    Entry *entry;
    fprintf(fp,
	"static void\n"
	"free_all(void)\n"
	"{\n"
	);
    for (entry = head; entry != NULL; entry = entry->next) {
	assert(entry->loc);
	if (strcmp(entry->loc, "none") == 0)
	    continue;
	if (strcmp(entry->name, "comment") == 0)
	    continue;
	if (entry->ifdef)
	    fprintf(fp, "#if %s\n", entry->ifdef);
	fprintf(fp, "\tfree_%s(&%s%s);\n",
	    entry->type, entry->loc,
	    entry->array_flag ? "[0]" : "");
	if (entry->ifdef)
	    fprintf(fp, "#endif\n");
    }
    fprintf(fp, "}\n\n");
}

static int
defined(char *name)
{
    int i = 0;
    if (!name)
	return 1;
    for (i = 0; strcmp(defines[i].name, name) != 0; i++) {
	assert(defines[i].name);
    }
    return defines[i].defined;
}

static const char *
available_if(char *name)
{
    int i = 0;
    assert(name);
    for (i = 0; strcmp(defines[i].name, name) != 0; i++) {
	assert(defines[i].name);
    }
    return defines[i].enable;
}

static void
gen_conf(Entry * head, FILE * fp)
{
    Entry *entry;
    char buf[8192];
    Line *def = NULL;

    for (entry = head; entry != NULL; entry = entry->next) {
	Line *line;
	int blank = 1;

	if (!strcmp(entry->name, "comment"))
	    (void) 0;
	else
	    fprintf(fp, "#  TAG: %s", entry->name);
	if (entry->comment)
	    fprintf(fp, "\t%s", entry->comment);
	fprintf(fp, "\n");
	if (!defined(entry->ifdef)) {
	    fprintf(fp, "# Note: This option is only available if Squid is rebuilt with the\n");
	    fprintf(fp, "#       %s option\n#\n", available_if(entry->ifdef));
	}
	for (line = entry->doc; line != NULL; line = line->next) {
	    fprintf(fp, "#%s\n", line->data);
	}
	if (entry->default_value && strcmp(entry->default_value, "none") != 0) {
	    sprintf(buf, "%s %s", entry->name, entry->default_value);
	    lineAdd(&def, buf);
	}
	if (entry->default_if_none) {
	    for (line = entry->default_if_none; line; line = line->next) {
		sprintf(buf, "%s %s", entry->name, line->data);
		lineAdd(&def, buf);
	    }
	}
	if (entry->nocomment)
	    blank = 0;
	if (!def && entry->doc && !entry->nocomment &&
	    strcmp(entry->name, "comment") != 0)
	    lineAdd(&def, "none");
	if (def && (entry->doc || entry->nocomment)) {
	    if (blank)
		fprintf(fp, "#\n");
	    fprintf(fp, "#Default:\n");
	    while (def != NULL) {
		line = def;
		def = line->next;
		fprintf(fp, "# %s\n", line->data);
		xfree(line->data);
		xfree(line);
	    }
	    blank = 1;
	}
	if (entry->nocomment && blank)
	    fprintf(fp, "#\n");
	for (line = entry->nocomment; line != NULL; line = line->next) {
	    fprintf(fp, "%s\n", line->data);
	}
	if (entry->doc != NULL) {
	    fprintf(fp, "\n");
	}
    }
}
