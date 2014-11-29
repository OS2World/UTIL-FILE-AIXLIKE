static char sccsid[]="@(#)10	1.1  src/which/which.c, aixlike.src, aixlike3  9/27/95  15:46:53";
/* which.c */

/*  Finds the path of the program or programs named on the command line,
    showing *which* copy would have been executed if the program name had
    been given on a command line.

    DOES NOT DO ALIASes.  There is no standard OS/2 way of doing aliases,

    It does expand wild cards.

    All command names are searched for if an extension is not specified.
    First exe then cmd.

    Instead of using the path in .cshrc, as the AIX version does, this
    version uses the environment variable PATH.

    Syntax

            which file [file [file...

*/

/************************************************************************/
/* Maintenance History                                                  */
/*  @1  grb  Feb 1 1993 Find .com files, and search in the right order  */
/************************************************************************/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <errno.h>
#include <time.h>
#include <fmf.h>
#include "which.h"


/* Global */
int  tryboth = 0;                                   //@1m


void main (int argc, char **argv)
{
   int i, numspecs;

   suffixes[0] = comsuff;
   suffixes[1] = exesuff;
   suffixes[2] = cmdsuff;
   numspecs = init(argc, argv);
   if (numspecs != BAILOUT)
     for (i = 1; i <= numspecs; do_a_filespec(argv[i++]));
}


int init(int argc, char **argv)
{
   char *p, *pathspec;
   struct charchain *pchain;

   if (argc < 2)
     {
       tell_usage();
       return(BAILOUT);
     }
   else
     {
        getcwd(cwd, CCHMAXPATH);
        pchain = (struct charchain *)malloc(sizeof(struct charchain));
        if (pchain == NULL)
          printf("Out of memory\n");
        else
          {
             pathroot = pchain;
             pchain->name = cwd;
             pchain->next = NULL;
             p = getenv("PATH");
             if (p)
             {
               pathspec = (char *)malloc(strlen(p) + 1);
               strcpy(pathspec, p);
               p = pathspec;
                 while (*p)
                   {
                      pchain->next = (struct charchain *)malloc(sizeof(struct charchain));
                      if (pchain->next)
                        {
                           pchain = pchain->next;
                           pchain->name = p;
                           pchain->next = NULL;
                           for (; *p && *p != ';'; p++);
                           if (*p)
                             *p++ = '\0';
                           else
                            *p == '\0';
                        }
                   }
             }
          }
     }
    return(argc - 1);
}

void do_a_filespec(char *filespec)
{
   char *fs, *p;

   free_compchain(ccroot);
   ccroot = NULL;
   fs = filespec;
   p = strrchr(filespec, '.');
   if (p)                      /* If their's a dot in the file spec */
     {                         /* Check and see if the suffix is .exe, .com */
                               /* or .cmd                                   */
        if ( (stricmp(p, exesuff) != 0) && (stricmp(p, cmdsuff) != 0)
                                        && (stricmp(p, comsuff) != 0) )  //@1a
          {                    /* If it is not one of those                 */
            if (*(p+1) == 0)   /* Delete the dot if it is terminal.         */
              *p = 0;          /* then use the whole string as the 'name'   */
            if (*(p+1) == '*')
              *p = 0;
            fs = (char *)malloc(strlen(filespec) + 5);
            strcpy(fs, filespec);
            tryboth = 1;       /* We will add .exe, .com and .cmd to the end*/
          }
     }
   else                        /* If there is no dot */
     {                         /* again we use the while string for the name */
        fs = (char *)malloc(strlen(filespec) + 5);
        strcpy(fs, filespec);
        tryboth = 1;
     }
   do_a_file(fs);                                               //@1a
}

void free_compchain(struct charchain *root)
{
   struct charchain *ccp, *temp;

   ccp = root;
   while (ccp)
     {
        free(ccp->name);
        temp = ccp->next;
        free(ccp);
        ccp = temp;
     }
}

int do_a_file(char *filename)    /* may still be ambiguous */
{
   int rc;
   struct charchain *pathp;

   if (isfq(filename))
     {
       rc = dofqfile(filename);                          //@1c
       return(DONE);
     }
   else
     {
       for (pathp = pathroot; pathp; pathp = pathp->next)
          {
                   rc = donotfqfile(filename, pathp->name);
                   if ( (rc == FOUND) && !isambig(filename) )
                     return(DONE);
          }   /* end FOR */
     }
   return(UNDONE);
}

