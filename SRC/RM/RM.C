static char sccsid[]="@(#)74	1.1  src/rm/rm.c, aixlike.src, aixlike3  9/27/95  15:45:35";
/************************************************/
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation, 1990, 1991. All rights reserved.*/
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 1       5/1/91                                           */
/* @1 06.06.91 delete directory when -r is specified for a directory obj */
/* @2 11.25.91 delete files with names that start with dots.             */
/* @3 03/17/92 handle relative filespec when drive is specified.         */
/* @4 06/28/94 users requested no error if file does not exist           */
/*-----------------------------------------------------------------------*/

/*  rm.c */

/*  An attempt to duplicate the rm utility from the Unix shell.  It takes
*   4 switches:
*
*       -f -- instructs program to delete even those modules that are READONLY.
*
*       -i -- prompt user before deleting each file.  When used in conjunction
*             with the -r switch, the user is also prompted as each
*             subdirectory is dived into.
*
*       -r -- recursive.  Delete all subdirectories below the indicated
*             file object.
*
*       -  -- consider everything following to be a file name.  Not very useful
*             in an OS/2 environment, where file names can't start with
*             '-', but we retain it for conformity.
*
*   Logic:
*            For each filespec on the command line
*  1            Do DosFindFirst
*  2            For each object that matches the spec
*                 If object is file
*                   If not ReadOnly or if -f specified
*                     If -i specified
*                       prompt for permission
*                     If permitted
*                       delete it
*                 else the object is a directory
*                   If -r specified
*                     If -i specified
*                       prompt for permission
*                     If permitted
*                       add directory to spec
*                       recurse
*                       remove directory
*
*  This won't work like the Unix version, because it doesn't handle wild cards
*  the same way.  For example, the file spec \aaa\bbb\c*\d.* should erase all
*  files starting d. in all subdirectories of \aaa\bbb that started with c.
*  That happens on AIX, but not in this OS/2 version.  It's a future
*  enhancement.
*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <ctype.h>
#include <conio.h>
#include <string.h>
#ifndef I16
#include <sys\stat.h>
#endif
#define INCL_BASE
#include <os2.h>
#include "rm.h"


int FirsObj;
int Prompt_Yes = NO;
int Recurs_Yes = NO;
int ROdel_Yes  = NO;
int DeleteDir_Yes = NO;                                             /* @1a */
int FirstObj, Lastobj;
int depth= 0;
char fspec[CCHMAXPATHCOMP];  /* holds the file spec part (i.e. *.c or work.*) */
char wkarea[CCHMAXPATH];     /* an area to construct a filespec for findfirst */

void myerror(int rc, char *area, char *details); /* prototype of error handler*/
                                          /* prototype of command line parser */
extern int getopt(int argc, char **argv, char *argstring);
extern int optind;           /* exposed by getopt */
extern char *myerror_pgm_name;    /* exposed by myerror */


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*    main                                                                   */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
main(int argc, char *argv[], char *envp[])
{
      int i, srchcnt;
      int rc = NO_ERROR;
      char *dirP;
      HDIR handle1 = 1;
      FILEFINDBUF3 ffb;

      myerror_pgm_name = "rm";
              /*--------------------------------------------------------------*/
              /* At least one command line argument is required.  Assume an   */
              /* invocation with no arguments is a request for help.          */
              /*--------------------------------------------------------------*/
      if (argc == 1)
        {
         tell_usage();
         return (-2);
        }
              /*--------------------------------------------------------------*/
              /* This one is tricky:  an invocation of rm ? is a legitimate   */
              /* request to delete all one-character-name objects in the      */
              /* current path.  It is dangerous, though -- many people will   */
              /* use that invocation as a request for help.  I'm treating it  */
              /* as a help request.  Users who want to erase 1-char names can */
              /* use "?.".                                                    */
              /*--------------------------------------------------------------*/
      if ( strcmp(argv[1], "?") == 0)
        {
         tell_usage();
         return (-1);
        }
              /*--------------------------------------------------------------*/
              /* Parse the command line.  Exit if something is wrong.         */
              /*--------------------------------------------------------------*/
      if (init(argc, argv))
              /*--------------------------------------------------------------*/
              /* For each object name specified on the command line...        */
              /*--------------------------------------------------------------*/
        for (i = FirstObj; i < argc && rc == NO_ERROR; i++)
          if (*argv[i] != '-')
              /*--------------------------------------------------------------*/
              /* Allocate space for the path portion of the name and delete   */
              /* all objects that match the name.                             */
              /*--------------------------------------------------------------*/
            if ( (dirP = (char *)malloc(CCHMAXPATHCOMP)) )
              {  srchcnt = 1;
                 rc = findfirst(argv[i], &ffb, dirP, fspec);
                 if (rc == NO_ERROR)
                   {                                                /* @1a */
                     rc = do_deletes(dirP, &ffb);
              /*------------------------------------------------------ @1a ---*/
              /* If the object specified by the user was a directory,  @1a    */
              /* we need to delete that object, too.                   @1a    */
              /*------------------------------------------------------ @1a ---*/
                     if (DeleteDir_Yes == YES)                      /* @1a */
#ifdef I16
                       if (DosRmDir(dirP, 0l) != NO_ERROR)          /* @1a */
#else
                       if (DosDeleteDir(dirP) != NO_ERROR)
#endif
                         fprintf(stderr,"rm: Could not remove %s\n", dirP);
                                                                    /* @1a */
                   }                                                /* @1a */
                 if ((rc != NO_ERROR) && (rc != ERROR_NO_MORE_FILES)
                     && (rc != RCNONEFOUND) )
                   myerror(rc, "DosFind", argv[i]);
                 free(dirP);                                        /* @1a */
              }
           else
             myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "257");
      if (rc == RCNONEFOUND)
        rc = NO_ERROR;
      return(rc);
}


