static char sccsid[]="@(#)00	1.1  src/tr/tr.c, aixlike.src, aixlike3  9/27/95  15:46:32";
/*
   tr.c

   Translates characters.

Syntax:

     tr [-A -c -Ac -cs -s] [-B] String1 String2
        |                |
        -----one of-------

     or

     tr [-d -cd] [-B] String1
        |      |
        -one of-

Description:

   The tr command copies characters form the standard input to the standard
   output with substitution or deletion of selected characters.  Input
   characters from string1 are replaced with the corresponding characters from
   String2.  The tr command cannot handle an ASCII NUL (\000) in String1
   or String2; it always deletes NUL from the input.

   For compatibility with BSD, the OS/2 version has the -B flag to force
   BSD semantics on the command line.  The discussion below describes
   'AIX' semantics and 'BSD' semantics as described in the AIX commands
   reference.

   In the AIX version abbreviations that can be used to introduce ranges
   of characters or repeated characters are as follows:

   [a-z]       Stands for a string of characters whose ASCII codes run
               from characters a to character z, inclusive.

   [a*num]     Stands for the number of repetitions of a.  The num
               variable is considreed to be indecimal unless the first digit
               of num is 0; then it is considered to be in octal.

   In the BSD version, the abbreviation that can be used to introduce
   ranges of characters is:

   a-z         Stands for a string of characters whose ASCII codes run
               from character a to character z, inclusive.  Note that
               brackets are not special characters and the hyphen is.

   Use the escape character \ (backslash) to remove special meaning from
   any character in a string.  Use the \ folllowed by 1, 2 or 3 octal digits
   for (the ascii code of a character.

Flags:

   -A       Translates on a byte-by-byte basis.  The OS/2 version *always*
            translates on a byte-by-byte basis.

   -B       Forces BSD semantics for String1 and String2.

   -c       Indicates that the characters (1-255 decimal) *not* in String1
            are to be replaced by the characters in string 2.

   -d       Deletes all input characters in String1.

   -s       If the translated characters are consecutively repeated in
            the output stream, output only one character.

Examples

   1. To translate braces into parentheses:

      tr '{}' '()' <textfile >outfile

      This translates each { to ( and each } to ).  All other characters
      remain unchanged.

   2. To translate lowercase characters to uppercase:

      tr '[a-z]' '[A-Z'] <textfile >outfile
      or
      tr -B a-z A-Z <textfile >outfile

   3. In the AIX version, this is what happens if the strings are not
      the same length:

      tr '[0-9]' '#' <textfile >outfile

      This translates each 0 to a # (number sign).
      NOTE: if the two character strings are not the same length, then
      the extra characters in the longer one are ignored.

   4. In the BSD version, if the strings are not the same length:

      tr -B 0-9 #  <textfile >outfile

      This translates each digit to a #.
      NOTE: If String2 is too short, it is padded to the length of String1
      by duplicating its last character.

   5. In the AIX version, to translate each digit to a #:

      tr '[0-9]' '[#*]' <textfile >outfile

      The * tells tr to repeat the # enough times to make the second
      string as long as the first one.

   6. To translate each string of digits to a single #:

      tr -s '[0-9]' '[#*]' <textfile >outfile

      or

      tr -Bs 0-9 # <textfile >outfile

   7. To translate all ASCII characters that are not specified:

      tr -c '[ -~]' '[A-_]?' <textfile >outfile

      This translates each nonprinting ASCII characters to the corresponding
      control key letter (/001 translates to "A", /002 to "B", etc.)
      ASCII DEL (/177), the character that follows ~ (tilde), translates
      to ?.

      Or with the BSD version:

      tr -Bc ' -~' 'A-_?' <textfile >outfile

   8. To create a list of the words in a file:

      tr -cs '[a-z][A-Z]' '[\012*]' <textfile >outfile

      or

      tr -Bcs a-z A-z '\012' <textfile >outfile

      This translates each string of nonalphabetic characters to a single
      new-line character.  The result is a list of all the words in textfile,
      one word per line.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BAIL_OUT   -1
#define OK          0
#define YES         1
#define NO          0

/* global variables */

