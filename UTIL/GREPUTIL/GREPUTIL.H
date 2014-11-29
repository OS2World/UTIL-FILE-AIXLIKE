/* @(#)39	1.1  util/greputil/greputil.h, aixlike.src, aixlike3  9/27/95  15:53:08 */
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
/* @1 08.05.91 Need to define the TAB character                          */
/*-----------------------------------------------------------------------*/
/* grep.h */

/* -------------------------------------------------------------------------- */
/*   regular expression token types                                           */
/* -------------------------------------------------------------------------- */

#define CHAR    1
#define BOL     2
#define EOL     3
#define ANY     4
#define CLASS   5
#define NCLASS  6
#define STAR    7
#define TPLUS   8
#define MINUS   9
#define ALPHA   10
#define DIGIT   11
#define NALPHA  12
#define PUNCT   13
#define RANGE   14
#define ENDPAT  15
#define MATCHM      16
#define MATCHMP     17
#define MATCHMTON   18
#define STARTSUBPAT 19
#define ENDSUBPAT   20
#define MATCHSUBPAT 21
#define BOP         22

#define CASTERISK        '*'
#define CPLUS            '+'
#define CMINUS           '-'
#define CCIRCUMFLEX      '^'
#define CDOLLARSIGN      '$'
#define CPERIOD          '.'
#define CCOLON           ':'
#define CBACKSLASH       '\\'
#define CLEFTBRACKET     '['
#define CRIGHTBRACKET    ']'
#define CHYPHEN          '-'
#define CCOMMA           ','
#define CLEFTCURLYBRACE  '{'
#define CRIGHTCURLYBRACE '}'
#define CLEFTPAREN       '('
#define CRIGHTPAREN      ')'
#ifndef CR
#define CR               13
#endif
#ifndef LF
#define LF               10
#define TAB              9                     /* @1a */
#endif

#ifndef SUBPATDEFINED
typedef struct _subpatdata {
        char *start;
        int  length;
} subpatdata;

typedef struct _subpat {
        int  numsubpats;
        int  activesubpats;
        subpatdata spdata[9];
} subpat;
#define SUBPATDEFINED
#endif
