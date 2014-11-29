static char sccsid[]="@(#)89	1.2  src/strings/strings.c, aixlike.src, aixlike3  1/4/96  23:34:24";
/*************************************************/
/* Author: G.R. Blair                            */
/*         BOBBLAIR @ AUSVM1                     */
/*         bobblair@bobblair.austin.ibm.com      */
/*                                               */
/* The following code is property of             */
/* International Business Machines Corporation.  */
/*                                               */
/* Copyright International Business Machines     */
/* Corporation, 1992, 1996  All rights reserved. */
/*************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 3      26Mar92                                           */
/* A1  09.16.92  grb   Make Return code explicit                         */
/*     01.03.96  gcw   Glob filenames with setargv--CMVC 14179           */
/*-----------------------------------------------------------------------*/
/* strings - finds printable ascii strings in a binary file */
/*
   Syntax

          strings [-] [-a] [-o] [-number] [file] [file] . . .

   Description

       The strings command looks for ascii strings in a binary file.  A
       string is any sequence of 4 or more printing characters ending
       with a newline or a null.  The strings command is useful for
       identifying random object files.

   Flags

       -a or -     Searches the entire file, not just data section, for
                   ascii strings.  (This is always done in OS/2).

       -o          Lists each string preceded by its offset in the file
                   (in octal).

       -number     Specified minimum string length rather than 4.

   Examples

       1. Searching a file

              strings strings

          outputs

              L@(#)strings.c

       2. Searches for strings at least 12 characters long

              strings -12 strings

       3. Searches for strings at least 20 characters long and also shows
          offset in file.

              strings -o -2- /usr/bin/strings

          outputs

              7036 1.10 com/cmd/scan, 3.1 8946H 12/14/91 17:00;24

*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "strings.h"

/***************************  Global Variables ****************************/
char buf[MAXLINE + 1];
int searchall, showdisp, minlen;



/********************************* MAIN **********************************/
int main(int argc, char **argv)
{                                                                  /* @A1c */
   int filearg;
   int rc;                                                         /* @A1a */
   int fileargc;
   char **fileargv;

   rc = 0;                                                         /* @A1a */
   fileargc = argc;
   fileargv = argv;
   filearg = init(&fileargc, &fileargv);
   if (filearg != BAILOUT)
     if (filearg)
       search_the_files(fileargc, fileargv);
     else
       search_stdin();
   else                                                            /* @A1a */
     rc = BAILOUT;                                                 /* @A1a */
   return(rc);                                                     /* @A1a */
}

int init (int *argc, char ***argv)
{
   int i;
   char *p;

   searchall = NO;
   showdisp =  NO;
   minlen   =  DEFAULT_MINLEN;
   for ((*argc)--, (*argv)++; *argc > 0; (*argc)--, (*argv)++)
      {
         p = **argv;
         if (*p == '-' || *p == '/')
           {
              p++;
              if (*p == '\0' || *p == 'a')
                searchall = YES;
              else
                if (*p == 'o')
                  showdisp = YES;
                else
                  {
                     minlen = atoi(p);
                     if (minlen <= 0)
                       {
                          tell_usage();
                          return(BAILOUT);
                       }
                  }
           }
         else
           return(*argc);
       }
    return(0);
}

void search_the_files(int argc, char **argv)
{
   FILE *f;

   for (; argc > 0; argc--, argv++) {
      f = fopen(*argv, "rb");
      if (f)
        {
          do_search(f);
          fclose(f);
        }
      else
        fprintf(stderr, "strings: Could not open input file %s\n", *argv);
   }
}

void search_stdin()
{
   do_search(stdin);
}

void do_search(FILE *f)
{
   int c;
   int i = 0;
   unsigned long offset = 0;
   unsigned long start;

   while  ( (c = fgetc(f)) != EOF)
     {
       if (isprint(c))
         {
           if (i == 0)
             start = offset;
           buf[i++] = (char)c;
           if (i >= MAXLINE)
             {
                buf[i] = '\0';
                printit(buf, start);
                i = 0;
             }
         }
       else
         if (c == 0 || c == CR || c == LF)
           {
              if (i >= minlen)
                {
                  buf[i] = '\0';
                  printit(buf, start);
                }
              i = 0;
           }
         else
           i = 0;
       offset++;
     }
}


void printit(char *line, unsigned long offset)
{
   if (showdisp == YES)
     printf("%o ", offset);
   printf("%s\n", line);
}

void tell_usage()
{

   printf("         Copyright IBM, 1996\n");
   printf("strings: finds printable strings in object or binary file\n");

   printf("\nUsage:    strings [-] [-a] [-o] [-number] [file] [file] . . .\n");
   printf("Flags:\n");
   printf("      -a or -     Searches the entire file, not just data section, for\n");
   printf("                     ascii strings.  (This is always done in OS/2).\n");
   printf("      -o          Lists each string preceded by its offset in the file\n");
   printf("                     (in octal).\n");
   printf("      -number     Specified minimum string length rather than 4.\n");
}
