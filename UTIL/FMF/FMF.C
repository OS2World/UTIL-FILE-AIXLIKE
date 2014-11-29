static char sccsid[]="@(#)31	1.1  util/fmf/fmf.c, aixlike.src, aixlike3  9/27/95  15:52:53";
/************************************************/
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation,1990,1991.  All rights reserved. */
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 1       5/1/91                                           */
/*  @1  05/03/91 Scott Emmons reported grep did not work on network      */
/*               drives.  Found out that I was assuming the archive bit. */
/*  @2  05/07/91 The logic was far worse than I thought.  Besides these  */
/*               file selection changes, 6 users of fmf_init also have   */
/*               to change.  There's a corequisite fix to fmf.h.         */
/*  @3  05/10/91 Still another glitch:  selection logic was wrong,       */
/*               sending directories to pgms that didn't ask for them.   */
/*  @4  04/13/93 Added conditional compilation for 16-bit toolkit        */
/*-----------------------------------------------------------------------*/

/* This may turn out to be a callable utility for programs that need to        *
*  find one or more files that match an ambiguous file spec.                   *
*                                                                              *
*  This version is NOT thread-tolerant.  Use fmfmt.c for code that exploits    *
*  threads.                                                                    *
*                                                                              *
*  There are three entry points:                                               *
*      int fmf_init(char *pattern, unsigned fmfmask, int fmfmode)                    *
*                              which initializes a search for the caller.      *
*                              The mode is 0 for specified directory           *
*                              only, or 1 for search-everything-below.         *
*                                                                              *
*             NOTE: Having the ARCHIVED bit on means that you want        @1@2 *
*                   files that do not have one of the other four          @2   *
*                   attributes (DIRECTORY, HIDDEN, SYSTEM, READONLY).     @2   *
*                   If the ARCHIVED bit is off, then the file will not    @2   *
*                   selected unless it has one of the other attributes    @2   *
*                   listed above, and you have requested that attribute.  @2   *
*                   I've tried to simplify this by making equates in fmf.h@2   *
*                                                                              *
*                                                                              *
*      int fmf_return_next( char *where_to_put_name, int *where_put_attribute )*
*                              which returns a pointer to a file name and  its *
*                              attributes that satisfies the pattern.  The     *
*                              returned is qualified relative to the name and  *
*                              path specified in the pattern.  For example, if *
*                              the pattern was "C:\OS2\*.EXE then the first    *
*                              name returned might be "C:\OS2\ANSI.EXE"; a     *
*                              later one might be                              *
*                              "C:\OS2\INSTALL\CIKSTART.EXE"                   *
*                                                                              *
*      fmf_close()             which frees resources reserved for the caller   *
*                              It doesn't have to be used.                     *
*/


#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <ctype.h>
#include "fmf.h"                                     // @2
#define INCL_BASE
#include <os2.h>

#define ALLF "*.*"
#define chBACKSLASH  '\\'
#define achBACKSLASH "\\"
#define chCOLON      ':'
#define chDOT        '.'
#define achDOT       "."
#define achDOTDOT    ".."
#define YES  1
#define NO   0
                              /*----------------------------------------------*/
                              /* Declare how many concurrent threads we are   */
                              /* willing to handle                            */
                              /*----------------------------------------------*/
#define MAX_THREADS_HERE 12
                              /*----------------------------------------------*/
                              /* structure that describes the context         */
                              /* information for each thread                  */
                              /*----------------------------------------------*/
typedef struct _context  {
                  HDIR    dhandle;
                  char    *path;
                  struct _context *BackPointer;
                } CONTEXT;
                              /*----------------------------------------------*/
                              /*  Semephore to serialize access to globals    */
                              /*----------------------------------------------*/
ULONG   fmfSem = 0;
                              /*----------------------------------------------*/
                              /* variables global to fmf                      */
                              /*----------------------------------------------*/
CONTEXT        *current_context;
char           *namespec, *matchspec;
char            drivespec;
int             fmfmode;
unsigned        fmfmask;
#ifdef I16
FILEFINDBUF     ffb;
#else
FILEFINDBUF3    ffb;
#endif
char            NameWkArea[CCHMAXPATH];
char           *allfs = "*.*";
                              /*----------------------------------------------*/
                              /* The private routines                         */
                              /*----------------------------------------------*/
