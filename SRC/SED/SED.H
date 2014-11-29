/* @(#)80	1.1  src/sed/sed.h, aixlike.src, aixlike3  9/27/95  15:45:47 */
/* sed.h    --     constants and data structures for sed */

#define OK        0
#define YES       1
#define NO        0
#define BAIL_OUT -1
#define MAXCMDS  99
#define ESC_CHAR '\\'
#define MAXPATH  260
#define MAXLINE  512


/* constants for the one-letter sed subcommands */

#define PUT_TEXT                   'a'
#define UNCONDITIONAL_BRANCH       'b'
#define REPLACE                    'c'
#define DELETE                     'd'
#define DELETE_FIRST_LINE          'D'
#define REPLACE_WITH_HOLD_DATA     'g'
#define APPEND_WITH_HOLD_DATA      'G'
#define REPLACE_HOLD_DATA          'h'
#define APPEND_TO_HOLD_DATA        'H'
#define IPUT_TEXT                  'i'    /* like PUT_TEXT */
#define WRITE_WITH_HEX             'l'
#define WRITE_AND_REFRESH          'n'
#define APPEND_TO_PATTERN          'N'
#define WRITE_TO_STDOUT            'p'
#define WRITE_FIRST_LINE_TO_STDOUT 'P'
#define BRANCH_TO_END              'q'
#define PUT_TEXT_FROM_FILE         'r'
#define CHANGE                     's'
#define CONDITIONAL_BRANCH         't'
#define APPEND_PATTERN_TO_FILE     'w'
#define EXCHANGE_WITH_HOLD_DATA    'x'
#define CHARACTER_REPLACE          'y'
#define PUT_LINE_NUMBER            '='
#define NEGATION                   '!'
#define LABEL_MARK                 ':'
#define COMMENT_MARK               '#'
#define BEGIN_SUBGROUP             '{'
#define END_SUBGROUP               '}'

#define BACKSLASH                  '\\'

struct cmd_table_entry {
                          int cmd_tag;
                          int max_addr;
                       };


struct addr  {
                int type;
                union     {
                            char *pch;
                            long  lineno;
                          }val;
             };
#define NO_ADDRESS 0
#define LINENO_ADDRESS 1
#define CONTEXT_ADDRESS 2
/* @12a */
#define LASTLINE_ADDRESS 3

struct subcmd {
                 int cmd_idx;
                 int status;
                 struct addr saddr[2];
                 char *args[3];
              };

#define READY  0
#define IN_RANGE 1

/* flags for store_to_from() */
#define CREP   1
#define NOCREP 0


struct wf    {
                char *filename;
                FILE *stream;
             };
#define MAXWF 10                       /* can't have more than this many */
                                       /* files open for writing         */
/* -------------------------Function Prototypes---------------------------- */
   /* the following functins' source is in sed.c       */
int parse_cmd_line (int argc, char ** argv, struct subcmd **cmdsp, int *numscp);
int process_script_file(char *filename, struct subcmd **cmdsp, int *numscp);
int add_subcmd(char *command_string, struct subcmd **cmdsp, int *numsc);
int open_files(struct subcmd **cmdsp,     int argc, char **argv);
int process_file_spec(char *FileSpec, struct subcmd **cmdsp);
int process_file(char *FileName, struct subcmd **cmdsp);

   /* the following functions' source is in sedparse.c */
extern int find_command_idx(char c);
extern char *skipwhite(char *start);
extern int isblankline(char *line);
extern char *store_to_from(char *start, char **where, int flag);
extern char *store_flags(char *start, char **where);
extern char * store_text(char *start, char **where);
extern char *store_next_string(char *start, char **where);
extern char *get_lineno_address(char *line, struct subcmd *scp, int *addr);
extern char *get_context_address(char *line, struct subcmd *scp, int *addr);

   /* the following functions' source is in sedapply.c */
extern char *applycmds(struct subcmd **scpp, char *line, FILE *f, long *lnp);
int select_line(struct subcmd *scp, char *line, int in_subgroup, FILE *f, long lineno); /* @12c */
struct subcmd **find_branch_address(struct subcmd **scpp,
                                    struct subcmd **topcmdp,
                                    int *ingrpflgp);
void dump_with_expansion(char *pattern_space);
int replace_chars(struct subcmd *scp, char *line);
int change(struct subcmd *scp, char **line);
struct subcmd **find_subgroup_end(struct subcmd **scpp);
char getesctrx(char c);

   /* the table cmd_table has source in sedtable.c   */
extern struct cmd_table_entry cmd_table[];

extern void myerror(int, char *, char *);
void xtoa(char *where, char *p);

char *private_fgets(char *string, int n, FILE *stream); /* @12a */
/* @12a */
#define fgets private_fgets
