static char sccsid[]="@(#)14	1.1  src/xx/xxdecode.c, aixlike.src, aixlike3  9/27/95  15:47:02";
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#ifdef AIXLIKE
#include "itstypes.h"
#else
#include <itstypes.h>
#endif /* AIXLIKE */

#define FALSE  0
#define TRUE   1

#define DECODE(ch) (szTable[(ch) & 0x7F])

#define CLIP_BYTE(n) ((UINT16) ((n) & 0x00FF))

static char szCharSet[] =
   "+-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static char szTable[128];

int main(int argc, char *argv[])
{
   register int nCount;
   FILE *pfileInput, *pfileOutput;
   int bBeginFound;
   char szBuffer[80];
   int nFileMode;
   char szDestinationFilename[128];
   UINT16 wChecksum;			/* See ITSTYPES.H */
   char *pszBufPtr;
   int c1;
   int c2;
   int c3;
   UINT16 wFileCRC;
   int bCRCFound;
   char *pszStrPtr;

#ifdef AIXLIKE
   if (argc == 1)
   {
      pfileInput = stdin;  
   }
   else if ((argc == 2) && (*argv[1] != '-'))
   {
      if ((pfileInput = fopen(argv[1], "r")) == NULL)
      {
         fprintf(stderr, "Unable to open \"%s\" for reading.\n", argv[1]);
         exit(1);
      } /* if */
   }
   else
   {
      if (*argv[1] == '-')
         fprintf(stderr, "xxdecode: Not a recognized flag: %c\n", *(argv[1]+1));
      fprintf(stderr, "Usage:  xxdecode [infile]\n");
      exit(2);
   } /* if */
      
#else
   if (argc != 2)
   {
      printf("Usage: xxdecode <input pathname>\n");
      exit(1);
   } /* if */
#endif /* AIXLIKE */

   bBeginFound = FALSE;

   memset(szTable, 0, sizeof(szTable));

   pszBufPtr = szCharSet;
   for (nCount = 64; nCount; --nCount)
   {
      szTable[*(pszBufPtr++) & 0x7F] = 64 - nCount;
   } /* for */

#ifndef AIXLIKE
   if ((pfileInput = fopen(argv[1], "r")) == NULL)
   {
      printf("Unable to open \"%s\" for reading.\n", argv[1]);
      exit(1);
   } /* if */
#endif /* AIXLIKE */

   while (TRUE)
   {
      while (TRUE)
      {
         if (!(fgets(szBuffer, sizeof(szBuffer), pfileInput)))
         {
            if (!bBeginFound)
            {
#ifdef AIXLIKE
               fprintf(stderr, "No begin line found.\n");
#else
               printf("No begin line found.\n");
#endif /* AIXLIKE */
            } /* if */

            fclose(pfileInput);
            exit(1);
         } /* if */

         if (!(strncmp(szBuffer, "begin ", 6)))
         {
            bBeginFound = TRUE;
            break;
         } /* if */
      } /* while */

      sscanf(szBuffer, "begin %o %s", &nFileMode, szDestinationFilename);

      pfileOutput = fopen(szDestinationFilename, "wb");
      if (pfileOutput == NULL)
      {
#ifdef AIXLIKE
         fprintf(stderr, "Unable to open \"%s\" for writing.", szDestinationFilename);
#else
         printf("Unable to open \"%s\" for writing.", szDestinationFilename);
#endif /* AIXLIKE */
         exit(1);
      } /* if */

#ifdef AIXLIKE
      fprintf(stderr, "  Decoding \"%s\": ", szDestinationFilename);
#else
      printf("  Decoding \"%s\": ", szDestinationFilename);
#endif /* AIXLIKE */

      wChecksum = 0;

      while (TRUE)
      {
         if (!(fgets(szBuffer, sizeof(szBuffer), pfileInput)))
         {
#ifdef AIXLIKE
            fprintf(stderr, "Short file\n");
#else
            printf("Short file\n");
#endif /* AIXLIKE */
            exit(1);
         } /* if */

         nCount = DECODE(szBuffer[0]);
         if (nCount <= 0)
         {
            break;
         } /* if */

         pszBufPtr = &szBuffer[1];
         while (nCount > 0)
         {
            c1 = DECODE(*pszBufPtr) << 2 | DECODE(pszBufPtr[1]) >> 4;
            c2 = DECODE(pszBufPtr[1]) << 4 | DECODE(pszBufPtr[2]) >> 2;
            c3 = DECODE(pszBufPtr[2]) << 6 | DECODE(pszBufPtr[3]);

            if (nCount >= 1)
            {
               putc(c1, pfileOutput);
            } /* if */
            if (nCount >= 2)
            {
               putc(c2, pfileOutput);
            } /* if */
            if (nCount >= 3)
            {
               putc(c3, pfileOutput);
            } /* if */

            wChecksum += CLIP_BYTE(c1);
            wChecksum += CLIP_BYTE(c2);
            wChecksum += CLIP_BYTE(c3);

            pszBufPtr += 4;
            nCount -= 3;
         } /* while */
      } /* while */

      if ((!(fgets(szBuffer, sizeof(szBuffer), pfileInput))) ||
          (strncmp(szBuffer, "end", 3)))
      {
#ifdef AIXLIKE
         fprintf(stderr, "No end line found.\n");
#else
         printf("No end line found.\n");
#endif /* AIXLIKE */
         exit(1);
      } /* if */

      bCRCFound = FALSE;
      pszStrPtr = strtok(szBuffer, " \t\n");
      if (pszStrPtr)
      {
         pszStrPtr = strtok(NULL, " \t\n");
         if (pszStrPtr)
         {
            sscanf(pszStrPtr, "%04x", &wFileCRC);
            bCRCFound = TRUE;
         } /* if */
      } /* if */

      fclose(pfileOutput);

      if (bCRCFound)
      {
         if (wFileCRC == wChecksum)
         {
#ifdef AIXLIKE
            fprintf(stderr, "CRC OK\n");
#else
            printf("CRC OK\n");
#endif /* AIXLIKE */
            chmod(szDestinationFilename, nFileMode);
         } /* if */
         else
         {
#ifdef AIXLIKE
            fprintf(stderr, "BAD CRC (output deleted)\n");
#else
            printf("BAD CRC (output deleted)\n");
#endif /* AIXLIKE */
            remove(szDestinationFilename);
         } /* else */
      } /* if */
      else
      {
#ifdef AIXLIKE
         fprintf(stderr, "WARNING: NO CRC AVAILABLE\n");
         chmod(szDestinationFilename, nFileMode);
#else
         printf("WARNING: NO CRC AVAILABLE (file mode ignored)\n");
#endif /* AIXLIKE */
      } /* else */
   } /* while */

   fclose(pfileInput);

   return(0);
} /* main */
