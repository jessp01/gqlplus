/*
   Module name: gqlplus.c
   Created by: Ljubomir J. Buturovic
Created: May 18, 2002
Maintained by: Jess Portnoy
Last modification date: Nov 22 2011 
Purpose: drop-in replacement for sqlplus with command line
editing/history and table/column name completion.

Copyright (C) 2004 Ljubomir J. Buturovic. All Rights Reserved.
*/

/*
   This file implements a command-line program 'gqlplus', a drop-in
   replacement for Oracle SQL utility sqlplus. The intention is to
   provide as much as possible of the sqlplus functionality, with the
   addition of command-line editing, history and table/column name
   completion, similar to tcsh and bash shells.

   To achieve this, the program uses GNU Readline Library.

   The program is licensed under the GNU General Public License.

   To compile:

   % cc -D_REENTRANT gqlplus.c -o gqlplus -lreadline -lcurses 

   Readline must be installed on your system.
   */

/*
 * Modification Log:
 * $Log: gqlplus.c,v $
 * Revision 1.17  2014/08/08 16:43:23  jessrpm
 * patch by Philip Marsh <marsh@philipmarsh.org> for MAC OSX.
 *
 * 1) Status Fatal: tmpdir is far too small for OSX which uses 80+? for a
 * mkstemp filename. I have arbitrarily increased it to 500.
 *
 * 2) Status Fatal: Using mkstemp and fdopen to handle a file is
 * inappropiate and unnecessary.
 *
 * 3) Status Idiosyncratic: The use of a second pointer (tmpname) to
 * replace tmpdir is unnecessary and a little confusing(at the time).
 *
 * 4) Status Fatal: OSX requires double quotes around the connect string,
 * similar to your define for GET_PROMPT
 *
 * Thank you, Philip.
 *
 * Revision 1.16  2012/02/12 12:28:47  jessrpm
 * We now print the location of the history file, if such was created.
 *
 * Revision 1.15  2012/01/31 09:24:00  jessrpm
 * We now print the location of the history file, if such was created.
 *
 * Revision 1.14  2012/01/23 19:14:47  jessrpm
 * Fixes segfault made out pure distraction and silliness.
 *
 * Revision 1.13  2012/01/23 09:10:57  jessrpm
 * Commit revision 1.12 triggered by a patch suggestion from Stefan Kuttler.
 * Credit where credit is due.
 *
 * Revision 1.12  2012/01/23 09:02:12  jessrpm
 * Use mkstemp() instead tmpnam().
 * This code also honors TMPDIR, TEMPDIR and TEMP ENV vars [in that order], otherwise, defaults to /tmp.
 *
 * Revision 1.10  2011/12/18 09:15:05  jessrpm
 * See change log for info.
 *
 * Revision 1.192	2011/11/20	12:13:14	jess
 * - Added an ENV var called SQLPLUS_BIN. 
 * This is to support Oracle instantclient installations on 64bit 
 * that do not call the binary sqlplus but rather sqlplus64.
 * - Added mkinstalldirs script to the source
 *
 * $Log: gqlplus.c,v $
 * Revision 1.17  2014/08/08 16:43:23  jessrpm
 * patch by Philip Marsh <marsh@philipmarsh.org> for MAC OSX.
 *
 * 1) Status Fatal: tmpdir is far too small for OSX which uses 80+? for a
 * mkstemp filename. I have arbitrarily increased it to 500.
 *
 * 2) Status Fatal: Using mkstemp and fdopen to handle a file is
 * inappropiate and unnecessary.
 *
 * 3) Status Idiosyncratic: The use of a second pointer (tmpname) to
 * replace tmpdir is unnecessary and a little confusing(at the time).
 *
 * 4) Status Fatal: OSX requires double quotes around the connect string,
 * similar to your define for GET_PROMPT
 *
 * Thank you, Philip.
 *
 * Revision 1.16  2012/02/12 12:28:47  jessrpm
 * We now print the location of the history file, if such was created.
 *
 * Revision 1.15  2012/01/31 09:24:00  jessrpm
 * We now print the location of the history file, if such was created.
 *
 * Revision 1.14  2012/01/23 19:14:47  jessrpm
 * Fixes segfault made out pure distraction and silliness.
 *
 * Revision 1.13  2012/01/23 09:10:57  jessrpm
 * Commit revision 1.12 triggered by a patch suggestion from Stefan Kuttler.
 * Credit where credit is due.
 *
 * Revision 1.12  2012/01/23 09:02:12  jessrpm
 * Use mkstemp() instead tmpnam().
 * This code also honors TMPDIR, TEMPDIR and TEMP ENV vars [in that order], otherwise, defaults to /tmp.
 *
 * Revision 1.10  2011/12/18 09:15:05  jessrpm
 * See change log for info.
 *
 * Revision 1.191  2006/12/02 08:34:42  ljubomir
 * Use EDITOR environment variable to define the editor.
 *
 * Revision 1.190  2006/10/04 04:16:29  ljubomir
 * Add ORACLE_PATH (contributed by Grant Hollingworth).
 *
 * Revision 1.189  2006/08/06 01:49:34  ljubomir
 * Reorder command-line options in the Usage message.
 *
 * Revision 1.188  2006/08/01 06:57:36  ljubomir
 * Kill sqlplus using SIGTERM.
 *
 * Revision 1.187  2006/07/20 05:39:29  ljubomir
 * *** empty log message ***
 *
 * Revision 1.186  2006/07/17 01:23:16  ljubomir
 * Introduce const_correctness.
 *
 * Revision 1.185  2006/07/16 07:53:17  ljubomir
 * Update usage() for -p switch.
 *
 * Revision 1.184  2006/07/16 07:50:21  ljubomir
 * Optionally support progress report and elapsed time information
 * (contributed by Mark Harrison of Pixar).
 *
 * Revision 1.183  2006/07/15 00:03:38  ljubomir
 * Add environment variable NLS_DATE_FORMAT. Contributed by Yen-Ming Lee.
 *
 * Revision 1.182  2006/03/28 20:10:13  ljubomir
 * Define the debugging function print_environment().
 *
 * Revision 1.181  2006/03/27 18:56:59  ljubomir
 * Add DYLD_LIBRARY_PATH environment variable (the Mac OSX version of LD_LIBRARY_PATH).
 *
 * Revision 1.180  2006/01/30 06:45:22  ljubomir
 * Handle, to some extent, the prompt-less ACCEPT command.
 *
 * Revision 1.179  2006/01/30 06:24:12  ljubomir
 * Add a sleep(1) in accept_sql(), to guarantee entire sqlplus message
 * arrives in a single read().
 *
 * Revision 1.178  2006/01/29 08:41:31  ljubomir
 * Attempt to handle sqlplus errors in accept_cmd().
 *
 * Revision 1.177  2006/01/29 01:54:09  ljubomir
 * Finally correctly implement ACCEPT command (however with prompt).
 *
 * Revision 1.176  2006/01/27 08:15:39  ljubomir
 * Move towards implementing the ACCEPT command.
 *
 * Revision 1.175  2006/01/25 05:56:35  ljubomir
 * 1.12
 *
 * Revision 1.174  2006/01/25 05:39:12  ljubomir
 * Fix terrible memory violation in file_set_editor().
 *
 * Revision 1.173  2006/01/22 06:52:21  ljubomir
 * Correct rescanning after CONNECT command.
 *
 * Revision 1.172  2006/01/22 04:22:48  ljubomir
 * Move installation of completion until after the CONNECT command has been sent
 * to sqlplus (that's the only way it makes sense).
*
* Revision 1.171  2006/01/22 03:48:47  ljubomir
* Completion after reconnect doesn't work... strange error SP2-0029.
*
* Revision 1.170  2006/01/22 03:22:47  ljubomir
* Reinstall completion names after each connect.
*
* Revision 1.169  2006/01/22 03:04:52  ljubomir
* Remove all references to the useless SIGCHLD handling... it does not solve the abnormal sqlplus
* termination problem, because it cannot distinguish between abnormal and normal termination.
* Instead, we exit if write() to sqlplus fails.
*
* Revision 1.168  2006/01/22 03:02:08  ljubomir
* Solve the sqlplus termination problem...
*
* Revision 1.167  2006/01/22 02:53:29  ljubomir
* Prepare to handle Broken pipe (sqlplus terminated).
*
* Revision 1.166  2006/01/22 01:51:11  ljubomir
* Close the reading end of parent-to-child pipe in the parent.
*
* Revision 1.165  2006/01/21 19:46:16  ljubomir
* Terminate futile efforts to handle SIGCHLD.
*
* Revision 1.164  2006/01/21 19:14:00  ljubomir
* More attempts at SIGCHLD handling. Still disabled, though.
*
* Revision 1.163  2006/01/21 15:27:59  ljubomir
* Add comment.
*
* Revision 1.162  2006/01/21 15:21:56  ljubomir
* Use kill_sqlplus().
*
* Revision 1.161  2006/01/21 15:02:53  ljubomir
* Prepare to handle SIGCHLD.
*
* Revision 1.160  2006/01/20 07:41:41  ljubomir
* Prepare to handle SIGCHLD.
*
* Revision 1.159  2006/01/20 07:15:28  ljubomir
* *** empty log message ***
*
* Revision 1.158  2005/12/08 01:03:29  ljubomir
* Initialize `pstat' in main(). Caused /nolog to fail.
*
* Revision 1.157  2005/07/01 05:51:36  ljubomir
* Properly check mode searching for an executable.
*
* Revision 1.156  2005/07/01 04:55:01  ljubomir
* Add logging code. Initialize variables in get_completion_names().
*
* Revision 1.155  2005/06/25 21:02:54  ljubomir
* Disable tablename completion in we are unable to query or parse ALL_TABLES/ALL_VIEWS.
*
* Revision 1.154  2005/06/25 20:40:03  ljubomir
* Robustify get_names() and get_column_names().
*
* Revision 1.153  2005/06/24 05:30:35  ljubomir
* Set NLS_COMP and NLS_SORT environment variables.
* Don't crash in get_names() if table owner is not defined (not sure
    * if this can happen, but just in case).
*
* Revision 1.152  2005/06/23 01:56:47  ljubomir
* Version 1.11.
*
* Revision 1.151  2005/06/23 01:54:30  ljubomir
* Optionally print sqlplus binary.
*
* Revision 1.150  2005/05/22 08:09:57  ljubomir
* Use sfree() to simplify code.
*
* Revision 1.149  2005/05/22 05:31:28  ljubomir
* Finish support for user-defined prompt combined with time.
*
* Revision 1.148  2005/05/21 22:18:03  ljubomir
* Correctly handle `set time on' in the presence of user-defined SQL prompt.
*
* Revision 1.147  2005/05/21 19:48:40  ljubomir
* Implement and comment the new rules for finding sqlplus executable: ${PATH},
  * followed by ${ORACLE_HOME}/bin, followed by ./sqlplus.
  *
  * Revision 1.146  2005/05/21 07:46:46  ljubomir
  * Prevent various crashes. Do not continue in get_sql_prompt() if system() fails.
  *
  * Revision 1.145  2005/05/21 05:25:31  ljubomir
  * Fix get_connect_string() in case password has the net service name.
  *
  * Revision 1.144  2005/05/20 03:22:39  ljubomir
  * Remove strtok_r().
  *
  * Revision 1.143  2005/05/19 07:12:39  ljubomir
  * Remove libgen.h - not needed?
  *
  * Revision 1.142  2005/05/19 06:20:01  ljubomir
  * Correct behavior in case of /nolog command-line argument.
  *
  * Revision 1.141  2005/05/16 01:13:13  ljubomir
  * Remove unused variables.
  *
  * Revision 1.140  2005/05/16 01:11:24  ljubomir
  * Intermediate revision. Support Oracle instantclient - no longer require ORACLE_HOME.
  *
  * Revision 1.139  2005/05/11 01:31:29  ljubomir
  * Fix endless loop in file_set_editor() (contributed by Jeff Mital).
  *
  * Revision 1.138  2005/05/11 00:46:11  ljubomir
  * Version 1.10.
  *
  * Revision 1.137  2004/06/29 06:24:29  ljubomir
  * Increase usleep() period in get_final_sqlplus().
  *
  * Revision 1.136  2004/06/29 06:02:34  ljubomir
  * Add Copyright statement. At exit, print the last remaining output from sqlplus.
  *
  * Revision 1.135  2004/06/29 02:11:01  ljubomir
  * Do not hang if QUIT command is issued in disconnected state.
  *
  * Revision 1.134  2004/06/23 05:34:03  ljubomir
  * Do not crash looking for glogin.sql.
  *
  * Revision 1.133  2004/06/21 07:53:34  ljubomir
  * Increase space for path.
  *
  * Revision 1.132  2004/06/21 00:19:57  ljubomir
  * Use DISPLAY environment variable for Emacs editor, do not hardcode the display.
  *
  * Revision 1.131  2004/06/20 23:40:07  ljubomir
  * Finally implement the correct rules for extracting editor informatio from
  * glogin.sql/login.sql files. Also, open Emacs in a separate window.
  *
  * Revision 1.130  2004/06/17 16:52:22  ljubomir
  * Fix crash when SET command given without arguments.
  *
  * Revision 1.129  2004/06/17 15:32:19  ljubomir
  * Do not crash in PAUSE command if the prompt is the default.
  *
  * Revision 1.128  2004/06/17 15:02:53  ljubomir
  * Support ACCEPT command.
  *
  * Revision 1.127  2004/06/17 03:05:34  ljubomir
  * Correctly handle numeric prompt when SET TIME ON is in effect.
  * Reported by Josh Trutwin.
  *
  * Revision 1.126  2004/06/16 17:16:01  ljubomir
  * Add handling of RECOVER_PROMPT, contributed by Eniac Zhang.
  *
  * Revision 1.125  2004/05/20 06:02:40  ljubomir
  * When looking for _define for editor, search local login.sql followed by
  * ${HOME}/login.sql.
  *
  * Revision 1.124  2004/05/19 08:55:02  ljubomir
  * Fix parsing of login.sql.
  *
  * Revision 1.123  2003/11/05 19:19:45  ljubomir
  * Add the TWO_TASK environment variable. Contributed by
  * Mladen Gogala.
  *
  * Revision 1.122  2003/10/29 06:28:46  ljubomir
  * Correctly get table names (for completion) if pagesize is set to 0.
  *
  * Revision 1.121  2003/10/23 19:03:26  ljubomir
  * Adjust to properly handle read() semantics on Linux, in pause_cmd().
  *
  * Revision 1.120  2003/10/23  18:15:22  ljubomir
  * Fix handling of CONNECT/DISCONNECT commands.
  *
  * Revision 1.119  2003/10/22 23:16:32  ljubomir
  * VERSION 1.8.
  *
  * Revision 1.118  2003/10/22 21:52:07  ljubomir
  * Finish - hopefully - implementing PAUSE, SET PAUSE.
  *
  * Revision 1.117  2003/10/22 21:39:08  ljubomir
  * Better implementation of PAUSE and SET PAUSE.
  *
  * Revision 1.116  2003/10/22 21:03:56  ljubomir
  * Attempt to implement the 'set pause on' command. Not very good yet.
  *
  * Revision 1.115  2003/10/20 23:22:13  ljubomir
  * Clean-up warning according to Jos Backus suggestions.
  *
  * Revision 1.114  2003/10/20 22:44:08  ljubomir
  * Beautify 'host' command - add the trailing newline.
  *
  * Revision 1.113  2003/10/19 00:43:40  ljubomir
  * Bug fix: do not lowercase arguments to HOST command.
  *
  * Revision 1.112  2003/10/19 00:35:24  ljubomir
  * Fix bug: shell launched whenever string 'ho' appears within a multi-line sqlplus command.
  *
  * Revision 1.111  2003/10/19 00:18:49  ljubomir
  * Support filename argument for 'edit' command.
  *
  * Revision 1.110  2003/10/18 23:47:31  ljubomir
  * Pause command actually takes three characters to be distinguished.
  *
  * Revision 1.109  2003/10/18 23:46:15  ljubomir
  * Finish the 'pause' command support.
  *
  * Revision 1.108  2003/10/18 19:54:49  ljubomir
  * First attempt to support PAUSE command. Imperfect, but at least it doesn't hang.
  *
  * Revision 1.107  2003/10/17 16:29:55  ljubomir
  * Attempt to fix the double-Ctrl-C problem - fix doesn't work, disabled for now.
  *
  * Revision 1.106  2003/05/06 17:08:02  ljubomir
  * Incorporate bug fix from Steven Kehlet:  gqlplus crashes if invoked like gqlplus "/ as sysdba"
  * and the database is closed.
  *
  * Revision 1.105  2003/03/25 21:49:42  ljubomir
  * Final beautification, usage().
  *
  * Revision 1.104  2003/03/25 17:58:35  ljubomir
  * Improve column name handling. Beautify usage().
  *
  * Revision 1.103  2003/03/25 17:06:08  ljubomir
  * Set history to 200, beautify usage() message.
  *
  * Revision 1.102  2003/03/24 14:33:29  ljubomir
  * Make sure USAGE_PROMPT is more specific.
  *
  * Revision 1.101  2003/03/24  01:02:57  ljubomir
  * Report gqlplus version.
  *
  * Revision 1.100  2003/03/24 00:38:00  ljubomir
  * Initialize history.
  *
  * Revision 1.99  2003/03/24 00:21:07  ljubomir
  * Make sure passwords do not end up in history.
  *
  * Revision 1.98  2003/03/24 00:12:27  ljubomir
  * Beautify usage(); re-enable echo after password command.
  *
  * Revision 1.97  2003/03/24 00:01:21  ljubomir
  * Complete 'password' command handling.
  *
  * Revision 1.96  2003/03/23 23:57:42  ljubomir
  * Add 'password' command handling.
  *
  * Revision 1.95  2003/03/23 22:56:06  ljubomir
  * Add gqlplus-specific usage message.
  *
  * Revision 1.94  2003/03/23 06:37:17  ljubomir
  * Synchronize with CVS version.
  *
  * Revision 1.6  2003/02/23 23:16:01  ljubomir
  * Untabify.
  *
  * Revision 1.5  2003/02/22 22:30:40  zhewu
  * change comment style from java/c++
  *
  * Revision 1.4  2003/02/22 22:23:02  zhewu
  *   one more change for completion when first connected.
  *
  * Revision 1.3  2003/02/22 17:37:32  zhewu
  * Add fix for haning problem when use gqlplus inside ADE version control
  * system.
  * Add command history handling and rebuild table/view name completion
  * after connect to a different schema.
  *
  */
  static char rcsid[] = "$Id: gqlplus.c,v 1.17 2014/08/08 16:43:23 jessrpm Exp $";

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