char string1[258];      /* characters selected in String1         */
char string2[258];
char *optstring = "ABcds";

int  DeleteMode;        /* Are we in input-delete mode? */
int  BSDMode;           /* Are we obeying BSD syntax? */
int  ComplementMode;    /* Is String1 the complement string? */
int  DupSuppress;       /* Are we suppressing duplicate output chars? */
int  cliteral;          /* boolean so we can distinguish syntactic elements */

/*   globals variables exported by getopt.obj */
extern char *optarg;
extern int   optind;
extern int   opterr;

/* Function Prototypes                        */
int init(int argc, char **argv);
int makeString1(char *s);
int makeString2(char *s);
unsigned char GetNextChar(char **s);
int GetScale(char **s);
int invert(char *s);
int processRange(unsigned char first, unsigned char last, unsigned char **ppPut);
unsigned int otoi(char *s);
int makePattern(char *pat);
int convert_stdin(char *pat);
void tell_usage(void);
extern int getopt(int, char **, char *);

/****************************************************************************/
int main(int argc, char ** argv)
{
   char conversion[258];
   int rc;

   string1[0] = 0;
   if (init(argc, argv) != BAIL_OUT)
     {
        if (makePattern(conversion) != BAIL_OUT)
          convert_stdin(conversion);
        rc = OK;
     }
   return(rc);
}

/************************************************************************/
/*  init() - initialize globals and parse the command line.  This       */
/*           includes parsing String1 and String2 and setting up the    */
/*           conversion string.                                         */
/************************************************************************/
int init(int argc, char **argv)
{
    int i, lastind, rc;
#ifdef I16
    char c;
#else
    int c;
#endif

    DeleteMode = NO;           /* we haven't seen a -d flag */
    BSDMode = NO;              /* we haven't seen -B */
    ComplementMode= NO;        /* we haven't seen -c */
    DupSuppress = NO;          /* we haven't seen -s */
    string1[0] = '\0';         /* FROM string starts empty */
    string2[0] = '\0';         /* ditto TO string   */

#ifdef I16
    while ( (c = (char)getopt(argc, argv, optstring)) != EOF)
#else
    while ( (c = (char)getopt(argc, argv, optstring)) != (char)EOF)
#endif
      {
         switch (c)
         {
            case 'A':   break;

            case 'B':   BSDMode = YES;
                        break;

            case 'c':   ComplementMode = YES;
                        break;

            case 'd':   if (DupSuppress)
                          {
                             tell_usage();
                             return(BAIL_OUT);
                          }
                        else
                          DeleteMode = YES;
                        break;

            case 's':   if (DeleteMode)
                          {
                             tell_usage();
                             return(BAIL_OUT);
                          }
                        else
                          DupSuppress = YES;
                        break;

            default:    tell_usage();
                        return(BAIL_OUT);
                        break;
         }
      }
    i = optind;         /* everything past the flags is string1 or string2 */
    if (DeleteMode)     /* The last thing is String2, except if the -d flag*/
      lastind = argc-1; /* was specified, in which case there is no Sting2 */
    else
      lastind = argc-2;
    if (i > lastind)
      {
         tell_usage();
         return(BAIL_OUT);
      }
    rc = OK;
    for (; i <= lastind && !rc; rc = makeString1(argv[i++]));
    if (ComplementMode)
      rc = invert(string1);
    if (!rc)
      rc = makeString2(argv[argc-1]);
    return(rc);
}