void AssembleName(char *where);
void AssemblePath(char *where);
int  wcmatch(char *string1, char *string2);
                              /*----------------------------------------------*/
                              /* Prototype for the error-message formatter,   */
                              /* which is external                            */
                              /*----------------------------------------------*/
void myerror(int rc, char *area, char *details);

/*----------------------------------------------------------------------------*/
/*    fmf_init                                                                */
/*                                                                            */
/* Find-matching-files initialization routine.  The user passes the pattern   */
/* and the search mode (0 or 1).  This routine sets up the data areas needed  */
/* to find and match the files.                                               */
/*----------------------------------------------------------------------------*/
int fmf_init(char *filespec, unsigned srchmask, int srchmode)
{

       int     rc, pathlen;                                           /* @4c */
#ifdef I16                                                            /* @4a */
       int     srchcnt;                                               /* @4a */
#else                                                                 /* @4a */
       unsigned long srchcnt;                                         /* @4a */
#endif                                                                /* @4a */
       HDIR    handle1 = 1;
       CONTEXT *cp;
       char    *p, *fnstart, *pathstart;
       char    LocalWkArea[CCHMAXPATH];
       void    fmf_close(void);

#ifdef DEBUG
printf("fmf_init - into the routine\n");
#endif

       if (current_context != 0)
         fmf_close();
       current_context = 0;
       namespec = 0;
       drivespec = '\0';
       fmfmode = 0;
       fmfmask = 0;

       if (!(cp = (CONTEXT *)malloc(sizeof(CONTEXT))) )
         {
//            perror("fmf_init malloc of context");
            myerror(ERROR_NOT_ENOUGH_MEMORY, "fmf_init", "malloc of context");
            return(-2);
         }
       current_context = cp;
       cp->BackPointer = 0;
       cp->path = 0;
       cp->dhandle = -1;

       fmfmode = srchmode;
//       fmfmask = srchmask;                             // @1
       fmfmask = 0;                                      // @2
       if (srchmask == -1)                               // @2
         fmfmask = FMF_ALL_DIRS_AND_FILES;               // @2
       else                                              // @2
         if (srchmask == 0)                              // @2
           fmfmask = FMF_FILES;                          // @2
         else                                            // @2
           fmfmask = srchmask;                           // @2

#ifdef DEBUG
printf("Search mode = %d\n", srchmode);
#endif
                                 /*-------------------------------------------*/
                                 /* There are a couple of quirks in filespecs */
                                 /* that we need to worry about.  First, a    */
                                 /* spec that consists only of the drive des- */
                                 /* ignation must be modified to have a path; */
                                 /* we do that by appending a dot.  Second,   */
                                 /* a spec that ends in a backslash is impli- */
                                 /* citly a directory, so we should append    */
                                 /* *.*. Dot directories get a name of *.*,   */
                                 /* too.                                      */
                                 /*-------------------------------------------*/
       cp->dhandle = -1;
       strcpy (NameWkArea, filespec); /* get the filespec where we can chg it*/
       p = NameWkArea;
       if (NameWkArea[1] == chCOLON) /* Is drive specified? If so, capture it */
         {
           drivespec = *p;
           p += 2;
         }
       pathstart = p;
       if (!*p)                   /* Was only the drive specified, or nothing?*/
         strcat(NameWkArea, achDOT);   /* Put in the assumed dot */
                                  /* If "\" terminates spec, add *.*          */
       for (p = NameWkArea; *p; p++);
       --p;
       if (*p == chBACKSLASH)
         strcat(NameWkArea, allfs);
                                 /*-------------------------------------------*/
                                 /* Do DosFindFirst on the path to make sure  */
                                 /* it exists.                                */
                                 /*-------------------------------------------*/
       srchcnt = 1;
       rc = DosFindFirst(NameWkArea,
                         &handle1,
                         FILE_SYSTEM | FILE_HIDDEN | FILE_DIRECTORY,
                         &ffb,
#ifdef I16
                         sizeof(FILEFINDBUF),
#else
                         sizeof(FILEFINDBUF3),
#endif
                         &srchcnt,
#ifdef I16
                         0l);
#else
                         1l);
