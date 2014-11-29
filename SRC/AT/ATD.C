static char sccsid[]="@(#)27	1.1  src/at/atd.c, aixlike.src, aixlike3  9/27/95  15:43:54";
/************************************************/
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation, 1992.       All rights reserved.*/
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 3      26Mar92                                           */
/* @1 05.03.93 grb Port to OS/2 v2                                       */
/* @2 09.26.94 grb DosAllocSharedSegment stopped working                 */
/* @3 09.14.95 gcw Redirect stderr to stdout in child cmd files          */
/*-----------------------------------------------------------------------*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <process.h>
#include <stddef.h>
#include <fmf.h>
#include "at.h"
#define INCL_DOSMEMMGR
#define INCL_DOSSESMGR
#define INCL_DOSPROCESS
#include <os2.h>

#define MAX_JOBS 100
#define SLEEP_INTERVAL 60000
#define LOGFILE "c:\\temp\\atd.log"
PBYTE pbSs;              /* This is a pointer to the shared segment */
char *tolaunch[MAX_JOBS];     /* We can launch up to 100 jobs a minute */
char *cmdexe = "C:\\OS2\\CMD.EXE";
char outdir[CCHMAXPATH];
char batfile[CCHMAXPATH];
unsigned numthreads;

#ifdef DEBUG
FILE *f = NULL;
#endif


int FindSharedSeg(void);
PBYTE CreateSharedSeg(void);
void GoFindWorkToDo(void);
void launchit(char *jobfile);
int findjobs(void);
void getoutdir(char *jobdir);
void _Optlink launchcontrol(char *jobfile);
#ifdef DEBUG
void logit(char *);
#endif

void main (int argc, char **argv)
{
   PSZ   p;

   if (!FindSharedSeg())     /* get out if a copy of us is already loaded */
     {
        pbSs = CreateSharedSeg();
        if (pbSs)
          {
            p = getenv(atjobs);
            if (p)
              strcpy(pbSs, p);
            else
              strcpy(pbSs, AtJobDir);
            getoutdir(pbSs);
            GoFindWorkToDo();
          }
     }
   else
     printf("A copy of the at daemon is already running\n");
}







int FindSharedSeg()
{
#ifdef I16                                                        /* @1a */
   SEL sel;
#else                                                             /* @1a */ 
   PVOID sel;                                                     /* @1a */ 
#endif                                                            /* @1a */ 
   USHORT rc;

#ifdef I16                                                        /* @1a */
   rc = DosGetShrSeg(AtSharedSeg, &sel);
#else                                                             /* @1a */ 
   rc = DosGetNamedSharedMem(&sel, AtSharedSeg,PAG_READ | PAG_WRITE); /* @1a */
#endif                                                            /* @1a */ 
   if (rc)
     return(0);
   else
     return(1);
}


PBYTE CreateSharedSeg()
{
#ifdef I16                                                        /* @1a */ 
   SEL sel;                                                       
#else                                                             /* @1a */ 
   PVOID sel;                                                     /* @1a */ 
#endif                                                            /* @1a */ 
   USHORT rc;

#ifdef I16                                                        /* @1a */  
   rc = DosAllocShrSeg(270, AtSharedSeg, &sel);
#else                                                             /* @1a */  
   rc = DosAllocSharedMem(&sel, AtSharedSeg, 270, PAG_READ | PAG_WRITE | PAG_COMMIT);  /* @2c */
#endif                                                            /* @1a */  
   if (rc)
     {
       return((PBYTE)NULL);
     }
   else
#ifdef I16                                                        /* @1a */  
     return((PBYTE)MAKEP(sel, 0));
#else                                                             /* @1a */  
     return((PBYTE)sel);                                          /* @1a */  
#endif                                                            /* @1a */  
}


void GoFindWorkToDo()
{
   while (1)           /* DO FOREVER */
     {
       if (findjobs() == 0 && numthreads == 0)
         break;
       DosSleep(SLEEP_INTERVAL);
     }
}


