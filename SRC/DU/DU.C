static char sccsid[]="@(#)41	1.1  src/du/du.c, aixlike.src, aixlike3  9/27/95  15:44:24";
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#include <idtag.h>    /* package identification and copyright statement */
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define  READ_ONLY          1
#define  HIDDEN_FILE        2
#define  SYSTEM_FILE        4
#define  SUB_DIR            0x10
#define  ARCHIVE_FILE       0x20
#define NODIRS READ_ONLY | HIDDEN_FILE | SYSTEM_FILE | ARCHIVE_FILE
#define EVERYTHING NODIRS | SUB_DIR

#define TOTALONLY 0x1
#define DIRONLY   0x2
#define SHOWALL 0x4
#define SHOWERRORS 0x8
#define SHOWBLOCKS 0x10

#define DIR 0x10 // Bit 4 indicates directory entry in attribute field

#define ISDOTDIR(x) ((strcmp((x), ".") == 0) || (strcmp((x), "..") == 0))

/************************************************************************/
/*   function prototypes                                                */
/************************************************************************/
void printSize(char * name, long int size);
long int sumDirectory(char *dName, char *tail, unsigned mask);
int isAmbiguous(char *fspec);
char *FindTail(char *fspec);
int isDirectory(char *fspec);
char *fsconcat(char *dirname, char *appendage);
void usage(char * name);
void printHelp(char *name);

/************************************************************************/
/*   Globals                                                            */
/************************************************************************/
#ifdef I16
FILEFINDBUF ffb;
#else
FILEFINDBUF3 ffb;
#endif
static USHORT mode = 0;
#ifdef I16
USHORT      fInDir;
#else
ULONG       fInDir;
#endif
long dirsumm;
char *pattern;
char *ptail;

/************************************************************************/
/*   Externals                                                          */
/************************************************************************/
extern int getopt(int argc, char **argv, char *optstr);
extern int optind;

/************************************************************************/
/*   printSize()                                                        */
/************************************************************************/
void printSize(char * name, long int size)
{
/*    */
   if (mode & SHOWBLOCKS)
     printf("%-8ld %s\n", (size+512)/1024, name);
   else
     printf("%-8ld %s\n", size, name);
}

/************************************************************************/
/*   sumDirectory()                                                     */
/************************************************************************/
long int sumDirectory(char *dName, char *tail, unsigned mask)
{
   long int    summ;
   HDIR        dHandle;
   USHORT      rc;
   char        *newdir;

    ptail = tail;
    if (!*ptail)
      if (isDirectory(dName))
        ptail = "*.*";
    pattern = fsconcat(dName, ptail);
    summ = 0;
    dHandle = 0xFFFF;
    fInDir = 1;
    rc = DosFindFirst(pattern, &dHandle, mask, &ffb, sizeof(ffb),
                      &fInDir, 
#ifdef I16
                      0L
#else
                      1L
#endif
                        );
    free(pattern);
    while(!rc)
      {
        dirsumm = ffb.cbFileAlloc;
        summ += dirsumm;
        if (!ISDOTDIR(ffb.achName))
          {
            newdir = fsconcat(dName, ffb.achName);
            if((ffb.attrFile & DIR)   // found a subdirectory, recurse
                      && !*tail)
              {
                 dirsumm = sumDirectory(newdir, "", EVERYTHING);
                 summ += dirsumm;
                 if(!(mode & TOTALONLY))
                   printSize(newdir, dirsumm);
              }
            else
              if (mode & SHOWALL)
                printSize(newdir, ffb.cbFileAlloc);
            free(newdir);
          }
        fInDir = 1;
        rc = DosFindNext(dHandle, &ffb, sizeof(ffb), &fInDir);
      }   /* end WHILE */
    if (dHandle != 0xFFFF)
      DosFindClose(dHandle);
    if(rc != ERROR_NO_MORE_FILES)
      if (mode & SHOWERRORS)
        {
          fprintf(stderr,"could not examine %s, omitting it\n",
                       dName);
          return(0);
        }
    else
      {
         if (summ && (mode & TOTALONLY))
           printSize(dName, summ);
      }
    return(summ);
}

