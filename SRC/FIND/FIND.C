static char sccsid[]="@(#)45	1.1  src/find/find.c, aixlike.src, aixlike3  9/27/95  15:44:34";
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
/* @1 05.07.91 -size predicate wouldn't match a 0-length file            */
/* @2 05.07.91 -ok prompt didn't start on new line                       */
/* @3 05.07.91 -multiple path specs could bomb with trap D               */
/* @4 08.23.91 - -name didn't find names that have no dots               */
/* @5 11.05.91 - -newer didn't work at all.  Marty Klos found it.        */
/* @6 02.25.91 - -o only worked in pairs.  Longer strings used last 2    */
/* @7 07.02.92 - -exec and -ok required a space before terminationg ;    */
/* @8 07.02.92 - -exec and -ok should invoke command processor           */
/* @9 09.16.92 Make return code explicit.                                */
/* @10 11.17.92 - make {} expand anywhere in the -exec arguments         */
/* @11 11.18.92 - we were failing when there  was only one predicate     */
/*-----------------------------------------------------------------------*/
/* find - finds files with a matching expression */

/*
   The find command recursively searches the directory tree for each specified
   path seeking files that match a Boolean Expression written using the terms
   given below.  The output from the find command depends on the terms
   used in Expression.

        find   Path [Path ...] Expression

   Note:  In the following descriptions, the parameter Number is a decimal
   integer that can be specified as +Number (more than Number), -Number
   (less than Number), or Number (exactly Number).

   -fstype Type            True if the file system to which the file
                           belongs is of the type Type, where Type is
                           jfs (journaled file system) or nfs (networked
                           file system).

                           In OS/2, ALWAYS TRUE.

   -inum Number            True if the i-node is Number.
                           In OS/2, ALWAYS TRUE.

   -name FileName          True if FileName matches the file name.  You
                           can use pattern-matching characters, provided
                           they are quoted.

   -perm OctalNumber       In OS/2, ALWAYS TRUE.

   -prune                  In OS/2, ALWAYS TRUE.

   -type Type              True if the file Type is of the specified type,
                           as follows:

                               b   Block special file (In OS/2, ALWAYS FALSE)
                               c   Character file     (In OS/2, ALWAYS FLASE)
                               d   Directory
                               f   Plain file
                               p   FIFO (a named pipe)
                               l   Symbolic link      (In OS/2, ALWAYS FALSE)
                               s   Socket

   -links Number           True if the file has Number links.
                           In OS/2, ALWAYS FALSE.

   -user UserName          True if the file belongs to user UserName.
                           In OS/2, ALWAYS TRUE.

   -group GroupName        True if the file belongs to group GroupName.
                           In OS/2, ALWAYS TRUE.

   -nogroup                True if the file belongs to no groups.
                           in OS/2, ALWAYS TRUE.

   -size Number            True if the file is Number blocks long (512 bytes
                           per block).  For this comparision, the file size
                           is rounded up to the next higher block.

   -atime Number           True if the file has been accessed in Number days.

   -mtime Number           True if the file has been modified in Number days.

   -ctime Number           True if the file has beeen changed in Number days.

   -exec Command           True if the Command runs and returns 0.  The end
                           of Command must be punctuated by a quoted or
                           escaped semicolon.  A command parameter of {} is
                           replaced by the current path name.

   -ok Command             The find command asks you whether it should start
                           Command.  If your response begins with Y, Command
                           is started.  The end of Command must be punctuated
                           by a quoted or escaped semicolon.

   -print                  Always true; causes the current path name to be
                           displayed.  The find command does not display path
                           names unless you specify this expression term.

   -cpio Device            Write the current file to Device in the cpio
                           command format.  Always fails in OS/2.

   -newer File             True if the current file has been modified more
                           recently than the file indicated by File.


   -\(Expression)\         True if the expression in parentheses is true.

   -depth                  Always true.  Supposed to cause the descent of
                           the directory hierarchy to be done so that all
                           entries in a directory are affected before the
                           directory itself.  This can be useful when the
                           find commdna is used with the cpio command to
                           transfer files that are contained in directories
                           without write permission.
                           Does nothing at all in OS/2.

   -ls                     Always true;  prints more path information.

   -xdev                   Always true;  no effect in OS/2.

   -o                      logical or.  Juxtaposition of two terms implies
                           a logical and.

   !                       Negation of an expression term

*/

#include <idtag.h>    /* package identification and copyright statement */
#include "..\fmf\fmf.h"
#include "..\grep\grep.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <process.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <conio.h>
#define  INCL_BASE
#include <os2.h>


/* Set up codes for primary expression terms */