int findjobs()
{
   int jobcnt, launchcnt, attrib, i;
   char jobpath[CCHMAXPATH];
   char fn[CCHMAXPATH];
   char *p, *ext;
   time_t curtime, filetime;

   jobcnt = 0;
   launchcnt = 0;
   strcpy(jobpath, pbSs);
   strcat(jobpath, "\\*.*");
   curtime = time(NULL);
   curtime /= 60;
   if (fmf_init(jobpath, FMF_ALL_FILES, FMF_NO_SUBDIR) == 0)
     {
       while ((fmf_return_next(fn, &attrib) == 0) && (launchcnt < MAX_JOBS))
         {
            for (p = fn; *p; p++);
            for (--p; *p != PERIOD && p != fn; p--);
            if (*p == PERIOD)
              {
                 ext = p+1;
                 for (--p; *p != BACKSLASH && *p != COLON && p != fn; p--);
                 filetime = (100 * atol(++p)) + atol(ext);
                 filetime /= 60;
                 if (filetime <= curtime)
                   {
                     tolaunch[launchcnt] = (char *)malloc(strlen(fn) + 1);
                     if (tolaunch[launchcnt] == NULL)
                       printf("Out of memory.\n");
                     strcpy(tolaunch[launchcnt], fn);
                     launchcnt++;
                   }
              }
            jobcnt++;
         }
       if (launchcnt > 1)
         qsort(tolaunch, launchcnt, sizeof(char *),
               (int (* _Optlink) (const void *, const void *)) strcmp);
       for (i = 0; i < launchcnt; i++)
          {
            launchit(tolaunch[i]);
//            remove(tolaunch[i]);
//            free(tolaunch[i]);
          }
     }
   return(jobcnt);
}



void launchit(char *jobfile)
{

#ifdef DEBUG
   time_t tim;
   struct tm *tm;
   tim = time(NULL);
   tm = localtime(&tim);
   printf("Launching job %s at %s\n", jobfile, asctime(tm));
#endif
   numthreads++;
   _beginthread( (void (* _Optlink) (void *)) launchcontrol, NULL, 4096,
                 jobfile);
}


void _Optlink launchcontrol(char *jobfile)
{
   unsigned tid;
   char *ourbat, *p;
   char parmstr[256];
#ifndef I16                                                  /* @2a */
   PTIB pptib;                                                  /* @2a */
   PPIB pppib;                                                  /* @2a */
#endif                                                       /* @2a */ 

#ifdef I16                                                   /* @2a */ 
   tid = *_threadid;
#else                                                        /* @2a */ 
   DosGetInfoBlocks(&pptib, &pppib);                             /* @2a */
   tid = (unsigned)((pptib)->tib_ordinal);                   /* @2a */
#endif                                                       /* @2a */ 
                               /* Create a bat file name unique to this thread*/
   ourbat = (char *)malloc(strlen(batfile) + 1);
   strcpy(ourbat, batfile);
   p = strstr(ourbat, "xxxxx");
   sprintf(p, "%05u", tid);
   strcat(ourbat, ".cmd");
   rename(jobfile, ourbat);
   strcpy(parmstr, " >>");
   strcat(parmstr, outdir);
   strcat(parmstr, "\\");
   for (p = jobfile; *p; p++);
   for (--p; *p != BACKSLASH && *p != COLON && p != jobfile; p--);
   if (p == jobfile)
     strcat(parmstr, p);
   else
     strcat(parmstr, ++p);
   strcat(parmstr, " 2>&1");				/* @3a */
#ifdef DEBUG
   printf("%s /C %s %s\n", cmdexe, ourbat, parmstr);
#endif
                               /* Build the CMD.EXE command line */
   spawnl(P_WAIT, cmdexe, cmdexe, "/C", ourbat, parmstr, NULL);
   remove(ourbat);
   free(ourbat);
   numthreads--;
   _endthread();
}



#ifdef DEBUG
void logit(char *s)
{
   if (f == NULL)
     f = fopen(LOGFILE, "w");
   fputs(s, f);
}
#endif



void getoutdir(char *jobdir)
{
   char *p;

   strcpy(outdir, jobdir);
   for (p = outdir; *p; p++);
   for (--p; *p != BACKSLASH && *p != COLON && p != outdir; p--);
   if (p != outdir)
     p++;
   strcpy(p, outjobdir);
   mkdir(outdir);
   strcpy(batfile, outdir);
   strcat(batfile, "\\xxxxx.cmd");
}

