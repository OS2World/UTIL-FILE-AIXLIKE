/* @(#)71	1.1  src/ps/ps.h, aixlike.src, aixlike3  9/27/95  15:45:29 */
/* ------------------------------------------------------------------------- */
/* --------------Definitions common to OS/2 V1.3 and OS/2 V2.0-------------- */
/* ------------------------------------------------------------------------- */

/* Prototype for DosQProcStatus()  */

#ifndef I16
USHORT _Far16 _Pascal DosQProcStatus( PVOID pBuf, USHORT cbBuf );
#else
USHORT APIENTRY DosQProcStatus( PVOID pBuf, USHORT cbBuf );
#endif /* I16 */

/* Size of the buffer into which process status info will be read */
#define BUFSIZE  (unsigned short)64000
#define VERS13  13
#define VERS20  20
#define BAIL_OUT -1
#define OK        0
#define THIS_VERSION 1       /* compiled for use with OS/2 1.x */
#define THIS_REVISION 3      /* specifically, 1.3 */


/* ------------------------------------------------------------------------- */
/* --------------------Definitions specific to OS/2 V1.3-------------------- */
/* ------------------------------------------------------------------------- */

/* structures needed to interpret the process information returned by */
/* DosQProcStatus(), plus some other things that might be generally useful */

struct node {
              USHORT       code;      /* type of node */
              USHORT       next;      /* offset within returned data of the */
                                      /* next node. This is actually the    */
                                      /* offset from the SEGMENT containing */
                                      /* the buffer, not from the buffer    */
                                      /* itself.  Ugly, huh?                */
            };

/* These are the possible values of node.code: */

#define PNODE         0               /* A process node: points to other data*/
#define TNODE         1               /* A thread node: thread info */
#define MNODE         2               /* A module node: module info */
#define SEMNODE       3               /* A system semaphone node */
#define SSNODE        4               /* A shared segment node */

/* There is one special value for next: */

#define EOC        0xffff             /* End of chain */

/* Process node:  contains pid and ppid, and points to associated info */

struct procnode {
            USHORT       code;        /* node type: 0 */
            USHORT       next;        /* offset of next node */
            USHORT       pid;         /* process id */
            USHORT       ppid;        /* process id of parent */
            USHORT       sid;         /* session id */
            USHORT       hmod;        /* module handle of executable code */
            USHORT       modinfo;     /* offset of first mnode */
            USHORT       mnodecnt;    /* number of mnodes associated */
            USHORT       ssinfo;      /* offset of first ssnode */
            USHORT       ssnodecnt;   /* number of ssnodes associated */
            USHORT       seginfo;     /* offset of first segnode */
            USHORT       segnodecnt;  /* number of segnodes associated */
                };

/* There is no explicit pointer or counter for thread nodes.  The first one */
/* follows immediately the process node.                                    */

struct threadnode {
            USHORT       code;        /* node type: 1 */
            USHORT       next;        /* offset of next thread node */
            USHORT       hmod;        /* module handle of executable code */
            USHORT       pid;         /* process id */
            USHORT       tid;         /* thread id */
            USHORT       priority;    /* thread priority */
            USHORT       state;       /* thread state */
            ULONG        blockid;     /* block id */
                  };

/* These are the possible thread states */

#define TSTART          0
#define TZOMBIE         1
#define TREADY          2
#define TBLOCKED        3
#define TFROZEN         4
#define TCRITSEC        5
#define TBACKGROUND     6
#define TBOOST          7

/* Structure of a module node */

struct modulenode {
            USHORT       code;        /* node type: 2 */
            USHORT       next;
            USHORT       hmod;
            USHORT       depcount;    /* number of dependent modules */
            USHORT       deps;        /* offset of first dependent module node*/
            USHORT       name;        /* offset of 0-terminated module name */
            USHORT       deplist;     /* dependency list ???? */
                  };

/* Structure of a semaphore node */

struct semaphorenode {
            USHORT       code;        /* note type:  3 */
            USHORT       next;
            USHORT       flag;
            BYTE         refs;        /* number of references */
            BYTE         reqs;        /* number of requests outstanding */
            USHORT       index;       /* ? */
            BYTE         name;        /* name of semaphore in ascii */
                     };

/* Structure of a shared segment node */

struct ssegnode {
            USHORT       code;        /* 4 */
            USHORT       next;
            USHORT       handle;      /* handle of the shared segment */
            SEL          sel;         /* selector of the shared segment */
            USHORT       refs;        /* number of references to this ss */
            char         name;        /* segment name */
              };

/* ------------------------------------------------------------------------- */
/* --------------------Definitions specific to OS/2 V2.0-------------------- */
/* ------------------------------------------------------------------------- */

