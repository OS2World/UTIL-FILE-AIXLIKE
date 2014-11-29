static char sccsid[]="@(#)34	1.1  util/fmf/fmfmt.c, aixlike.src, aixlike3  9/27/95  15:52:59";
/* This may turn out to be a callable utility for programs that need to        *
*  find one or more files that match an ambiguous file spec.                   *
*                                                                              *
*  This code is thread-tolerant, and should be used serially.  In a program    *
*  with multiple threads, use the following calling sequence:                  *
*                                                                              *
*                                                                              *
*  There are three entry points:                                               *
*      int fmf_init(char *pattern, unsigned mask, int mode)                                   *
*                              which initializes a search for the calling      *
*                              thread.  The mode is 0 for specified directory  *
*                              only, or 1 for search-everything-below.         *
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
*      int fmf_query_max_threads()                                             *
*                              which returns the maximum number of concurrent  *
*                              threads this routine is willing to handle.      *
*                                                                              *
*      fmf_close()             which frees resources reserved for the calling  *
*                              thread.  It doesn't have to be used, but if     *
*                              the caller does a lot of thread starting and    *
*                              stopping, he'd better use this when he kills    *
*                              his thread.                                     *
*/


#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
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
int             ntids_here = 0;
TID             threads_here[MAX_THREADS_HERE];
CONTEXT *current_context[MAX_THREADS_HERE];
char           *namespec[MAX_THREADS_HERE];
char            drivespec[MAX_THREADS_HERE];
int             mode[MAX_THREADS_HERE];
unsigned        mask[MAX_THREADS_HERE];
FILEFINDBUF     ffb;
char            NameWkArea[CCHMAXPATH];
char           *allfs = "*.*";
                              /*----------------------------------------------*/
                              /* The private routines                         */
                              /*----------------------------------------------*/
void AssembleName(int slot, char *where);
void AssemblePath(int slot, char *where);