/*************************************************************************/
/* makeString1()                                                         */
/*              Use the arguments of the string passed to create a       */
/*              string in string1                                        */
/*************************************************************************/
int makeString1(char *s)
{
   char *p, *q;
   int inRange = 0;
   unsigned char c, lastc;

   q = string1+strlen(string1);
   p = s;
   lastc = 0;
   while ( *p )
     {
        c = GetNextChar(&p);
        if (BSDMode)                /* In BSD, only the - is special */
          switch (inRange)
          {
             case 0:      if (c == '-' && !cliteral && lastc)
                            inRange = 1;
                          else
                            {
                              if (lastc)
                                *q++ = lastc;
                              lastc = c;
                            }
               break;
             case 1:      processRange(lastc, c, &q);
                          inRange = 0;
                          lastc = 0;
               break;
          }
        else                       /* The AIX syntax is hairier */
          switch (inRange)
          {
             case 0:      if (c == '[')      /* a range is starting */
                            inRange = 1;
                          else               /* just a plain old character */
                            *q++ = c;
               break;
             case 1:      lastc = c;         /* 1st char of range expected */
                          inRange = 2;
               break;
             case 2:      if (c == '-' && !cliteral)  /* '-' expected */
                            inRange = 3;
                          else
                            {
                              fprintf(stderr,"tr: %s contains an invalid range expression\n",s);
                              return(BAIL_OUT);
                            }
               break;
             case 3:      processRange(lastc, c, &q); /* 2nd char of range expected */
                          inRange = 4;
               break;
             case 4:      if (c == ']' && !cliteral)  /* closing bracket expected */
                            {
                              inRange = 0;
                              lastc = 0;
                            }
                          else
                            {
                              fprintf(stderr,"tr: %s contains an invalid range expression\n",s);
                              return(BAIL_OUT);
                            }
               break;
           }
     }  /* end WHILE */
   if (inRange != 0)
     {
       fprintf(stderr,"tr: %s contains an invalid range expression\n",s);
       return(BAIL_OUT);
     }
   if (lastc)
     *q++ = lastc;
   *q = '\0';
   return(OK);
}


/*************************************************************************/
/* makeString2()                                                         */
/*                String2 has some semantic nuances                      */
/*************************************************************************/
int makeString2(char *s)
{
   unsigned char *p, *q;
   unsigned char c, lastc;
   int s1len, s2len, i, inRange;

   inRange = 0;
   lastc = 0;
   q = string2;
   p = s;
   while ( *p )
     {
        c = GetNextChar(&p);
        if (BSDMode)                /* In BSD, only the - is special */
          switch (inRange)
          {
             case 0:      if (c == '-' && !cliteral && lastc)
                            inRange = 1;
                          else
                            if (c == '*'&&!cliteral)    /* A trailing asterisk in String2 */
                              {                         /* (followed by an integer) means */
                                 i = GetScale(&p);      /* repeat the preceding char that */
                                 if (i)                 /* many times                     */
                                   while (i)
                                     {
                                        *q++ = lastc;
                                        i--;
                                     }
                                 else
                                   {
                                      if (lastc)
                                        *q++ = lastc;
                                      lastc = '*';
                                   }
                              }
                            else
                              {
                                if (lastc)
                                  *q++ = lastc;
                                lastc = c;
                              }
               break;
             case 1:      processRange(lastc, c, &q);
                          inRange = 0;
                          lastc = 0;
               break;
          }
        else                       /* The AIX syntax is hairier */
          switch (inRange)
          {
             case 0:      if (c == '[')      /* a range is starting */
                            inRange = 1;
                          else               /* just a plain old character */
                            *q++ = c;
               break;
             case 1:      lastc = c;         /* 1st char of range expected */
                          inRange = 2;
               break;
             case 2:      if (c == '-' && !cliteral)  /* '-' or '*' expected */
                            inRange = 3;
                          else
                            if (c == '*' && !cliteral)
                              {
                                i = GetScale(&p);          /* In AIX notation, an asterisk*/
                                if (i)                     /* terminating a range means   */
                                  {
                                    while (i)              /* "pad to the length of       */
                                      {                    /* String1", which must needs  */
                                        *q++ = lastc;     /* terminate processing of     */
                                        i--;              /* String2                     */
                                      }
                                    inRange = 4;
                                  }
                                else
                                  {
                                    s2len = q - string2;
                                    s1len = strlen(string1);
                                    while (s1len > s2len)
                                      {
                                        *q++ = lastc;
                                        s2len++;
                                      }
                                    *q = '\0';
                                    return(OK);
                                  }
                              }
                            else
                              {
                                fprintf(stderr,"tr: %s contains an invalid range expression\n",s);
                                return(BAIL_OUT);
                              }
               break;
             case 3:      processRange(lastc, c, &q); /* 2nd char of range expected */
                          inRange = 4;
               break;
             case 4:      if (c == ']' && !cliteral)  /* closing bracket expected */
                            {
                              inRange = 0;
                              lastc = 0;
                            }
                          else
                            {
                              fprintf(stderr,"tr: %s contains an invalid range expression\n",s);
                              return(BAIL_OUT);
                            }
               break;
           }
     }  /* end WHILE */
   if (inRange != 0)
     {
       fprintf(stderr,"tr: %s contains an invalid range expression\n",s);
       return(BAIL_OUT);
     }
   if (lastc)
     *q++ = lastc;
   *q = '\0';
   s1len = strlen(string1);
   s2len = strlen(string2);
   if (BSDMode)        /* in BSD semantics, pad string2 if it is short */
     {
       p = q - 1;               /* last character in string2 */
       while (s2len < s1len)
         {
           *q++ = *p;
           s2len++;
         }
       *q = '\0';
     }
   else                /* in AIX semantics, truncate the longer of the two */
     if (s1len > s2len)
       string1[s2len] = '\0';
     else
       if (s2len > s1len)
         string2[s1len] = '\0';
   return(OK);
}

