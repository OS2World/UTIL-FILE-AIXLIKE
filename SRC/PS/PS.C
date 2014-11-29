static char sccsid[]="@(#)69	1.1  src/ps/ps.c, aixlike.src, aixlike3  9/27/95  15:45:25";
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
/*  @1  05/02/91 rc = 8 reported by Ron Schwabel                         */
/*  @2  05/02/92 Add support for OS/2 V2.0                               */
/*  @3  09/16/92 Make return code specific.                              */
/*  @4  10/26/94 Make code compile under C-Set++ (GCW)                   */
/*  @5  09/06/95 Add AIX_CORRECT--sort, lowercase, forward slashes       */
/*-----------------------------------------------------------------------*/
/* The OS/2 process model is so different, you should be happy to get */
/* anything at all when you enter ps.                                 */

/* You can put anything on the command line.  Anything but -? or      */
/* -help will be ignored.                                             */

/* See tell_usage() for an explanation of the fields displayed */

#include <idtag.h>	/* package identification and copyright statement */
#include <stdio.h>
#include <string.h>
#ifdef AIX_CORRECT						/* @5a */
#include <stdlib.h>						/* @5a */
#include <ctype.h>						/* @5a */
#endif /* AIX_CORRECT *						/* @5a */
#define INCL_BASE
#ifndef I16		/* Pull in 16-bit headers @4 */
#define INCL_16
#endif /* I16 */
#include <os2.h>
#include "ps.h"

//char buf[20000];            /* increase buf from 8k to 20k  @1 @2d*/
char buf[BUFSIZE];            /* use 64k buffer as recommended for V2   @2*/

//struct pnode {   USHORT code;          //@2d
//                 USHORT next;          //@2d
//             } pn;                     //@2d
struct node pn;                          //@2a

#ifdef AIX_CORRECT						/* 5a */
struct plist_node {						/* 5a */
                    struct qsPrec prec;				/* 5a */
                    struct plist_node *next;			/* 5a */
                  };						/* 5a */
struct plist_node *add_node(struct plist_node *list, struct qsPrec *prec); /* 5a */
void print_nodes(struct plist_node *list);			/* 5a */
#endif /* AIX_CORRECT */					/* 5a */

int  version_check(void);
void qProcStatus(int version);
int  parse_command_line(int, char **);
int threadcnt(struct node *);
char *modname(struct node *, USHORT);
char *modname2(unsigned short module);
void tell_usage(void);
void *makeptr(void *ref);
#ifdef AIX_CORRECT						/* @5a */
int aix_correct;						/* @5a */
#endif /* AIX_CORRECT */					/* @5a */

/************************************************************************/
/*    main()                                                            */
/************************************************************************/
int main(int argc, char **argv)                                 /* @3c */
{
   int version;

#ifdef AIX_CORRECT						/* @5a */
   aix_correct = (getenv("AIX_CORRECT") != NULL);		/* @5a */
#endif /* AIX_CORRECT */					/* @5a */
   version = version_check();
   if (version != BAIL_OUT)
     if (parse_command_line(argc, argv) != BAIL_OUT)
       qProcStatus(version);
     else                                                      /* @3a */
       return(BAIL_OUT);                                       /* @3a */
   else                                                        /* @3a */
     return(BAIL_OUT);                                         /* @3a */
   return(0);                                                  /* @3a */
}

/************************************************************************/
/*    version_check()                                                   */
/************************************************************************/
int version_check()
{
   int ver, major, minor;
#ifndef I16			/* Use V2.X DosQuerySysInfo() in  @4 */
   unsigned long buffer[2];	/* place of V1.X DosGetVersion()     */

   DosQuerySysInfo(11, 12, buffer, sizeof(buffer));
   major = buffer[0] / 10;
   minor = buffer[1] / 10;
#else
   DosGetVersion(&ver);
   major = HIBYTE(ver) / 10;
   minor = LOBYTE(ver) / 10;
#endif /* I16 */
   if (major != THIS_VERSION)
     {
//        printf("ps: this version is not compatible with OS/2 version %d\n",
//               major);
//        return(BAIL_OUT);
        return(VERS20);
     }
   if (minor != THIS_REVISION)
     printf("ps: warning.  This version is guaranteed to work only on OS/2 %d.%d and higher\n",
             THIS_VERSION, THIS_REVISION);
   return(VERS13);
}

