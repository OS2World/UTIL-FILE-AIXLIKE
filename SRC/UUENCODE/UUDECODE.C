static char sccsid[]="@(#)04	1.1  src/uuencode/uudecode.c, aixlike.src, aixlike3  9/27/95  15:46:41";
/* uudecode                                          */
/*****************************************************/
/* encodes a datastream using uu encoding.  Converts */
/* the whole file to printable ascii characters by   */
/* forcing them to a value of 0-63                   */
/* Groups of 3 bytes are made into 4-byte groups of  */
/* printable chars, viz:                             */
/*     Bits 0-5 of Input Byte 1 --> Bits 2-7 of Output Byte 1 */
/*     Bits 6-7 of Input Byte 1 --> Bits 2-3 of Output Byte 2 */
/*     Bits 0-3 of Input Byte 2 --> Bits 4-7 of Output Byte 2 */
/*     Bits 4-7 of Input Byte 2 --> Bits 2-5 of Output Byte 3 */
/*     Bits 0-1 of Input Byte 3 --> Bits 6-7 of Output Byte 3 */
/*     Bits 2-7 of Input Byte 3 --> Bits 2-7 of Output Byte 4 */
/* Bits 0 and 1 of all output bytes are 0            */
/*                                                   */
/* The archive file has a header line that contains  */
/* the string "begin " followed by an octal permissions */
/* string ("777 " in all cases for the OS/2 version) */
/* followed by the output file name (i.e. zulu.zip). */
/* Lines of up to 45 (input) bytes are allowed; the first    */
/* byte of each line gives the (input) character count. */
/* The last line must have an input count of 0 (i.e. */
/* the last line must consist of exactly one space). */
/* The decode algorithem makes all values mod 077,   */
/* so '`' (ascii 96) can be used to substitute for   */
/* space (ascii 32).                                 */

/* The actual encoding is bracketed by the 'begin'   */
/* line mentioned above, and an 'end' line which     */
/* consists entirely of the string end.              */

#include <stdio.h>
#include <io.h>
#include <string.h>
#include <errno.h>

#define OK   0
#define BAILOUT 1
#define CCHMAXPATH 264
#define RBSIZE 1024

char readbuff[RBSIZE+1];
int lineno = 0;

char *sourcefile;
char *remotedestination;

int init(int, char **);
int do_the_file(void);
int decode (FILE *, FILE *);
void tell_usage(void);

int main(int argc, char **argv)
{
   if (init(argc, argv) != BAILOUT)
     return(do_the_file());
}

int init(int argc, char **argv)
{
   if (argc < 2)
     {
        sourcefile = NULL;
     }
   else
     {
        sourcefile = argv[1];
     }
   if (sourcefile && access(sourcefile, 0x04))
     {
       fprintf(stderr,"uudecode: cannot read source file %s\n", sourcefile);
       return(BAILOUT);
     }
   return(OK);
}

int do_the_file()
{
   FILE *fi, *fo;
   int found = 0;
   char *p, *q;

   if (sourcefile == NULL)
     {
       freopen("", "rb", stdin);
       fi = stdin;
     }
   else
     {
       fi = fopen(sourcefile, "rb");
       if (!fi)
         {
            fprintf(stderr, "uudecode: could not open source file %s\n", sourcefile);
            return(BAILOUT);
         }
     }
   while (!found && fgets(readbuff, RBSIZE, fi))
     {
       lineno++;
       if (strncmp(readbuff,"begin ", 6) == 0)
            found = 1;
     }

   if (found)
     {
        for (p = readbuff;*p && *p != ' ';p++);
        for (; *p && *p == ' '; p++);
        for (; *p && *p != ' ';p++);
        for (; *p && *p == ' '; p++);
        for (q = p; *q && *q != 10 && *q != 13; q++);
        *q = '\0';
        fo = fopen(p, "wb");
        if (!fo)
          {
             fprintf(stderr, "uudecode: could not open output file %s\n", p);
             printf("Error number was %d\n",errno);
             return(BAILOUT);
          }
     }
   else
     {
        fprintf(stderr, "uudecode: could not find start of uuencoded data\n");
        return(BAILOUT);
     }
   return(decode(fi, fo));
}


int decode(FILE *filein, FILE *fileout)
{
   unsigned char out, *p;
   unsigned int c;
   int linelen;
   int obyte;

   linelen = 0;
   while (fgets(readbuff, RBSIZE, filein))
     {
        obyte = 0;
        linelen = (*readbuff - 32) & 077;
        lineno++;
//fprintf(stderr,"%d ",lineno);
        if (linelen == 0)
          break;
        if (strcmp(readbuff, "end") == 0)
          break;
        p = readbuff + 1;
        while (linelen)
          {
            switch (obyte)
            {
              case 0:  c = (*p++ - 32) & 077;
                       out =  (char)(c<<2);
                       c = (*p++ - 32) & 077;
                       break;
              case 1:  out |= c>>4;
                       fputc(out, fileout);
                       linelen--;
                       out = (char)(c<<4);
                       c = (*p++ - 32) & 077;
                       break;
              case 2:  out |= c>>2;
                       fputc(out, fileout);
                       linelen--;
                       out = (char)(c<<6);
                       c = (*p++ - 32) & 077;
                       break;
              case 3:  out |= c;
                       fputc(out, fileout);
                       linelen--;
                       break;
            }
            obyte++;
            obyte &= 3;
          }
     }
   fclose(fileout);
   return(0);
}

void tell_usage()
{
   printf("Usage:\n");
   printf("   uuencode SourceFile RemoteDestination\n");
}