#endif
#ifdef DEBUG
printf("First DosFindFirst on %s returned %d\n", filespec, rc);
#endif
                                 /*-------------------------------------------*/
                                 /* If the first FindFirst fails, the caller  */
                                 /* may mean "all files with this name in the */
                                 /* specified directory or lower".  If so, we */
                                 /* look at all the files and throw out the   */
                                 /* ones we don't want.                       */
                                 /*-------------------------------------------*/
       if ( (rc != NO_ERROR) && (fmfmode == 1) )
         {
           strcpy(LocalWkArea, NameWkArea);
           if ( (p = strrchr(pathstart, chBACKSLASH)) )
             fnstart = LocalWkArea + (p - NameWkArea + 1);
           else
             fnstart = LocalWkArea + (pathstart - NameWkArea);
           strcpy(fnstart, allfs);
           handle1 = 1;
           srchcnt = 1;
           rc = DosFindFirst(LocalWkArea,
                             &handle1,
                             FILE_SYSTEM | FILE_HIDDEN | FILE_DIRECTORY,
                             &ffb,
#ifdef I16
                             sizeof(FILEFINDBUF),
#else
                             sizeof(FILEFINDBUF3),
#endif
                             &srchcnt,
#ifdef I16
                             0l);
#else
                             1l);
#endif
#ifdef DEBUG
printf("Second DosFindFirst on %s returned %d\n", LocalWkArea, rc);
#endif
         }

       if ( rc != NO_ERROR)
         {
           free(cp);
           current_context = 0;
           drivespec = '\0';
           return(rc);
         }
                                 /*-------------------------------------------*/
                                 /* Find the start of the name part of the    */
                                 /* path specification.  It's either the part */
                                 /* after the last backslash, the part after  */
                                 /* the colon, or the whole thing.            */
                                 /*-------------------------------------------*/

       if ( (p = strrchr(pathstart, chBACKSLASH)) )
         fnstart = p + 1;
       else
         fnstart = pathstart;
#ifdef DEBUG
printf("Name within input spec is %s, in ffb %s\n", fnstart, ffb.achName);
#endif
                                 /*-------------------------------------------*/
                                 /* If the path has been specified 'shorthand'*/
                                 /* by just naming the directory, put a *.*   */
                                 /* on the end.                               */
                                 /*-------------------------------------------*/
       if ((ffb.attrFile & FILE_DIRECTORY) && /* FindFirst yields a directory */
           (stricmp(fnstart, ffb.achName)==0)) /*with same name as file sought*/
         {
           namespec = allfs;                      /* *.* implied              */
           matchspec = allfs;
         }
       else                                     /* Just \ specified as path */
         if ( (strcmp(pathstart, achBACKSLASH) == 0) ||
              (*fnstart == chDOT) )             /* or a dot directory named */
           {
             namespec = allfs;                    /* Also implies *.*         */
             matchspec = allfs;
           }
         else                                 /* A file object was specified */
           {
             if (strcmp(fnstart, allfs) == 0) /* If it wasn't *.* */
               {
                 namespec = allfs;
                 matchspec = allfs;
               }
             else                             /*    allocate some space for it*/
               if ( (namespec = (char *)malloc(strlen(fnstart) + 1)) )
                 {
                   strcpy(namespec, fnstart);
                   if (fmfmode != 0)        /* if subtree search specified */
                     {
                       matchspec = namespec;  /* use alloc space for match nm*/
                       namespec = allfs;      /* we will look at everything */
                     }
                 }
               else
                 {
                   free(cp);
                   current_context = 0;
                   drivespec = '\0';
//                   perror("fmf_init malloc of path");
                   myerror(ERROR_NOT_ENOUGH_MEMORY, "fmf_init", "malloc of path");
                   return(-2);
                 }
             if (fnstart - pathstart > 1)     /* back up to the backslash    */
               --fnstart;                     /* unless it's the root        */
             *fnstart = '\0';       /* and overlay it to mark end of path     */
           }
       pathlen = strlen(pathstart);
       if (pathlen)
         if ((cp->path = (char *)malloc(pathlen + 1)) )
           strcpy(cp->path, pathstart);
         else
           {
              if (namespec != allfs)
                free(namespec);
              free(cp);
              current_context = 0;
//              perror("fmf_init malloc of path");
              myerror(ERROR_NOT_ENOUGH_MEMORY, "fmf_init", "malloc of path");
              return(-2);
           }
       else
         cp->path = 0;


