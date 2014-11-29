static char sccsid[]="@(#)08	1.1  src/wc/wc.c, aixlike.src, aixlike3  9/27/95  15:46:49";
/************************************************/
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation, 1991.  All rights reserved.     */
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 1       5/1/91                                           */
/* @1 05.08.91 changed fmf_init to look for all files including hidden   */
/* @2 09.16.92 made return code explicit                                 */
/* @3 09.12.95 added freopen of stdin to binary mode                     */
/*-----------------------------------------------------------------------*/

/* The wc command counts the number of lines, words, or characters in File or
   in the standard input if you do not specify any Files.  The command writes
   The results to standard output and keeps a total count for all named files.
   A word is defined as a string of characters delimted by spaces, tabs, or
   new-line characters.  By default, the wc command counts lines, words,
   and characters (as bytes).

   Usage:
     wc [-c]||[-cl]||[-cw]||[-cwl]||[-l]||[-lw]||[-w]||[-k] {File ...

   Flags:
          -c          Count bytes.
          -l          Count lines.
          -w          Count words.
          -k          Count actual characters, not bytes.  Specifying the -k
                      flag is equivalent to specifying -kclw.
          -s          (OS2 only). Specifies that Files matching the pattern
                      are searched for in subdirectories of the specified
                      directory.

   The order of the -c, -w and -l flags is important.  It determines the
   order of the columns.  Default is "lines" "words" "characters".

*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#define  INCL_BASE
#define  INCL_NOPM
#include <os2.h>
#include "fmf.h"

#define ISAMBIGUOUS(x) ( (strchr(argv[optind], '*') != 0) || \
                         (strchr(argv[optind], '?') != 0) )
#define YES 1
#define NO 0
#define BAILOUT -1
#define CR  0x0D
#define LF  0x0A
#define TAB 0x09
#define SPACE 0x20
#define BUFSIZE 50000
#define DEFAULT_COLUMNS "LWC"

unsigned long total_lines, total_words, total_bytes, total_chars;
int count_chars, showname;
int subtree_search;
char *opstring = "cklsw";
char buf[BUFSIZE];
char head_pattern[4];

/* Function Prototypes */

int init(int argc, char **argv);
unsigned int dofilespec(char *filespec);
int dofile(char *filename);
void dostream(FILE *stream, char *filename);
void prtline(unsigned long, unsigned long, unsigned long, unsigned long,
             char *);
void prttotals(void);
void tell_usage(void);
extern int getopt(int argc, char **argv, char *ops);
extern int optind;

int main(argc, argv, envp)                                      /* @2c */
   int argc;
   char *argv[];
   char *envp[];
{
   int fnindex;
   unsigned int filesdone = 0;

   if ( (fnindex = init(argc, argv)) != BAILOUT)
     if (fnindex)      /* if at least one filespec given */
       {
         for (; fnindex < argc; fnindex++)
           filesdone += dofilespec(argv[fnindex]);
         if (filesdone > 1)
           prttotals();
       }
     else
       {							/* @3a */
         freopen("", "rb", stdin);				/* @3a */
         dostream(stdin, "STDIN");
       }							/* @3a */
   else                                                         /* @2a */
     return(BAILOUT);                                           /* @2a */
   return(0);                                                   /* @2a */
}

int init(int argc, char **argv)
{
#ifdef I16
   char c;
#else
   int c;
#endif
   char *p;
   struct stat sbuf;
   int swnum;

   total_lines = total_words = total_bytes = total_chars = 0;
   showname = NO;
   *head_pattern = '\0'; /* default is to print lines, words, bytes */
   count_chars = NO;


        swnum = 0;
#ifdef I16
        while ( (c = (char)getopt(argc, argv, opstring)) != EOF)
#else
        while ( (c = getopt(argc, argv, opstring)) != EOF)
#endif
          {
            switch (toupper(c))
            {
               case 'C'  :   for (p = head_pattern; *p; p++)
                               if (*p == c)
                                 {
                                    tell_usage();
                                    return(BAILOUT);
                                 }
                             head_pattern[swnum++] = (char)toupper(c);
                             head_pattern[swnum] = 0;
                         break;
               case 'L'  :   for (p = head_pattern; *p; p++)
                               if (*p == c)
                                 {
                                    tell_usage();
                                    return(BAILOUT);
                                 }
                             head_pattern[swnum++] = (char)toupper(c);
                             head_pattern[swnum] = 0;
                         break;
               case 'W'  :   for (p = head_pattern; *p; p++)
                               if (*p == c)
                                 {
                                    tell_usage();
                                    return(BAILOUT);
                                 }
                             head_pattern[swnum++] = (char)toupper(c);
                             head_pattern[swnum] = 0;
                         break;
               case 'K'  :   count_chars = YES;
                             strcpy(head_pattern, DEFAULT_COLUMNS);
                         break;
               case 'S'  :   subtree_search = YES;
                             showname = YES;
                         break;
               default   :   tell_usage();
                             return(BAILOUT);
            }  /* end switch */
          }


     if (*head_pattern == '\0')
       strcpy(head_pattern, DEFAULT_COLUMNS);
     if ( optind < argc)
       {
         if (showname == NO)
           if ( (argc - optind) > 1)
             showname = YES;
           else
             if (ISAMBIGUOUS(argv[optind]))
               showname = YES;
             else
               if (stat(argv[optind], &sbuf) == 0)
                 if (sbuf.st_mode & S_IFDIR)
                   showname = YES;
         return(optind);
       }
     else
       return(0);
}

