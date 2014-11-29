static char sccsid[]="@(#)31	1.1  src/compress/btoa.c, aixlike.src, aixlike3  9/27/95  15:44:03";
/* implementation of btoa for OS/2 */
/* Trying to get around problems with the unixy one that comes with compress */
/* The program is a filter, reading from stdin and writint to stdout */
/* Input is a binary file, output is ascii.  Each 4 binary bytes is converted*/
/* to 5 ascii bytes. The ascii bytes are base 85 added to '!'                */
/* This works because 2**32 is slightly smaller than 85**5.                  */
/************************************************/
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation, 1992.       All rights reserved.*/
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 4      16Mar93                                           */
/*-----------------------------------------------------------------------*/
//#include <idtag.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#define EXP85_4 ((unsigned long)85*85*85*85)
#define EXP85_3 ((unsigned long)85*85*85)
#define EXP85_2 ((unsigned long)85*85)
#define EXP85_1 ((unsigned long)85)
#define MAXLINE 78
#define BAIL_OUT   -1

int init(int argc, char **argv);
void encode(unsigned int c);
void charout(unsigned long c);

unsigned long input = 0;         /* cumulation of 4 input characters    */
unsigned int  pos = 0;           /* current position in the input (0-3) */
unsigned long Ceor = 0;          /* cumulative or of input */
unsigned long Csum = 0;          /* cumulative sum of input */
unsigned long Crot = 0;          /* crude CRC */
unsigned int  linecnt = 0;       /* number of characters on the current line */
unsigned long charsout = 0;      /* number of characters written out */
FILE *f;

int main(int argc, char **argv)
{
   unsigned long n;                  /* number of bytes of input */
   unsigned int c;                   /* the input character from the binary */
                                     /*    file                             */

   if (init(argc, argv) != BAIL_OUT)
     {
       freopen("", "rb", stdin);
       printf("xbtoa Begin\n");
       n  = 0;
       while ((c = getc(f)) != EOF)
         {
           encode(c);
           n++;
         }
       while (pos)
         encode(0);
       printf("\nxbtoa End N %ld %lx E %lx S %lx R %lx\n", n, n, Ceor, Csum, Crot);
       return(0);
     }
   else
     return(1);
}

int init(int argc, char **argv)
{
   if (argc > 2)
     {
       printf("Usage: btoa binary-file >ascii-file\n");
       printf("       binary-file is the name of the file to be encoded.\n");
       return(BAIL_OUT);
     }
   if (argc == 1)
     {
       f = fdopen(0, "rb");             /* make sure stdin is opened binary */
       if (!f)
         printf("btoa: could not open stdin\n");
     }
   else
     {
       f = fopen(argv[1], "rb");
       if (!f)
         printf("btoa: could not open file %s\n", argv[1]);
     }
   if (!f)
     return(BAIL_OUT);
   else
     return(0);
}

void encode(unsigned int c)
{

  Ceor ^= c;
  Csum += c;
  Csum += 1;
  if ((Crot & (unsigned long)0x80000000)) {
    Crot <<= 1;
    Crot += 1;
  } else {
    Crot <<= 1;
  }
  Crot += c;

  input <<= 8;
  input |= c;

  if (pos == 3)            /* This fills up the input word */
    {
       if (input == 0)
         {
           charout('z' - '!');  /* little z marks 4 consecutive nulls */
           charsout += 4;
         }
       else
       {
         charout(input / EXP85_4);
         input %= EXP85_4;
         charout(input / EXP85_3);
         input %= EXP85_3;
         charout(input / EXP85_2);
         input %= EXP85_2;
         charout(input / EXP85_1);
         input %= EXP85_1;
         charout(input);
       }
       pos = 0;
    }
  else
    pos++;
}

void charout(unsigned long val)
{
   putchar((unsigned int)val + '!');
   charsout++;
   linecnt++;
   if (linecnt >= MAXLINE)
     {
       putchar('\n');
       linecnt = 0;
     }
}