/*------------------------------------------------------------------------*/
/*                                                                        */
/*  init                                                                  */
/*                                                                        */
/*  Parses command line arguments.  Makes sure at least one argument      */
/*  represents a file name.  Sets three program switches: Prompt_Yes,     */
/*  which determines whether the user is prompted for each object to be   */
/*  deleted; ROdel_Yes, which determines whether Read Only files will be  */
/*  deleted; and Recurs_Yes, which determines whether subdirectories will */
/*  also be erased.                                                       */
/*------------------------------------------------------------------------*/

init(int argc, char *argv[])
{
#ifdef I16
     char c;
#else
     int c;
#endif
     char *optstring = "fir";

    Prompt_Yes = NO;
    Recurs_Yes = NO;
    ROdel_Yes  = NO;
#ifdef I16
    while ( (c = (char)getopt(argc, argv, optstring)) != EOF)
#else
    while ( (c = getopt(argc, argv, optstring)) != EOF)
#endif
      {
          switch (c)
          {
            case 'f':         ROdel_Yes = YES;
                    break;

            case 'i':         Prompt_Yes = YES;
                    break;

            case 'r':         Recurs_Yes = YES;
                    break;

            default:          tell_usage();
                              return(NO);
                    break;
          }
      }

    if (optind == argc)
       {
          tell_usage();
          return(NO);
       }
    else
       FirstObj = optind;

    return(YES);
}

/*------------------------------------------------------------------------*/
/*                                                                        */
/*  findfirst                                                             */
/*                                                                        */
/*  Finds first object that matches the File specified on the command     */
/*  line.  If the command line specifies a directory name, \*.* is        */
/*  assumed.  The file specification is broken down into a directory      */
/*  part and a file part, so that you can specify \a\file.* and, with -r, */
/*  be able to cause  files like \a\b\file.c \a\b\c\file.d, etc.          */
/*  also be erased, which is how I assume the Unix version works.         */
/*------------------------------------------------------------------------*/

#ifdef I16
findfirst(char *infilespec, PFILEFINDBUF ffbP, char *dirP, char *flsP)
#else
findfirst(char *infilespec, PFILEFINDBUF3 ffbP, char *dirP, char *flsP)
#endif
{
      HDIR handle1 = 1;
      int rc;
#ifdef I16
      int srchcnt = 1;
#else
      ULONG srchcnt = 1;
#endif
      char *p1, *p2, *q;

              /*--------------------------------------------------------------*/
              /* Find the first instance of the filespec in the directory     */
              /*--------------------------------------------------------------*/
      rc = DosFindFirst(infilespec, &handle1, ALLFILES, ffbP,
                        sizeof(FILEFINDBUF), &srchcnt, 1l);

      if (rc == NO_ERROR)
              /*--------------------------------------------------------------*/
              /* If it's a directory, give it a default filename spec of *.*  */
              /*--------------------------------------------------------------*/
/*      if ( (ISDIRECTORY(ffbP)) &&  (*(ffbP->achName) != DOT) )       @2d */
        if ( (ISDIRECTORY(ffbP)) &&  !ISDOTDIR(ffbP) )              /* @2a */
            { strcpy(dirP, infilespec);
              DeleteDir_Yes = YES;                                  /* @1a */
              strcpy(flsP, "*.*");
            }
        else
            {                                                       /* @1a */
              DeleteDir_Yes = NO;                                   /* @1a */
              /*--------------------------------------------------------------*/
              /* If it's a file, seperate path from file name.                */
              /*--------------------------------------------------------------*/
              if ( (q = strrchr(infilespec, '\\')) ) /*name contains backslash*/
                { for (p1=dirP, p2=infilespec; p2 != q ; *p1++ = *p2++);
                  *p1 = '\0';
                  for (p1=flsP, ++p2; *p2 ; *p1++ = *p2++)
                  *p1 = '\0';                                       /* @1a */
                }
              else                                   /*name has no backslash  */
                {                                    /* is it like x:filename?*/
                  q = strchr(infilespec, ':');                       /* @3a10 */
                  if (q)                             /* Yes! leave off drive  */
                    {
                      strcpy(flsP, ++q);
                      strncpy(dirP, infilespec, 2);
                      strcpy((dirP + 2), ".");
                    }
                  else
                    {
                      strcpy(flsP, infilespec);
                      strcpy(dirP, ".");
                    }
                }
            }
      else
        if (rc == ERROR_NO_MORE_FILES)
/*          printf("%s does not exist\n", infilespec);      @4d */
          rc = RCNONEFOUND;                              /* @4a */


      return(rc);
}