int dofqfile(char *filename)
{
   int i, found;
   char *p;

   found = NOTFOUND;
   if (tryboth)         /* No suffix was supplied: we add .com, .exe and .cmd */
     {
       p = filename + strlen(filename);
       for (i = 0; i < NUMSUFFIXES; i++)
          {
             strcpy(p, suffixes[i]);
             found = dofsfile(filename);
             if (found == FOUND && !isambig(filename))
               return(FOUND);
          }
     }        /* file was specified with an extension of exe, com or cmd */
   else
     found = dofsfile(filename);
   return(found);
}

int dofsfile(char *filename)
{
   char result[CCHMAXPATH];
   int  rc, attrib;
   int  found = NOTFOUND;

   if (fmf_init(filename, FMF_ALL_FILES, FMF_NO_SUBDIR))
     return(NOTFOUND);
   else
     {
        do
          {
             rc = fmf_return_next(result, &attrib);
             if (rc == 0)
               {
                 showresult(result);
                 found = FOUND;
               }
          } while (!rc && isambig(filename));
     }
   fmf_close();
   return(found);
}

int donotfqfile(char *filename, char *pathname)
{
   char fq[CCHMAXPATH];

   strcpy(fq, pathname);
   if (fq[strlen(fq)-1] != '\\')
     strcat(fq, "\\");
   strcat(fq, filename);
   return(dofqfile(fq));
}

int isfq(char *fn)
{
   return(strchr(fn, '\\') ? 1 : 0);
}

int isambig(char *fn)
{
   if ( strchr(fn, '*') || strchr(fn, '?') )
     return(1);
   else
     return(0);
}

void showresult(char *fqname)
{
   struct charchain *ccp, *lastccp;
   char *namepart;
   struct stat sb;
   struct tm *ltimep;
   char *atimep;
   char mytime[22];                          //@1c
   char myattr[11];

   namepart = getnamepart(fqname);
   for (ccp = lastccp = ccroot; ccp; ccp = ccp->next)
     {
       if (stricmp(ccp->name, namepart) == 0)
         break;
       lastccp = ccp;
     }
   if (ccp)
     free(namepart);
   else
     {
        if (!lastccp)
          {
             ccroot = (struct charchain *)malloc(sizeof(struct charchain));
             ccroot->name = namepart;
             ccroot->next = NULL;
          }
        else
          {
            for (ccp = lastccp; ccp->next; ccp = ccp->next);
            ccp->next = (struct charchain *)malloc(sizeof(struct charchain));
            ccp = ccp->next;
            ccp->name = namepart;
            ccp->next = NULL;
          }
        if (stat(fqname, &sb))
          printf("stat failed for %s\n", fqname);
        else
          {
            ltimep = localtime(&sb.st_mtime);
            atimep = asctime(ltimep);
//@1d            strncpy(mytime, atimep + 4, 15);
            strncpy(mytime,atimep+4,6);                            //@1c
            strncpy(mytime+6,atimep+19,5);                         //@1a
            strncpy(mytime+11,atimep+10,9);                        //@1a
            mytime[20] = '\0';
//@1d            if (sb.st_mode & S_IFDIR)
//@1d               strcpy(myattr, "dir ");
//@1d             else
//@1d               strcpy(myattr, "    ");
            myattr[0] = 0;                                         //@1a
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

            printf("%5ld%4s%2d%5s%7ld %20s %s\n",
                        sb.st_size/1024 + 1,                  /* blocks */
                            myattr,                            /* attributes */
                                1,                             /* hard links */
                                   "bin",                      /* owner */
                                      sb.st_size,              /* bytes */
                                           mytime,             /* date & time */
                                                fqname);        /* file name */
          }
     }
}

char *getnamepart(char *fqname)
{
   char *p, *q, *name;

   p = strrchr(fqname, '\\');
   if (!p)
     {
       p = strrchr(fqname, ':');
       if (!p)
         p = fqname;
       else
         p++;
     }
   else
     p++;
   q = strrchr(fqname, '.');
   if (!q)
     {
        name = (char *)malloc(strlen(p) + 1);
        strcpy(name, p);
     }
   else
     {
        name = (char *)malloc(q - p + 1);
        strncpy(name, p, q - p);
        name[q-p] = '\0';
     }
   return(name);
}
void tell_usage()
{
   printf("        Copyright IBM, 1991\n");
   printf("which - find the executable\n");
   printf("Usage:  which name [name [name ...\n");
   printf("\nNames should be commands: for example   a.exe b.cmd mycmd myprog\n");
}
