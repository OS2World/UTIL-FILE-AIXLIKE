/* @(#)91	1.2  src/strings/strings.h, aixlike.src, aixlike3  1/4/96  23:34:42 */
/* include file for strings.c */


#define MAXLINE 1024
#define YES 1
#define NO  0
#define BAILOUT -1
#define CR  0x0d
#define LF  0x0a
#define DEFAULT_MINLEN 4

int init(int *argc, char ***argv);
void search_the_files(int argc, char **argv);
void search_stdin(void);
void do_search(FILE *f);
void printit(char *line, unsigned long offset);
void tell_usage(void);
