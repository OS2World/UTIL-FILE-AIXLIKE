static char sccsid[]="@(#)38	1.1  util/greputil/greputil.c, aixlike.src, aixlike3  9/27/95  15:53:07";
/************************************************/
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation, 1991.  All rights reserved.     */
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 1       5/1/91                                           */
/* @1 06.07.91 in pmatch, 'ANY' should NOT match a terminating new line; */
/*             also, the EOL matching code did NOT match CRLF or LFCR    */
/*             correctly - it didn't advance the line pointer correctly. */
/* @2 08.05.91 Regular expression compiler did not compile \n and \t     */
/*             correctly                                                 */
/* @3 07.20.93 Wrong end-point was being returned for patterns like .*$  */
/* @4 02.14.94 Fix for @3 broke everything else.                         */
/*-----------------------------------------------------------------------*/


/* -------------------------------------------------------------------------- */
/*  greputil.c                                                                */
/*                                                                            */
/*  Contains routines to compile and interpret regular expressions.           */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*    compile_reg_expression(from, to, to_length) compiles a regular          */
/*    a regular expression and stores the compiles pattern.  It returns the   */
/*    length of the compiled pattern, or 0 if the compile was not successful. */
/*    If the number of bytes specified for the resulting pattern is           */
/*    insufficient, the value returned is one more than to_length; this       */
/*    signals an error.                                                       */
/*                                                                            */
/*    compile_class(from, to, to_end)    compiles a specific kind of regular  */
/*    expression: a "class".  These are those collections between square      */
/*    brackets.                                                               */
/*                                                                            */
/*    compile_matchcnt(from, to, end) returns an integer: 0, 2 or 3.  It      */
/*    returns 0 if the occurance operation (in the form \\{[1-9],*[1-9]*\\} ) */
/*    is invalid; and the length of the compiled pattern otherwise.           */
/*    The side effects are that the compiled pattern is updated and the       */
/*    source pointer is also.                                                 */
/*                                                                            */
/*    matches_reg_expression(line, pattern) is boolean.  It returns 0 if      */
/*    the compiled regular expression is not found in the line, 1 otherwise.  */
/*                                                                            */
/*    count_reg_expression_matches(line, pattern) returns the number of       */
/*    times that the compiled regular expression is found in the line.        */
/*                                                                            */
/*    find_reg_expression(line, pattern, PutLength) returns a pointer to      */
/*    the first occurance of the pattern in the line, or NULL if the pattern  */
/*    does not occur.  If the pattern does occur, the length is placed at the */
/*    address specified.                                                      */
/*                                                                            */
/*    pmatch(current_position, pattern, subpatdata) returns a pointer to      */
/*    the byte after the end of the pattern if the pattern starts at the      */
/*    current position; or NULL otherwise.  The caller is responsibile for    */
/*    providing a context in which to keep subpattern data.                   */
/* -------------------------------------------------------------------------- */
/*    Maintenance History:                                                    */
/*       . Created during Christmas vacation, 1990, by Bob Blair (grb).       */
/*       . Added occurance operators, Jan 6 1991 (grb).                       */
/*       . Added subpatterns, Jan 12 1991 (grb).                              */
/*       . Completely redid subpatterns to retain thread tolerance,           */
/*                            Jan 13 1991 (grb).                              */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/* -------------------------------------------------------------------------- */


#include "greputil.h"
// #include "grep.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#define  GETCHAR() (*fromP++)
#define  UNGETCHAR() (--fromP)

unsigned char *compile_class(unsigned char **from, unsigned char *to,
                             unsigned char *end);
unsigned int  compile_matchcnt(unsigned char **from, unsigned char *to,
                             unsigned char *end);
unsigned char *pmatch(unsigned char *line, unsigned char *pattern,
                      subpat *spdP);

void error(char *);
/* -------------------------------------------------------------------------- */
/*   compile_reg_expression                                                   */
/* -------------------------------------------------------------------------- */

