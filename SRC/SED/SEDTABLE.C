static char sccsid[]="@(#)84	1.1  src/sed/sedtable.c, aixlike.src, aixlike3  9/27/95  15:45:56";
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

/* sedtable.c  --  tables for use by the various sed modules */

#include <stdio.h>
#include "sed.h"


struct cmd_table_entry cmd_table[] = {
                                      {PUT_TEXT                  , 1},
                                      {UNCONDITIONAL_BRANCH      , 2},
                                      {REPLACE                   , 2},
                                      {DELETE                    , 2},
                                      {DELETE_FIRST_LINE         , 2},
                                      {REPLACE_WITH_HOLD_DATA    , 2},
                                      {APPEND_WITH_HOLD_DATA     , 2},
                                      {REPLACE_HOLD_DATA         , 2},
                                      {APPEND_TO_HOLD_DATA       , 2},
                                      {IPUT_TEXT                 , 1},
                                      {WRITE_WITH_HEX            , 2},
                                      {WRITE_AND_REFRESH         , 2},
                                      {APPEND_TO_PATTERN         , 2},
                                      {WRITE_TO_STDOUT           , 2},
                                      {WRITE_FIRST_LINE_TO_STDOUT, 2},
                                      {BRANCH_TO_END             , 1},
                                      {PUT_TEXT_FROM_FILE        , 2},
                                      {CHANGE                    , 2},
                                      {CONDITIONAL_BRANCH        , 2},
                                      {APPEND_PATTERN_TO_FILE    , 2},
                                      {EXCHANGE_WITH_HOLD_DATA   , 2},
                                      {CHARACTER_REPLACE         , 2},
                                      {PUT_LINE_NUMBER           , 1},
                                      {NEGATION                  , 2},
                                      {LABEL_MARK                , 0},
                                      {COMMENT_MARK              , 1},
                                      {BEGIN_SUBGROUP            , 2},
                                      {END_SUBGROUP              , 1},
                                      {0                         , 0}
                                     };
#ifdef DEBUG
char *cmdnames[] = {
                    "PUT_TEXT",
                    "UNCONDITIONAL_BRANCH",
                    "REPLACE",
                    "DELETE",
                    "DELETE_FIRST_LINE",
                    "REPLACE_WITH_HOLD_DATA",
                    "APPEND_WITH_HOLD_DATA",
                    "REPLACE_HOLD_DATA",
                    "APPEND_TO_HOLD_DATA",
                    "IPUT_TEXT",
                    "WRITE_WITH_HEX",
                    "WRITE_AND_REFRESH",
                    "APPEND_TO_PATTERN",
                    "WRITE_TO_STDOUT",
                    "WRITE_FIRST_LINE_TO_STDOUT",
                    "BRANCH_TO_END",
                    "PUT_TEXT_FROM_FILE",
                    "CHANGE",
                    "CONDITIONAL_BRANCH",
                    "APPEND_PATTERN_TO_FILE",
                    "EXCHANGE_WITH_HOLD_DATA",
                    "CHARACTER_REPLACE",
                    "PUT_LINE_NUMBER",
                    "NEGATION",
                    "LABEL_MARK",
                    "COMMENT_MARK",
                    "BEGIN_SUBGROUP",
                    "END_SUBGROUP",
                     NULL
                 };
#endif