#ifdef DEBUG
AssembleName(NameWkArea);
printf("Reassembled input file spec: %s\n", NameWkArea);
#endif

       return(NO_ERROR);
}


/*----------------------------------------------------------------------------*/
/*    fmf_query_max_threads                                                   */
/*                                                                            */
/* Returns an integer reporting the number of concurrently-active threads are */
/* suported by the fmf routines.  An 'active' thread is one who has made an   */
/* fmf_init() call since its last call to fmf_close().                        */
/*----------------------------------------------------------------------------*/
int fmf_query_max_threads()
{
   return(1);
}

/*----------------------------------------------------------------------------*/
/*   fmf_close                                                                */
/*                                                                            */
/* Removes the current thread from the list of active threads.  Deallocates   */
/* any resources devoted to context for the thread.  A thread that has issued */
/* an fmf_init call should call this routine before it exits.                 */
/*----------------------------------------------------------------------------*/
void fmf_close()
{
       CONTEXT *CurrContext, *NextContext;

       CurrContext = current_context;
       do
         {
           if (namespec != allfs)
             free(namespec);
           else
             if ( (matchspec != allfs) && (fmfmode == 1) )
               free(matchspec);
           drivespec = '\0';
           if (CurrContext->path)
             free(CurrContext->path);
           NextContext = CurrContext->BackPointer;
           free(CurrContext);
           CurrContext = NextContext;
         }    while (CurrContext);
       current_context = 0;
       namespec = 0;
       matchspec = 0;

}

/*----------------------------------------------------------------------------*/
/*    fmf_return_next                                                         */
/*                                                                            */
/* Return a code of NO_ERROR, along with the name of the next file that       */
/* matches the pattern specified in fmf_init; or return a code of             */
/* ERROR_NO_MORE_FILES to indicate that there are no more files in the path   */
/* that match the pattern; or return a code indicating an error occured.      */
/*----------------------------------------------------------------------------*/
int fmf_return_next(char *CopyNameTo, int *CopyAttrTo)
{
       CONTEXT *cp;
       int rc;                                                         /* @4c */
#ifdef I16                                                             /* @4a */
       int srchcnt;                                                    /* @4a */
#else                                                                  /* @4a */
       unsigned long srchcnt;                                          /* @4a */
#endif                                                                 /* @4a */

       int fmfProcess(int *rc, char *NameTo, int *AttrTo);

       if  (current_context)
         while (1)
           {
             cp = current_context;
             srchcnt = 1;
#ifdef DEBUG
printf("fmf_next: dhandle is %d\n", cp->dhandle);
#endif

             if (cp->dhandle == -1)              /* No FindFirst yet */
               {
                 AssembleName(NameWkArea);
#ifdef DEBUG
printf("fnf_next FindFirst name is %s\n", NameWkArea);
#endif
                 rc = DosFindFirst(NameWkArea,
                                   &(cp->dhandle),
                                   FILE_SYSTEM | FILE_HIDDEN | FILE_DIRECTORY,
//                                   fmfmask | FILE_DIRECTORY,
                                   &ffb,
#ifdef I16
                                   sizeof(FILEFINDBUF),
#else
                                   sizeof(FILEFINDBUF3),
#endif
                                   &srchcnt,
#ifdef I16
                                   0l);
#else
                                   1l);
#endif
               }
             else                               /* FindFirst already done */
                 rc = DosFindNext( cp->dhandle,
                                   &ffb,
#ifdef I16
                                   sizeof(FILEFINDBUF),
#else
                                   sizeof(FILEFINDBUF3),
#endif
                                   &srchcnt);
#ifdef DEBUG
printf("rc from DosFind* was %d\n", rc);
#endif

             if (fmfProcess(&rc, CopyNameTo, CopyAttrTo))
               return(rc);
           }

       return(NO_ERROR);
}