#if !defined(MAXPATHLEN)
#  define MAXPATHLEN 512
#endif

#define VERSION          "1.15"
#define INIT_LENGTH      10
#define MAX_LINE_LENGTH  100000
#define INIT_LINE_LENGTH 100
#define BUF_LEN          1000
#define MAX_PROMPT_LEN   100000 /* hard to believe anyone would want a longer prompt */
#define NPROMPT          1
#define INIT_NUM_TABLES  50
#define INIT_NUM_COLUMNS 50
#define MAX_NARGS        100
#define MAX_NENV         50
#define SQL_PROMPT       "SQL> "
#define USER_PROMPT      "Enter user-name: "
#define PASSWORD_PROMPT  "Enter password: "
#define OLD_PASSWORD     "Old password: "
#define NEW_PASSWORD     "New password: "
#define RETYPE_PASSWORD  "Retype new password: "
#define QUIT_PROMPT_1    "Disconnected from Oracle"
#define QUIT_PROMPT_2    "unable to CONNECT to ORACLE"
#define USAGE_PROMPT     "Usage: SQLPLUS"
#define VALUE_PROMPT     "Enter value for "
#define RECOVER_PROMPT   "Specify log: {<RET>=suggested | filename | AUTO | CANCEL}"
#define SQLPROMPT        "sqlprompt"
#define SQLEXT           ".sql"
#define LIST_CMD         "list\n"
#define DEL_CMD          "del 1 LAST\n"
#define EDIT_CMD         "ed"
#define CLEAR_CMD        "cl"
#define SCREEN           "scr"
#define WHITESPACE       " \t"
#define DIGITS           "0123456789"
#define HOST_CMD         "ho"
#define DEFINE_CMD       "define"
#define SET_CMD          "set"
#define SQLPROMPT_CMD    "sqlprompt"
#define PAUSE_CMD        "pause"
#define SELECT_CMD       "select"
#define CONNECT_CMD      "connect"
#define DISCONNECT_CMD   "disconnect"
#define QUIT_CMD         "quit"
#define ON_CMD           "on"
#define PAGESIZE_CMD     "show pagesize\n"
#define ACCEPT_CMD       "accept"
#define GET_PROMPT       "echo \"show sqlprompt;\" | "
#define TAIL_PROMPT      ""
#define SELECT_TABLES_1  "select distinct table_name, owner from all_tables where owner != 'SYS' union "
#define SELECT_TABLES_2  "select distinct view_name, owner from all_views where owner != 'SYS';\n"
#define DESCRIBE         "describe"
#define VI_EDITOR        "/bin/vi"
#define EDITOR           "_editor"
#define AFIEDT           "afiedt.buf"
#define NO_LINES         "No lines in SQL buffer.\n"
#define NOTHING_TO_SAVE  "Nothing to save.\n"
#define AFIEDT_ERRMSG    "Cannot create save file \"afiedt.buf\"\n"
#define ORACLE_HOME      "ORACLE_HOME"
#define ORACLE_SID       "ORACLE_SID"
#define SQLPATH          "SQLPATH"
#define TNS_ADMIN        "TNS_ADMIN"
#define TWO_TASK         "TWO_TASK"  /* Added by M. Gogala, 11/5/2003 */
#define NLS_LANG         "NLS_LANG" /* Added by dbakorea */
#define ORA_NLS33        "ORA_NLS33" /* Added by dbakorea */
#define NLS_DATE_FORMAT  "NLS_DATE_FORMAT" /* Added by leeym */
#define NLS_COMP         "NLS_COMP" 
#define NLS_SORT         "NLS_SORT" 
#define LD_LIBRARY_PATH  "LD_LIBRARY_PATH" 
#define DYLD_LIBRARY_PATH  "DYLD_LIBRARY_PATH" 
#define ORACLE_PATH      "ORACLE_PATH" /* contributed by Grant Hollingworth */
#define PATH             "PATH"
#define TERM             "TERM"
#define SPACETAB         " \t"
#define STARTUP          1
#define CONNECTED        2
#define DISCONNECTED     3

  static  int    fds1[2];
  static  int    fds2[2];
  static  int    pipe_size;
  static  int    state;
  static  int    progress; /* display progress report/elapsed time information */
  /* functionality contributed by Mark Harrison of Pixar */
  static  char   *_editor;
  static  pid_t  sqlplus_pid;
  static  pid_t  edit_pid;
  static  int    quit_sqlplus = 0;
  static  int    complete_columns = 1; /* if 0, don't do column name completion */
  struct  sigaction iact;
  struct  sigaction qact;
  struct  sigaction cact;
  struct  sigaction pact;
  static  FILE   *lptr;
  static  char   *sql_prompt = (char *) 0; /* user-defined prompt */
  static  char   *username = (char *) 0;


  static char* szCmdPrefix = "--!";
  static char *histname;
  static int histmax = 200;