/*************************************************************************/
/* GetNextChar(**s)                                                      */
/*             interpret the next part of the input string and position  */
/*             the pointer at the position past the piece.  Return the   */
/*             piece as an unsigned int.                                 */
/*************************************************************************/
unsigned char GetNextChar(char **s)
{
   unsigned char *p;
   unsigned int val;
   int i;
   unsigned char c;

   p = (unsigned char *)*s;
   c = *p++;
   if (c == '\\')
     {
        cliteral = 1;
        if (*p >= '0' && *p <= '8')        /* It's an octal value */
          {
             val = otoi(p);
             for (i = 0; *p >= '0' && *p <= '8' && i < 3; p++, i++);
             c = (unsigned char)val;
          }
        else
          c = *p++;
     }
   else
     cliteral = 0;
   *s = p;
   return(c);
}


/*************************************************************************/
/* GetScale(**p)                                                         */
/*     The pointer is to an asterisk followed by 0 or more digits.       */
/*     If the digits start with 0, they are octal, otherwise decimal.    */
/*     return their value.                                               */
/*************************************************************************/
int GetScale(char **s)
{
   int val;
   char *p;

   p = *s;
   p++;
   if (*p == '0')
     {
       val = otoi(p);
       while (*p >= '0' && *p <= '7')
         p++;
     }
   else
     {
        val = atoi(p);
        while (*p >= '0' && *p <= '9')
          p++;
     }
   *s = p;
   return(val);
}

/*************************************************************************/
/* invert(string)                                                        */
/*               takes the complement of the string with respect to      */
/*               the extended ascii character set.  Gives up if the      */
/*               string is out of order.                                 */
/*************************************************************************/

int invert(char *s)
{
   int i;
   unsigned char istring[258];
   unsigned char *p, *q;

   q = istring;
   for (i = 0; i < 256; istring[i++] = '\0');
   for (p = (unsigned char *)s; *p; istring[*p++] = 'x');
   p = (unsigned char *)s;
   for (i = 1; i < 256; i++)
      if (!istring[i])
        *p++ = (unsigned char)i;
   return(OK);
}