/************************************************************************/
/*    qProcStatus()                                                     */
/************************************************************************/
void qProcStatus(int version)
{

   struct node *pn;
   struct procnode *ppn;
   struct qsPrec *pPr;
   struct qsTrec *pTr;
   char   *p;
   int rc;
   unsigned short pid, ppid, hMte, cTCB, cLib, c16Sem, cShrMem;
   unsigned long  sgid;
#ifdef AIX_CORRECT						/* @5a */
   struct plist_node *plist = NULL;				/* @5a */
#endif /* AIX_CORRECT */					/* @5a */

   if ( (rc = DosQProcStatus(buf, sizeof(buf))))
     printf("ps: DosQProcStatus failed. rc = %d.\n", rc);
   else
     {
        printf("  PID  PPID   SID MHNDL THRDS  MODS  SEMS  SSEG COMMAND\n");

                                       /* ============> Version 1.x        */

        if (version == VERS13)
          {
            pn = (struct node *)buf;
            while (pn->code != -1)
              {
                if (pn->code == PNODE)
                  {
                    ppn = (struct procnode *)pn;
                    printf("%5u %5u %5u %5u %5d %5d %5d %5d %s\n",
                            ppn->pid,
                                ppn->ppid,
                                    ppn->sid,
                                        ppn->hmod,
                                            threadcnt(pn),
                                                ppn->mnodecnt,
                                                    ppn->ssnodecnt,
                                                        ppn->segnodecnt,
                                                            modname(pn, ppn->hmod));
              }
            OFFSETOF(pn) = pn->next;
          }
       }

                                       /* ============> Version 2.x        */

     else  /* if version == VERS20 */
       {
         pPr = (struct qsPrec *)makeptr(((struct qsPtrrec *)buf)->pProcRec);
         while (pPr->RecType == PROCESS_RECORD_TYPE)
           {
#ifdef AIX_CORRECT
             if(aix_correct)					/* @5a */
               plist = add_node(plist, pPr);			/* @5a */
             else {						/* @5a */
               p       = modname2(pPr->hMte);			/* @5a */
               pid     =    pPr->pid;				/* @5a */
               ppid    =   pPr->ppid;				/* @5a */
               sgid    =   pPr->sgid;				/* @5a */
               hMte    =   pPr->hMte;				/* @5a */
               cTCB    =   pPr->cTCB;				/* @5a */
               cLib    =   pPr->cLib;				/* @5a */
               c16Sem  = pPr->c16Sem;				/* @5a */
               cShrMem = pPr->cShrMem;				/* @5a */
  //             printf("Here's process %5u ppid %5u sgid %5Ld hMte %5u\n", pid, ppid, sgid, hMte); /* @5a */
               printf("%5u %5u %5u %5u %5u %5u %5u %5u %s\n",	/* @5a */
                       pid,
                           ppid,				/* @5a */
                               (unsigned short)sgid,		/* @5a */
                                   hMte,			/* @5a */
                                       cTCB,			/* @5a */
                                           cLib,		/* @5a */
                                               c16Sem,		/* @5a */
                                                   cShrMem,	/* @5a */
                                                        p);	/* @5a */
             }							/* @5a */
  #else								/* @5a */
             p       = modname2(pPr->hMte);
             pid     =    pPr->pid;
             ppid    =   pPr->ppid;
             sgid    =   pPr->sgid;
             hMte    =   pPr->hMte;
             cTCB    =   pPr->cTCB;
             cLib    =   pPr->cLib;
             c16Sem  = pPr->c16Sem;
             cShrMem = pPr->cShrMem;
//             printf("Here's process %5u ppid %5u sgid %5Ld hMte %5u\n", pid, ppid, sgid, hMte);
             printf("%5u %5u %5u %5u %5u %5u %5u %5u %s\n",
                     pid,
                         ppid,
                             (unsigned short)sgid,
                                 hMte,
                                     cTCB,
                                         cLib,
                                             c16Sem,
                                                 cShrMem,
                                                      p);
#endif /* AIX_CORRECT */					/* @5a */
             pTr = (struct qsTrec *)makeptr(pPr->pThrdRec);
             pTr += pPr->cTCB;
             pPr = (struct qsPrec*)pTr;
           }
       }
#ifdef AIX_CORRECT						/* @15a */
       if(aix_correct)						/* @15a */
         print_nodes(plist);					/* @15a */
#endif /* AIX_CORRECT */					/* @15a */
     }
}