void save_history(void)
{
  if (history_list() == NULL)
    /* If history is of length 0 (because set as is or slave is not executable) */
    unlink(histname);
  else {
    write_history(histname);
    chmod(histname, 0600);
    printf("\nSession history saved to: %s\n", histname);
  }
}

void initialize_history(char *appl_name)
{
  char buffer[MAXPATHLEN];

  /* Find the real name of the history file using the tilde_expand function 
   * which is in the readline library.
   */
  sprintf(buffer, "~/.%s_history", appl_name);
  histname = tilde_expand(buffer);

  using_history();  
  if (histmax >= 0) {
    stifle_history(histmax);
    atexit(save_history);
    read_history(histname);
  } else {
    max_input_history = -histmax;
  }
}

/*
   Remove leading and trailing spaces from str.
   */
static char *trim(char *str)
{
  int  len;
  char *xtr = (char *) 0;

  if (str){
    len = strlen(str)-1;
    while (str[len] == ' '){
      str[len--] = '\0';
    }
    xtr = strdup(str+strspn(str, " "));
  }
  return xtr;
}


/* 
   Returns 1 if a string szOrg starts with szPrefix.
   Note that leading blanks/tabs are ignored.

   Returns 0 otherwise.

   @param szOrg can be NULL.
   @param szPrefix can be NULL.
   */
int startsWith(char* szOrg, const char* szPrefix)
{

  int iMatch = 0;
  if ((szOrg  == (char*) 0) || (szPrefix == (char*) 0)) {
    /* do nothing. */
  }else {
    char* szOrgTrimmed = trim(szOrg);
    if (strstr(szOrgTrimmed, szPrefix) == szOrgTrimmed){
      iMatch = 1;
    }
  }
  return iMatch;
}


/*
   Return 1 if the following conditions satisfy:
   a) current line startsWith szPrefix AND
   b) the remaining part after szPrefix in szOrg starts with szCmd

   Return 0 otherwise.
   */
int matchCommand(char* szOrg, char* szPrefix, char* szCmd)
{
  int iMatch = 0;

  if ((szOrg    == (char*) 0) || (szPrefix == (char*) 0) || (szCmd    == (char*) 0)) {
    /* do nothing. */
  }else {
    char* szOrgTrimmed = trim(szOrg);
    if (strstr(szOrgTrimmed, szPrefix) == szOrgTrimmed) {
      char* szRemaining = szOrgTrimmed + strlen(szPrefix);
      iMatch = startsWith(szRemaining, szCmd);
    }
  }

  return iMatch;
}


/*
   Return lowercase and trimmed version of 'str'.
   */
char *tl(const char *str)
{
  int  i;
  char *xtr = (char *) 0;
  char *lline = (char *) 0;

  if (str){
    lline = strdup(str);
    for (i = 0; i < strlen(str); i++){
      lline[i] = tolower((int) lline[i]);
    }
  }
  xtr = trim(lline);
  free(lline);
  return xtr;
}

/*
   Set 'str' to lowercase.
   */
static void tl2(char *str)
{
  if (!str){
    return;
  }
  int  i = 0;
  while (str[i] != '\0'){
    str[i] = tolower((int) str[i]);
    i++;
  }
}

/*
   Utility function for check_numeric_prompt().
   */

static int check_numeric(const char *str)
{
  int check_prompt = 0;

  if (strlen(str) == 5)
    if (isspace((int) str[4]))
      if (isspace((int) str[3]) || (str[3] == '*') || (str[3] == 'i'))
        if (isdigit((int) str[2]))
        {
          check_prompt = 1;
          if (isdigit((int) str[0]) && isspace((int) str[1]))
            check_prompt = 0;
        }
  return check_prompt;
}

/*
   Return 1 if 'str' is a correct Oracle numeric prompt. The numeric
   prompt has up to three numeric characters, followed by a space, or
   an asterisk, or an 'i' (for Insert command).

   ljb, 06/16/2004: there is another version of a numeric prompt, when
   'time' is set. The prompt looks like this:

   SQL> set time on
19:41:37 SQL> select * from scott.salgrade
19:41:42   2  ;
*/
static int check_numeric_prompt(const char *str)
{
  if (!str){
     return 0;
  }
  int check_prompt = 0;
    /*
       Check for the short version of the numeric prompt:
       */
    if (strlen(str) == 5){
      check_prompt = check_numeric(str);
    /*
       Check for the long version of the numeric prompt (when time is set):
       */
    }else if (strlen(str) == 14){
      if (isdigit((int) str[0]) && isdigit((int) str[1]) &&
          str[2] == ':' && isdigit((int) str[3]) && isdigit((int) str[4]) &&
          str[5] == ':' && isdigit((int) str[6]) && isdigit((int) str[7]) &&
          isspace((int) str[8]))
        check_prompt = check_numeric(str+9);
    }
    return check_prompt;
}

/*
   Return 1 if 'str' is a correct Oracle time prompt. The time prompt
   has time followed by a space followed by `sqlprompt', like this:

19:41:37 SQL>
*/
static int check_time_prompt(char *str, char *sqlprompt)
{
  if (!str){
    return 0;
  }
  int check_prompt = 0;
  if (isdigit((int) str[0]) && isdigit((int) str[1]) &&
      str[2] == ':' && isdigit((int) str[3]) && isdigit((int) str[4]) &&
      str[5] == ':' && isdigit((int) str[6]) && isdigit((int) str[7]) &&
      isspace((int) str[8]) && !strcmp(&str[9], sqlprompt)){
        check_prompt = 1;
  }
  return check_prompt;
}

void ignore_sigint()
{
  sigemptyset(&iact.sa_mask);
  iact.sa_flags = 0;
#ifdef SA_RESTART
  iact.sa_flags |= SA_RESTART;
#endif
  iact.sa_handler = SIG_IGN;
  sigaction(SIGINT, &iact, (struct sigaction *) 0);
}

static void ignore_sigpipe()
{
  sigemptyset(&pact.sa_mask);
  pact.sa_flags = 0;
#ifdef SA_RESTART
  pact.sa_flags |= SA_RESTART;
#endif
  pact.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &pact, (struct sigaction *) 0);
}

/*
   SIGINT handler. Passes the signal to the child (i.e., sqlplus)
   unless we are in the middle of an editing session (edit_pid !=
   0). The reason is this: Emacs redefines Ctrl-G to be
   SIGINT-generating key (instead of Ctrl-C). So, during Emacs editing
   session, Ctrl-G generates SIGINT which we don't want to pass to
   sqlplus because it disrupts its' operation and does not make sense
   anyway (that is not the interrupt we want to catch; we only care
   about Ctrl-C typed at gqlplus prompt).
   */
static void sigint_handler(int signo)
{
  if (edit_pid == 0)
  {
    kill(sqlplus_pid, SIGINT);
    //ignore_sigint();
  }
}

   //SIGQUIT handler.
static void sigquit_handler(int signo)
{
  kill(sqlplus_pid, signo);
  printf("Quit\n");
  _exit(0);
}

void install_sigint_handler()
{
  sigemptyset(&iact.sa_mask);
  iact.sa_flags = 0;
#ifdef SA_RESTART
  iact.sa_flags |= SA_RESTART;
#endif
     //Use sigaction(), for portability and reliability.
  iact.sa_handler = sigint_handler;
  sigaction(SIGINT, &iact, (struct sigaction *) 0);
}

void install_sigquit_handler()
{
  sigemptyset(&qact.sa_mask);
  qact.sa_flags = 0;
#ifdef SA_RESTART
  qact.sa_flags |= SA_RESTART;
#endif
     //Use sigaction(), for portability and reliability.
  qact.sa_handler = sigquit_handler;
  sigaction(SIGQUIT, &qact, (struct sigaction *) 0);
}

// Install signal handlers.
void sig_init(void)
{
  if (getenv("EMACS_MODE")){
    printf("Operating in Emacs mode.\nUnset EMACS_MODE if this is not desired.\n");
    install_sigint_handler();
  }
  install_sigquit_handler();
  ignore_sigpipe();
}

static void kill_sqlplus()
{
  kill(sqlplus_pid, SIGTERM);
}

//Return 1 if 'str' is one of the Oracle password prompts.
static int check_password_prompt(char *str)
{
  if (!str){
    return 0;
  }
  int check_prompt = 0;

    if (!strcmp(str, PASSWORD_PROMPT) ||!strcmp(str, OLD_PASSWORD) ||!strcmp(str, NEW_PASSWORD) ||!strcmp(str, RETYPE_PASSWORD)){
      check_prompt = 1;
    }
  return check_prompt;
}

/*
   Get sqlplus output from `fd' and display it (*outstr is NULL)
   without prompt, or store it in *outstr. The prompt is returned and
   will be displayed later, by readline() (if we display it here, it
   would get overwritten by readline()).
   */