int compile_reg_expression(unsigned char *from,
                               unsigned char *to,
                               int to_length)
{

   unsigned int     c,                     /* Current character              */
                    temp, i,               /* Temp                           */
                    numsubpats,            /* Number of subpatterns started  */
                    activesubpats,         /* Number of subpats not yet closed*/
                    inoccuranceop;         /* A state variable               */
   unsigned char    *toP,                  /* destination string pointer     */
                    *lastP,                /* Last pattern pointer           */
                    *toEndP,               /* end of space available for
                                              pattern.                       */
                    *fromP,                /* current position in source     */
                    *savePatternP,         /* Beginning of current pattern   */
                    tempbuf[10];           /* A short buffer into which to   */
                                           /* compile an occurance operation */

   toP = to;                             /* find start of result space  */
   toEndP = to + to_length - 1;          /* find last safe position for output*/
   fromP = from;                         /* find start of input */
   inoccuranceop = 0;                    /* initialize a state variable */
   numsubpats = activesubpats = 0;       /* initialize subpattern controls */

   while ( (c = GETCHAR()) && (toP <= toEndP) )
   {
                                         /* an asterisk and a plus-sign are  */
                                         /* prefix in effect but postfix in  */
                                         /* syntax, so we handle them        */
                                         /* seperately.  'Occurs M times' is */
                                         /* handled similarly.               */
      if (c == CBACKSLASH)               /* 'Occurs M times' is signalled by */
        {                                /* an escaped left curly brace.     */
           if ( (temp = GETCHAR()) == CLEFTCURLYBRACE)
             inoccuranceop = 1;
           else
             UNGETCHAR();
        }
      if (c == CASTERISK || c == CPLUS || inoccuranceop)
        {
          temp = *(toP - 1);             /* we will need to look at the char */
                                         /* we last put into the pattern.    */
          if (toP  == to    ||               /* * and + must follow something,*/
              temp == BOL   ||               /* can't act at the start of any */
              temp == EOL   ||               /* line,                         */
              temp == STAR  ||               /* and do not nest.              */
              temp == TPLUS ||
              temp == STARTSUBPAT)
            {
              error( "illegal occurrance op" );
              return(0);
            }

          *toP++ = ENDPAT;               /* put in an "END pattern"           */
          if (toP > toEndP) break;
          if (inoccuranceop)  /* if we have the form \{m... */
            {                       /* compile it into the special buffer */
              if ( (temp = compile_matchcnt(&fromP, tempbuf,
                                            tempbuf+sizeof(tempbuf)-1))
                         == 0 )
                return(0);
            }
          else
            temp = 1;
          toP += temp;
          if (toP > toEndP) break;

          savePatternP = toP;            /* move the pattern down 1, 2 or 3  */
          while (--toP > lastP)
            *toP = *(toP - temp);
          if (inoccuranceop)
            {
               inoccuranceop = 0;        /* this is the end of occurance op */
               for (i = 0; i < temp; i++) /* move occurance info to the front */
                 *toP++ = tempbuf[i];       /* of the pattern */
            }
          else
            {
               temp = (c == CASTERISK) ? STAR : TPLUS;
               *toP = (unsigned char)temp;
            }
          toP = savePatternP;            /* Restore our place in the output */
          continue;                      /* go get the next character       */
         }

                                         /* all the other cases are natural: */

       lastP = toP;                      /* Remember start       */
       switch(c)
         {
           case CCIRCUMFLEX :   *toP++ = BOL;
                            break;
           case CDOLLARSIGN :   *toP++ = EOL;
                            break;
           case CPERIOD     :   *toP++ = ANY;
                            break;
           case CLEFTBRACKET:   toP = compile_class(&fromP, toP, toEndP);
                                if (toP == NULL)
                                  return(0);
                            break;
           case CCOLON      :
                                if ( (c=GETCHAR()) )
                                  {
                                    switch( tolower( c ) )
                                    {
                                      case 'a':   *toP++ = ALPHA;
                                              break;
                                      case 'd':   *toP++ = DIGIT;
                                              break;
                                      case 'n':   *toP++ = NALPHA;
                                              break;
                                      case ' ':   *toP++ = PUNCT;
                                              break;
                                      default:
                                           error( "unknown ':' type" );
                                           return(0);
                                    }
                                  }
                                else
                                  {
                                    error( "no ':' type" );
                                    return(0);
                                  }
                            break;
           case CBACKSLASH  :   temp = GETCHAR();
                                if (temp == CLEFTPAREN)
                                  {
                                    *toP++ = STARTSUBPAT;
                                    numsubpats++;
                                    activesubpats++;
                                    break;
                                  }
                                else
                                  if (temp == CRIGHTPAREN)
                                    if (activesubpats == 0)  /* none started*/
                                      return(0);
                                    else
                                      {
                                        *toP++ = ENDSUBPAT;
                                        activesubpats--;
                                        break;
                                      }
                                  else
                                    if ( ((i = temp - '0') > 0) && (i <= 9) )
                                      if (numsubpats &&
                                          ((i + activesubpats) <= numsubpats) )
                                        {
                                          *toP++ = MATCHSUBPAT;
                                          *toP++ = (unsigned char)i;
                                          break;
                                        }
                                      else
                                        {
                                          error("Refers to unclosed subpat");
                                          return(0);
                                        }
                                    else
                                      if (temp == 'n')               /*@2a*/
                                        {                            /*@2a*/
                                           *toP++ = EOL;             /*@2a*/
                                           break;                    /*@2a*/
                                        }                            /*@2a*/
                                      else                           /*@2a*/
                                        {                            /*@2a*/
                                          if (temp == 't')           /*@2a*/
                                            temp = TAB;              /*@2a*/
                                          if (temp == '0')           /*@2a*/
                                            temp = 0;                /*@2a*/
                                        }
                                      c = temp;  /* this falls thru to default*/
           default          :   *toP++ = CHAR;
                                if (toP <= toEndP)
                                  *toP++ = (unsigned char) c;
         }
   } /* end WHILE */

   if (activesubpats != 0)            /* any subpatterns have to close */
     {
       error("Open subpatterns");
       return(0);
     }

   if (toP < toEndP)                  /* If there is room, terminate the   */
     {                                /*  compiled pattern                 */
       *toP++ = ENDPAT;
       *toP++ = '\0';
       return(toP - to);              /* and return its length */
     }
   else
     {
       error("Pattern overflows caller's buffer");
       return(0);
     }
}