/************************************************************************/
/*    threadcnt() - in 1.x, you have to count the thread records        */
/************************************************************************/
int threadcnt(struct node *pn)
{
    struct node *tpn;
    int thds;

    thds = 0;
    tpn = pn;
    for (OFFSETOF(tpn)=pn->next; tpn->code == TNODE ; OFFSETOF(tpn) = tpn->next)
      thds++;
    return(thds);
}

/************************************************************************/
/*    modname() - this is how you get the module name in 1.x            */
/************************************************************************/
char *modname(struct node *pn, USHORT hmod)
{
   struct node *mpn;
   struct modulenode *mn;
   int done = 0;
   char *name;

   mpn = pn;
   for (OFFSETOF(mpn) = pn->next;
           mpn->code != -1 && !done;
               OFFSETOF(mpn)=mpn->next)
     {
       if (mpn->code == MNODE)
         {
           mn = (struct modulenode *)mpn;
           if (mn->hmod == hmod)
             {
               name = (char *)mn;
               OFFSETOF(name) = mn->name;
               if (mn->name)
                 return(name);
               else
                 done = 1;
             }
         }
     }
   return("not found");
}

/************************************************************************/
/*    modname2() - this is how you get the module name in 2.0           */
/************************************************************************/
char *modname2(unsigned short module)
{
   struct qsLrec *q;
#ifdef AIX_CORRECT
   char *c, *n;
#endif /* AIX_CORRECT */

//   for (q = (struct qsLrec *)makeptr(((struct qsPtrrec *)buf)->pLibRec);
//          q->hmte != module && q->pNextRec;
//            q = (struct qsLrec *)makeptr(q->pNextRec));
   q = (struct qsLrec *)makeptr(((struct qsPtrrec *)buf)->pLibRec);
   while (q->hmte != module && q->pNextRec)
      q = (struct qsLrec *)makeptr(q->pNextRec);
   if (q->hmte == module)
#ifdef AIX_CORRECT						/* @5a */
     {								/* @5a */
       n = (char *)makeptr(q->pName);				/* @5a */
       if(aix_correct && (n != NULL))				/* @5a */
         {							/* @5a */
           for (c = n; *c != '\0'; c++) {			/* @5a */
             if (isupper(*c)) {					/* @5a */
               *c = (char)tolower(*c);				/* @5a */
             }							/* @5a */
             else if (*c == '\\') {				/* @5a */
               *c = '/';					/* @5a */
             }							/* @5a */
           }							/* @5a */
         }							/* @5a */
       return((char *)makeptr(q->pName));			/* @5a */
     }								/* @5a */
#else								/* @5a */
     return((char *)makeptr(q->pName));
#endif /* AIX_CORRECT */					/* @5a */
   else
     return("not found");
}