static char *get_sqlplus(int fd, char *line, char **outstr)
{
  int  done;
  int  cntr;
  int  nread = 0;
  int  plen;
  int  llen;
  int  olen;
  int  capacity;
  int  ocapacity;
  int  pdiff;
  char *lline;
  char *xtr;
  char *last_line;
  char *otr = (char *) 0;
  char *ptr = (char *) 0;
  char *prompt = (char *) 0;

  done = 0;
  cntr = 0;
  capacity = INIT_LINE_LENGTH;
  ocapacity = INIT_LINE_LENGTH;
     
  //lline has the current undisplayed sqlplus content. llen is the length of the content.
  
  lline = malloc((capacity+1)*sizeof(char)); 
  lline[0] = '\0';
  llen = 0;
  olen = 0;
  if (outstr != (char **) 0)
  {
    otr = malloc((ocapacity+1)*sizeof(char));
    ptr = otr;
  }
  /*
     Read sqlplus output. We read until sqlplus sends a prompt. We
     recognize the following prompts:

     'SQL> '
     'Enter user-name: '
     'Enter password: '
     'Disconnected from Oracle...' 
     'Enter value for ...' 
     'Specify log: {<RET>=suggested | filename | AUTO | CANCEL}'
     numeric prompt (for multi-line SQL statements)
     user-defined prompt (see function set_sql_prompt())

     So far looks like these are the only prompts. If sqlplus sends
     anything else, we are doomed. ljb, 05/20/2002
     */
  while (done == 0)
  {
    nread = read(fd, line, pipe_size);
    if (nread > 0)
    {
      line[nread] = '\0';
      fflush(stdout);
      if (nread+llen > capacity)
      {
        while (nread+llen > capacity)
          capacity += capacity;
        lline = realloc(lline, capacity+1);
      }
      if ((outstr != (char **) 0) && (nread+olen > ocapacity))
      {
        while (nread+olen > ocapacity)
          ocapacity += ocapacity;
        pdiff = ptr-otr;
        otr = realloc(otr, ocapacity+1);
        ptr = otr+pdiff;
      }
      memcpy(&lline[llen], line, nread);
      llen += nread;
      lline[llen] = '\0';
      if ((!strncmp(lline, QUIT_PROMPT_1, strlen(QUIT_PROMPT_1)) && (state == STARTUP)) || 
          (strstr(lline, QUIT_PROMPT_1) && (state == STARTUP)) || 
          strstr(lline, QUIT_PROMPT_2) || strstr(lline, USAGE_PROMPT))
      {
        done = 1;
        quit_sqlplus = 1;
      }

      /*
         The following change is to address in ADE view gqlplus 
         invocation problem.
         */
      if ((!strncmp(lline, VALUE_PROMPT, strlen(VALUE_PROMPT)))
          || strstr(lline, SQL_PROMPT)
          || strstr(lline, RECOVER_PROMPT)
         ) 
        done = 1;

      /*
         Display everything up to the last newline.
         */
      xtr = strrchr(lline, '\n');
      if (xtr)
      {
        last_line = strdup(xtr+1);
        plen = xtr-lline;
        if (!outstr)
        {
          if (plen > 0)
            write(STDOUT_FILENO, lline, plen);
        }
        else
        {
          memcpy(ptr, lline, plen);
          ptr[plen] = '\0';
          ptr += plen;
          olen += plen;
        }
        /*
           Adjust lline. After adjustment, only the last line
           received from sqlplus, so far, remains in lline.
           */
        llen -= plen;
        memcpy(lline, xtr, llen);
        lline[llen] = '\0';
        if ((sql_prompt && !strcmp(last_line, sql_prompt)) || 
            !strcmp(last_line, USER_PROMPT) ||
            !strncmp(last_line, VALUE_PROMPT, strlen(VALUE_PROMPT)) ||
            check_time_prompt(last_line, sql_prompt) ||
            check_password_prompt(last_line))
          done = 1;
        free(last_line);
      }
      else if ((sql_prompt && !strcmp(lline, sql_prompt)) || 
          check_numeric_prompt(lline) ||
          check_time_prompt(lline, sql_prompt) ||
          check_password_prompt(lline))
        done = 1;
    }
    else
    {
      if (nread < 0)
        perror((char *) 0);
    }
  }
  /*
     Display the remaining content, up to the last newline. Everything
     beyond that is the prompt.
     */
  xtr = strrchr(lline, '\n');
  if (xtr)
  {
    xtr++;
    prompt = strdup(xtr);
    llen -= strlen(prompt);
    if (!outstr)
    {
      if (llen > 0)
        write(STDOUT_FILENO, lline, llen);
    }
    else 
    {
      memcpy(ptr, lline, llen);
      ptr[llen] = '\0';
    }
  }else{
    prompt = strdup(lline);
  }
  free(lline);
  if (outstr != (char **) 0){
    *outstr = otr;
  }
  /*install_sigint_handler();*/
  return prompt;
}

/*
   A special version of get_sqlplus() supporting the 'set sqlprompt'
   command. This function is retrieving one line from sqlplus, and that
   line is the new sql_prompt.
   */
static char *set_sql_prompt(int fd, char *line)
{
  int  cntr;
  int  nread = 0;
  int  llen;
  char *lline;

  cntr = 0;
  /* 
     lline has the current undisplayed sqlplus content. llen is the
     length of the content.
     */
  lline = malloc((MAX_PROMPT_LEN+1)*sizeof(char)); 
  lline[0] = '\0';
  llen = 0;
  /*
     Get bytes from sqlplus. We gamble here that everything will come
     from sqlplus in one read(), since 'set sqlprompt' is a simple and
     quick operation. If it doesn't, we are doomed.
     */
  nread = read(fd, line, pipe_size);
  if (nread > 0){
    line[nread] = '\0';
    fflush(stdout);
    memcpy(&lline[llen], line, nread);
    llen += nread;
    lline[llen] = '\0';
    cntr++;
  }else{
    if (nread < 0){
      perror((char *) 0);
      free(lline);
      lline = (char *) 0;
    }
  }
  return lline;
}

/*
   Single line read from sqlplus. 
   */
static char *read_sqlplus(int fd, char *line)
{
  int  nread;
  char *rline;

  rline = (char *) 0;
  /*
  Get bytes from sqlplus. We gamble here that everything will come
  from sqlplus in one read(). If it doesn't, we are doomed.
   */
  nread = read(fd, line, pipe_size);
  if (nread > 0){
    line[nread] = '\0';
    rline = strdup(line);
  }else if (nread < 0){
    perror("read_sqlplus()");
  }
  return rline;
}

/*
   Special processing for the ACCEPT sqplus command `cmd'. The special
   processing is required because the command essentially redefines
   prompt while the user enters value of the variable defined in the
   ACCEPT command, thus we cannot use the standard gqlplus loop to read
   message from sqlplus.
   */
static char *accept_cmd(char *cmd, int fdin, int fdout, char *sql_prompt, char *line)
{
  int  status;
  int  len;
  char *accept;
  char *rline;
  char *prompt;
  char *ptr;
  char *fline;

  accept = (char *) 0;
  /*
     Is there a prompt? If there is, read sqlplus message (which will
     be either the ACCEPT prompt, or an error message).
     */
  if (strstr(cmd, " prompt "))
  {
    /* 
       Ensure that entire sqlplus message will arrive in a single
       read(). This is particularly important in case of error
       messages, which seem to arrive in two pieces.
       */
    sleep(1);
    /*
       Read the sqlplus message in response to the ACCEPT
       command. Typically, that would be `prompt' parameter of the ACCEPT
       command.
       */
    accept = read_sqlplus(fdin, line);
  }
  /*
     If this message is terminated with `sql_prompt', that means that
     sqlplus actually returned error message. If that is the case,
     print the first line, and return.
     */
  if (accept && (ptr = strchr(accept, '\n'))){
    len = ptr-accept+1;
    fline = malloc((len+1)*sizeof(char));
    memcpy(fline, accept, len);
    fline[len] = '\0';
    printf("%s", fline);
    fflush(stdout);
    free(fline);
    prompt = strdup(sql_prompt);
  }else{
    /*
       Now place user input in `rline' using readline().
       */
    rline = readline(accept);
    /*
       Send it to sqlplus.
       */
    write(fdout, rline, strlen(rline));
    /*
       Terminate the user input sent to sqlplus.
       */
    status = write(fdout, "\n", 1);
    if (status == -1){
      fprintf(stderr, "sqlplus terminated - exiting...\n");
      exit(-1);
    }
    /*
       Now read the message sent from sqlplus, which should be the SQL
       prompt. We don't display it, since that will be done by the
       subsequent readline() call.
       */
    prompt = read_sqlplus(fdin, line);
  }
  return prompt;
}

/*
   Extract tokens (separated by characters in 'delimiter' string) from
   'str'. Return NULL-terminated array of tokens.

   The function allocates the space for the returned array. It is the
   caller's responsibility to free it.
   */
static char **str_tokenize(char *str, char *delimiter)
{
  int  len;
  int  i;
  int  rlen;
  int  cntr;
  int  capacity = INIT_LENGTH;
  char *xtr;
  char *ptr;
  char **tokens;
  char **xtokens;

  tokens = (char **) 0;
  cntr = 0;
  if ((str != (char *) 0) && (delimiter != (char *) 0))
  {
    rlen = strlen(str);
    if (rlen > 0)
    {
      tokens = malloc(INIT_LENGTH*sizeof(char *));
      capacity = INIT_LENGTH;
    }
    xtr = str;
    while (rlen > 0)
    {
      len = strspn(xtr, delimiter);
      xtr += len;
      rlen -= len;
      len = strcspn(xtr, delimiter);
      rlen -= len;
      if (len > 0)
      {
        ptr = malloc(len+1);
        memcpy(ptr, xtr, len);
        ptr[len] = '\0';
        xtr += len;
        tokens[cntr++] = ptr;
        if (cntr >= capacity)
        {
          capacity += capacity;
          xtokens = malloc(capacity*sizeof(char *));
          for (i = 0; i < cntr; i++)
          {
            xtokens[i] = strdup(tokens[i]);
            free(tokens[i]);
          }
          free(tokens);
          tokens = xtokens;
        }
      }
    }
    if (tokens)
      tokens[cntr] = (char *) 0;
  }
  return tokens;
}

static void str_free(char **tokens)
{
  int i;

  i = 0;
  if (tokens)
    while (tokens[i] != (char *) 0)
      free(tokens[i++]);
  free(tokens);
}

static char *sfree(char *str)
{
  free(str);
  return (char *) 0;
}

/*
   Return 1 if file described by `st_mode' is regular executable file.
   */
static int check_mode(mode_t st_mode)
{
  int mode;
  mode_t mode1;
  mode_t mode2;

  mode = 0;
  mode1 = S_ISREG(st_mode);
  if (mode1)
  {
    mode2 = S_IXUSR;
    mode = st_mode & S_IXUSR;
  }
  return mode;
}

/*
   Search PATH for executable `exe'. Return full path to exe.
   */
static char *search_path(char *exe)
{
  int  i;
  int  status;
  int  done;
  int  mode;
  char *path;
  char *env;
  char *str;
  char *xtr;
  char **dirs;
  struct stat buf;

  path = (char *) 0;
  env = getenv(PATH);
  if (env)
  {
    dirs = str_tokenize(env, ":");
    i = 0;
    str = malloc(strlen(env)+strlen(exe)+2);
    done = 0;
    while (((xtr = dirs[i++]) != (char *) 0) && !done)
    {
      sprintf(str, "%s/%s", xtr, exe);
      status = stat(str, &buf);
      if (!status)
      {
        mode = check_mode(buf.st_mode);
        if (mode)
        {
          path = str;
          done = 1;
        }
      }
    }
    str_free(dirs);
  }
  return path;
}

/*
   Search PATH for file named `exe' with proper permissions. Return
   full path to the file.
   */
static char *search_exe(char *exe)
{
  int  status;
  int  mode;
  char *path;
  char *lpath;
  struct stat buf;

  path = (char *) 0;
  lpath = search_path(exe);
  if (lpath)
  {
    /*
       Check permissions.
       */
    status = stat(lpath, &buf);
    if (!status) 
    {
      mode = check_mode(buf.st_mode);
      if (mode)
        path = lpath;
    }
  }
  return path;
}

char *get_env(char *name)
{
  char *xtr;
  char *env;

  xtr = getenv(name);
  if (xtr != (char *) 0)
  {
    env = malloc(strlen(name)+strlen(xtr)+2);
    sprintf(env, "%s=%s", name, xtr);
  }
  else
    env = strdup("");
  return env;
}

char **get_environment(void)
{
  int  idx;
  char *env;
  char **enx;

  enx = calloc(MAX_NENV+1, sizeof(char *));
  env = get_env(ORACLE_HOME);
  idx = 0;
  enx[idx++] = env;
  enx[idx++] = get_env(ORACLE_SID);
  enx[idx++] = get_env(TNS_ADMIN);
  enx[idx++] = get_env(SQLPATH);
  enx[idx++] = get_env(NLS_LANG); /* added by dbakorea */
  enx[idx++] = get_env(ORA_NLS33); /* added by dbakorea */
  enx[idx++] = get_env(NLS_DATE_FORMAT); /* added by leeym */
  enx[idx++] = get_env(NLS_COMP); 
  enx[idx++] = get_env(NLS_SORT); 
  enx[idx++] = get_env(LD_LIBRARY_PATH); 
  enx[idx++] = get_env(DYLD_LIBRARY_PATH); 
  enx[idx++] = get_env(ORACLE_PATH);
  enx[idx++] = get_env(TWO_TASK); 
  return enx;
}