/*------------------------------------------------------------------------*/
/*                                                                        */
/*  do_deletes                                                            */
/*                                                                        */
/*  For a given file spec, delete all files that match that spec.  If     */
/*  subdirectories are encountered and -r is specified, call yourself     */
/*  recursively to delete the subdirectory files and then delete the      */
/*  subdirectory.                                                         */
/*------------------------------------------------------------------------*/

#ifdef I16
do_deletes(char *startingDir, PFILEFINDBUF ffbP)
#else
do_deletes(char *startingDir, PFILEFINDBUF3 ffbP)
#endif
{
      int rc;
#ifdef I16
      int searchcnt;
#else
      ULONG searchcnt;
#endif
      HDIR dhandle;
      char *dirP;

              /*--------------------------------------------------------------*/
              /* Create a string naming the directory object(s) to be deleted.*/
              /*--------------------------------------------------------------*/
      strcpy(wkarea, startingDir);
      strcat(wkarea, "\\");
      strcat(wkarea, fspec);
      dhandle = -1;
      searchcnt = 1;

              /*--------------------------------------------------------------*/
              /* Find the first instance of that name in the directory space  */
              /*--------------------------------------------------------------*/
#ifdef I16
      rc = DosFindFirst(wkarea, &dhandle, ALLFILES, ffbP, sizeof(FILEFINDBUF),
                        &searchcnt, 0l);
#else
      rc = DosFindFirst(wkarea, &dhandle, ALLFILES, ffbP, sizeof(FILEFINDBUF),
                        &searchcnt, 1l);
#endif

      while (rc == NO_ERROR)
              /*--------------------------------------------------------------*/
              /* Skip dot directories, and ReadOnly files unless -f specified */
              /*--------------------------------------------------------------*/
        {
          if ( !ISDOTDIR(ffbP)  &&
             (!ISREADONLY(ffbP) || ROdel_Yes) )
            {
              /*--------------------------------------------------------------*/
              /* Create a string naming this particular directory object      */
              /*--------------------------------------------------------------*/
               strcpy(wkarea, startingDir);
               strcat(wkarea, "\\");
               strcat(wkarea, ffbP->achName);
              /*--------------------------------------------------------------*/
              /* Skip subdirectories if -r was not specified.                 */
              /*--------------------------------------------------------------*/
               if (!ISDIRECTORY(ffbP) || Recurs_Yes)
                                          /* if -i, let user say No */
              /*--------------------------------------------------------------*/
              /* If -i was specified, prompt operator for permission to go on */
              /*--------------------------------------------------------------*/
                 if ( (Prompt_Yes == NO) ||
                      (Prompt_Yes && rmprompt(wkarea, ffbP)) )
              /*--------------------------------------------------------------*/
              /* If the object is a subdirectory, allocate space to store its */
              /* path name, then call this subroutine recursively.  On        */
              /* return, attempt to delete the subdirectory.                  */
              /*--------------------------------------------------------------*/
                   if (ISDIRECTORY(ffbP))
                     if ( (dirP = (char*)malloc(CCHMAXPATHCOMP)) )
                       {
                         strcpy(dirP, wkarea);
                         do_deletes(dirP, ffbP);
#ifdef I16
                         if ((rc = DosRmDir(dirP, 0l)) != NO_ERROR)
#else
                         if ((rc = DosDeleteDir(dirP)) != NO_ERROR)
#endif
                           myerror(rc, "DosRmDir", dirP);
                         free(dirP);
                       }
                     else
                       myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "257");
                     /* endif malloc() */
                   else  /* is NOT directory */
                     {
              /*--------------------------------------------------------------*/
              /* If the object ReadOnly, reset its protection to notReadOnly  */
              /*--------------------------------------------------------------*/
                       if (ISREADONLY(ffbP))
#ifdef I16
                         DosSetFileMode (wkarea,
                                         ffbP->attrFile ^ FILE_READONLY,
                                         0l);
#else
                         _chmod(wkarea, S_IWRITE);
#endif
              /*--------------------------------------------------------------*/
              /* If the object is NOT a subdirectory, delete it.              */
              /*--------------------------------------------------------------*/
#ifdef I16
                       if ((rc = DosDelete(wkarea, 0l)) != NO_ERROR)
#else
                       if ((rc = DosDelete(wkarea)) != NO_ERROR)
#endif
                         myerror(rc, "DosDelete", wkarea);

                     }
                   /* endif ISDIRECTORY */
                 /* endif interactive prompt */
               /* endif Subdirectories allowed*/
            } /* endif deletable file */

              /*--------------------------------------------------------------*/
              /* Find the next object that matches the name specification.    */
              /*--------------------------------------------------------------*/
          searchcnt = 1;
          rc = DosFindNext(dhandle, ffbP, sizeof(FILEFINDBUF), &searchcnt);
        } /* end while */


              /*--------------------------------------------------------------*/
              /* Close the directory handle used in this call.                */
              /*--------------------------------------------------------------*/
      DosFindClose(dhandle);
              /*--------------------------------------------------------------*/
              /* Note that NO_MORE_FILES is the normal exit status.           */
              /*--------------------------------------------------------------*/
      if (rc == ERROR_NO_MORE_FILES)
        return(NO_ERROR);
      else
        return(rc);
}


