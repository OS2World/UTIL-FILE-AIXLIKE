static char sccsid[]="@(#)65	1.1  src/paste/paste.c, aixlike.src, aixlike3  9/27/95  15:45:16";
/************************************************/
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation, 1994.  All rights reserved.     */
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 1 9/18/94                                                */
/*-----------------------------------------------------------------------*/

/* 
The paste command merges the lines of several files or subsequent lines
in one file.

Syntax:

     paste -dList -s File1 [File2 [File3...

The paste command reads input frm the files specified by the file1 and
file2 parameters (standard input if you specify a - as a file name),
concatenatesthe corresponding lines of the given input files, and writes
the resulting lines to standard output.  Output lines are restricted to
511 characters.

By default, the paste command treats each file as a column and joins them
horizontally with a tab character (parallel merging).  You can think of
paste as the counterpart of the cat command (which contatenates files
vertically, that is, one file after another).

With the -s flag, the paste command combines subsequent lines of an input
file (serial mergina).  These lines are joined with the tab character by
default.

Flags:

        -dList  changes the delimiter that seperates corresponding lines
                in the output with one or more characters specified in
                the List parameter, then they are repeated in order until
                the end of the output.  In parallel merging, the lines
                from the last file always end with a new-line character,
                instead of one from the List parameter.

                The following special characters can also be used in the
                List parameter:

                \n      New-line character
                \t      Tab
                \\      Backslash
                \0      Empty string (not a NULL)
 
                You must put quotation marks around characters that
                have special meaning to the shell.

        -s      Merges subsequent lines from the first file horizontally.
                With this flag, paste works through one entire file before
                starting on the next.  When it finishes merging
                the lines in one file, it forces a new line and then
                merges the lines in the next input file, continuing in 
                the same way through the remaining input files, one at a
                time.  A tab seperates the lines unless you use the -d 
                flag.  Regardless of the List parameter, the last 
                character of the file is forced to be a new-line character.
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
#define DEFAULT_SEPERATOR_LIST "\\t"
#define SPACE 0x20
#define NULLCHAR 0xff
#define MAXOUTLINE 511
#define MAXLINE 50000
#define MAXLISTCHARS 99
#define COLON ':'

struct filelist {
                  char *filename;
                  FILE *stream;
                  struct filelist *next;
                };

/* globals */
char *opstring = "d:s";
char inbuf[MAXLINE+1];
char outbuf[MAXOUTLINE+1];
char listbuf[MAXLISTCHARS + 1];
#define PARALLEL_MERGING 1
#define SERIAL_MERGING 2
int mergemode = PARALLEL_MERGING;

/* Function Prototypes */

int init(int argc, char **argv);
int buildlistbuf(char *list);
void serialmerge(struct filelist *root, int filecount);
void parallelmerge(struct filelist *root, int filecount);
int getline(char *buf, FILE *stream);
void insertsep(char sep);
void addfilespec(struct filelist **rootp,char *filespec);
void addfile(struct filelist **rootp,char *filename);
void tell_usage(void);
extern int getopt(int argc, char **argv, char *ops);
extern int optind;
extern char *optarg;

int main(argc, argv, envp)                                      /* @2c */
   int argc;
   char *argv[];
   char *envp[];
{
   int fnindex, filecount, i;
   struct filelist *root = NULL;
   struct filelist *pfl;

   if ( (fnindex = init(argc, argv)) != BAILOUT)
     {
       for (i = fnindex; i < argc; i++)
         addfilespec(&root, argv[i]);
       for (filecount = 0,pfl = root; pfl; pfl = pfl->next)
         filecount++;
       if (mergemode == SERIAL_MERGING)
         serialmerge(root,filecount);
       else           /* mergemode == PARALLEL_MERGING */
         parallelmerge(root,filecount);
       printf("\n");
     }
   else                                                         /* @2a */
     return(BAILOUT);                                           /* @2a */
   return(0);                                                   /* @2a */
}

/********************************************************/
/* serialmerge - concatenate lines of each file in turn */
/********************************************************/
void serialmerge(struct filelist *root, int filecount) 
{
   struct filelist *pfl;
   int i, start;
   char *p;

   if (filecount == 0)
     {
        root = (struct filelist *)malloc(sizeof(struct filelist));
        root->filename = "stdin";
        root->stream = stdin;
        root->next = NULL;
     }
   start = 0;
   p = listbuf;
   for (pfl = root; pfl; pfl = pfl->next)
     {
       if (pfl->filename == NULL || (pfl->stream = fopen(pfl->filename, "rb")))
         {
           while (getline(inbuf,pfl->stream))
             {
               if (start)
                 insertsep(*p++);
               if (!*p)
                 p = listbuf;
               printf("%s",inbuf);
               start = 1;
             }
           fclose(pfl->stream);
         }
     }
}

/********************************************************/
/* parallelmerge - concatenate corresponding lines from */
/* each file.  We are guaranteed to have at least 1     */
/* file.                                                */
/********************************************************/
void parallelmerge(struct filelist *root, int filecount)
{
   struct filelist *pfl;
   int i, filesdone, start;
   char *p, *q;
 

   start = 0;
   filesdone = 0;
   p = listbuf;
   while (filesdone < filecount)
      {
         filesdone = 0;
         start = 0;
         p = listbuf;
         for (pfl = root; pfl; pfl = pfl->next)
           {
             if (getline(inbuf, pfl->stream))
               {
                 if (start)
                   {
                     insertsep(*p++);
                     if (!*p)
                       p = listbuf;
                   }
                 printf("%s",inbuf);
               }
             else
               filesdone++;
             start++;
           }
         printf("\n");
      }
   for (pfl = root; pfl; pfl = pfl->next)
     fclose(pfl->stream);
}


