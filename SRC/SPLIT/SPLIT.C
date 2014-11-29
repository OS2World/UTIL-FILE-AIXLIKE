static char sccsid[]="@(#)87	1.1  src/split/split.c, aixlike.src, aixlike3  9/27/95  15:46:03";
/* split


   splits a file into pieces.

SYNTAX

   split -Number File Prefix Prefix

where

   -Number gives the number of lines to each piece of the output file
           set.  It defaults to 1000 lines.

   File    is the name of the file to be split

   Prefix  is the leading part of the output file name.  It defaults to 'x'.

DESCRIPTION

   The split command reads the specified file and writes it in Number-line
   pieces (default 1000 lines) to a set of output files.  The name of the
   first output file is constructed by combining the specified prefix
   (x by default) with aa, the second by combining the prefix with ab, and
   so on lexicongraphically, through zz (a maximum of 676 files).  You
   cannot specify a Prefix longer than 12 characters.

   For OS2, we check the first output file name.  If it is illegal for the
   file system, we exit.
*/

/*************************************************************************/
/* Maintentance History                                                  */
/*   Release 3   July, 1992                                              */
/* A1  09.16.92  grb  Make return code explicit.                         */
/*************************************************************************/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define OK  1
#define BAILOUT -1
#define MAXLEX 26
#define MAXLINE 32766

char ibuf[MAXLINE];
unsigned int number;
char *filename;
char *prefix;
char *lexstring = "abcdefghijklmnopqrstuvwxyz";

int init(int argc, char **argv);
int writefile(FILE *fi, int idx1, int idx2);
void tell_usage(void);
extern isValidFSName(char *filename);

int main(int argc, char **argv)                                 /* @A1c */
{
   int i1, i2;
   FILE *f;
   int rc;                                                      /* @A1a */

   number = 1000;       /* default */
   filename = NULL;
   prefix = "x";        /* default */

   rc = init(argc, argv);                                       /* @A1a */
   if (rc != BAILOUT)                                           /* @A1c */
     {
        if (filename)
          f = fopen(filename, "r");
        else
          f = stdin;
        if (f)
          {
            for (i1 = 0; i1 < MAXLEX; i1++)
              {
                 for (i2 = 0; i2 < MAXLEX; i2++)
                   if (writefile(f, i1, i2) == BAILOUT)
                     return(0);                                 /* @A1c */
              }
            fprintf(stderr,"split: more than %d output files required\n",
                                                               MAXLEX*MAXLEX);
          }
        else
          {                                                     /* @A1a */
             rc = BAILOUT;                                      /* @A1a */
             fprintf(stderr,"split: can not open input file %s\n", filename);
          }                                                     /* @A1a */
     }
   return(rc);                                                  /* @A1c */
}

/***************************************************************************/
/* init()                                                                  */
/*        parse and validate command line                                  */
/***************************************************************************/
int init(int argc, char **argv)
{
   int rc = OK;
   char *p;
   char path[262];
   int argpos;

   argpos = 1;
   if (argc > argpos)
     {
       p = argv[argpos];
       if (*p++ == '-')
         {
           number = atoi(p);
           if (number == 0)
             {
               tell_usage();
               rc = BAILOUT;
             }
           argpos++;
         }
       if (argc > argpos)
               {
                  if (strcmp(argv[argpos], "-"))
                    filename = argv[argpos++];
                  if (argc > argpos)
                    prefix = argv[argpos];
               }
     }

   if (strlen(prefix) > 259)
     {
       fprintf(stderr,"split: prefix %s is too long.\n", prefix);
       rc = BAILOUT;
     }
   else
     {
        strcpy(path, prefix);
        strcat(path, "xx");
        if (!isValidFSName(path))
          {
             fprintf(stderr,"split: can not create output files with prefix %s\n",
                              prefix);
             rc = BAILOUT;
          }
     }
   return(rc);
}

/*************************************************************************/
/* writefile()                                                           */
/*             write Number lines to an output file                      */
/*************************************************************************/
int writefile(FILE *fi, int idx1, int idx2)
{
   char filename[262];
   char *p;
   FILE *fo;
   unsigned int i;
   int rc;

   strcpy(filename, prefix);
   for (p = filename; *p; p++);
   *p++ = lexstring[idx1];
   *p++ = lexstring[idx2];
   *p = 0;
   rc = OK;
   fo = fopen(filename, "w");
   if (fo)
     {
       for (i = 0; i < number; i++)
         {
           p = fgets(ibuf, MAXLINE, fi);
           if (p)
             fputs(p, fo);
           else
             {
                rc = BAILOUT;
                break;
             }
         }
       fclose(fo);
     }
   else
     {
        fprintf(stderr,"split: unable to open output file %s\n", filename);
        rc = BAILOUT;
     }
   return(rc);
}

/*************************************************************************/
/* tell_usage()                                                          */
/*************************************************************************/
void tell_usage()
{
   printf("split - Copyright IBM, 1992\n");
   printf("Usage:\n");
   printf("    split -Number File prefix\n");
}
