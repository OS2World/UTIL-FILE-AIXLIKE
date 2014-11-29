static char sccsid[]="@(#)06	1.1  src/uuencode/uuencode.c, aixlike.src, aixlike3  9/27/95  15:46:45";
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
 
#define OK   0
#define BAILOUT 1
#define CCHMAXPATH 264
 
char *sourcefile;
char *remotedestination;
 
int init(int, char **);
int do_the_file(void);
int encode (FILE *, FILE *);
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
        tell_usage();
        return(BAILOUT);
     }
   if (argc > 2)
     {
        sourcefile = argv[1];
        remotedestination = argv[2];
     }
   else
     {
       sourcefile = NULL;
       remotedestination = argv[1];
     }
   if (sourcefile && access(sourcefile, 0x04))
     {
       fprintf(stderr,"uuencode: cannot read source file %s\n", sourcefile);
       return(BAILOUT);
     }
   return(OK);
}
 
int do_the_file()
{
   FILE *fi, *fo;
 
   fo = stdout;
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
            fprintf(stderr, "uuencode: could not open source file %s\n", sourcefile);
            return(BAILOUT);
         }
     }
   fprintf(fo,"begin 777 %s\n", remotedestination);
   if (encode(fi, fo)==OK)
     {
       fprintf(fo,"end\n");
       return(OK);
     }
   else
     return(BAILOUT);
}
 
 
/* Postprocess line, converting ascii 32 (space)   */
/*  to ascii 96 (`). VM mail program can trash     */
/*  trailing spaces.                               */
/* Get the old behavior changing the comments      */
/*  on the following definition lines for          */
/*  OUTPUT_SPACE                                   */
 
#define OUTPUT_SPACE '`'        /* new */
//#define OUTPUT_SPACE ' '        /* old */
 
int convertSpace(char *outline)
{
   int obyte;
   
   for (obyte=0; outline[obyte]; obyte++) {
      /* convert from space */
      if (outline[obyte] == ' ')
         outline[obyte] = OUTPUT_SPACE;
   }
   return 0;
}
 
int encode(FILE *filein, FILE *fileout)
{
   char outline[62];     /* room for 45 input chars + length byte + 1 */
   int linelen;
   int obyte;
   unsigned inchar;
 
   linelen = 0;
   obyte = 1;
   while ((inchar = fgetc(filein)) != EOF)
     {
        if (linelen == 45)
          {
             outline[obyte] = '\0';
             *outline = (char)(linelen + 32);
 
             /* Postprocess line, converting ascii 32 (space)   */
             /*  to ascii 96 (`). VM mail program can trash     */
             /*  trailing spaces.                               */
             convertSpace(outline+1);
 
             fprintf(fileout,"%s\n", outline);
             linelen = 0;
             obyte = 1;
          }
        switch (linelen % 3)
        {
          case 0:  outline[obyte] = (char)(inchar>>2);
                   outline[obyte++] += 32;
                   outline[obyte] = (char)((inchar & 3) << 4);
                   break;
          case 1:  outline[obyte] |= (char)(inchar>>4);
                   outline[obyte++] += 32;
                   outline[obyte] = (char)((inchar & 15) << 2);
                   break;
          case 2:  outline[obyte] |= (char)(inchar>>6);
                   outline[obyte++] += 32;
                   outline[obyte] = (char)(inchar & 63);
                   outline[obyte++] += 32;
                   break;
        }
        linelen++;
     }
   if ((linelen % 3) < 2)     /* handle any stub bytes */
     outline[obyte++] += 32;
   outline[obyte] = '\0';
   *outline = (char)(linelen + 32);
 
   /* Postprocess line, converting ascii 32 (space)   */
   /*  to ascii 96 (`). VM mail program can trash     */
   /*  trailing spaces.                               */
   convertSpace(outline+1);
 
   fprintf(fileout,"%s\n", outline);
   //fprintf(fileout," \n");
   fprintf(fileout,"%c\n", OUTPUT_SPACE);
   fclose(filein);
   return(0);
}
 
void tell_usage()
{
   printf("Usage:\n");
   printf("   uuencode SourceFile RemoteDestination\n");
}