/*----------------------------------------------------------------------------*/
/*    fmf_init                                                                */
/*                                                                            */
/* Find-matching-files initialization routine.  The user passes the pattern   */
/* and the search mode (0 or 1).  This routine sets up the data areas needed  */
/* to find and match the files.                                               */
/*----------------------------------------------------------------------------*/
int fmf_init(char *filespec, unsigned srchmask, int srchmode)
{

       int     i, rc, ts, srchcnt, pathlen;
       HDIR    handle1 = 1;
       CONTEXT *cp;
       char    *p, *fnstart, *pathstart;
       PIDINFO pidinfo;

#ifdef DEBUG
printf("fmf_init - into the routine\n");
#endif
                              /*----------------------------------------------*/
                              /* Find out what thread called us               */
                              /*----------------------------------------------*/
       if ( (rc = DosGetPID(&pidinfo)) != NO_ERROR )
         {  perror("fmf_init getPID");
            return(rc);
         }
#ifdef DEBUG
printf("fmf_init - thread id is %d\n", pidinfo.tid);
#endif
                              /*----------------------------------------------*/
                              /* If there are no threads currently active,    */
                              /* initialize the data areas                    */
                              /*----------------------------------------------*/
       DosSemWait(&fmfSem, -1);     /* serialize this section */
       DosSemSet(&fmfSem);
       ts = MAX_THREADS_HERE;
       if (ntids_here == 0)
         for (i = 0; i < MAX_THREADS_HERE ; i++)
           {
             threads_here[i] = 0;
             current_context[i] = 0;
             namespec[i] = 0;
             drivespec[i] = '\0';
             mode[i] = 0;
             mask[i] = 0;
           }
                              /*----------------------------------------------*/
                              /* If there are currently active threads, see   */
                              /* if we are one of them.                       */
                              /*----------------------------------------------*/
       else
         for  (i = 0; i < MAX_THREADS_HERE; i++)
           if ( threads_here[i] == pidinfo.tid )
             {
               ts = i;
               break;
             }
                              /*----------------------------------------------*/
                              /* If this is the first call for our thread,    */
                              /* try to find an empty slot for us.            */
                              /*----------------------------------------------*/
       if (ts == MAX_THREADS_HERE)
         for(i = 0; i < MAX_THREADS_HERE; i++)
           if (threads_here[i] == 0)
                                 /*-------------------------------------------*/
                                 /* If there is an empty slot, allocate memory*/
                                 /* in which to keep our context information  */
                                 /* and initialize the context                */
                                 /*-------------------------------------------*/
             {
               threads_here[i] = pidinfo.tid;
               ts = i;
               ntids_here++;
               if (!(cp = (CONTEXT *)malloc(sizeof(CONTEXT))) )
                 {
                    threads_here[i] = 0;
                    ntids_here--;
                    perror("fmf_init malloc of context");
                    DosSemClear(&fmfSem);     /* Clear semaphone before exit */
                    return(-2);
                 }
               current_context[i] = cp;
               cp->BackPointer = 0;
               cp->path = 0;
               cp->dhandle = -1;
               break;
             }
       DosSemClear(&fmfSem);     /* no need to serialize here */

       if (ts == MAX_THREADS_HERE)
         {  perror("fmf_init No room for more threads");
            return(-1);
         }
#ifdef DEBUG
printf("The thread was placed in slot %d\n", ts);
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
       mode[ts] = srchmode;
       mask[ts] = srchmask | FILE_ARCHIVED;
       cp = current_context[ts];
       cp->dhandle = -1;
       DosSemWait(&fmfSem, -1);     /* serialize this section */
       DosSemSet(&fmfSem);
       strcpy (NameWkArea, filespec); /* get the filespec where we can chg it*/
       p = NameWkArea;
       if (NameWkArea[1] == chCOLON) /* Is drive specified? If so, capture it */
         {
           drivespec[ts] = *p;
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
                         sizeof(FILEFINDBUF),
                         &srchcnt,
                         0l);
#ifdef DEBUG
printf("DosFindFirst on %s returned %d\n", filespec, rc);
#endif
       if ( rc != NO_ERROR)
         {
           DosSemClear(&fmfSem);      /* clear semaphore before exiting */
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
         namespec[ts] = allfs;                   /* *.* implied              */
       else                                     /* Just \ specified as path */
         if ( (strcmp(pathstart, achBACKSLASH) == 0) ||
              (*fnstart == chDOT) )             /* or a dot directory named */
           namespec[ts] = allfs;                 /* Also implies *.*         */
         else                                 /* A file object was specified */
           {
             if (strcmp(fnstart, allfs) == 0) /* If it wasn't *.* */
               namespec[ts] = allfs;
             else                             /*    allocate some space for it*/
               if ( (namespec[ts] = (char *)malloc(strlen(fnstart) + 1)) )
                 strcpy(namespec[ts], fnstart);
               else
                 {
                   free(cp);
                   current_context[ts] = 0;
                   threads_here[i] = 0;
                   drivespec[i] = '\0';
                   ntids_here--;
                   perror("fmf_init malloc of path");
                   DosSemClear(&fmfSem);  /* clear semaphore before exiting */
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
              if (namespec[ts] != allfs)
                free(namespec[ts]);
              free(cp);
              current_context[ts] = 0;
              threads_here[i] = 0;
              ntids_here--;
              perror("fmf_init malloc of path");
              DosSemClear(&fmfSem);     /* Clear semaphone before exit */
              return(-2);
           }
       else
         cp->path = 0;





       DosSemClear(&fmfSem);     /* no need to serialize here */

#ifdef DEBUG
AssembleName(ts, NameWkArea);
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
   return(MAX_THREADS_HERE);
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
       int i, rc;
       CONTEXT *CurrContext, *NextContext;
       PIDINFO pidinfo;

                              /*----------------------------------------------*/
                              /* Find out what thread called us               */
                              /*----------------------------------------------*/
       if ( (rc = DosGetPID(&pidinfo)) != NO_ERROR )
            perror("fmf_init getPID");
       else
         pidinfo.tid = 0;
#ifdef DEBUG
printf("fmf_close - thread id is %d\n", pidinfo.tid);
#endif
                              /*----------------------------------------------*/
                              /* If that thread is among the active threads,  */
                              /* deallocate resources devoted to the thread   */
                              /* and take it off the active list.             */
                              /*----------------------------------------------*/
       for (i = 0; i < MAX_THREADS_HERE; i++)
         if  (threads_here[i] == pidinfo.tid)
           { CurrContext = current_context[i];
             do
               {
                 if (namespec[i] != allfs)
                   free(namespec[i]);
                 drivespec[i] = '\0';
                 if (CurrContext->path)
                   free(CurrContext->path);
                 NextContext = CurrContext->BackPointer;
                 free(CurrContext);
                 CurrContext = NextContext;
               }    while (CurrContext);
             current_context[i] = 0;
             if (namespec[i])
               if (namespec[i] != allfs)
                 free(namespec[i]);
             namespec[i] = 0;
             threads_here[i] = 0;
             DosSemWait(&fmfSem, -1);     /* serialize this section */
             DosSemSet(&fmfSem);
             ntids_here--;
             DosSemClear(&fmfSem);     /* no need to serialize here */
             break;
           }
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
       int i, rc, srchcnt;
       PIDINFO pidinfo;
       int fmfProcess(int indx, int *rc, char *NameTo, int *AttrTo);
                              /*----------------------------------------------*/
                              /* Find out what thread called us               */
                              /*----------------------------------------------*/
       if ( (rc = DosGetPID(&pidinfo)) != NO_ERROR )
         {  perror("fmf_next getPID");
            return(rc);
         }
#ifdef DEBUG
printf("fmf_next - thread id is %d\n", pidinfo.tid);
#endif
                              /*----------------------------------------------*/
                              /* If that thread is among the active threads,  */
                              /* decide if a FindFirst has been done yet.  Do */
                              /* a FindFirst or a FindNext accordingly.       */
                              /* The routine fmfProcess returns               */
                              /* YES if the work is done and we can return to */
                              /* our caller; or NO to indicate we need to go  */
                              /* through the while loop another time.         */
                              /*----------------------------------------------*/
       DosSemWait(&fmfSem, -1);     /* serialize this section */
       DosSemSet(&fmfSem);
       for (i = 0; i < MAX_THREADS_HERE; i++)
         if  (threads_here[i] == pidinfo.tid)
             while (1)
               {
                 cp = current_context[i];
                 srchcnt = 1;
#ifdef DEBUG
printf("fmf_next: dhandle is %d\n", cp->dhandle);
#endif

                 if (cp->dhandle == -1)              /* No FindFirst yet */
                   {
                   AssembleName(i, NameWkArea);
#ifdef DEBUG
printf("fnf_next FindFirst name is %s\n", NameWkArea);
#endif
                   rc = DosFindFirst(NameWkArea,
                                     &(cp->dhandle),
                                     mask[i] | FILE_DIRECTORY,
                                     &ffb,
                                     sizeof(FILEFINDBUF),
                                     &srchcnt,
                                     0l);
                   }
                 else                               /* FindFirst already done */
                   rc = DosFindNext( cp->dhandle,
                                     &ffb,
                                     sizeof(FILEFINDBUF),
                                     &srchcnt);
#ifdef DEBUG
printf("rc from DosFind* was %d\n", rc);
#endif

                 if (fmfProcess(i, &rc, CopyNameTo, CopyAttrTo))
                   {
                     DosSemClear(&fmfSem);     /* Clear semephore before exit */
                     return(rc);
                   }
                }
       DosSemClear(&fmfSem);     /* no need to serialize here */
       return(NO_ERROR);
}



/*----------------------------------------------------------------------------*/
/*  fmfProcess                                                                */
/*                                                                              */
/* Handle the logic that required to decide whether to return a file name,    */
/* to look for another one in the current search, or to start a new search    */
/* context.  The logic is common after either a FindFirst or a FindNext       */
/*----------------------------------------------------------------------------*/

int fmfProcess(int index, int *ReturnCode, char *PutFileName, int *PutAttr)
{
     CONTEXT *cp, *sBP, *newcp;
     int     rc, pathlen, new_context;

#ifdef DEBUG
printf("fmfProcess - into routine\n");
#endif

     cp = current_context[index];
     rc = *ReturnCode;   /* return code from DosFindFirst or DosFindNext */
                              /*----------------------------------------------*/
                              /* If a system error occured, or the path was   */
                              /* bad, exit.                                   */
                              /*----------------------------------------------*/
     if (rc != NO_ERROR)
       {
         if (rc != ERROR_NO_MORE_FILES)
           {
             perror("fmf_next DosFindFirst");
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
             current_context[index] = sBP;
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
       if (mode[index] == 1)       /* We are walking subdirectories */
         {                         /* create a new context */
           if ( (newcp = (CONTEXT *)malloc(sizeof(CONTEXT))) == 0)
             {  perror("fmf_process malloc of context");
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
               perror("fmf_process malloc of path");
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
           current_context[index] = newcp;
           new_context = YES;
#ifdef DEBUG
printf("fmfProcess: created new context, path = %s\n", newcp->path);
#endif
         }
                              /*----------------------------------------------*/
                              /* If it the attributes indicate it's one on the*/
                              /* files the caller wants to see, return it     */
                              /*----------------------------------------------*/
#ifdef DEBUG
printf("Search mask: %x  attribute: %x\n", mask[index], ffb.attrFile);
#endif
     *ReturnCode = NO_ERROR;
     if (mask[index] & ffb.attrFile)
       {
         AssemblePath(index, PutFileName);
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
void AssembleName(int slot, char *where)
{
  CONTEXT *cp;

  cp = current_context[slot];
  AssemblePath(slot, where);
  if ( (cp->path) && (strcmp(cp->path, achBACKSLASH) != 0) )
    strcat (where, achBACKSLASH);
  strcat(where, namespec[slot]);
}

/*----------------------------------------------------------------------------*/
/* private routine to assemble a proper path name                             */
/*----------------------------------------------------------------------------*/
void AssemblePath(int slot, char *where)
{
  char *p;

  p = where;
  *p = '\0';
  if (drivespec[slot])
    {
      *p++ = drivespec[slot];
      *p++ = chCOLON;
    }
  if (current_context[slot]->path)
    strcpy(p, current_context[slot]->path);
  else
    *p = '\0';
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

   int attr, wkctr, threads, maxthreads, i, rc;
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


   threads = 0;
   for (i = 2; i < argc; i++)
      if (_beginthread(dir, NULL, 4096, argv[i]) == -1)
        perror("main - starting thread");
      else
        threads++;

   if ( (rc = fmf_init(argv[1], FILE_HIDDEN | FILE_DIRECTORY | FILE_SYSTEM, 1)
                                                                  != NO_ERROR) )
     printf("Thread 1: fmf_init failed, rc = %d\n", rc);
   else
     while (rc == NO_ERROR)
       {
         DosSleep(75L);
         rc = fmf_return_next(name, &attr);
         if (rc == NO_ERROR)
           printf("Thread 1: %s\n", name);
       }

   do
     {
       DosSemWait(&ctrSem, SEM_INDEFINITE_WAIT);
       DosSemSet(&ctrSem);
       wkctr = ctr;
       DosSemClear(&ctrSem);
       if (wkctr < threads)
         DosSleep(500L);
     } while (wkctr < threads);

   printf("Thread %d ending\n", *_threadid);
}

void dir(char *spec)
{
   int tid, attr, rc;
   char name[CCHMAXPATH];

   tid = *_threadid;
   if ( (rc = fmf_init(spec, FILE_ARCHIVED, 0) != NO_ERROR) )
     printf("Thread %d: fmf_init failed, rc = %d\n", tid, rc);
   else
     while (rc == NO_ERROR)
       {
         DosSleep(75L);
         rc = fmf_return_next(name, &attr);
         if (rc == NO_ERROR)
           printf("Thread %d: %s\n", tid, name);
       }

   if (tid != 1)
     {
      DosSemWait(&ctrSem, SEM_INDEFINITE_WAIT);
      DosSemSet(&ctrSem);
      ctr++;
      DosSemClear(&ctrSem);
      printf("Thread %d ending\n", tid);
     }
}

#endif