/********************************************************/
/* getline - read in a line. return 0 on eof            */
/********************************************************/
int getline(char *buf, FILE *stream)
{
   char *p;

   if (fgets(buf,MAXLINE,stream))
     {
       for (p = buf + strlen(buf) - 1; p > inbuf; p--)
          if (*p == '\r' || *p == '\n')
            *p = '\0';
          else
            break;
     }
   else
     return(0);

   return(1);
}

/********************************************************/
/* insertsep - put in the seperator                     */
/********************************************************/
void insertsep(char sep)
{
   if (sep != NULLCHAR)
     printf("%c",sep);
}

/********************************************************/
/* init()
/********************************************************/
int init(int argc, char **argv)
{
   int c;
   int dspecified = NO;
   char *p, *tlist;

   tlist = NULL;
   while ( (c = getopt(argc, argv, opstring)) != EOF)
          {
            switch (c)
            {
               case 's'  :   mergemode = SERIAL_MERGING;
                         break;
               case 'd'  :   tlist = optarg;
                         break;
               default   :   tell_usage();
                             return(BAILOUT);
            }  /* end switch */
          }

     if (tlist == NULL)
       tlist = DEFAULT_SEPERATOR_LIST;

     buildlistbuf(tlist);

     if ( optind < argc)
       return(optind);
     else
       if (mergemode == PARALLEL_MERGING)
         {
            tell_usage();
            return(BAILOUT);
         }
       else
         return(0);
}

/*******************************************************/
/* buildlistbuf - create the list of column seperators */
/*******************************************************/
int buildlistbuf(char *list)
{
   char *p, *q;
   int len;

   p = listbuf;
   len = 0;

   for (q = list; *q && len <= MAXLISTCHARS; q++)
     {
       if (*q == '\\')
         switch (*++q)
         {
            case 'n':     *p++ = '\n';
                          break;
            case 't':     *p++ = TAB;
                          break;
            case '0':     *p++ = NULLCHAR;
                          break;
            case  0 :     tell_usage();
                          return(BAILOUT);
                          break;
            default :     *p++ = *q;
                          break;
         }
       else
         *p++ = *q;
       len++;
     }
   *p = 0;
   return(0);
}

void addfilespec(struct filelist **rootp,char *filespec)
{
   char filename[CCHMAXPATH];
   int attrib;

   if (strcmp(filespec,"-") == 0)
      addfile(rootp,filename);
   else
     if (fmf_init(filespec, FMF_ALL_FILES, 0) == FMF_NO_ERROR) // @1
       while (fmf_return_next(filename, &attrib) == NO_ERROR)
         addfile(rootp,filename);
     else
       fprintf(stderr,"paste: no files matching %s\n",filespec);
}
   
void addfile(struct filelist **rootp,char *filename)
{
   FILE *stream;
   struct filelist *p, *newp;

   newp = (struct filelist *)malloc(sizeof(struct filelist));
   if (newp)
     {
        newp->filename = NULL;
        newp->next = NULL;
     }
   if (strcmp(filename,"-") == 0)
     newp->stream = stdin;
   else
     if (mergemode == PARALLEL_MERGING)
       if ((stream = fopen(filename, "rb")))
         {           
           newp->filename = (char *)malloc(strlen(filename) + 1);
           strcpy(newp->filename,filename);
           newp->stream = stream;
         }
       else
         {
           fprintf(stderr,"paste: could not open file %s\n",filename);
           return;
         }
     else
       {
         newp->filename = (char *)malloc(strlen(filename) + 1);
         strcpy(newp->filename,filename);
       }
 

   if (*rootp)
     {
       for (p = *rootp; p->next; p = p->next);
       p->next = newp;
     }
   else
     *rootp = newp;
}



void tell_usage()
{
   printf("paste - Copyright IBM, 1994\n");
   printf("The paste command merges the lines of several files or subsequent lines\n");
   printf("in one file.\n");
   printf("Usage:\n");
   printf("      paste -dList -s File1 [File2 [File3...\n");
   printf("Substituting - (hyphen) for a file name means use stdin.\n");
   printf("By default, the paste command treats each file as a column and joins them\n");
   printf("horizontally with a tab character (parallel merging).\n");
   printf("With the -s flag, the paste command combines subsequent lines of an input\n");
   printf("file (serial mergina).  These lines are joined with the tab character by\n");
   printf("default.\n");
   printf("Flags:\n");
   printf("  -dList  changes the delimiter that seperates corresponding lines\n");
   printf("          in the output with one or more characters specified in\n");
   printf("          the List parameter, then they are repeated in order.\n");
   printf("             \"\\n\" indicates new line\n");
   printf("             \"\\t\" indicates tab (the default\n");
   printf("             \"\\0\" indicates no seperator\n");
   printf("  -s      Merges subsequent lines from the first file horizontally.\n");

}