/* -------------------------------------------------------------------------- */
/*   compile_class                                                            */
/* -------------------------------------------------------------------------- */

unsigned char *compile_class(
                    unsigned char **from, unsigned char *to, unsigned char *end)
{

   unsigned char    *toP,                  /* destination pattern pointer */
                    *fromP,                /* input pointer */
                    *startP;               /* Pattern start     */
   unsigned int     c,                     /* Current character */
                    temp;                  /* Temp              */

   toP = to;
   fromP = *from;

   if ( (c = GETCHAR()) == 0)
     {
       error( "class terminates badly" );
       return(NULL);
     }

   if ( c == CCIRCUMFLEX)    /* if first char is circumflex, the class is */
                                  /* reverse-logic -- exclusionary */
       *toP++ = NCLASS;
   else
     {
       UNGETCHAR();
       *toP++ = CLASS;
     }

   startP = toP;               /* remember where byte count is */
   *toP++ = '\0';                /* and initialize byte count */

   while ( (c = GETCHAR())  )
     {
       temp = GETCHAR();                  /* peek at next char */
       if (c == CBACKSLASH)            /* Handle escaped characters */
         {
           if ( !temp )
             {
                error( "class terminates badly" );
                return(NULL);
             }
           *toP++ = (unsigned char)temp;
         }
       else
         if ( (c == (unsigned int)CHYPHEN) &&
                  ((toP - startP) > 1) &&
                      (temp != CRIGHTBRACKET) &&
                          (temp) )
           {
             c = *(toP - 1);                 /* range starts with prev char */
             *(toP - 1) = RANGE;             /* Range mark   */
             *toP++ = (unsigned char)c;      /* Put starting char after RANGE */
             if (toP >= end)
               return(NULL);
             *toP++ = (unsigned char)temp;   /* This is the ending char */
           }
         else
           {
             UNGETCHAR();                    /* lose the peeked character */
             if (c == (unsigned)CRIGHTBRACKET)
               if ( (toP - startP) > 1)
                 break;                      /* Normal exit: end of class */
             *toP++ = (unsigned char)c;      /* Just a normal character */
           }
     }  /* end WHILE */

   if (c != CRIGHTBRACKET)
     {
       error( "unterminated class" );
       return(NULL);
     }

   if ( (c = toP - startP) >= 256 )
     {
       error( "class too large" );
       return(NULL);
     }

   *from = fromP;      /* update place in the input stream */
   if (c)
     {
       *startP = (unsigned char)c;
       return(toP);
     }
   else
     {
       error("empty class");
       return(NULL);
     }
}