#define PT_FSTYPE     0
#define PT_INUM       1
#define PT_NAME       2
#define PT_PERM       3
#define PT_PRUNE      4
#define PT_TYPE       5
#define PT_LINKS      6
#define PT_USER       7
#define PT_NOUSER     8
#define PT_GROUP      9
#define PT_NOGROUP   10
#define PT_SIZE      11
#define PT_ATIME     12
#define PT_MTIME     13
#define PT_CTIME     14
#define PT_EXEC      15
#define PT_OK        16
#define PT_PRINT     17
#define PT_CPIO      18
#define PT_NEWER     19
#define PT_DEPTH     20
#define PT_EXPR      21
#define PT_SLEXPR    22
#define PT_EEXPR     23
#define PT_SLEEXPR   24
#define PT_LS        25
#define PT_XDEV      26
#define PT_OR        27
/* derived expression terms */
#define PT_GNAME     28
#define PT_DEBUG     29
#define NUM_TERMS    30      /* update this if new terms are added */

#define NEGATION   '!'
#define PATH_SEP_CHAR  '\\'
#define PATH_SEP_STR   "\\"
#define MAXPATH        261
#define GBUFSIZE       2*MAXPATH

#define NEGATION_OP    "!"

#define ON  1
#define OFF 0
#define OK  0
#define BAIL_OUT -1
#define TRUE 1
#define FALSE 0

#define FT_BLOCK 'b'
#define FT_CHAR  'c'
#define FT_DIR   'd'
#define FT_FILE  'f'
#define FT_FIFO  'p'
#define FT_LINK  'l'
#define FT_SOCK  's'

/* define a structure to hold arg and link data for each expression term */

typedef struct expr_node {
                            int   en_code;      /* term_table value         */
                            int   en_nega;      /* negation flag            */
                            int (*en_proc)();   /* handler routine          */
                                                /* pointer to args for term */
                            union               /*   Could be a             */
                                  { char *ena_pch;     /* char string       */
                                    struct expr_node *ena_node;
                                    long  ena_int;     /* integer value     */
                                    char **ena_argl;   /* list of args      */
                                  } en_args;
                            struct expr_node *en_next;  /* ptr to next term */
                         }EXPR_NODE;

/* define a structure to hold a chain of paths */
typedef struct path_node {
                            char *pn_path;               /* path name */
                            struct path_node *pn_next;   /* chain */
                         }PATH_NODE;

/* define a structure to hold processing instructions for each expression type*/

typedef struct term_entry {
                            int te_primary;     /* the expression term type */
                            char *te_tag;       /* cmd line identifier */
                            int (*te_parser)(); /* routine to parse cmd line */
                            int (*te_proc)();   /* routine to handle it */
                          }TERM_ENTRY;

/* initialize a table of processing instructions */

int defint(EXPR_NODE *enp, int *currarg, int argc, char **argv);
int defstr(EXPR_NODE *enp, int *currarg, int argc, char **argv);
int defoct(EXPR_NODE *enp, int *currarg, int argc, char **argv);
int pars_name(EXPR_NODE *enp, int *currarg, int argc, char **argv);
int pars_exec(EXPR_NODE *enp, int *currarg, int argc, char **argv);
int pars_newer(EXPR_NODE *enp, int *currarg, int argc, char **argv);
int pars_expr(EXPR_NODE *enp, int *currarg, int argc, char **argv);
int pars_eexpr(EXPR_NODE *enp, int *currarg, int argc, char **argv);

int always_true(EXPR_NODE *enp);
int always_false(EXPR_NODE *enp);
int proc_name(EXPR_NODE *enp);
int proc_type(EXPR_NODE *enp);
int proc_atime(EXPR_NODE *enp);
int proc_mtime(EXPR_NODE *enp);
int proc_exec(EXPR_NODE *enp);
int proc_ok(EXPR_NODE *enp);
int proc_print(EXPR_NODE *enp);
int proc_cpio(EXPR_NODE *enp);
int proc_newer(EXPR_NODE *enp);
int proc_ls(EXPR_NODE *enp);
int proc_size(EXPR_NODE *enp);
int proc_debug(EXPR_NODE *enp);

