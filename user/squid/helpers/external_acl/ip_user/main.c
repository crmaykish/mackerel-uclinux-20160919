/* $Id: main.c,v 1.2.2.1 2002/07/12 08:33:10 hno Exp $ 
* Copyright (C) 2002 Rodrigo Campos
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
* Author: Rodrigo Campos (rodrigo@geekbunker.org)
* 
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#include "ip_user.h"


void
usage (char *program_name)
{
  fprintf (stderr, "Usage:\n%s -f <configuration file>\n",
	   program_name);
}

int
main (int argc, char *argv[])
{
  FILE *FH;
  char *filename = NULL;
  char *program_name = argv[0];
  char *cp;
  char *username, *address;
  char line[BUFSIZE];
  struct ip_user_dict *current_entry;
  int ch;

  setvbuf (stdout, NULL, _IOLBF, 0);
  while ((ch = getopt (argc, argv, "f:")) != -1) {
    switch (ch) {
    case 'f':
      filename = optarg;
      break;
    default:
      usage (program_name);
      exit (1);
    }
  }
  if (filename == NULL) {	
    usage (program_name);
	exit(1);
  }
  FH = fopen (filename, "r");
  current_entry = load_dict (FH);

  while (fgets (line, sizeof (line), stdin)) {
    if ((cp = strchr (line, '\n')) != NULL) {
      *cp = '\0';
    }
    if ((cp = strtok (line, " \t")) != NULL) {
      address = cp;
      username = strtok (NULL, " \t");
    } else {
      fprintf (stderr, "helper: unable to read tokens\n");
      printf ("ERR\n");
      continue;
    }
#ifdef DEBUG
    printf ("result: %d\n",
	    dict_lookup (current_entry, username, address));
#endif
    if ((dict_lookup (current_entry, username, address)) != 0) {
      printf ("OK\n");
    } else {
      printf ("ERR\n");
    }


  }


  fclose (FH);
  return 0;
}
