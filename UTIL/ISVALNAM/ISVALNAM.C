static char sccsid[]="@(#)40	1.1  util/isvalnam/isvalnam.c, aixlike.src, aixlike3  9/27/95  15:53:10";
/* Boolean routine that returns TRUE if the string argument passed is a */
/* valid name on the file system; or FALSE if not.                      */
/* To test a file system other than the current one, include the drive  */
/* name in the string.                                                  */
/*                                                                      */
/* Examples:                                                            */
/*      result = isValidFSName("abc.def") tests the name on the current */
/*                                        drive.                        */
/*      result = isValidFSName("d:\abc.def") tests it on drive d:       */
/*                                                                      */
/* Uses DosQPathInfo Level 6                                            */

#define INCL_DOSFILEMGR
#include <os2.h>

#define IBUFSIZE 50
#define TRUE 1
#define FALSE 0

int isValidFSName(char *path)
{
   USHORT PathInfoLevel;
   USHORT PathInfoBufSize;
   USHORT rc;
   BYTE PathInfoBuf[IBUFSIZE];

   PathInfoLevel = 6;
   PathInfoBufSize = IBUFSIZE;
   if (path && *path)
     {
       rc = DosQPathInfo(path,
                          PathInfoLevel,
                          PathInfoBuf,
                          PathInfoBufSize,
                          0L);
       if (!rc)
         return(TRUE);
     }
   return(FALSE);
}