static void print_environment(char **enx)
{
  int  idx;
  char *env;

  idx = 0;
  while (enx[idx])
    printf("enx[%d]: '%s'\n", idx, enx[idx++]);
}

/*
   Find the location of `sqlplus' binary. `orahome' is the value of
   ORACLE_HOME environment variable, if it is defined.

   The rules are as follows:

   - look for executable binary `sqlplus' in PATH
   - if not found, look for executable binary `sqlplus' in
   ORACLE_HOME/bin
   - if not found, look for executable binary `sqlplus' in the current
   directory

   Return full path of the binary. Return NULL if it doesn't exist or
   isn't executable.
   */
static char *stat_sqlplus(const char *orahome)
{
  int status = 0;
  int mode;
  char *spath;
  struct stat buf;

  /*
     Check PATH for sqlplus binary.
     */
  spath = getenv("SQLPLUS_BIN");
  if (!spath){
    spath = search_exe("sqlplus");
    if (orahome){
      /*
         Not found, check $ORACLE_HOME/bin.
         */
      spath = malloc(strlen(orahome)+30);
      sprintf(spath, "%s/bin/sqlplus", orahome);
      status = stat(spath, &buf);
      if (!status) {
        mode = check_mode(buf.st_mode);
        if (!mode){
          spath = sfree(spath);
        }
      }else{
        spath = sfree(spath);
      }
    }
    if (!spath){
      /*
         Check the current directory.
         */
      spath = strdup("./sqlplus");
      status = stat(spath, &buf);
      if (!status) {
        mode = check_mode(buf.st_mode);
        if (!mode){
          spath = sfree(spath);
        }
      }else{
        spath = sfree(spath);
      }
    }
  }
  return spath;
}

/*
   Read 'fname' into string.
   */
static char *read_file(const char *fname, char *line)
{
  int  status;
  int  len;
  char *str = (char *) 0;
  char *xtr;
  struct stat buf;
  FILE *fptr;

  fptr = fopen(fname, "r");
  if (fptr != (FILE *) 0){
    status = stat(fname, &buf);
    if (status == 0){
      str = malloc((buf.st_size+1)*sizeof(char));
      if (str != (char *) 0){
        xtr = str;
        while (fgets(line, MAX_LINE_LENGTH, fptr)){
          len = strlen(line);
          memcpy(xtr, line, len);
          xtr += len;
        }
        fclose(fptr);
        *xtr = '\0';
      }
    }
  }
  return str;
}


/*
   Extract basename of 'path'. Like basename() (not available on all
   platforms).
   */
static char *bname(char *path)
{
  char *xtr;
  char *bname = (char *) 0;

  if (path && *path)
  {
    xtr = strrchr(path, '/');
    if (xtr)
    {
      xtr++;
      bname = strdup(xtr);
    }
    else
      bname = strdup(path);
  }
  return bname;
}

/*
   Extract EDITOR values from 'str'. 
   */
static char **set_editor(char *str)
{
  char *ptr;
  char *xtr;
  char **editor = (char **) 0;

  if (str && *str && (ptr = strrchr(str, '=')))
  {
    ptr++;
    ptr += strspn(ptr, " \"");
    xtr = strdup(ptr);
    ptr = strrchr(xtr, ';');
    if (ptr)
      *ptr = '\0';
    ptr = strrchr(xtr, '"');
    if (ptr)
      *ptr = '\0';
    /*
       We have the 'define _editor' string, now extract tokens.
       */
    editor = str_tokenize(xtr, " ");
    /*
       Remove end-of-line.
       */
    if ((ptr = strchr(editor[0], '\n')))
      *ptr = '\0';
    _editor = strdup(editor[0]);
    if (*editor[0] != '/')
      editor[0] = search_path(editor[0]);
    free(xtr);
  }
  return editor;
}

/*
   Check if file 'fptr' contains an editor definition line.
   */
static char **file_editor(FILE *fptr, char *line)
{
  char **editor = (char **) 0;

  while (fgets(line, MAX_LINE_LENGTH, fptr) && (editor == (char **) 0))
    if ((line[0] != '#') && strstr(line, DEFINE_CMD) && (strstr(line, EDITOR)))
      editor = set_editor(line);
  fclose(fptr);
  return editor;
}

/*
   Extract EDITOR definition from glogin.sql and login.sql files.

   sqlplus takes settings from glogin.sql in
   ${ORACLE_HOME}/sqlplus/admin directory first. Then it looks for
   login.sql file in the local directory, followed by directories in
   ${SQLPATH}. Note that as soon as login.sql is found in any of these
   directories, the search stops.
   */
static char **file_set_editor(char *line)
{
  int  found;
  int  cntr;
  char *path;
  char **editor = (char **) 0;
  char **sqlpath;
  char *oracle_home;
  FILE *fptr;

  path = malloc(1024);
  /*
     Check local directory first.
     */
  found = 0;
  strcpy(path, "login.sql");
  fptr = fopen(path, "r");
  if (fptr)
  {
    found = 1;
    editor = file_editor(fptr, line);
  }
  /*
     If login.sql is not found in the local directory, check SQLPATH
     directories.
     */
  sqlpath = str_tokenize(getenv("SQLPATH"), ":");
  if (sqlpath)
  {
    cntr = 0;
    while (!found && sqlpath[cntr])
    {
      sprintf(path, "%s/login.sql", sqlpath[cntr]);
      cntr++;
      fptr = fopen(path, "r");
      if (fptr)
      {
        found = 1;
        editor = file_editor(fptr, line);
      }
    }
    str_free(sqlpath);
  }
  if (!found)
  {
    /*
       login.sql not found. Check ${ORACLE_HOME}/sqlplus/admin/gqlplus.sql.
       */
    oracle_home = getenv("ORACLE_HOME");
    if (oracle_home)
    {
      sprintf(path, "%s/sqlplus/admin/glogin.sql", oracle_home);
      fptr = fopen(path, "r");
      if (fptr)
      {
        found = 1;
        editor = file_editor(fptr, line);
      }
    }
  }
  free(path);
  return editor;
}

/*
   Send 'xtr' to sqlplus using 'i' command.
   */                        
static void insert_line(int fdin, int fdout, char *xtr, char *ccmd, char *line)
{
  char *str;
  char *prompt;

  if (xtr && *xtr)
  {
    sprintf(ccmd, "i %s\n", xtr);
    write(fdout, ccmd, strlen(ccmd));
    prompt = get_sqlplus(fdin, line, &str);
    free(prompt);
    free(str);
  }
}

static void free_nta(char **nta)
{
  int i;

  if (nta != (char **) 0)
  {
    i = 0;
    while (nta[i] != (char *) 0)
      free(nta[i++]);
    free(nta);
  }
}

/*
   Implement 'PAUSE ON' mode. Requires special processing, since during
   the dialogue with sqlplus no prompt is returned.
   */
static void pause_cmd(int fdin, int fdout, char *line, char *sql_prompt, int first_flag)
{
  int  result;
  int  done;
  int  dn;
  int  llen;
  int  capacity;
  int  flags;
  int  process_response;
  int  len;
  char *lx;
  char *response;
  char *sp;
  char *prompt;
  char buffer[BUF_LEN];

  /*
     Get sqlplus output without blocking.
     */
  flags = fcntl(fdin, F_GETFL, 0);
  fcntl(fdin, F_SETFL, (flags | O_NDELAY));
  /*
     Send the command to sqlplus.
     */
  write(fdout, line, strlen(line));
  write(fdout, "\n", 1);
  if (first_flag)
    process_response = 1;
  else
    process_response = 0;
  lx = malloc((MAX_LINE_LENGTH+1)*sizeof(char));
  response = malloc((INIT_LINE_LENGTH+1)*sizeof(char));
  response[0] = '\0';
  capacity = INIT_LINE_LENGTH;
  llen = 0;
  if (lx && response)
  {
    done = 0;
    while (!done)
    {
      /*
         Read and display response.
         */
      dn = 0;
      if (process_response)
        while (!dn)
        {
          result = read(fdin, buffer, 1000);
          if (result > 0)
          {
            if (result+llen > capacity)
            {
              while (capacity <= result+llen)
                capacity += capacity;
              response = realloc(response, capacity+1);
            }
            memcpy(&response[llen], buffer, result);
            llen += result;
            response[llen] = '\0';
          }
          /*
             We should get out of the loop when sqlplus finishes
             sending it's response. But there is really no
             guaranteed way to know when sqlplus is done. So the
             best thing we can do here is to wait for at least one
             character; that means sqlplus has started sending, and
             with the small usleep() delay below, it probably means
             it will be done by the time we reach the next read()
             above.

             Again, this is not foolproof, but seems to work OK. If
             problems are encountered, increase the usleep()
             interval.
             */
          else if ((!result || ((result < 0) && (errno == EAGAIN))) && (llen > 0))
            dn = 1;
          else if ((result < 0) && (errno != EAGAIN))
            perror(NULL);
          usleep(1);
        }
      process_response = 1;
      /*
         Verify if the response ends in user-defined prompt or
         default prompt. If so, terminate. Otherwise, display it.
         */
      if (sql_prompt)
        prompt = sql_prompt;
      else
        prompt = SQL_PROMPT;
      sp = strstr(response, prompt);
      if (sp && (strlen(sp) == strlen(prompt)))
      {
        done = 1;
        if (!first_flag)
        {
          len = sp-response;
          fwrite(response, sizeof(char), len, stdout);
          fflush(stdout);
        }
      }
      else
      {
        printf("%s", response);
        fflush(stdout);
        response[0] = '\0';
        llen = 0;
        /*
           Get a line from keyboard and send it to sqlplus.
           */
        fgets(lx, MAX_LINE_LENGTH, stdin);
        write(fdout, lx, strlen(lx));
      }
    }
  }
  else
    perror(NULL);
  /*
     Reset pipe from sqlplus to O_NDELAY.
     */
  fcntl(fdin, F_SETFL, (flags &~ O_NDELAY));
  free(lx);
  free(response);
}

/*
   Implement sqlplus 'edit' command. The command works in two modes:
   first, if no 'fname' is given, it launches editing session of
   afiedt.buf file, and sends the edited file to to sqlplus; if fname
   is given, it opens it.
   */