/*----------------------------------------------------------------------------*/
/*  fmfProcess                                                                */
/*                                                                              */
/* Handle the logic that required to decide whether to return a file name,    */
/* to look for another one in the current search, or to start a new search    */
/* context.  The logic is common after either a FindFirst or a FindNext       */
/*----------------------------------------------------------------------------*/

int fmfProcess(int *ReturnCode, char *PutFileName, int *PutAttr)
{
     CONTEXT *cp, *sBP, *newcp;
     int     rc, pathlen, new_context;

#ifdef DEBUG
printf("fmfProcess - into routine\n");
#endif

     cp = current_context;
     rc = *ReturnCode;   /* return code from DosFindFirst or DosFindNext */
                              /*----------------------------------------------*/
                              /* If a system error occured, or the path was   */
                              /* bad, exit.                                   */
                              /*----------------------------------------------*/
     if (rc != NO_ERROR)
       {
         if (rc != ERROR_NO_MORE_FILES)
           {
//             perror("fmf_next DosFindFirst");
             myerror(rc, "fmf_next", "DosFindFirst");
             *ReturnCode = rc;
             return(YES);    /* we want the error returned to fmf caller */
           }
#ifdef DEBUG
printf("fmfProcess - no more files. ");
#endif
                              /*----------------------------------------------*/
                              /* If the path is good, but there are no        */
                              /* matching file names, go back to previous     */
                              /* context, or exit if we are on top level      */
                              /*----------------------------------------------*/
         DosFindClose(cp->dhandle);
         if ((sBP = cp->BackPointer) != 0)  /* there is a higher context */
           {
             if (cp->path)
               free(cp->path);
             free(cp);
             current_context = sBP;
             *ReturnCode = (NO_ERROR);
#ifdef DEBUG
printf("Restored previous context\n");
#endif
             return(NO);      /* we don't want to return to fmf caller */
           }
         else                               /* we're at the highest context */
           {
             cp->dhandle = -1;
             *ReturnCode = rc;
#ifdef DEBUG
printf("Already at top-level context\n");
#endif
             return(YES);
           }
       }
                              /*----------------------------------------------*/
                              /* If DosFind*     completed successfully, look */
                              /* at the file returned.                        */
                              /*                                              */
                              /* If the file found is a legitimate member of  */
                              /* the family of files requested, check if it's */
                              /* a directory.  If so, either ignore it or     */
                              /* create a new context to handle it.           */
                              /*----------------------------------------------*/
#ifdef DEBUG
printf("fmfProcess: Good return of name %s\n", ffb.achName);
#endif

     if ( (strcmp(ffb.achName, achDOT) == 0) ||
          (strcmp(ffb.achName, achDOTDOT) == 0) )
       {
         *ReturnCode = NO_ERROR;
#ifdef DEBUG
printf("fmfProcess: Skipped dot directory %s\n", ffb.achName);
#endif
         return(NO);  /* we want to skip over dot directories */
       }

     new_context = NO;
     if ((ffb.attrFile & FILE_DIRECTORY))
       if (fmfmode == 1)       /* We are walking subdirectories */
         {                         /* create a new context */
           if ( (newcp = (CONTEXT *)malloc(sizeof(CONTEXT))) == 0)
             {  // perror("fmf_process malloc of context");
                myerror(ERROR_NOT_ENOUGH_MEMORY, "fmf_process", "malloc of context");
                return(-2);
             }

           newcp->dhandle = -1;
           newcp->BackPointer = cp;
           pathlen = 2;
           if (cp->path)
             pathlen += strlen(cp->path);
           pathlen += strlen(ffb.achName);
           if ( (newcp->path = (char *)malloc(pathlen)) == 0)
             {
              free(newcp);
//               perror("fmf_process malloc of path");
               myerror(ERROR_NOT_ENOUGH_MEMORY, "fmf_process", "malloc of path");
               *ReturnCode = -2;
               return(YES);
             }
           *(newcp->path) = '\0';
           if (cp->path)
             {
               strcpy(newcp->path, cp->path);
               if (strcmp(cp->path, achBACKSLASH) != 0)
                 strcat(newcp->path, achBACKSLASH);
             }
           strcat(newcp->path, ffb.achName);
           current_context = newcp;
           new_context = YES;
#ifdef DEBUG
printf("fmfProcess: created new context, path = %s\n", newcp->path);
#endif
         }
                              /*----------------------------------------------*/
                              /* If its    attributes indicate it's one on the*/
                              /* files the caller wants to see, return it     */
                              /*----------------------------------------------*/
#ifdef DEBUG
printf("Search mask: %x  attribute: %x  fmfmode: %d\n", fmfmask, ffb.attrFile, fmfmode);
#endif
     *ReturnCode = NO_ERROR;
     if (fmfmode == 1)   /* if we are walking subtrees, we return all files, so */
                           /* see it this one's name matches the spec */
       {
         if (wcmatch(matchspec, ffb.achName) == 0)
           {
#ifdef DEBUG
printf("File name %s did not match input spec %s\n", ffb.achName, matchspec);
#endif
             return(NO);          /* if not, skip it */
           }
       }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -@2 */
/* The rules for accepting a file based on its attributes are these:    @2 */
/*  1 If the FILE_HIDDEN, FILE_READONLY, FILE_DIRECTORY,                @2 */
/* FILE_SYSTEM or FILE_ARCHIVED bit was ON in the request mask, and     @2 */
/* the corresponding attribute belongs to the file,                     @2 */
/*          ACCEPT the file                                             @2 */
/*  2 Otherwise, if the ARCHIVED bit is on                              @2 */
/*          ACCEPT the file if it is normal or archived-only            @2 */
/*  3 Otherwise, REJECT the file                                        @2 */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -@2 */
    rc = YES;
    if ((fmfmask & FMF_DIRS_AND_FILES) != FMF_DIRS_AND_FILES) /* if we can  @3*/
      if ( (fmfmask & FMF_DIRS) != (ffb.attrFile & FILE_DIRECTORY) )
        rc = NO;
    if (rc == YES)
      if (!(fmfmask & FMF_ALLMARK))      /* can we weed on basis of attrib? */
        if (( fmfmask & FMF_SYSTEM) != (ffb.attrFile & FILE_SYSTEM)  ||
            ( fmfmask & FMF_READONLY) != (ffb.attrFile & FILE_READONLY) ||
            ( fmfmask & FMF_HIDDEN) != (ffb.attrFile & FILE_HIDDEN) )
           rc = NO;
//     if (fmfmask & ffb.attrFile)                                          @1
//     if ( (fmfmask & ffb.attrFile) || (fmfmask == ffb.attrFile) ||       // @1
//          ( (fmfmask == FILE_NORMAL) && (ffb.attrFile == FILE_ARCHIVED) ) )@2
//          ( (fmfmask | ffb.attrFile) == FILE_ARCHIVED) )                 // @2

//     rc = NO;                                                           //  @2
//     if (fmfmask & ffb.attrFile)                     // Rule 1              @2
//       rc = YES;                                                        //  @2
//     else                                                               //  @2
//       if (fmfmask & FILE_ARCHIVED))                 // Rule 2              @2
//         if (ffb.attrFile == FILE_NORMAL || ffb.attrFile == FILE_ARCHIVED)//@2
//           rc = YES;                                                    //  @2

     if (rc == YES)                                                     //  @2
       {
         AssemblePath(PutFileName);
         if (!new_context)
           {
             if (cp->path)
               if (strcmp(cp->path, achBACKSLASH) != 0)
                 strcat(PutFileName, achBACKSLASH);
             strcat(PutFileName, ffb.achName);
           }
         *PutAttr = ffb.attrFile;
         return(YES);
       }
     else
       {
         return(NO);
#ifdef DEBUG
printf("fmfProcess: Skipped %s %s\n", (ffb.attrFile & FILE_DIRECTORY) ?
                                           "Subdirectory" : "File",
                                       ffb.achName);
#endif
       }
     return(YES);
}