/* -------------------------------------------------------------------------- */
/*   compile_matchcnt  -  number(s) within escaped curly brackets indicate    */
/*                        the number of times a pattern may be repeated:      */
/*                           \{m\} means exactly m times;                     */
/*                           \{m,\} means at least m times;                   */
/*                           \{m,n\} means at least m but no more than n times*/
/*                                                                            */
/*   On entry, from points to the first digit of m.                           */
/*                                                                            */
/*   This routine returns the length of the compiled string.  It also updates */
/*   the pointer into the source string.                                      */
/* -------------------------------------------------------------------------- */

unsigned int compile_matchcnt(
                    unsigned char **from, unsigned char *to, unsigned char *end)
{

   unsigned char    *toP,                  /* destination pattern pointer */
                    *fromP,                /* input pointer */
                    *startP;               /* Pattern start     */
   unsigned int     c, m, n,               /* Current character */
                    digitfound;            /* state variable */

   toP = to;
   startP = toP++;
   fromP = *from;

   *startP = MATCHM;
   digitfound = 0;
   m = n = 0;
   for (c = GETCHAR(); isdigit(c); c = GETCHAR())
     {
       m = (m * 10) + c - '0';
       digitfound = 1;
     }
   if (!digitfound)
     {
        error("Invalid occurance operation syntax");
        return(0);
     }
   if (m > 255)
     {
        error("match count too large");
        return(0);
     }

   *toP++ = (unsigned char)m;
   if (c == CCOMMA)
     {
       *startP = MATCHMP;
       for (c = GETCHAR(); isdigit(c); c = GETCHAR())
         n = (n * 10) + c - '0';
       if (n > 255)
         {
            error("match count n too large");
            return(0);
         }
       if (n > 0)
         {
           *startP = MATCHMTON;
           *toP++ = (unsigned char)n;
         }
     }
   if (c != CBACKSLASH)
     {
        error("Bad match m pattern (missing \\)");
        return(0);
     }
   if ( (c = GETCHAR()) != CRIGHTCURLYBRACE)
     {
        error("Bad match m pattern (missing })");
        return(0);
     }
   *from = fromP;    /* send back correct place in input stream */
   return(toP - startP);      /* and the length of the result (2 or 3) */
}

/* -------------------------------------------------------------------------- */
/*   count_reg_expression_matches                                             */
/* -------------------------------------------------------------------------- */
int count_reg_expression_matches(unsigned char *line, unsigned char *pattern )
{
  unsigned char          *lineP,
                         *patternP,
                         *nextP;
  int                    matches;
  subpat                 spd;

  spd.numsubpats = spd.activesubpats = 0;
  matches = 0;
  lineP = line;
  patternP = pattern;
  if (*pattern == BOL)
    patternP++;
  if (nextP = pmatch(lineP, patternP, &spd))
    {
      lineP = nextP;
      ++matches;
    }
  else
    if ( (*pattern == BOL) || (!*lineP) )
      return(0);

  for (lineP++; *lineP; lineP++)
    {
      spd.numsubpats = spd.activesubpats = 0;
      if ( nextP = pmatch(lineP, pattern, &spd) )
        {
          lineP = nextP - 1;
          ++matches;
        }
    }

  return(matches);
}

