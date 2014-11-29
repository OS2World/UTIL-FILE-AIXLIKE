/* @(#)49	1.1  src/grep/grep.h, aixlike.src, aixlike3  9/27/95  15:44:43 */
/* prototypes for the grep functions */
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

int compile_reg_expression(      unsigned char *from,
                                 unsigned char *to,
                                 int      size_of_pattern_buff);

int count_reg_expression_matches(unsigned char *line,
                                 unsigned char *pattern);

int matches_reg_expression(      unsigned char *line,
                                 unsigned char *pattern);

char *find_reg_expression(       unsigned char *line,
                                 unsigned char *pattern,
                                 int           *where_to_put_length);

unsigned char *pmatch(           unsigned char *line,
                                 unsigned char *pattern,
                                 subpat        *subPatternData);
