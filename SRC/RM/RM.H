/* @(#)76	1.1  src/rm/rm.h, aixlike.src, aixlike3  9/27/95  15:45:38 */
/* Header file for rm - a Unix-like file erase utility. */

                   /*-------------------------------------*/
                   /*  Author:  Bob Blair                 */
                   /*           Austin Programming Center */
                   /*                                     */
                   /*  IBM INTERNAL USE ONLY              */
                   /*                                     */
                   /*  Copyright IBM, 1990                */
                   /*-------------------------------------*/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 1       5/1/91                                           */
/* @2 11.25.91 delete files with names that start with dots.             */
/* @4
/*-----------------------------------------------------------------------*/

/*  Some useful definitions */

#define YES 1
#define NO  0
#define YESCHAR 'Y'
#define NOCHAR  'N'
#define ALLFILES FILE_DIRECTORY | FILE_SYSTEM | FILE_HIDDEN
#define DOT '.'
// #define ERROR_OUT_OF_MEMORY  -2
#define RCNONEFOUND 23237                   /* @4a */

/* macros */

#define ISDIRECTORY(x)  (x->attrFile & FILE_DIRECTORY)
#define ISREADONLY(x)   (x->attrFile & FILE_READONLY)
#define ISHIDDEN(x)     (x->attrFile & FILE_HIDDEN)
#define ISSYSTEM(x)     (x->attrFile & FILE_SYSTEM)
#define ISARCHIVED(x)   (x->attrFile & FILE_ARCHIVED)
/* #define ISDOTDIR(x)     (*(x->achName) == '.')                @2d */
#define ISDOTDIR(x) ((strcmp(x->achName,".")==0) || \
                     (strcmp(x->achName,"..")==0))/*@2a*/
#define ISEXTCHAR(c)    ( (c==0x00) || (c==0xe0) )

/* Function prototypes */

void tell_usage(void);
int  init(int argc, char **argv);
#ifdef I16
int  findfirst(char *inputFileSpec, PFILEFINDBUF ffbP, char *dirP, char *flsP);
int  do_deletes(char *directory, PFILEFINDBUF ffbp);
int  rmprompt(char *name, PFILEFINDBUF ffbP);
#else
int  findfirst(char *inputFileSpec, PFILEFINDBUF3 ffbP, char *dirP, char *flsP);
int  do_deletes(char *directory, PFILEFINDBUF3 ffbp);
int  rmprompt(char *name, PFILEFINDBUF3 ffbP);
#endif
int  wcmatch(char *, char *);
// void rmerr(int, char *, char *);