static int edit(int fdin, int fdout, char *line, char **editor, char *fname)
{
  int   child_stat;
  int   status;
  int   i;
  int   xc;
  int   len;
  int   done;
  pid_t pid;
  char  *path;
  char  *str;
  char  *xtr;
  char  *ptr;
  char  *afiedt;
  char  *prompt;
  char  *ccmd;
  char  *rname;
  char  *newline;
  char  **xrgs = (char **) 0;
  char  **enx = (char **) 0;
  FILE  *fptr;

  status = 0;
  str = (char *) 0;
  prompt = (char *) 0;
  rname = (char *) 0;
  if (fname)
  {
    /*
       Optionally add .sql extension to fname.
       */
    ptr = strstr(fname, SQLEXT);
    if (ptr && (strlen(ptr) == strlen(SQLEXT)))
      rname = strdup(fname);
    else
    {
      rname = malloc((strlen(fname)+strlen(SQLEXT)+1)*sizeof(char));
      sprintf(rname, "%s%s", fname, SQLEXT);
    }
  }
  else
  {
    /*
       Get last SQL statement from sqlplus and put it in afiedt.buf, then
       open it in editor.
       */
    write(fdout, LIST_CMD, strlen(LIST_CMD));
    prompt = get_sqlplus(fdin, line, &str);
    /* TBD: afiedt.buf filename hardcoded. */
    rname = strdup(AFIEDT);
  }
  if (fname || !strstr(str, NO_LINES))
  {
    if (!fname)
      fptr = fopen(rname, "w");
    if (fptr || fname)
    {
      if (!fname)
      {
        /*
           Have to clean-up the numeric prompt (5 characters) coming back
           from the LIST_CMD.
           */
        xtr = str;
        while ((ptr = strchr(xtr, '\n')) != (char *) 0)
        {
          len = ptr-xtr;
          *ptr = '\0';
          fprintf(fptr, "%s\n", xtr+5);
          xtr += len+1;
        }
        fprintf(fptr, "/\n");
        status = fclose(fptr);
      }
      if (!status)
      {
        free(str);
        free(prompt);
        path = editor[0];
        xc = 0;
        while (editor[xc++] != (char *) 0);
        xrgs = calloc(xc+5, sizeof(char *));
        xc = 0;
        xrgs[xc++] = bname(path);
        i = 1;
        while (editor[i] != (char *) 0)
        {
          if ((newline = strchr(editor[i], '\n')))
            *newline = '\0';
          xrgs[xc++] = strdup(editor[i]);
          i++;
        }
        xrgs[xc++] = strdup(rname);
        /*
           Launch emacs in a separate window.
           */
        if (!strcmp(xrgs[0], "emacs"))
          if (!(xrgs[1] && !strcmp(xrgs[1], "-nw")))
          {
            if (getenv("DISPLAY"))
            {
              xrgs[xc++] = strdup("-d");
              xrgs[xc++] = strdup(getenv("DISPLAY"));
            }
          }
        enx = calloc(10, sizeof(char *));
        xc = 0;
        enx[xc++] = get_env(TERM);
        enx[xc++] = get_env(PATH);
        enx[xc++] = get_env("HOME");
        edit_pid = fork();
        errno = 0;
        if (edit_pid != 0)
        {
          /*
             This is to restart waitpid() after SIGCHLD signal is
             generated (this happens when user presses Ctrl-G in
             Emacs). Perhaps we should just block the signal
             here. Also, you would expect that SA_RESTART would
             achieve this, but couldn't get it to work. Anyways,
             this is simple and it works.
             */
          done = 0;
          while (done == 0)
          {
            pid = waitpid(edit_pid, &child_stat, 0);
            if ((pid == 0) || ((pid < 0) && (errno != EINTR)))
              done = 1;
          }
        }
        else 
        { 
          if (execve(path, xrgs, enx) < 0) 
          {
            str = malloc(100);
            sprintf(str, "execve() failure; %s", path);
            perror(str);
            _exit(-1);
          }
        }
        edit_pid = 0;
        /*
           Restore signal handling.
           */
        if (1 && !fname)
        {
          /*
             Done editing. Use 'del' and 'i' commands to send contents of
             afiedt.buf to sqlplus. Algorithm: delete all lines in the
             buffer using 'del 1 LAST' command, then send all lines from
             'afiedt' to sqlplus using the 'i' command.
             */
          afiedt = read_file(AFIEDT, line);
          if (afiedt != (char *) 0)
          {
            /*
               Remove the trailing '/'.
               */
            len = strlen(afiedt);
            if (!strcmp(&(afiedt[len-3]), "\n/\n"))
            {
              len -= 3;
              afiedt[len] = '\0';
            }
            write(fdout, DEL_CMD, strlen(DEL_CMD));
            prompt = get_sqlplus(fdin, line, &str);
            free(prompt);
            free(str);
            xtr = afiedt;
            ccmd = malloc(MAX_LINE_LENGTH*sizeof(char));
            while ((ptr = strchr(xtr, '\n')) != (char *) 0)
            {
              len = ptr-xtr;
              *ptr = '\0';
              if (*xtr)
              {
                insert_line(fdin, fdout, xtr, ccmd, line);
                xtr += len+1;
              }
            }
            /*
               Last line, if not empty.
               */
            insert_line(fdin, fdout, xtr, ccmd, line);
            write(fdout, LIST_CMD, strlen(LIST_CMD));
            prompt = get_sqlplus(fdin, line, (char **) 0);
            free(prompt);
            free(ccmd);
            free(afiedt);
          }
          else
            status = -1;
        }
      }
      else
        fprintf(stderr, "%s", AFIEDT_ERRMSG);
      free_nta(enx);
      free_nta(xrgs);
    }
    else
    {
      fprintf(stderr, "%s", AFIEDT_ERRMSG);
      status = -1;
    }
  }
  else
    write(STDOUT_FILENO, NOTHING_TO_SAVE, strlen(NOTHING_TO_SAVE));
  free(rname);
  return status;
}

FILE *open_log_file()
{
  char *fname; 
  FILE *fptr = (FILE *) 0;

  fname = malloc(100*sizeof(char));
  sprintf(fname, "gqlplus_%d.log", (int) getpid());
  fptr = fopen(fname, "w");
  free(fname);
  return fptr;
}

struct table 
{
  char  *name;
  char  *owner;
  char  **columns;
};

static struct table *tables;

/*
   Parse column names from output of 'DESCRIBE' Oracle command.
   */
static char **parse_columns(char *str)
{
  int  i;
  int  j;
  int  cflag;
  int  capacity;
  int  clen;
  int  idx;
  char *cname;
  char **columns;
  char **tokens;

  capacity = INIT_NUM_COLUMNS;
  columns = calloc(capacity+1, sizeof(char *)); 
  tokens = str_tokenize(str, "\n");
  idx = 1;
  i = 0;
  cflag = 0;
  while ((cname = tokens[idx]))
  {
    idx++;
    if (cflag == 0)
    {
      if (*cname && strstr(cname, "------"))
        cflag = 1;
    }
    else
    {
      clen = strspn(cname, SPACETAB);
      cname += clen;
      clen = strcspn(cname, SPACETAB);
      columns[i] = malloc((clen+1)*sizeof(char));
      memcpy(columns[i], cname, clen);
      (columns[i])[clen] = '\0';
      tl2(columns[i]);
      i++;
      if (i >= capacity)
      {
        capacity += capacity;
        columns = realloc(columns, (capacity+1)*sizeof(char *));
        for (j = i; j <= capacity; j++)
          columns[j] = (char *) 0;
      }
    }
  }
  return columns;
}

static char **get_column_names(char *tablename, char *owner, int fdin, int fdout, char *line)
{
  int  len;
  char *ccmd = (char *) 0;
  char *str = (char *) 0;
  char *prompt = (char *) 0;
  char **columns = (char **) 0;

  if (tablename)
  {
    len = strlen(DESCRIBE)+strlen(tablename)+4;
    if (owner)
      len += strlen(owner);
    ccmd = malloc(len*sizeof(char));
    if (owner)
      sprintf(ccmd, "%s %s.%s\n", DESCRIBE, owner, tablename);
    else
      sprintf(ccmd, "%s %s\n", DESCRIBE, tablename);
    write(fdout, ccmd, strlen(ccmd));
    prompt = get_sqlplus(fdin, line, &str);
    free(prompt);
    columns = parse_columns(str);
    free(str);
    free(ccmd);
  }
  return columns;
}

static int get_pagesize(int fdin, int fdout, char *line)
{
  int  pagesize;
  int  len;
  char *str;
  char *xtr;

  write(fdout, PAGESIZE_CMD, strlen(PAGESIZE_CMD));
  get_sqlplus(fdin, line, &str);
  xtr = strchr(str, ' ')+1;
  len = strspn(xtr, DIGITS);
  xtr[len] = '\0';
  pagesize = atoi(xtr);
  free(str);
  return pagesize;
}

/*
   Extract table names from 'str', where 'str' is the string
   representation of the results of SELECT_TABLES query.

   If 'complete_columns' is 1, get column names as well.
   */
static struct table *get_names(char *str, int fdin, int fdout, int pagesize, char *line)
{
  int  i;
  int  j;
  int  idx;
  int  capacity;
  char *tname;
  char *ltr;
  char **toks;
  char **tokens;
  struct table *tables = NULL;
  FILE *fptr;

  /*fptr = fopen("gqlplus.log", "a");*/
  fptr = (FILE *) 0;
  if (fptr)
  {
    fprintf(fptr, "str:`%s'\n", str);
    fprintf(fptr, "pagesize: %d\n", pagesize);
  }
  capacity = INIT_NUM_TABLES;
  tables = calloc(capacity+1, sizeof(struct table)); 
  toks = str_tokenize(str, "\n");
  idx = 0;
  if (pagesize > 0)
  {
    idx = 1;
    ltr = (char *) 0;
  }
  else
    ltr = str;
  i = 0;
  while ((tname = toks[idx]))
  {
    idx++;
    ltr = (char *) 0;
    if (*tname && strcmp(tname, "TABLE_NAME") && strcmp(tname, "VIEW_NAME") &&
        !strstr(tname, "------") && !strstr(tname, "rows selected"))
    {
      tokens = str_tokenize(tname, " \t");

      if (fptr)
      {
        if (tname)
          fprintf(fptr, "tname:\n%s\n", tname);
        if (tokens[0])
          fprintf(fptr, "tokens[0]: `%s'\n", tokens[0]);
        if (tokens[1])
          fprintf(fptr, "tokens[1]: `%s'\n", tokens[1]);
      }

      if (tokens[0])
      {
        tables[i].name = strdup(tokens[0]);
        tl2(tables[i].name);
      }
      if (tokens[1])
      {
        tables[i].owner = strdup(tokens[1]);
        tl2(tables[i].owner);
      }
      str_free(tokens);
      if (complete_columns == 1)
        tables[i].columns = get_column_names(tables[i].name, tables[i].owner, 
            fdin, fdout, line);
      i++;
    }
    if (i >= capacity)
    {
      capacity += capacity;
      tables = realloc(tables, (capacity+1)*sizeof(struct table));
      for (j = i; j <= capacity; j++)
      {
        tables[j].name = (char *) 0;
        tables[j].owner = (char *) 0;
        tables[j].columns = (char **) 0;
      }
    }
  }
  if (fptr)
    fclose(fptr);
  return tables;
}

/*
   Get the list of all tables and views for this user. If
   'complete_columns' is 1, get all column names as well.
   */
static struct table *get_completion_names(int fdin, int fdout, char *line)
{
  int  pagesize;
  char *str;
  char *ccmd;
  struct table *tables;

  str = (char *) 0;
  tables = (struct table *) 0;
  pagesize = get_pagesize(fdin, fdout, line);
  ccmd = malloc((strlen(SELECT_TABLES_1)+strlen(SELECT_TABLES_2)+1)*sizeof(char));
  sprintf(ccmd, "%s%s", SELECT_TABLES_1, SELECT_TABLES_2);
  write(fdout, ccmd, strlen(ccmd));
  get_sqlplus(fdin, line, &str);
  /* kehlet: ORA- error is likely because the database isn't open */
  if (!strstr(str, "ORA-")) {
    tables = get_names(str, fdin, fdout, pagesize, line);
  }
  free(str);
  write(fdout, DEL_CMD, strlen(DEL_CMD));
  get_sqlplus(fdin, line, &str);
  free(ccmd);
  return tables;
}