/**********************************************************************/
/* isAmbiguous()                                                      */
/**********************************************************************/
int isAmbiguous(char *fspec)
{
   char *p;

   if (*fspec == '*')
     return(1);
   for (p = fspec; *p; p++);
   for (--p; *p != '\\' && *p != ':' && p != fspec; p--)
      if (*p == '*')
        return(1);
   return(0);
}

/**********************************************************************/
/* isDirectory()                                                      */
/**********************************************************************/
int isDirectory(char *fspec)
{
   USHORT rc;
   HDIR dHandle;

   dHandle = -1;
   fInDir = 1;
#ifdef I16
   rc = DosFindFirst(fspec, &dHandle, EVERYTHING, &ffb, sizeof(ffb),
                           &fInDir, 0L);
#else
   rc = DosFindFirst(fspec, &dHandle, EVERYTHING, &ffb, sizeof(ffb),
                           &fInDir, 1L);
#endif
   if (dHandle != -1)
     {
       DosFindClose(dHandle);
       if (!rc)
         if (ffb.attrFile & DIR)
           return(1);
     }
   return(0);
}

/**********************************************************************/
/* FindTail()                                                         */
/**********************************************************************/
char *FindTail(char *fspec)
{
   char *p;
   char *tail;
   int size = 0;

   for (p = fspec; *p; p++);
   if (p != fspec)
     for (--p; *p != '\\' && *p != ':' && p != fspec; p--);
   size = strlen(fspec) - (p - fspec);
   tail = malloc(size + 1);
   if (p != fspec)
     {
       *p++ = '\0';
       strcpy(tail, p);
     }
   else
     {
       strcpy(tail, p);
       *p = '\0';
     }
   return(tail);
}

/**********************************************************************/
/* fsconcat()                                                         */
/**********************************************************************/
char *fsconcat(char *dirname, char *appendage)
{
   char *p, *fs;
   int size;

   size = strlen(dirname) + strlen(appendage) + 2;
   fs = malloc(size);
   strcpy(fs, dirname);
   if (strcmp(fs, "\\") == 0)
     strcat(fs, appendage);
   else
     {
       for (p = fs; *p; p++);
       if (p != fs)
         if (*--p == '\\')
           *p = '\0';
       if (p != fs)
         p--;

       if (appendage && *appendage)
         {
           if (*fs)
             strcat(fs, "\\");
           strcat(fs, appendage);
         }
     }
   return(fs);
}

/************************************************************************/
/*   usage()                                                            */
/************************************************************************/
void usage(char * name)
{
   fprintf(stderr,"usage: %s -as? [directories]\n", name);
   exit(0);
}

/************************************************************************/
/*   printHelp()                                                        */
/************************************************************************/
void printHelp(char *name)
{
   fputs("du - Copyright IBM, 1992\n", stderr);
   fputs("-a: report allocated blocks\n", stderr);
   fputs("-r: show file-access errors\n", stderr);
   fputs("-s: report total summary only\n", stderr);
   fputs("-?: prints this info\n", stderr);
   fputs("directories are given as a blank seperated list\n", stderr);
   usage(name);
}



/************************************************************************/
/*   main()                                                             */
/************************************************************************/
void main(int argc, char **argv)
{
  int  optIndex;
  char *p, *tail;

  mode = 0;
  while((optIndex = getopt(argc, argv, "ablrs?")) != EOF) {
     switch(optIndex) {
        case 'a': mode |= SHOWALL;
                  break;
        case 'b': mode |= SHOWBLOCKS;
        case 'l': break;
        case 'r': mode |= SHOWERRORS;
                  break;
        case 's': mode |= TOTALONLY;
                  break;
        case '?': printHelp(argv[0]);
                  exit(0);
        default :
           usage(argv[0]);
     }
  }
  if(optind == argc) {
    printSize(".", sumDirectory(".", "", EVERYTHING));
  }
  else {
     while(optind < argc) {
        p = argv[optind++];
        if (isAmbiguous(p))
          {
             tail = FindTail(p);
             sumDirectory(p, tail, NODIRS);
          }
        else
          if (isDirectory(p))
            printSize(p, sumDirectory(p, "", EVERYTHING));
          else
            sumDirectory(p, "", NODIRS);
     }
  }
}