/****************************************************************************/
/* processRange(range_spec,tostring)                                        */
/*           expand a range expressed in the form x-y where - is literal.   */
/*           The range must obey x < y.                                     */
/****************************************************************************/
int processRange(unsigned char first, unsigned char last, unsigned char **ppPut)
{
   unsigned char *p;
   unsigned char  i, f, l;

   p   = *ppPut;
   f = first;
   l = last;
   if (first > last)
     {
        l = first;
        f = last;
     }
   for (i = f; i <= l; *p++ = i++);
   *ppPut = p;
   return(OK);
}

/****************************************************************************/
/* int otoi(string)                                                         */
/****************************************************************************/
unsigned int otoi(char *s)
{
   char *p;
   unsigned int i = 0;

   for (p = s; *p >= '0' && *p <= '7'; i = (i * 8)+(*p++ - '0'));
   return(i);
}

/****************************************************************************/
/* makePattern(pattString)                                                  */
/*              create the pattern string used to translate the input.      */
/*              The pattern starts out as an identity: each char translates */
/*              to itself.                                                  */
/****************************************************************************/
int makePattern(char *pat)
{
   unsigned char *p, *q;
   int i;
   unsigned int s1val, s2val;

   if (DeleteMode || DupSuppress)
     for (i = 0; i < 256; pat[i++] = 0);
   else
     for (i = 0; i < 256; i++)
        pat[i] = (char)i;
   for (p = string1, q = string2; *p; p++, q++)
        {
          s1val = (unsigned int)*p;
          s2val = (unsigned int)*q;
//        printf("*p = %u; *q = %u\n", s1val, s2val);
          pat[s1val] = (char)s2val;
        }

   return(OK);
}

/****************************************************************************/
/* convert_stdin(pattern)                                                   */
/*          use the pattern string to convert each byte of input to a byte  */
/*          of output.                                                      */
/****************************************************************************/
int convert_stdin(char *pat)
{
   unsigned int c;
   unsigned char lastc;
   FILE *f;

   f = fdopen(0, "rb");
   freopen("", "wb", stdout);
   if (!f)
     {
        printf("tr: Could not open stdin\n");
        return(BAIL_OUT);
     }

   if (DeleteMode)
     {
//       while ((c = (unsigned int)getchar()) != EOF)
       while ((c = (unsigned int)getc(f)) != EOF)
         if (c && !pat[c])
           putchar(c);
     }
   else
     if (DupSuppress)
       {
//         while ((c = (unsigned int)getchar()) != EOF)
         while ((c = (unsigned int)getc(f)) != EOF)
           if (c && !pat[c])
             {
               putchar(c);
               lastc = 0;
             }
           else
             {
               if (pat[c] != (char)lastc)
                 putchar(pat[c]);
               lastc = (unsigned char)pat[c];
             }
       }
     else
//       while ((c = (unsigned int)getchar()) != EOF)
       while ((c = (unsigned int)getc(f)) != EOF)
         if (pat[c])
           putchar(pat[c]);
   return(OK);
}

/****************************************************************************/
/* tell_usage()                                                             */
/****************************************************************************/
void tell_usage()
{
     fprintf(stderr,"tr [-A -c -Ac -cs -s] [-B] String1 String2\n");
     fprintf(stderr,"or\n");
     fprintf(stderr,"tr [-d -cd] [-B] String1\n\n");

   fprintf(stderr,"-A       Translates on a byte-by-byte basis.  The OS/2 version *always*\n");
   fprintf(stderr,"         translates on a byte-by-byte basis.\n");

   fprintf(stderr,"-B       Uses BSD semantics for String1 and String2.\n");

   fprintf(stderr,"-c       Indicates that the characters (1-255 decimal) *not* in String1\n");
   fprintf(stderr,"         are to be replaced by the characters in string 2.\n");

   fprintf(stderr,"-d       Deletes all input characters in String1.\n");

   fprintf(stderr,"-s       If the translated characters are consecutively repeated in\n");
   fprintf(stderr,"         the output stream, output only one character.\n");
}
