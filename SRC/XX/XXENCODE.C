static char sccsid[]="@(#)16	1.1  src/xx/xxencode.c, aixlike.src, aixlike3  9/27/95  15:47:06";
#include <stdio.h>
#include <stdlib.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <limits.h>
#ifdef AIXLIKE
#include "itstypes.h"
#include <string.h>
#else
#include <itstypes.h>
#endif /* AIXLIKE */

#define TRUE  1

#define ENCODE(ch) (szCharSet[(ch) & 0x3F])

static char szCharSet[] =
   "+-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int main(int argc, char *argv[])
{
   int nIndex;
   int nNumberBytesRead;
   FILE *pfileInput;
   FILE *pfileOutput;
   struct stat sbStatBuffer;
   int nFileMode;
   UINT16 wChecksum;			/* See ITSTYPES.H */
   char szBuffer[80];
   char *pszBufPtr;
   int c1;
   int c2;
   int c3;
   int c4;
#ifdef AIXLIKE
   char *remoteFilename;
#endif /* AIXLIKE */

#ifdef AIXLIKE
   if ((argc == 2) && (*argv[1] != '-'))
   {
      freopen("","rb",stdin);
      pfileInput = stdin;
      remoteFilename = argv[1];
   }
   else if ((argc == 3) && (*argv[1] != '-'))
   {
      if ((pfileInput = fopen(argv[1], "rb")) == NULL)
      {
         fprintf(stderr, "Unable to open \"%s\" for reading.\n", argv[1]);
         exit(1);
      } /* if */
      remoteFilename = argv[2];
    }
   else
   {
      if (*argv[1] == '-')
         fprintf(stderr, "xxencode: Not a recognized flag: %c\n", *(argv[1]+1));
      fprintf(stderr, "Usage:  xxencode [infile] remotefile\n");
      exit(2);
   } /* if */
   freopen("","wb",stdout);
   pfileOutput = stdout;
#else
   if (argc != 3)
   {
      printf("Usage: xxencode <input pathname> <output pathname>\n");
      exit(1);
   } /* if */

   if ((pfileInput = fopen(argv[1], "rb")) == NULL)
   {
      printf("Unable to open \"%s\" for reading.\n", argv[1]);
      exit(1);
   } /* if */

   if ((pfileOutput = fopen(argv[2], "w")) == NULL)
   {
      fclose(pfileInput);
      printf("Unable to open \"%s\" for writing.\n", argv[2]);
      exit(1);
   } /* if */
#endif /* AIXLIKE */

   fstat(fileno(pfileInput), &sbStatBuffer);
   nFileMode = sbStatBuffer.st_mode & 0x1FF;
#ifdef AIXLIKE
   fprintf(pfileOutput, "begin %o %s\n", nFileMode, remoteFilename);
#else
   fprintf(pfileOutput, "begin %o %s\n", nFileMode, argv[1]);
#endif /* AIXLIKE */

   wChecksum = 0;

#ifdef AIXLIKE
   fprintf(stderr, "  Encoding \"%s\": ", argv[1]);
#else
   printf("  Encoding \"%s\": ", argv[1]);
#endif /* AIXLIKE */

   while (TRUE)
   {
      nNumberBytesRead = (int) fread(szBuffer, 1, 45, pfileInput);
      fputc(ENCODE(nNumberBytesRead), pfileOutput);

      pszBufPtr = szBuffer;
      for (nIndex = 0; nIndex < nNumberBytesRead; nIndex += 3)
      {
         c1 = (*pszBufPtr) >> 2;
         c2 = ((*pszBufPtr) << 4) & 060 | (pszBufPtr[1] >> 4) & 017;
         c3 = (pszBufPtr[1] << 2) & 074 | (pszBufPtr[2] >> 6) & 03;
         c4 = pszBufPtr[2] & 077;

         fputc(ENCODE(c1), pfileOutput);
         fputc(ENCODE(c2), pfileOutput);
         fputc(ENCODE(c3), pfileOutput);
         fputc(ENCODE(c4), pfileOutput);

         wChecksum += *pszBufPtr;
         wChecksum += pszBufPtr[1];
         wChecksum += pszBufPtr[2];

         pszBufPtr += 3;
      } /* for */

      fputc('\n', pfileOutput);

      if (!nNumberBytesRead)
      {
         break;
      } /* if */
   } /* while */

   fprintf(pfileOutput, "end %04x\n", wChecksum);

   fclose(pfileInput);
   fclose(pfileOutput);

#ifdef AIXLIKE
   fprintf(stderr, "Done\n");
#else
   printf("Done\n");
#endif /* AIXLIKE */

   return(0);
} /* main */