/* Layout of the pointer record (first 24 bytes of the structure) */
struct qsPtrrec {
                  struct qsGrec       *pGlobalRec; /* 'global' record */
                  struct qsPrec       *pProcRec;   /* first process record */
                  struct qsS16Headrec *p16SemRec;  /* 1st 16-bit sem record */
                  void                *Reserved1;  /* ***not in docs***     */
                  struct qsMrec       *pShrMemRec; /* 1st shared mem record */
                  struct qsLrec       *pLibRec;    /* 1st module (mte) rec  */
                };

/* Layout of the Global Record (next 12 bytes)  */
struct qsGrec   {
                  unsigned long       cThrds;   /* active threads in system  */
                                                /* at time of call           */
                  unsigned long       Reserved1;
                  unsigned long       Reserved2;
                };

/* Layout of the Process record (idendified by record type 1) */
#define PROCESS_RECORD_TYPE      1
struct qsPrec   {
                  unsigned long       RecType;     /* PROCESS_RECORD_TYPE */
                  struct qsTrec      *pThrdRec;    /* First thread record */
                  unsigned short      pid;         /* Process id */
                  unsigned short      ppid;        /* Parent pid */
                  unsigned long       type;        /* Process type */
                  unsigned long       stat;        /* Process status */
                  unsigned long       sgid;        /* Screen group id */
                  unsigned short      hMte;        /* Module handle */
                  unsigned short      cTCB;        /* number of threads */
                  unsigned long       Reserved1;
                  void               *Reserved2;
                  unsigned short      c16Sem;      /* number of 16-bit sems */
                  unsigned short      cLib;        /* number of dlls        */
                  unsigned short      cShrMem;     /* number of shared mem handles */
                  unsigned short      Reserved3;
                  unsigned short     *p16SemInd;   /* ptr to 16-bit sem indices*/
                  unsigned short     *pLibRec;     /* ptr to dll mte handles */
                  unsigned short     *pShrMemRec;  /* ptr to shared mem handles */
                  void               *Reserved4;
                };
/* each Process record is followed by
       - a list of 16-bit semephore indices (c16Sem of them).
       - a list of dll mte handles (cLib of them).
       - a list of shared memory handles (cShrMem of them).
       - 1 or more thread records (cTCB of them).
*/

/* Layout of the thread record */
#define THREAD_RECORD_TYPE  256
struct qsTrec   {
                  unsigned long       RecType;     /* THREAD_RECORD_TYPE */
                  unsigned short      tid;         /* relative thread id */
                  unsigned short      slot;        /* unique thread id   */
                  unsigned long       sleepid;     /* sleep id if thread is sleeping */
                  unsigned long       priority;    /* thread priority */
                  unsigned long       systime;     /* thread system time */
                  unsigned long       usertime;    /* thread user time */
                  unsigned char       state;       /* thread state */
                  unsigned char       PAD1;
                  unsigned short      PAD2;
                };
/* Here are some of the thread states: */
#define TS_READY   1
#define TS_BLOCKED 2
#define TS_RUNNING 5

/* Layout of the DLL Module Record */
struct qsLrec  {
                  struct LibRec      *pNextRec;    /* next DLL record */
                  unsigned short     hmte;         /* handle for this module */
                  unsigned short     Reserved1;
                  unsigned long      cImpMod;      /* number of imported modules */
                  unsigned long      Reserved2;
                  void              *Reserved3;
                  unsigned char     *pName;        /* module path and name */
                };

/* The DLL record is followed by a list of imported module handles (cImpMod
   of them); and by the pathname (pointed to by pName), which is an
   ASCIIZ string.
*/

/* Layout of the 16-bit semaphore record */
struct qsS16Head {
                  unsigned long      SRecType;
                  unsigned long      SpNextRec;
                  unsigned long      S32SemRec;
                  unsigned long      S16TblOff;
                  unsigned long      pSem16Rec;
                 };

struct SysSemTbl {
                  unsigned long      s_SysSemOwner;  /* owning thread */
                  unsigned char      s_SysSemFlag;   /* bit flags */
                  unsigned char      s_SysSemRefCnt; /* number of references */
                  unsigned char      s_SysSemProcCnt;/* requests by owner */
                  unsigned char      Reserved1;
                  unsigned long      Reserved2;
                  unsigned long      Reserved3;
                 };

struct qsS16rec {
                  unsigned long      NextRec;
                  struct SysSemTbl   SysSemData;
                  unsigned char      SemName;    /* first byte of asciiz name */
                };

/* Layout of the shared memory segment record */
struct qsMrec   {
                  unsigned long      MemNextRec;
                  unsigned short     hmem;           /* handle */
                  unsigned short     sel;            /* shared mem selector */
                  unsigned short     refcnt;         /* reference count */
                  unsigned char      MemName;        /* first byte of name */
                };
