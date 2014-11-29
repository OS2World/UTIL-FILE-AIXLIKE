static char sccsid[]="@(#)77	1.1  src/sed/pr_fgets.c, aixlike.src, aixlike3  9/27/95  15:45:41";
/************************************************************************/
/*                                                                      */
/*  Private fgets() for sed.                                            */
/*                                                                      */
/*  This one reads the EOF character!                                   */
/*                                                                      */
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *private_fgets (char *string, int n, FILE *stream) {

	char *p;
	int ch;

	p = string;

	if(feof(stream) || ferror(stream) || (n < 1))
		return(NULL);

	while(n-- > 0) {
		ch = getc(stream);
		*p++ = ch;
		if(ch == '\n') {
			ch = getc(stream);
			if(ch != EOF) {
				ungetc(ch, stream);
			}
			else if (ferror(stream)) {
				return(NULL);
			}
			break;
		}
		else if (ch == EOF) {
			if(ferror(stream)) {
				return(NULL);
			}
			else {
				break;
			}
		}
	}

	*p = '\0';

	return(string);
}