TERM_ENTRY term_table[NUM_TERMS] =
                          {
                             {PT_FSTYPE, "-fstype", defint,    always_true },
                             {PT_INUM,   "-inum",   defint,    always_true },
                             {PT_NAME,   "-name",   pars_name, proc_name   },
                             {PT_PERM,   "-perm",   defoct,    always_true },
                             {PT_PRUNE,  "-prune",  NULL   ,   always_true },
                             {PT_TYPE,   "-type",   defstr,    proc_type   },
                             {PT_LINKS,  "-links",  defint,    always_false},
                             {PT_USER,   "-user",   defstr,    always_true },
                             {PT_NOUSER, "-nouser", NULL   ,   always_true },
                             {PT_GROUP,  "-group",  defstr,    always_true },
                             {PT_NOGROUP,"-nogroup",NULL   ,   always_true },
                             {PT_SIZE,   "-size",   defstr,    proc_size   },
                             {PT_ATIME,  "-atime",  defstr,    proc_atime  },
                             {PT_MTIME,  "-mtime",  defstr,    proc_mtime  },
                             {PT_CTIME,  "-ctime",  defstr,    proc_mtime  },
                             {PT_EXEC,   "-exec",   pars_exec, proc_exec   },
                             {PT_OK,     "-ok",     pars_exec, proc_ok     },
                             {PT_PRINT,  "-print",  NULL   ,   proc_print  },
                             {PT_CPIO,   "-cpio",   defstr,    proc_cpio   },
                             {PT_NEWER,  "-newer",  pars_newer,proc_newer  },
                             {PT_DEPTH,  "-depth",  NULL   ,   always_true },
                             {PT_EXPR,   "(",       pars_expr, NULL        },
                             {PT_SLEXPR, "\\(",     pars_expr, NULL        },
                             {PT_EEXPR,  ")",       pars_eexpr,NULL        },
                             {PT_SLEEXPR,"\\)",     pars_eexpr,NULL        },
                             {PT_LS,     "-ls",     NULL   ,   proc_ls     },
                             {PT_XDEV,   "-xdev",   NULL   ,   always_true },
                             {PT_OR,     "-o",      NULL,      always_true },
                             {PT_GNAME,  NULL,      NULL,      NULL        },
                             {PT_DEBUG,  "-debug",  NULL,      proc_debug  }
                          };

extern char *myerror_pgm_name;
#define MAX_SUB_EXPR             12
int subexpr_depth;
EXPR_NODE *subexpr_returns[MAX_SUB_EXPR];

char wholename[MAXPATH];
char *justname;
int attr;
int debug = 0;

char *incmsg = "%s: incomplete -exec or -ok statement\n";
char *cmdexe = "CMD.EXE";
char *slashc = "/C";

/* misc function prototypes */

int parse_command_line(EXPR_NODE *rootexpr, PATH_NODE *rootpath,
                       int argc,            char **argv);
void process_a_path(PATH_NODE *pnp, EXPR_NODE *enp);
int process_a_file(char *filepath, EXPR_NODE *enp);
char *unquote(int *argp, int argc, char **argv);
long otol(char *s);
int findtag(char *p);
void tell_usage(void);

extern myerror(int rc, char *area, char *details);

/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                         M A I N  ( )                                     */
/*                                                                          */
/* ------------------------------------------------------------------------ */
int main (int argc, char ** argv)                                   /* @9c */
{
   EXPR_NODE root_node;
   PATH_NODE root_path;
   PATH_NODE *pnp;
   int rc;                                                          /* @9a */

   myerror_pgm_name = argv[0];
   root_node.en_proc = NULL;
   root_node.en_args.ena_pch = NULL;
   root_node.en_next = NULL;
   root_path.pn_path = NULL;
   root_path.pn_next = NULL;

   rc = parse_command_line(&root_node, &root_path, argc, argv);    /* @9a */
   if (rc == OK)                                                   /* @9c */
     for (pnp = &root_path; pnp; pnp = pnp->pn_next)
        process_a_path(pnp, &root_node);
   return(rc);                                                     /* @9a */
}


/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                    EXPRESSION PROCESSING ROUTINES                        */
/*                                                                          */
/* ------------------------------------------------------------------------ */
void process_a_path(PATH_NODE *pnp, EXPR_NODE *enp)
{
   char *p;
                         /* initialize the file finder: look for all types   */
                         /* of files (the -1), on all subdirectories (the 1) */
   if (fmf_init(pnp->pn_path, -1, 1) == FMF_NO_ERROR)
                         /* then process each file returned */
     while (fmf_return_next(wholename, &attr) == FMF_NO_ERROR)
         {
            for (p = wholename; *p; p++)   /* fold to upper case */
               *p = (char)toupper(*p);
            if ( (justname = strrchr(wholename, '\\')) == 0)
              justname = wholename;
            else
              justname++;
            if (process_a_file(wholename, enp) == BAIL_OUT)
              break;
         }
}

int process_a_file(char *filepath, EXPR_NODE *enp)
{
   EXPR_NODE *np;
   int subexpr_nega[MAX_SUB_EXPR];
   int rc;

   subexpr_depth = 0;
   for (np = enp; np; np = np->en_next)
     {
       if (np->en_code == PT_EXPR)      /* start of subexpression */
         {
           subexpr_nega[subexpr_depth] = np->en_nega;
           subexpr_depth++;
         }
       else
         if (np->en_code == PT_EEXPR)        /* end of a subexpression */
           {
              subexpr_depth--;
           }
         else
           {                       /* execute the handler function */
             rc = (*np->en_proc)(np);
                                   /* apply any negations */
             rc ^= np->en_nega;
             if (subexpr_depth)
               rc ^= subexpr_nega[subexpr_depth - 1];

             if (rc == FALSE)
               {                   /* if we check out false */
                                     /* see if an 'or' follows      */
                 if (np->en_next)
                   if ( (np->en_next)->en_code != PT_OR)
                     return(OK);       /* if not, we have a no-match */
               }
             else               /* if we're already true, skip an or clause */
               while ( (np->en_next) && ((np->en_next)->en_code == PT_OR)) //@6a
//               if (np->en_next)                                    @6d
//                 if ( (np->en_next)->en_code == PT_OR)             @6d
                   np = (np->en_next)->en_next;
                                   /* this is safe, because the parser makes */
                                   /* sure that something follows a -o       */
           }
     }
   return(OK);
}