/* -------------------------------------------------------------------------- */
/*   matches_reg_expression                                                   */
/* -------------------------------------------------------------------------- */
int matches_reg_expression(unsigned char *line, unsigned char *pattern )
{
   unsigned char    *lineP,
                    *patternP,
                    *nextP;
   int              matches;
   subpat           spd;

   matches = 0;
   spd.numsubpats = spd.activesubpats = 0;
   lineP = line;
   patternP = pattern;
   if (*pattern == BOL)
     patternP++;
   if (nextP = pmatch(lineP, pattern, &spd))
     return(1);
   else
     if ( (*pattern == BOL) || (!*lineP) )
       return(0);

   for (lineP++; *lineP; lineP++)
     {
       spd.numsubpats = spd.activesubpats = 0;
       if ( nextP = pmatch(lineP, pattern, &spd) )
         return(1);
     }
   return(0);
}
/* -------------------------------------------------------------------------- */
/*   find_reg_expression                                                      */
/* -------------------------------------------------------------------------- */
char *find_reg_expression(unsigned char *line, unsigned char *pattern, int *len)
{
   unsigned char    *lineP,
                    *patternP,
                    *nextP;
   int              matches;
   subpat           spd;

   matches = 0;
   spd.numsubpats = spd.activesubpats = 0;
   lineP = line;
   patternP = pattern;
   if (*pattern == BOL)
     patternP++;
   if (nextP = pmatch(lineP, patternP, &spd))
     {
       *len = nextP - lineP;
       return(lineP);
     }
   else
     if ( (*pattern == BOL) || (!*lineP) )
       return(NULL);

   for (lineP++; *lineP; lineP++)
     {
       spd.numsubpats = spd.activesubpats = 0;
       if ( nextP = pmatch(lineP, pattern, &spd) )
         {
           *len = nextP - lineP;
           return(lineP);
         }
     }
   return(0);

}
/* -------------------------------------------------------------------------- */
/*   pmatch                                                                   */
/* -------------------------------------------------------------------------- */
unsigned char *pmatch(unsigned char *line, unsigned char *pattern,
                      subpat *spdP)
{
        unsigned char    *lineP,        /* Current line pointer         */
                         *patternP,     /* Current pattern pointer      */
                         *extendP,      /* End for STAR and TPLUS match  */
                         *extstartP;    /* Start of STAR match          */
        unsigned int     c,mm,nn;       /* Current character            */
        unsigned int     matches;       /* counts matches for MATCHM, etc */
        int     op;                     /* Pattern operation            */
        int     n;                      /* Class counter                */
        int     numsubpatsav;           /* Number of subpatterns in use */
        int     activesubpatsav;        /* Subpats opened but not closed */

   lineP = line;
   patternP = pattern;

   while ((op = *patternP++) != ENDPAT)
     {
       switch(op)
       {
         case CHAR:
                         if ( *lineP++ != *patternP++ )
                           return(NULL);
                  break;
         case EOL:
/* @1c */                if ( (c=*lineP++) != '\0')
                           if ( (c != CR) && (c != LF) )            /* @1a */
                             return(NULL);
                           else                                     /* @1a */
                             {                                      /* @1a */
                               c = (c == CR) ? LF : CR;             /* @1a */
                               if (*lineP == (unsigned char)c)      /* @1a */
                                 ++lineP;                           /* @1a */
                             }                                      /* @1a */
                                                                    /* @1a */
                  break;
         case ANY:
/* @1c */                if ( (c = *lineP++) == '\0' )
                           return(NULL);
                         else
                           if ( (c == CR) || (c == LF))             /* @1a */
                             if (*lineP == '\0')                    /* @1a */
                               return(NULL);                        /* @1a */
                             else                                   /* @1a */
                               {                                    /* @1a */
                                  c = (c == CR) ? LF : CR;          /* @1a */
                                  if (*lineP == (unsigned char)c)   /* @1a */
                                    if (*++lineP == '\0')           /* @1a */
                                      return(NULL);                 /* @1a */
                               }                                    /* @1a */
                  break;
         case DIGIT:
                         if ( (c = *lineP++) < '0' || (c > '9') )
                           return(NULL);
                  break;
         case ALPHA:
                         c = tolower( *lineP++ );
                         if (c < 'a' || c > 'z')
                           return(NULL);
                  break;
         case NALPHA:
                         c = tolower(*lineP++);
                         if (c < 'a' || c > 'z')
                           return(NULL);
                         else
                           if (c < '0' || c > '9')
                             return(NULL);
                  break;
         case PUNCT:
                         c = *lineP++;
                         if (c == 0 || c > ' ')
                           return(NULL);
                  break;
         case CLASS:
         case NCLASS:
                         c = *lineP++;
                         n = *patternP++ & 0xff;
                         do
                           {
                             if (*patternP == RANGE)
                               {
                                 patternP += 3;
                                 n -= 2;
                                 if ( (c >= *(patternP - 2)) &&
                                      (c <= *(patternP - 1))    )
                                   break;
                               }
                             else
                               if (c == *patternP++)
                                 break;
                           } while (--n > 1);
                         if (op == CLASS)
                           if (n <= 1)
                             return(NULL);
                           else
                             patternP += (n - 2);
                         else
                           if (n > 1)
                             return(NULL);
                  break;
         case TPLUS:                      /* One or more ...     */
                         if ((lineP = pmatch(lineP,patternP,spdP)) == NULL)
                           return(NULL);
                                         /* otherwise fall thru to */
         case STAR:                      /* Zero or more ...    */
         case MATCHM:
         case MATCHMP:
         case MATCHMTON:
                         matches = 0;
                         mm = 0;
                         nn = 0xffff;  /* some ridiculously high number */
                         if ( (op == MATCHM) || (op == MATCHMP) )
                           mm = (unsigned int)*patternP++;
                         else
                           if (op == MATCHMTON)
                             {
                                mm = (unsigned int)*patternP++;
                                nn = (unsigned int)*patternP++;
                             }
                         extstartP = lineP;
                         numsubpatsav = spdP->numsubpats;
                         activesubpatsav = spdP->activesubpats;
                         while (*lineP && (extendP = pmatch(lineP,patternP,spdP)))
                           {
                             lineP = extendP;       /* Get longest match   */
                             matches++;
                             spdP->numsubpats = numsubpatsav;
                             spdP->activesubpats = activesubpatsav;
                           }
                         if (matches < mm)
                           return(NULL);
                         while (*patternP++ != ENDPAT);
/* There is an unfortunate ambiguity about the interpretation of EOL.  @3a */
/* You just about have to allow a null to match EOL.  Unforutnately,   @3a */
/* in a pattern like .*$, we are pointing at the terminating null      @3a */
/* now, and the pmatch call below will match it, returning the address @3a */
/* of the byte past the null as the start of the next string.  This    @3a */
/* screws up any program that relies on this pointer (like sed).       @3a */
/* I think it's safe to back up one character in this case, so we      @3a */
/* should be pointing at the real EOL.                                 @3a */
//                         if (*lineP == '\0')               @4d      /*@3a */
//                           lineP--;                        @4d      /*@3a */
/* Unfortunately, that didn't work for squat.  It completed screwed    @4a */
/* up any pattern that really did hit end there.  The following is     @4a */
/* a horrible kludge, but I'm going to try it.                         @4a */
                         if (*lineP == '\0' && *patternP == EOL)    /* @4a */
                           return lineP;                            /* @4a */
                         while (lineP >= extstartP)
                           {
                             numsubpatsav = spdP->numsubpats;
                             activesubpatsav = spdP->activesubpats;
                             if (extendP = pmatch(lineP, patternP, spdP))
                               {
                                 if (op == MATCHM)
                                   if (matches == mm)
                                     return(extendP);
                                   else
                                     return(NULL);
                                 else
                                   if ( (matches >= mm) && (matches <= nn) )
                                     return(extendP);
                                   else
                                     return(NULL);
                               }
                             --lineP;            /* Nope, try earlier   */
                             --matches;
                             spdP->numsubpats = numsubpatsav;
                             spdP->activesubpats = activesubpatsav;
                           }
                         return(NULL);          /* Nothing else worked */

         case STARTSUBPAT:                      /* start a subpattern */
                         spdP->spdata[spdP->numsubpats].start = lineP;
                         spdP->spdata[spdP->numsubpats].length = 0;
                         spdP->numsubpats++;
                         spdP->activesubpats++;
                         break;

         case ENDSUBPAT: n = spdP->numsubpats - spdP->activesubpats;
                         spdP->spdata[n].length = lineP - spdP->spdata[n].start;
                         spdP->activesubpats--;
                         break;

         case MATCHSUBPAT:
                         n = *patternP++;  /* number of the subpat  to matcn */
                         if ( strncmp(spdP->spdata[n-1].start,
                                      lineP,
                                      spdP->spdata[n-1].length) )
                           return(NULL);
                         break;

         case BOL:       if ( (lineP!=line)&&((c=lineP[-1])!=CR) && (c!=LF))
                           return(NULL);
                         break;

         default:
                         error( "bad op code");
                         return(NULL);
       }  /* end SWITCH */
     }  /* end WHILE */
   return(lineP);
}

void error(char *str)
{
#ifdef DEBUG
  printf("       %s\n", str)
#endif
}