unsigned int dofilespec(char *filespec)
{
   unsigned int files_in_filespec = 0;
   char filename[CCHMAXPATH];
   int attrib;

//   if (fmf_init(filespec, 0, subtree_search) == NO_ERROR)                  @1
   if (fmf_init(filespec, FMF_ALL_FILES, subtree_search) == FMF_NO_ERROR) // @1
     while (fmf_return_next(filename, &attrib) == NO_ERROR)
       {
         if (dofile(filename))
           files_in_filespec++;
       }
   return(files_in_filespec);
}

int dofile(char *filename)
{
   FILE *stream;

   if ( (stream = fopen(filename, "rb")) == NULL)
     return(NO);
   else
     {
       dostream(stream, filename);
       fclose(stream);
       return(YES);
     }
}

void dostream(FILE * stream, char *filename)
{
   unsigned long lines, words, bytes, chars;
   unsigned bytesread, i;
   int in_word, in_newline;
   char *p, c;

   lines = words = bytes = chars = 0;
   in_word = in_newline = NO;
   do
      {
        bytesread = fread(buf, sizeof(char), (size_t)BUFSIZE, stream);
        for (i = 0, p = buf; i < bytesread; i++, p++)
          {
            bytes++;
            if ( ((c = *p) == CR) || (c == LF) )
              {
                if (in_newline && (in_newline != c) )
                  in_newline = NO;
                else
                  {
                    lines++;
                    in_newline = c;
                    if (in_word == YES)
                    {
                      words++;
                      in_word = NO;
                    }
                  }
              }
            else
              if ( (c == TAB) || (c == SPACE) )
                {
                  chars++;
                  if (in_word)
                    {
                      words++;
                      in_word = NO;
                    }
                }
             else
               {
                 in_word = YES;
                 chars++;
               }
          } /* end FOR */
      }  while (!feof(stream));
   total_bytes += bytes;
   total_chars += chars;
   total_words += words;
   total_lines += lines;
   prtline(bytes, chars, words, lines, filename);
}

void prtline(unsigned long bytes, unsigned long chars, unsigned long words,
             unsigned long lines, char *filename)
{
   char *p;

   for (p = head_pattern; *p; p++)
      switch (*p)
      {
         case 'L':
                             printf("%7ld ", lines);
                             break;

         case 'W':
                             printf("%7ld ", words);
                             break;

         case 'C':
                             if (count_chars)
                               printf("%7ld ", chars);
                             else
                               printf("%7ld ", bytes);
                             break;
      }



   if (showname)
     printf("%s", filename);
   printf("\n");
}

void prttotals()
{
   char *p;

   for (p = head_pattern; *p; p++)
      switch (*p)
      {
         case 'L':
                             printf("%7ld ", total_lines);
                             break;

         case 'W':
                             printf("%7ld ", total_words);
                             break;

         case 'C':
                             if (count_chars)
                               printf("%7ld ", total_chars);
                             else
                               printf("%7ld ", total_bytes);
                             break;
      }
   printf("total\n");
}

void tell_usage()
{
   printf("wc - Copyright IBM, 1990, 1991\n");

   printf("\nThe wc command counts the number of lines, words, or characters in File or\n");
   printf("in the standard input if you do not specify any Files.  The command writes\n");
   printf("The results to standard output and keeps a total count for all named files.\n");
   printf("A word is defined as a string of characters delimted by spaces, tabs, or\n");
   printf("new-line characters.  By default, the wc command counts lines, words,\n");
   printf("and characters (as bytes).\n");
   printf("\nUsage:\n");
   printf("  wc [-c]||[-cl]||[-cw]||[-cwl]||[-l]||[-lw]||[-w]||[-k] {File ...\n");

   printf("\nFlags:\n");
   printf("       -c          Count bytes.\n");
   printf("       -l          Count lines.\n");
   printf("       -w          Count words.\n");
   printf("       -k          Count actual characters, not bytes.  Specifying the -k\n");
   printf("                   flag is equivalent to specifying -kclw.\n");
   printf("       -s          (OS2 only). Specifies that Files matching the pattern\n");
   printf("                   are searched for in subdirectories of the specified\n");
   printf("                   directory.\n");
}