int always_true(EXPR_NODE *enp)
{
   return(TRUE);
}

int always_false(EXPR_NODE *enp)
{
   return(FALSE);
}

int proc_name(EXPR_NODE *enp)
{
   return(matches_reg_expression(justname, enp->en_args.ena_pch));
}


int proc_type(EXPR_NODE *enp)
{
  char type;

  type = *(enp->en_args.ena_pch);
  if (type == FT_DIR && (attr & FILE_DIRECTORY))
    return(TRUE);
  if (type == FT_FILE && !(attr & FILE_DIRECTORY))
    return(TRUE);
  return(FALSE);
}


int proc_atime(EXPR_NODE *enp)
{
   char *p;
   char sign;
   struct stat sb;
   int elapreq;
   time_t  currtime, age;

   p = enp->en_args.ena_pch;
   if (*p == '+' || *p == '-')
     sign = *p++;
   else
     sign = ' ';
   currtime = time(NULL);
   elapreq = atoi(p);
   if (stat(wholename, &sb) == 0)
     {
       age = currtime - sb.st_atime;
       age /= 88400L;
       if (sign == '-' && age < (long)elapreq)
         return(TRUE);
       else
         if (sign == '+' && age > (long)elapreq)
           return(TRUE);
         else
           if (sign == ' ' && age == (long)elapreq)
             return(TRUE);
           else
             return(FALSE);
     }
   else
     {
        myerror(errno, "stat for atime", wholename);
        return(FALSE);
     }
}


int proc_mtime(EXPR_NODE *enp)
{
   char *p;
   char sign;
   struct stat sb;
   int elapreq;
   time_t  currtime, age;

   p = enp->en_args.ena_pch;
   if (*p == '+' || *p == '-')
     sign = *p++;
   else
     sign = ' ';
   currtime = time(NULL);
   elapreq = atoi(p);
   if (stat(wholename, &sb) == 0)
     {
       age = currtime - sb.st_mtime;
       age /= 86400L;
       if (sign == '-' && age < (long)elapreq)
         return(TRUE);
       else
         if (sign == '+' && age > (long)elapreq)
           return(TRUE);
         else
           if (sign == ' ' && age == (long)elapreq)
             return(TRUE);
           else
             return(FALSE);
     }
   else
     {
        myerror(errno, "stat for mtime", wholename);
        return(FALSE);
     }
}
/**********************************************************************/
/* proc_exec()                                                        */
/*   changed 11/17/92 (@10) to make a copy of the arguments, and      */
/*   substitute expansions of {} where necessary                      */
/**********************************************************************/
int proc_exec(EXPR_NODE *enp)
{
   int rc;
   int i,j;                                                /* @10a2 */
   char *p, *q, *mark;
   char **execargs, **freelist;

/* rc = spawnvp(P_WAIT, *(enp->en_args.ena_argl), enp->en_args.ena_argl); @8d */
   for (i = 0; enp->en_args.ena_argl[i]; i++);                 /* @10a19 */
   freelist = (char **)malloc((i+1) * sizeof(char *));
   execargs = (char **)malloc((i+1) * sizeof(char *));
   if (execargs)
     {
       for (i = 0, j = 0; enp->en_args.ena_argl[i]; i++)
          {
             p = enp->en_args.ena_argl[i];
             if ((mark = strstr(p, "{}")))
               {
                 q = malloc(strlen(p) + strlen(wholename));
                 if (q)
                   {
                     execargs[i] = q;
                     for (; p != mark; *q++ = *p++);
                     strcpy(q, wholename);
                     strcat(q, mark+2);
                     freelist[j++]=execargs[i];
                   }
               }
             else
               execargs[i] = p;
          }
       execargs[i] = NULL;
       rc = spawnvp(P_WAIT, cmdexe, execargs);        /* @8a@10c */
       for (i = 0; i < j; free(freelist[i++]));       /* @10a3 */
       free(freelist);
       free(execargs);
       if (rc)
         return(FALSE);
       else
         return(TRUE);
     }
   else
     return(FALSE);
}

int proc_ok(EXPR_NODE *enp)
{
   int yorn, i;
   char **pp;

//   printf("< ");                                              @2
   printf("\n< ");                                           // @2
/*   for (pp = enp->en_args.ena_argl; *pp; pp++)                    @8d */
   pp = enp->en_args.ena_argl;                                   /* @8a */
   for (i = 2; pp[i]; i++)                                       /* @8a */
/*     printf("%s ", *pp);                                          @8d */
     printf("%s ", pp[i]);                                       /* @8a */

   printf("> ?    ");
//   scanf("%c\n", yorn);
   yorn = getch();
   if (yorn == 'y' || yorn == 'Y')
     return(proc_exec(enp));
   else
     return(TRUE);
}

