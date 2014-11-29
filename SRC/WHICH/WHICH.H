/* @(#)12	1.1  src/which/which.h, aixlike.src, aixlike3  9/27/95  15:46:57 */
/* equates for which.c */

#define CCHMAXPATH 262

#define FOUND 1
#define NOTFOUND 0
#define DONE 1
#define UNDONE 0
#define BAILOUT -1

char cwd[CCHMAXPATH];
struct charchain {
                    char *name;
                    struct charchain *next;
                 };
struct charchain *ccroot = NULL;
struct charchain *pathroot = NULL;

char *exesuff = ".exe";                 //@1c
char *cmdsuff = ".cmd";                 //@1c
char *comsuff = ".com";                 //@1a

#define NUMSUFFIXES   3
char *suffixes[NUMSUFFIXES];

/* Function prototypes */

int init(int argc, char **argv);
void do_a_filespec(char *filespec);
void free_compchain(struct charchain *root);
int do_a_file(char *filename);
int dofsfile(char *filename);
int dofqfile(char *filename);
int donotfqfile(char *filename, char *pathname);
int isfq(char *fn);
int isambig(char *fn);
void showresult(char *fqname);
char *getnamepart(char *fqname);
void tell_usage(void);