/*----------------------------------------------------------------------------*/
/* private routine to assemble a proper file name                             */
/*----------------------------------------------------------------------------*/
void AssembleName(char *where)
{
  CONTEXT *cp;

  cp = current_context;
  AssemblePath(where);
  if ( (cp->path) && (strcmp(cp->path, achBACKSLASH) != 0) )
    strcat (where, achBACKSLASH);
  strcat(where, namespec);
}

/*----------------------------------------------------------------------------*/
/* private routine to assemble a proper path name                             */
/*----------------------------------------------------------------------------*/
void AssemblePath(char *where)
{
  char *p;

  p = where;
  *p = '\0';
  if (drivespec)
    {
      *p++ = drivespec;
      *p++ = chCOLON;
    }
  if (current_context->path)
    strcpy(p, current_context->path);
  else
    *p = '\0';
}

/*----------------------------------------------------------------------------*/
/* Private routine to                                                         */
/* match two strings: both may be ambiguous in the DOS (OS/2) sense:          */
/* using a question mark for a place holder, and a * to mean "up to           */
/* the next dot"                                                              */
/*----------------------------------------------------------------------------*/

/* returns 1 for match, 0 for not */

#define NEXTDOT(p) for(; *p && (*p != '.'); p++)
#define MATCH   1
#define NOMATCH 0