int proc_print(EXPR_NODE *enp)
{
   printf("%s\n", wholename);
   return(TRUE);
}

int proc_cpio(EXPR_NODE *enp)
{
   printf("find:  cannot create %s\n", enp->en_args.ena_pch);
   return(FALSE);
}

int proc_newer(EXPR_NODE *enp)
{
   struct stat sb;

   if (stat(wholename, &sb))
     {
        myerror(errno, "stat for newer", wholename);
        return(FALSE);
     }
   else
     {
        if (sb.st_mtime > enp->en_args.ena_int)                  /* @5c */
          return(TRUE);
        else
          return(FALSE);
     }
}

int proc_ls(EXPR_NODE *enp)
{
   struct stat sb;
   struct tm *ltimep;
   char *atimep;
   char mytime[16];
   char myattr[11];

   if (stat(wholename, &sb))
     {
        myerror(errno, "stat for ls", wholename);
        return(FALSE);
     }
   else
     {
       ltimep = localtime(&sb.st_mtime);
       atimep = asctime(ltimep);
       strncpy(mytime, atimep + 4, 15);
       mytime[15] = '\0';
       if (sb.st_mode & S_IFDIR)
         strcpy(myattr, "dir ");
       else
         strcpy(myattr, "    ");
       if (sb.st_mode & S_IREAD)
         strcat(myattr, "r");
       else
         strcat(myattr, "-");
       if (sb.st_mode & S_IWRITE)
         strcat(myattr, "w");
       else
         strcat(myattr, "-");
       if (sb.st_mode & S_IEXEC)
         strcat(myattr, "x");
       else
         strcat(myattr, "-");

       printf("%2d%5ld%8s%2d%5s%7ld %14s %s\n",
                0,                                       /* inode */
                   sb.st_size/1024 + 1,                  /* blocks */
                       myattr,                            /* attributes */
                           1,                             /* hard links */
                              "bin",                      /* owner */
                                 sb.st_size,              /* bytes */
                                      mytime,             /* date & time */
                                           wholename);     /* file name */

     }
   return(TRUE);
}


proc_size(EXPR_NODE *epn)
{
   long compsize, fsize;                                       // @1
   struct stat sb;
   char sign, *p;

   if (attr & FILE_DIRECTORY)    /* OS/2 directories don't have a size */
     return(FALSE);

   if (stat(wholename, &sb))

     {
        myerror(errno, "stat for size", wholename);
        return(FALSE);
     }
   else
     {
        p = epn->en_args.ena_pch;
        if (*p == '+' || *p == '-')
          sign = *p++;
        else
          sign = ' ';
        compsize = atol(p);
        if (sb.st_size == 0)                                   // @1
          fsize = 0L;                                          // @1
        else                                                   // @1
          fsize = sb.st_size/512 + 1;                          // @1
        if ( (sign == '+') && (compsize < fsize) )             // @1
            return(TRUE);
        else
          if ( (sign == '-') && (compsize > fsize) )           // @1
            return(TRUE);
          else
            if ( (sign == ' ') && (compsize == fsize) )        // @1
              return(TRUE);
            else
              return(FALSE);
     }
}



int proc_debug(EXPR_NODE *epn)
{
   debug ^= 1;
   return(TRUE);
}

/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                   COMMAND LINE PARSING ROUTINES                          */
/*                                                                          */
/* ------------------------------------------------------------------------ */
/* handling shell-type command lines is a real pain, because we don't have */
/* a 'real' shell to help us, and because can be either a seperator or     */
/* an escape character.  My aim is for any syntax that works in the        */
/* korn or bourne shells to work here, but I may not succeed the first time*/