/* 
   Generator function for table/column name completion.  STATE lets us
   know whether to start from scratch; without any state (i.e. STATE
   == 0), then we start at the top of the list.
   */
char *tablecolumn_generator(const char *text, int state)
{
  static int table_index;
  static int len;
  static int column_index;
  char *ltext;
  char *name;
  char *cname;

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (!state)
  {
    table_index = 0;
    column_index = 0;
    len = strlen (text);
  }

  ltext = tl((char *) text);
  /* Return the next name which partially matches from the command list. */
  while ((name = tables[table_index].name))
  {
    table_index++;
    if (strncmp(name, ltext, len) == 0)
    {
      free(ltext);
      return (strdup(name));
    }
    /*
       Table name did not match. Try column names:
       */
    else if (complete_columns == 1)
    {
      while ((cname = tables[table_index-1].columns[column_index]))
      {
        column_index++;
        if (strncmp(cname, ltext, len) == 0)
        {
          table_index--;
          free(ltext);
          return (strdup(cname));
        }
      }
      column_index = 0;
    }
  }
  free(ltext);

  /* If no names matched, then return NULL. */
  return ((char *) 0);
}

/*
   Detect gqlplus-specific commmand-line switch `sw'.
   */
static int gqlplus_switch(char **argv, char *sw)
{
  int i = 0;
  int carg = 0;
  int index = -1;

  while (argv[i])
  {
    carg++;
    if (!strcmp(argv[i], sw))
      index = i;
    i++;
  }
  if (index != -1)
  {
    for (i = index; i < carg; i++)
      argv[i] = argv[i+1];
    carg--;
    argv[carg] = (char *) 0;
  }
  return carg;
}

static int install_completion(char *line, int *all_tables)
{
  int completion_names = 0;

  rl_completion_entry_function = tablecolumn_generator;
  if (progress == 1)
  {
    if (complete_columns == 1)
      printf("gqlplus: scanning tables and columns...\n");
    else
      printf("gqlplus: scanning tables...\n");
  }
  tables = get_completion_names(fds2[0], fds1[1], line);
  if (tables)
    completion_names = 1;
  else
  {
    /*
       Something's wrong - we can't read or parse ALL_TABLES. Abandon
       tablename completion. But warn the user.
       */
    fprintf(stderr, "Warning: couldn't query or parse ALL_TABLES or ALL_VIEWS. Tablename completion disabled.\n");
    *all_tables = 0; 
  }
  return completion_names;
}

/*
   Return the shell command (the argument of '!' or 'HO' SQLPLUS
   commands).
   */
static char *get_shellcmd(char *lline)
{
  int  len;
  char *shellcmd = (char *) 0;
  char *xtr;

  if (lline[0] == '!')
  {
    if (lline[1] == '\0')
      shellcmd = getenv("SHELL");
    else
      shellcmd = &lline[1];
  }
  else if (!strncmp(lline, HOST_CMD, strlen(HOST_CMD)))
  {
    xtr = lline;
    len = strcspn(lline, " ");
    xtr += len;
    len = strspn(xtr, " ");
    xtr += len;
    if (strlen(xtr) != 0)
      shellcmd = xtr;
    else
      shellcmd = getenv("SHELL");
  }
  return shellcmd;
}

/*
   Check if we have the complete connect string on the command line.
   */
static char *get_connect_string(int argc, char **argv)
{
  int  i;
  int  done = 0;
  char *str;
  char *sid;
  char *connect_string = (char *) 0;

  sid = getenv(ORACLE_SID);
  for (i = 1; (i < argc) && (done == 0); i++)
  {
    str = argv[i];
    if ((str[0] != '-') && (str[0] != '@'))
    {
      if (strchr(str, '/'))
      {
        if (strchr(str, '@'))
          connect_string = strdup(str);
        else if (sid != (char *) 0)
        {
          connect_string = malloc(strlen(str)+strlen(sid)+2);
          sprintf(connect_string, "%s@%s", str, sid);
        }
      }
      else
        username = strdup(str);
    }
  }
  return connect_string;
}

/*
   Build connect string from username, password, ORACLE_SID.
   */
static char *build_connect_string(char *user, char *password)
{
  char *connect_string = (char *) 0;
  char *sid;
  char **tokens;

  sid = getenv(ORACLE_SID);
  if (strchr(user, '@'))
  {
    if (strchr(user, '/'))
      connect_string = strdup(user);
    else if ((password != (char *) 0))
    {
      tokens = str_tokenize(user, "@");
      connect_string = malloc(strlen(user)+strlen(password)+3);
      sprintf(connect_string, "%s/%s@%s", tokens[0], password, tokens[1]);
      str_free(tokens);
    }
  }
  else if (password && (strchr(password, '@')))
  {
    connect_string = malloc(strlen(user)+strlen(password)+3);
    sprintf(connect_string, "%s/%s", user, password);
  }
  else
  {
    if (sid != (char *) 0)
    {
      if (strchr(user, '/'))
      {
        connect_string = malloc(strlen(user)+strlen(sid)+2);
        sprintf(connect_string, "%s@%s", user, sid);
      }
      else if (password != (char *) 0)
      {
        connect_string = malloc(strlen(user)+strlen(password)+strlen(sid)+3);
        sprintf(connect_string, "%s/%s@%s", user, password, sid);
      }
    }
  }
  return connect_string;
}

/*
   Call sqlplus to extract the initial sqlplus prompt. This is needed
   in case a user has a (g)login.sql file which redefines the standard
   prompt.
   */
static char *get_sql_prompt(char *old_prompt, char *sqlplus, char *connect_string, char *line, int *status)
{
  int  len;
  char *sqlprompt = (char *) 0;
  char *cmd;
  char *xtr;
  char *ptr;
  char tmpdir[500]="";
  char *env_tmpdir;
  int  fd;

  if (connect_string){
    if (env_tmpdir=getenv("TMPDIR")){
    }else if (env_tmpdir=getenv("TEMPDIR")){
    }else{
      env_tmpdir=getenv("TEMP");
    }
    if (env_tmpdir){
      strcpy(tmpdir, env_tmpdir);
    }else{
      strcpy(tmpdir, "/tmp");
    }
    strcat(tmpdir, "/gqlplus.XXXXXX");
    //printf("tmpdir: %s\n", tmpdir);
    fd = mkstemp(tmpdir);
    unlink(tmpdir);
    if (fd < 0) {
        fprintf(stderr, "%s: %s\n", tmpdir, strerror(errno));
        close(fd);
        return (NULL);
    }
    *status = 0;
    /* still a lot todo */
    cmd = malloc(strlen(GET_PROMPT)+strlen(TAIL_PROMPT)+strlen(sqlplus)+strlen(connect_string)+strlen(tmpdir)+20);
    sprintf(cmd, "%s%s -s \"%s\" %s > %s", GET_PROMPT, sqlplus, connect_string, TAIL_PROMPT, tmpdir);
    /*printf("cmd: `%s'\n", cmd);*/
    *status = system(cmd);
    if (!*status){
      free(cmd);
      xtr = read_file(tmpdir, line);
      unlink(tmpdir);
      
         //Get the last line.
      xtr = strstr(xtr, SQLPROMPT);
      if (xtr){
        if (!strcmp(xtr, "\n") || strncmp(xtr, SQLPROMPT, strlen(SQLPROMPT)))
          sqlprompt = old_prompt;
        else{
            /*
             Is this user using (g)login.sql? And if so, is she
             setting sqlprompt?
             */
          ptr = strchr(xtr, '"');
          ptr++;
          len = strcspn(ptr, "\"");
          sqlprompt = malloc(len+1);
          memcpy(sqlprompt, ptr, len);
          sqlprompt[len] = '\0';
        }
      }
    }
  }
  return sqlprompt;
}

static void usage(int argc, char **argv)
{
  int i;
  int done;

  done = 0;
  for (i = 0; (i < argc) && !done; i++)
    if (!strcmp(argv[i], "-h"))
    {
      done = 1;
      printf("\ngqlplus version %s; usage: gqlplus [sqlplus_options] [-h] [-d] [-p]\n", VERSION);
      printf("      \"-h\" this messsage\n");
      printf("      \"-d\" disable column name completion\n");
      printf("      \"-p\" show progress report and elapsed time\n");
      printf("      SQL> %sr: rescan tables (for completion)\n", szCmdPrefix);
      printf("      SQL> %sh: display command history\n", szCmdPrefix);
      printf("To kill the program, use SIGQUIT (Ctrl-\\)\n");
    }
}

static char *str_expand(char *str, int *capacity, int length, int extension)
{
  int  new_length;
  char *xstr;

  xstr = str;
  new_length = length+extension;
  if (new_length > *capacity)
  {
    while (*capacity <= new_length)
      *capacity += *capacity;
    xstr = realloc(str, *capacity+1);
  }
  return xstr;
}

static void get_final_sqlplus(int fdin)
{
  int  flags;
  int  result;
  int  capacity;
  int  llen;
  char *response;
  char buffer[BUF_LEN];

  usleep(100000);
  response = malloc((INIT_LINE_LENGTH+1)*sizeof(char));
  response[0] = '\0';
  capacity = INIT_LINE_LENGTH;
  llen = 0;
  /*
     Get sqlplus output without blocking.
     */
  flags = fcntl(fdin, F_GETFL, 0);
  fcntl(fdin, F_SETFL, (flags | O_NDELAY));
  result = read(fdin, buffer, BUF_LEN);
  if (result > 0)
  {
    response = str_expand(response, &capacity, llen, result);
    memcpy(&response[llen], buffer, result);
    llen += result;
    response[llen] = '\0';
  }
  printf("%s", response);
}

/*
   Get current time information. Utility function to display time
   information in the prompt. Contributed by Mark Harrison of Pixar.
   */
static double now()
{
  struct timeval tod;
  gettimeofday(&tod, NULL);
  return tod.tv_sec + (tod.tv_usec/1000000.0);
}