wcmatch(char *string1, char *string2)
{
   char *p, *q;

   if (!strcmp(string1, allfs) || !strcmp(string2, allfs))
     return(MATCH);
   if (!strcmp(string1, "*")   || !strcmp(string2, "*"))
     return(MATCH);
   for (p = string1, q = string2;
        *p && *q;
        p++, q++)
     {
       if ((*p == '?') || (*q == '?'))
         continue;
       else
         if ((*p == '*') || (*q == '*'))
           {
             NEXTDOT(p); NEXTDOT(q);
             if (!*p && !*q)
               return(MATCH);
             else
               if (!*p || !*q)
                 return(NOMATCH);
           }
         else
           if (toupper(*p) != toupper(*q))
             return(NOMATCH);
     }
     if (toupper(*p) == toupper(*q))
       return(MATCH);
     else
       if (*p == '*')
         NEXTDOT(p);
       else
         if (*q == '*')
           NEXTDOT(q);
       if (toupper(*p) == toupper(*q))
         return(MATCH);
     return(NOMATCH);
}

#ifdef DBGMAIN

#define FSFTEST
#include "fmf.h"
#include <process.h>
#include <stddef.h>
#define STACKSIZE 4096
ULONG ctrSem = 0;
int   ctr = 0;


main(argc, argv, envp)
   int argc;
   char *argv[];
   char *envp[];
{

   int attr, maxthreads, rc;
   void dir(char *spec);
   char name[CCHMAXPATH];

   if (argc < 2)
     {
      printf("\nTest scaffold for FindMatchingFile routines\n");
      printf("invoke as foo filespec filespec filespec...\n");
      return(0);
     }

   maxthreads = fmf_query_max_threads();
   printf("\nNumber of threads supported is %d.\n", maxthreads);


   printf("WITH THE MODEFLAG SET TO 0\n");
   if ( (rc = fmf_init(argv[1], FILE_HIDDEN | FILE_DIRECTORY | FILE_SYSTEM, 0)
                                                                  != NO_ERROR) )
     printf("Thread 1: fmf_init failed, rc = %d\n", rc);
   else
     while (rc == NO_ERROR)
       {
         rc = fmf_return_next(name, &attr);
         if (rc == NO_ERROR)
           printf("%s\n", name);
       }

   printf("WITH THE MODEFLAG SET TO 0\n");
   if ( (rc = fmf_init(argv[1], FILE_HIDDEN | FILE_DIRECTORY | FILE_SYSTEM, 1)
                                                                  != NO_ERROR) )
     printf("Thread 1: fmf_init failed, rc = %d\n", rc);
   else
     while (rc == NO_ERROR)
       {
         rc = fmf_return_next(name, &attr);
         if (rc == NO_ERROR)
           printf("%s\n", name);
       }

}

#endif