int parse_command_line(EXPR_NODE *rootexpr, PATH_NODE *rootpath,
                       int argc,            char **argv)
{
   EXPR_NODE *enp;
   PATH_NODE *pnp;
   char *p, *q;
   int i, currarg, negation_flag, tempargc, code, or_precedes;
   int (*fp)();

   pnp = rootpath;
   if (argc < 2)
     {
        tell_usage();
        return(BAIL_OUT);
     }

   currarg = 1;        /* search paths come first.  store them */
   for (p = argv[currarg]; currarg < argc ; p = argv[++currarg])
     {                      /* first, see if we're done with paths: */
                            /*   An arg beginning with - can NOT be a path @3*/
//       if ((*p == '-') && (findtag(p) != -1))                          @3
       if (*p == '-')                                                 // @3
         break;                                                       // @3
       if (*p == '(')       /*   This could be a subgroup starting */
         break;
       if (*p == '\\' && *(p+1) == '(')
         break;
       if (strcmp(p, NEGATION_OP)==0)  /* Or the first operator could be negation*/
         break;
       if (*p == '\'')       /* Quoted section: could be path or expression */
         {
           tempargc = currarg;
           if ( (q = unquote(&tempargc, argc, argv)) == NULL)
             return(BAIL_OUT);
           if ( ((*q == '-') && (findtag(q) != -1))     ||
                ((*q == '(') || strcmp(q, NEGATION_OP)) ||
                ((*q == '\\') && (*(q+1) == '(')) )
             {
               free(q);
               break;
             }
           currarg = tempargc;
           p = q;
         }

       if (pnp->pn_path != NULL)    /* except the first time thru */
         if ((pnp->pn_next=(PATH_NODE *)malloc(sizeof(PATH_NODE))) == NULL)
           {
              myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "pathnode struct");
              return(BAIL_OUT);
           }
         else
           {                                                    // @3
             pnp = pnp->pn_next;
             pnp->pn_next = NULL;                               // @3
           }                                                    // @3
       pnp->pn_path = p;
     }

   if (currarg == 1)       /* if no paths were given */
     {
        tell_usage();
        return(BAIL_OUT);
     }

   negation_flag = OFF;
   enp = rootexpr;         /* now go after the expression terms */
   subexpr_depth = 0;
   for (p = argv[currarg]; currarg < argc; p = argv[++currarg])
     {
        if (*p == '\\')
          p++;

        if (strcmp(p, NEGATION_OP) == 0)
          {
             negation_flag = ON;
             continue;
          }

        if ( (i = findtag(p)) == -1)
          {
            printf("%s: bad option %s\n", argv[0], p);
            return(BAIL_OUT);
          }
                                        /* make sure conjunctions are */
                                        /* reasonable                 */
        code = term_table[i].te_primary;
        if (or_precedes)
          if (code == PT_EEXPR || code == PT_SLEEXPR || code == PT_OR)
            {
              printf("%s: Something needs to follow -o\n", argv[0]);
              return(BAIL_OUT);
            }
          else
            or_precedes = 0;
        else
          if (code == PT_OR)
            or_precedes = 1;

        if (enp->en_proc != NULL)  /* every time except the first */
          if ( (enp->en_next = (EXPR_NODE *)malloc(sizeof(EXPR_NODE))) == NULL)
            {
              myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "exprnode");
              return(BAIL_OUT);
            }
          else
            {
              enp = enp->en_next;
              enp->en_next = NULL;
              enp->en_args.ena_pch = NULL;
            }
        enp->en_code = code;
        enp->en_proc = term_table[i].te_proc;
        enp->en_nega = negation_flag;
        negation_flag = OFF;            /* process the option: */
                                     /* by calling the parsing routine */
        if ( (fp = term_table[i].te_parser) != NULL)
          if ( (*fp)(enp, &currarg, argc, argv))
            return(BAIL_OUT);

     }
   if (subexpr_depth)        /* if an expression didn't get closed */
     {
        printf("%s: missing )\n", argv[0]);
        return(BAIL_OUT);
     }

// @11d  if (enp == rootexpr)      /* if there were no expressions at all */
   if (rootexpr->en_proc == NULL)                     /* @11a */
     {
        printf("%s: no expressions given.\n", argv[0]);
        return(BAIL_OUT);
     }

   return(OK);
}

/* ---------------------  option parsing routines -------------------- */

                               /* defone: one argument                */
int defint(EXPR_NODE *enp, int *currarg, int argc, char **argv)
{
   int i;

   enp->en_args.ena_int = 0L;
   i = *currarg + 1;
   if (i >= argc)
     return(OK);
   enp->en_args.ena_int = atol(argv[i]);
   *currarg = i;
   return(OK);
}

int defstr(EXPR_NODE *enp, int *currarg, int argc, char **argv)
{
   int i;

   enp->en_args.ena_pch = NULL;
   i = *currarg + 1;
   if (i >= argc)
     return(OK);
   if (*argv[i] == '\'')
     {
       if ( (enp->en_args.ena_pch = unquote(&i, argc, argv)) == NULL)
         return(BAIL_OUT);
     }
   else
     enp->en_args.ena_pch = argv[i];
   *currarg = i;
   return(OK);
}

int defoct(EXPR_NODE *enp, int *currarg, int argc, char **argv)
{
   int i;
   long otol(char *);

   enp->en_args.ena_int = 0L;
   i = *currarg + 1;
   if (i >= argc)
     return(OK);
   enp->en_args.ena_int = otol(argv[i]);
   *currarg = i;
   return(OK);
}


int pars_expr(EXPR_NODE *enp, int *currarg, int argc, char **argv)
{                      /* start of a sub-expression */
    if (++subexpr_depth > MAX_SUB_EXPR)
      {
         printf("%s: too many nested expressions.  Maximum nesting %d\n",
                 argv[0], MAX_SUB_EXPR);
         return(BAIL_OUT);
      }

    subexpr_returns[subexpr_depth - 1] = enp;
    enp->en_code = PT_EXPR;
    return(OK);
}