int main(int argc, char **argv)
{
  int    status;
  int    i;
  int    fd;
  int    flags;
  int    completion_names = 0;
  int    all_tables = 1; /* set to 0 if we cannot query or parse ALL_TABLES or ALL_VIEWS */
  int    pause_mode;
  int    pstat;
  char   *password = (char *) 0;
  char   *connect_string;
  char   *path;
  char   *spath;
  char   *prompt;
  char   *prompt2;
  double tod1, tod2;
  char   *line;
  char   *rline;
  char   *lline;
  char   *oline;
  char   *nptr;
  char   *shellcmd;
  char   *accept;
  char   *ed;
  char   **editor;
  char   **xrgs;
  char   **tokens;
  char   **enx;
  FILE   *fptr2;
  struct termios buf;
  struct termios save_termios;

  /* stop the compiler from complaining */
  int iTemp = strlen(rcsid);
  iTemp++;

  tod1 = now();
  state = STARTUP;
  pstat = 0;
  /*
     Is there a '/nolog' argument? If so, switch state to DISCONNECTED.
     */
  for (i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "/nolog"))
      state = DISCONNECTED;
    if (!strcmp(argv[i], "-V"))
      quit_sqlplus = 1;
  }
  pause_mode = 0;
  initialize_history("sqlplus");
  lptr = (FILE *) 0;
  /*lptr = open_log_file();*/
  editor = calloc(2, sizeof(char *));
  if (ed = getenv("EDITOR"))
  {
    editor[0] = search_path(ed);
    if (!editor[0])
      editor[0] = strdup(ed);
  }
  else
    editor[0] = strdup(VI_EDITOR); /* if EDITOR is not defined, vi is the default */
  line = malloc((MAX_LINE_LENGTH+1)*sizeof(char));
  /*
     Check if the user or the site are setting the editor in the
     login.sql/glogin.sql files.
     */
  xrgs = file_set_editor(line);
  if (xrgs)
    editor = xrgs;
  if (gqlplus_switch(argv, "-dc") != argc)
  {
    argc--;
    complete_columns = 0;
  }
  if (gqlplus_switch(argv, "-d") != argc)
  {
    argc--;
    complete_columns = 0;
  }
  if (gqlplus_switch(argv, "-p") != argc)
  {
    argc--;
    progress = 1;
  }
  sig_init();
  status = pipe(fds1); /* parent to child pipe */
  pipe_size = fpathconf (fds1[0], _PC_PIPE_BUF);
  if (status == 0)
  {
    status = pipe(fds2); /* child to parent pipe */
    if (status == 0)
    {
      enx = get_environment();
      /*print_environment(enx);*/
      /*
         Get status of sqlplus executable. `path' is the value of
         ORACLE_HOME environment variable, if it is defined.
         */
      path = strchr(enx[0], '=');
      if (path)
        path++;
      spath = stat_sqlplus(path);
      if (spath)
      {
        /*printf("sqlplus binary: %s\n", spath);*/
        /*
           Extract sqlplus prompt, if there is a connect string:
           */
        if (state != DISCONNECTED)
        {
          connect_string = get_connect_string(argc, argv);
          sql_prompt = get_sql_prompt(sql_prompt, spath, connect_string, line, &pstat);
        }
        else
          sql_prompt = SQL_PROMPT;
        if (!pstat)
        {
          if (progress)
            printf("gqlplus: starting sqlplus...\n");
          sqlplus_pid = fork();
          if (sqlplus_pid == 0)
          { 
            /*
               Child.
               */
            fd = dup2(fds1[0], STDIN_FILENO);
            if (fd == STDIN_FILENO)
            {
              fd = dup2(fds2[1], STDOUT_FILENO);
              if (fd == STDOUT_FILENO)
              {
                fptr2 = fdopen(STDOUT_FILENO, "w");
                /*
                   We have to disconnect sqlplus from
                   controlling terminal. Otherwise,
                   keyboard special keys (interrupt,
                   delete, suspend) would go to sqlplus
                   and disrupt it. See earlier comments
                   for sigint_handler().
                   */
                status = (int) setsid();
                xrgs = calloc(MAX_NARGS, sizeof(char *));
                for (i = 1; i < argc; i++){
                  xrgs[i] = argv[i];
                }
                xrgs[0] = spath;
                if (execve(spath, xrgs, enx) < 0) 
                {
                  line = malloc(100);
                  sprintf(line, "execve() failure; %s", spath);
                  perror(line);
                  status = -1;
                }
              }
              else
              {
                perror("Child dup2() error:");
                status = -1;
              }
            }
            else
            {
              perror("Child dup2() error:");
              status = -1;
            }
          }
          else if (sqlplus_pid > 0)
          {
            /*
               fds1[1] is used to send messages to
               sqlplus. fds2[0] is used to receive messages
               from sqlplus.
               */
            close(fds1[0]);
            flags = fcntl(fds2[0], F_GETFL, 0);
            if (flags != -1)
            {
              /*fcntl(fds2[0], F_SETFL, (flags | O_NDELAY));*/
              /*
                 Print initial sqlplus message.
                 */
              if (!quit_sqlplus)
                prompt = get_sqlplus(fds2[0], line, (char **) 0);
              /*
                 Loop until we quit sqlplus.
                 */
              while (quit_sqlplus == 0)
              {
                /*
                   Disable echo if password prompt.
                   */
                if (check_password_prompt(prompt))
                {
                  status = tcgetattr(STDIN_FILENO, &save_termios);
                  if (status >= 0)
                  {
                    buf = save_termios;
                    buf.c_lflag &= ~(ECHO | ICANON);
                    buf.c_cc[VMIN] = 1;
                    buf.c_cc[VTIME] = 0;
                    status = tcsetattr(STDIN_FILENO, TCSAFLUSH, &buf);
                  }
                }
                else if (prompt && (state != DISCONNECTED) && all_tables &&
                    ((( sql_prompt && !strcmp(prompt, sql_prompt)) && (completion_names == 0))
                     || ((!sql_prompt && !strcmp(prompt, SQL_PROMPT)) && (completion_names == 0))
                     || ((!sql_prompt && !strcmp(prompt, RECOVER_PROMPT)) && (completion_names == 0)))
                    )
                  completion_names = install_completion(line, &all_tables);
                /*
                   Read line from user and send it to sqlplus.
                   */
                tod2 = now();
                if (progress == 1)
                {
                  prompt2=malloc(strlen(prompt)+100);
                  sprintf(prompt2, "[%.2f] %s", tod2-tod1, prompt);
                  rline = readline(prompt2);
                  free(prompt2);
                }
                else
                  rline = readline(prompt);
                tod1 = now();
                if (rline)
                {
                  if (*rline)
                  {
                    if (matchCommand(rline, szCmdPrefix, "rebuild") || 
                        matchCommand(rline, szCmdPrefix, "r")) 
                      completion_names = 0; 

                    if (matchCommand(rline, szCmdPrefix, "history") || 
                        matchCommand(rline, szCmdPrefix, "h") 
                       ) 
                    {
                      HIST_ENTRY** ppHistEntry = history_list();
                      if (ppHistEntry != (HIST_ENTRY**) 0) 
                      {
                        int idx = 0;

                        while (ppHistEntry[idx] != (HIST_ENTRY*) 0) 
                        {
                          printf("\n%s", ppHistEntry[idx]->line);
                          idx++;
                        }

                        printf("\nEnd of History\n");
                      }
                    }

                    if (prompt && 
                        (!strcmp(prompt, PASSWORD_PROMPT) || 
                         !strcmp(prompt, USER_PROMPT)))
                    {
                      if (!strcmp(prompt, USER_PROMPT))
                        username = strdup(rline);
                      else
                        password = strdup(rline);
                      connect_string = build_connect_string(username, password);
                      sql_prompt = 
                        get_sql_prompt(sql_prompt, spath, connect_string, line, &pstat);
                    }
                    else if (!check_password_prompt(prompt))
                      add_history(rline);
                  }
                  lline = tl(rline);
                  oline = trim(rline);
                  nptr = lline+strcspn(lline, WHITESPACE); /* pointer to first parameter */
                  nptr += strspn(nptr, WHITESPACE);
                  if (!strlen(nptr))
                    nptr = (char *) 0;
                  status = 0;
                  xrgs = str_tokenize(lline, " \t");
                  if (!strncmp(lline, SET_CMD, strlen(SET_CMD)) && xrgs && xrgs[1] &&
                      !strncmp(xrgs[1], SQLPROMPT_CMD, 4))
                  {
                    write(fds1[1], rline, strlen(rline));
                    write(fds1[1], "\n", 1);
                    sql_prompt = set_sql_prompt(fds2[0], line);
                    prompt = strdup(sql_prompt);
                  }
                  else
                  {
                    /*
                       Detect change of state
                       (STARTUP/CONNECTED/DISCONNECTED).
                       */
                    if (!strncmp(lline, CONNECT_CMD, 4))
                    {
                      state = CONNECTED;
                      if (!username)
                      {
                        tokens = str_tokenize(oline,WHITESPACE);
                        connect_string = get_connect_string(2,tokens);
                        free(tokens);
                      }
                    }
                    if (!strncmp(lline, DISCONNECT_CMD, 4))
                      state = DISCONNECTED;

                    if (!strncmp(lline, PAUSE_CMD, 3))
                      pause_cmd(fds2[0], fds1[1], rline, sql_prompt, 1);
                    else if (!strcmp(lline, QUIT_CMD) && (state != STARTUP))
                    {
                      quit_sqlplus = 1;
                      kill_sqlplus();
                    }
                    else if (pause_mode && !strncmp(lline, SELECT_CMD, strlen(SELECT_CMD)))
                      pause_cmd(fds2[0], fds1[1], rline, sql_prompt, 0);
                    else if (xrgs && xrgs[1] && !strncmp(lline, SET_CMD, strlen(SET_CMD)) &&
                        !strncmp(xrgs[1], PAUSE_CMD, 3))
                    {
                      write(fds1[1], rline, strlen(rline));
                      if (xrgs[2] && !strcmp(xrgs[2], ON_CMD))
                        pause_mode = 1;
                      else
                        pause_mode = 0;
                    }
                    else if (strncmp(lline, EDIT_CMD, 2) == 0)
                    {
                      if (editor[0])
                        status = edit(fds2[0], fds1[1], line, editor, nptr);
                      else
                        printf("Editor executable %s not found.\n", _editor);
                    }
                    /*
                       Special handling for 'clear
                       screen' command - have to do
                       it locally, not transmit to
                       sqlplus.
                       */
                    else if (!strncmp(lline, CLEAR_CMD, 2) && nptr &&
                        !strncmp(nptr, SCREEN, strlen(SCREEN)))
                      system("clear");
                    else if (!check_numeric_prompt(prompt) && (shellcmd = get_shellcmd(oline)))
                    {
                      system(shellcmd);
                      fprintf(stdout, "\n");
                      fflush(stdout);
                    }else {
                      if (check_password_prompt(prompt)){
                        status = tcsetattr(STDIN_FILENO, TCSAFLUSH, &save_termios);
                      }
                      write(fds1[1], rline, strlen(rline));
                      if (strstr(lline, DEFINE_CMD) && (strstr(lline, EDITOR))){
                        editor = set_editor(lline);
                      }
                    }
                    free(prompt);
                    if (!quit_sqlplus){
                      status = write(fds1[1], "\n", 1);
                      if (status == -1){
                        fprintf(stderr, "sqlplus terminated - exiting... :( \n");
                        quit_sqlplus = 1;
                      }else{

                           //ACCEPT command requires special processing.

                        if (!strncmp(lline, ACCEPT_CMD, 3)){
                          prompt = accept_cmd(lline, fds2[0], fds1[1], sql_prompt, line);
                        }else{
                          prompt = get_sqlplus(fds2[0], line, (char **) 0);
                          if (!strncmp(lline, CONNECT_CMD, 4)){
                            /*
                               In case of CONNECT command, rescan tables.
                               */
                            completion_names = install_completion(line, &all_tables);
                         }
                        }
                      }
                    }
                  }
                  str_free(xrgs);
                  free(rline);
                  free(lline);
                }
                else
                {
                  quit_sqlplus = 1;
                  kill_sqlplus();
                }
              }
              /*
                 Quitting. Get the remaining output sent
                 from sqlplus, if any.
                 */
              get_final_sqlplus(fds2[0]);
            }
            else
              status = -1;
          }
          else
            status = -1;
        }
      }
      else
        fprintf(stderr, "sqlplus binary could not be found or is not executable :(\n");
    }
    else
      perror((char *)0);
  }
  else
    perror((char *)0);
  free(line);
  /*
     Optionally print gqlplus-specific usage message.
     */
  usage(argc, argv);
  return status;
}

