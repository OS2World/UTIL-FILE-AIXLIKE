static char sccsid[]="@(#)35	1.1  src/compress/comprutl.c, aixlike.src, aixlike3  9/27/95  15:44:12";
/* Two OS/2-specific utility routines needed by compress                 */

/*   isValidFSName(name) returns 1 if the name passed is syntactically   */
/*   correct for the file system it's on.                                */

/*   DoDosCopy(To, From) copies a file.                                  */


#define INCL_DOSFILEMGR
#include <os2.h>

#define IBUFSIZE 50

int isValidFSName(char *name)
{
#ifdef I16
   USHORT PathInfoLevel;
   USHORT PathInfoBufSize;
#else
   ULONG PathInfoLevel;
   ULONG PathInfoBufSize;
#endif
   USHORT rc;
   BYTE PathInfoBuf[IBUFSIZE];

//   PathInfoLevel = 6;
   PathInfoLevel = 5;
   PathInfoBufSize = IBUFSIZE;
   if (name && *name)
     {
#ifdef I16
       rc = DosQPathInfo(name,
                          PathInfoLevel,
                          PathInfoBuf,
                          PathInfoBufSize,
                          0L);
#else
       rc = DosQPathInfo(name,
                          PathInfoLevel,
                          PathInfoBuf,
                          PathInfoBufSize);
#endif
       if (!rc)
         return(1);
     }
   return(0);
}


int DoDosCopy(char *toname, char *fromname)
{
   USHORT rc;

//   rc = DosCopy(fromname, toname, 1, 0L);    /*@Z9d*/
   rc = DosCopy(fromname, toname, 1);          /*@Z9a*/
   if (rc)
     return(1);
   else
     return(0);
}