int pars_eexpr(EXPR_NODE *enp, int *currarg, int argc, char **argv)
{
   if (subexpr_depth == 0)
     {
        printf("%s: missing (\n", argv[0]);
        return(BAIL_OUT);
     }
   enp->en_args.ena_node = subexpr_returns[subexpr_depth - 1];
   subexpr_depth--;
    enp->en_code = PT_EEXPR;
   return(OK);
}

/* ----------------------------------- */
/* pars_name()                         */
/*                                     */
/* assembles the argument to the -name */
/* term; puts in a new-line pattern    */
/* for grep; inserts escape characters */
/* before all periods so that the grep */
/* compiler doesn't treat them as      */
/* ambiguous; converts any DOS-type    */
/* asterisks to the grep equivalent    */
/* (.*); converts any question marks   */
/* to dots, and then compiles the      */
/* file name as a regular expression.  */
/* The address of the compiled expres- */
/* sion is saved as the term argument. */
/* ----------------------------------- */
int pars_name(EXPR_NODE *enp, int *currarg, int argc, char **argv)
{
   int i, dots, rc;
   char *p, *q, *s, *tmem;
   char grepbuf[GBUFSIZE];

   tmem = NULL;
   dots = 0;
   i = *currarg + 1;
   if (i >= argc)
     return(OK);
   q = argv[i];
   if (*q == '\'')
     if ((q = unquote(&i, argc, argv)) == NULL)
       return(BAIL_OUT);
   for (p = q; *p; p++)
      if (*p == '.')
        dots++;
      else
        if (*p == '*')
          dots++;

//   if (!dots)         /* if there's nothing to escape out of */  /* @4d */
//     enp->en_args.ena_pch = q;                                   /* @4d */
//   else                                                          /* @4d */
//     {                                                           /* @4d */

   if ((tmem = (char *)malloc(strlen(q) + dots + 3)) == NULL)
     {
        myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", q);
        return(BAIL_OUT);
     }
                /* escape out any dots so that grep doesn't generalize them */
   s = tmem;
   *s++ = '^';        /* new-line pattern for grep */
   for (p = q; *p; p++, s++)
      {
        if (*p == '.')       /* escape out dots */
          *s++ = '\\';
        if (*p == '*')       /* normalize asterists */
          *s++ = '.';
        if (*p == '?')       /* convert question marks */
          *s = '.';
        else
          *s = (char)toupper(*p);   /* upper case all alpha */
      }
   *s++ = '$';    /* end-of-line indicator for grep */
   *s = '\0';
   q = tmem;
//     }                                                           /* @4d */
   if ( (rc = compile_reg_expression(q, grepbuf, GBUFSIZE)) == 0)
     {
        printf("%s: invalid regular expression for -name: %s\n", argv[0], q);
        return(BAIL_OUT);
     }
   if (rc > GBUFSIZE)
     {
        printf("%s: -name too long: %s\n", argv[0], q);
        return(BAIL_OUT);
     }
   if ( (p = (char *)malloc(rc + 1)) == NULL)
     {
        myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", q);
        return(BAIL_OUT);
     }
   strcpy(p, grepbuf);
   enp->en_args.ena_pch = p;
   if (tmem)
     free(tmem);
   *currarg = i;
   return(OK);
}