/************************************************************************/
/*    parse_command_line()                                              */
/************************************************************************/
int parse_command_line(int argc, char **argv)
{
   int i;

   for (i = 1;  i < argc; i++)
      {
         if ( (strcmp(argv[i], "-?") == 0) || (strcmp(argv[i], "-help") == 0) )
           {
              tell_usage();
              return(BAIL_OUT);
           }
      }
   return(OK);
}


/************************************************************************/
/*    tell_usage()                                                      */
/************************************************************************/
void tell_usage()
{
   printf("ps - Copyright IBM, 1990, 1992\n");
   printf("Usage:   ps\n\n");
   printf("Fields displayed are\n");
   printf("\tPID    \tProcess ID\n");
   printf("\tPPID   \tParent's Process ID\n");
   printf("\tSID    \tSession ID\n");
   printf("\tMHNDL  \tModule handle\n");
   printf("\tTHRDS  \tNumber of threads\n");
   printf("\tMODS   \tNumber of module entries\n");
   printf("\tSEMS   \tNumber of System Semaphores\n");
   printf("\tSSEG   \tNumber of Shared Segments\n");
   printf("\tCOMMAND\t\'Name\' field from def file\n");
}

/**********************************************************************/
/*  make a pointer out of the offset of the pointer passed and the    */
/*  selector of the procstat buffer.                                  */
/*  Not elegant, but utilitarian.                                     */
/**********************************************************************/
void *makeptr(void *p)
{
   unsigned long wk1, wk2, wk3;

   wk1 = (unsigned long)buf;
   wk2 = (unsigned long)p;
   wk3 = (wk1 & 0xffff0000) | (wk2 & 0x0000ffff);
   return((void *)wk3);
}
#ifdef AIX_CORRECT						/* @5a */
struct plist_node *						/* @5a */
add_node(struct plist_node *list, struct qsPrec *prec) {	/* @5a */
								/* @5a */
   struct plist_node *list_p, *newnode, *out_list;		/* @5a */
								/* @5a */
   out_list = list;						/* @5a */
   newnode = (struct plist_node *)malloc(sizeof(struct plist_node)); /* @5a */
   memcpy((void *)&(newnode->prec), (void *)prec, sizeof(struct qsPrec)); /* @5a */
   list_p = out_list;						/* @5a */
   if(list_p != NULL) {						/* @5a */
      while((list_p->next != NULL) && (list_p->next->prec.pid < prec->pid)) { /* @5a */
         list_p = list_p->next;					/* @5a */
      }								/* @5a */
   }								/* @5a */
   if(list_p == out_list) {					/* @5a */
      newnode->next = list_p;					/* @5a */
      out_list = newnode;					/* @5a */
   }								/* @5a */
   else {							/* @5a */
      newnode->next = list_p->next;				/* @5a */
      list_p->next = newnode;					/* @5a */
   }								/* @5a */
   return(out_list);						/* @5a */
}								/* @5a */
								/* @5a */
void								/* @5a */
print_nodes(struct plist_node *list) {				/* @5a */
								/* @5a */
   /* struct plist_node *list_p, *last; */			/* @5a */
   struct plist_node *list_p;					/* @5a */
								/* @5a */
   list_p = list;						/* @5a */
   while(list_p != NULL) {					/* @5a */
      printf("%5u %5u %5u %5u %5u %5u %5u %5u %s\n",		/* @5a */
             list_p->prec.pid,					/* @5a */
             list_p->prec.ppid,					/* @5a */
             (unsigned short)list_p->prec.sgid,			/* @5a */
             list_p->prec.hMte,					/* @5a */
             list_p->prec.cTCB,					/* @5a */
             list_p->prec.cLib,					/* @5a */
             list_p->prec.c16Sem,				/* @5a */
             list_p->prec.cShrMem,				/* @5a */
             modname2(list_p->prec.hMte));			/* @5a */
      /* last = list_p; */					/* @5a */
      list_p = list_p->next;					/* @5a */
      /* free(last); */						/* @5a */
   }								/* @5a */
}								/* @5a */
#endif /* AIX_CORRECT */					/* @5a */