#ifdef I16
rmprompt(char *name, PFILEFINDBUF ffbP)
#else
rmprompt(char *name, PFILEFINDBUF3 ffbP)
#endif
{
   char c;
              /*--------------------------------------------------------------*/
              /* Describe the attributes of the object to be deleted.         */
              /*--------------------------------------------------------------*/
   c = ' ';
   if (ISHIDDEN(ffbP))
     printf("hidden ");
   if (ISSYSTEM(ffbP))
     printf("system ");
   if (ISREADONLY(ffbP))
     printf("read-only ");
   if (ISDIRECTORY(ffbP))
     printf("subdirectory ");
   printf("%s",name);
              /*--------------------------------------------------------------*/
              /* Force the operator to enter Y or N.  Exit appropriately.     */
              /*--------------------------------------------------------------*/
   do
     { printf("?");
       c = (char) getch();
       if (ISEXTCHAR(c))     /* read second byte if extended char pressed */
         getch();
       c = (char) toupper(c);
     }
       while ( (c != NOCHAR) && (c!= YESCHAR) );

   if (c == NOCHAR)
     { printf(" No\n");
       return(NO);
     }
   else
     { printf(" Yes\n");
       return(YES);
     }
}


/*------------------------------------------------------------------------*/
/*                                                                        */
/*  tell_usage                                                            */
/*                                                                        */
/*  Writes program usage information to STDOUT                            */
/*------------------------------------------------------------------------*/

void tell_usage()
{
  printf("rm - Copyright IBM, 1990, 1992\n");
  printf("\nUsage:\n    rm [-f][-i][-r] file [file [file...\n");
  printf("    where\n        -f allows files marked READONLY to be deleted\n");
  printf("        -i instructs rm to prompt you before each file is deleted\n");
  printf("        -r specifies that all subdirectories and their contents\n");
  printf("           are to be deleted also.\n");
}

              /*--------------------------------------------------------------*/
              /* These two routines are invoked only for debug.  They prevent */
              /* any subdirectory from actually being removed and any file    */
              /* from really being erased.                                    */
              /*                                                              */
              /*--------------------------------------------------------------*/
#ifdef DEBUG
USHORT APIENTRY DosRmDir(PSZ dir, ULONG a)
{
   printf("I didn't really delete the subdirectory.\n");
   return(NO_ERROR);
}

USHORT APIENTRY DosDelete(PSZ file, ULONG a)
{
   printf("PSYCH! You thought you were going to delete that file!\n");
   return(NO_ERROR);
}

USHORT APIENTRY DosSetFileMode(PSZ name, USHORT attr, ULONG a)
{
   printf("Outside of DEBUG, this would reset the READONLY flag for %s\n",name);
   return(NO_ERROR);
}
#endif