/* ----------------------------------- */
/*   pars_exec()                       */
/*                                     */
/* Creates a list of pointers to the   */
/* arguments of an -ok or -exec term.  */
/* The list should have the command    */
/* name as element 0, the first        */
/* command line argument at element 1, */
/* and so on.  The list is terminated  */
/* with a null pointer.  If the string */
/* {} appears in the list, the address */
/* where (later on) the full path to   */
/* a file will be is substituted.      */
/* ----------------------------------- */
int pars_exec(EXPR_NODE *enp, int *currarg, int argc, char **argv)
{
   int i, j, k, eargs, lastarg;
   char **arglist;
   char *p;

   i = *currarg + 1;
   if (i >= argc)
     {
       printf(incmsg, argv[0]);
       return(BAIL_OUT);
     }

   for (; i < argc; i++)       /* Look for an arg that ends in ; */
     {
/*       p = argv[i];                                               @7d */
       for (p = argv[i]; *p; p++)                                /* @7a */
         {                                                       /* @7a */
           j = 0;                                                /* @7a */
           if (*p == '\\')                                       /* @7a */
             j = 1;                                              /* @7a */
           if ((p[j] == ';') && (p[j+1] == '\0'))                /* @7a */
             break;                                              /* @7a */
         }                                                       /* @7a */
       if (*p == ';' || *p == '\\')                              /* @7a */
         {                                                       /* @7a */
            *p = '\0';                                           /* @7a */
            break;                                               /* @7a */
         }                                                       /* @7a */
//       p++;                                                    /* @7d */
//     else                                                      /* @7d */
//       if (*p == '\'')                                         /* @7d */
//         p++;                                                  /* @7d */
//     if (*p++ == ';')                                          /* @7d */
//       if (*p == '\0')                                         /* @7d */
//         break;                                                /* @7d */
//       else                                                    /* @7d */
//         if (*p++ == '\'')                                     /* @7d */
//           if (*p == '\0')                                     /* @7d */
//             break;                                            /* @7d */
     }

   eargs = i - *currarg;
   lastarg = i;                                                  /* @7a */
   if (i < argc && argv[i][0])                                   /* @7a */
     {                                                           /* @7a */
       eargs++;                                                  /* @7a */
       lastarg++;                                                /* @7a */
     }                                                           /* @7a */

/*   if ( (i >= argc) || (eargs <= 1) )             */           /* @7d */
   if ( (i >= argc) || (eargs < 1) )                             /* @7a */
     {
       printf(incmsg, argv[0]);
       return(BAIL_OUT);
     }
/*   arglist = (char **)malloc(eargs * sizeof(char *) + 1);         @8d */
   arglist = (char **)malloc((eargs+2) * sizeof(char *) + 1);    /* @8a */
   if (arglist == NULL)
     {
        myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "arglist");
        return(BAIL_OUT);
     }

   enp->en_args.ena_argl = arglist;

   arglist[0] = cmdexe;                                          /* @8a */
   arglist[1] = slashc;                                          /* @8a */
/*   for (k = 0, j = *currarg+1; j < i; j++, k++)                   @8d */
   for (k = 2, j = *currarg+1; j < lastarg; j++, k++)            /* @8a */
     {
       if (strcmp(argv[j], "{}") == 0)   /* substitute complete path name */
         arglist[k] = wholename;          /* for {} */
         else
           arglist[k] = argv[j];
     }
   arglist[k] = NULL;    /* last pointer in list must be null */
   *currarg = i;         /* update arg list pointer */
   return(OK);
}

int pars_newer(EXPR_NODE *enp, int *currarg, int argc, char **argv)
{
   int i;
   struct stat sb;

   enp->en_args.ena_int = 0L;
   i = *currarg + 1;
   if (i >= argc)
     return(OK);
   if (stat(argv[i], &sb))
     {
        printf("%s: cannot access %s\n", argv[0], argv[i]);
        return(BAIL_OUT);
     }
   enp->en_args.ena_int = sb.st_mtime;                             /* @5c */
   *currarg = i;                                                   /* @5a */
   return(OK);                                                     /* @5a */
}


/* try to match argument with tags found in term_table */
int findtag(char *p)
{
   int i;

   for (i = 0; i < NUM_TERMS; i++)       /* find option in table */
     if (term_table[i].te_tag)
       if (strcmp(term_table[i].te_tag, p) == 0)
         return(i);
   return(-1);
}

/* convert octal string to binary */
long otol(char *s)
{
   long answer;
   char *p;

   answer = 0L;
   for (p = s; *p; p++)
     {
        if (*p >= '0' && *p <= '7')
          {
             answer <<= 3;
             answer += (*p - '0');
          }
        else
          break;
     }
   return(answer);
}

/* concatenate command line arguments that are within single quotes */
char *unquote(int *argp, int argc, char **argv)
{
   int i, lastarg;
   int len, endfound;
   char *p, *uqbuf;

   len = 0;
   endfound = 0;

   for (i = *argp; i < argc && !endfound; i++)
     {
       for (p = argv[i]; *p; len++, p++);
       if (*--p == '\'')
         endfound = 1;
       else
         len++;
     }

   lastarg = --i;

   if ( (uqbuf = (char *)malloc(len + 1)) == NULL)
     {
        myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "unquote");
        return(NULL);
     }

   i = *argp;
   p = argv[*argp];
   if (*p == '\'')
     p++;
   strcpy(uqbuf, p);
   for (++i; i <= lastarg; i++)
     {
       strcat(uqbuf, " ");
       strcat(uqbuf, argv[i]);
     }
   if (endfound)
     uqbuf[len - 1] = '\0';
   return(uqbuf);
}

void tell_usage()
{
   printf("find - Copyright IBM, 1990, 1992\n");
   printf("Usage:  find   path-list  predicate-list\n\n");
   printf("Predicates:\n");
printf("-fstype [jfs][nfs] -inum [+][-]Number -name FileName -perm Octal -prune\n");
printf("-type b,c,d,f,p,l or s  -links [+][-]Number  -user UserName  -group GroupName\n");
printf("-nogroup  -size [+][-]Number  -atime [+][-]Number  -ctime [+][-]Number\n");
printf("-mtime [+][-]Number  -exec Command  -ok Command  -print  -cpio Device\n");
printf("-newer FileName  -depth  -ls  -xdev  -o  ( Expression )  !\n");
}
