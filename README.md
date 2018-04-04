## WHAT IS GQLPLUS?

gqlplus is a UNIX front-end program for Oracle's command-line utility
sqlplus. gqlplus is functionally nearly identical to sqlplus, with the
addition of command-line editing, history, table-name and optional
column-name completion.  The editing is similar to tcsh and bash
shells.


## BUILDING AND INSTALLING GQLPLUS

GQLPLUS comes in 3 forms:
A tar.gz of archived code, a deb package and an RPM package.

To unpack gqlplus tar.gz, download the tar/compressed file
gqlplus-n.nn.tar.gz and type:
```
$ gunzip -c gqlplus-n.nn.tar.gz | tar xvf -
$ cd gqlplus-n.nn
```
To unpack the gqlplus-n.nn-n.src.rpm type:
```
$ rpm -ihv gqlplus-n.nn-n.src.rpm
```
You can then build it by running:
```
$ rpmbuild -ba /path/to/spec/files/gqlplus.spec
```
*NOTE: this requires the rpm-build package to be installed.*

The executable is called `gqlplus'. Prebuilt executables are provided
for RPM based Linux distros.
One can also extract the contents of this RPM by issuing:
```
$ rpm2cpio gqlplus-n.n-n.%{_arch}.rpm | cpio -idmv
```
gqlplus-n.n-n.%{_arch}.src.rpm is also available.

To build gqlplus for another UNIX-like platform, type:
```
$ autoreconf --install
$ ./configure
$ make
```
The executable gqlplus will be built in the current directory. 

gqlplus uses the GNU Readline Library to achieve its
functionality. Version 4.3 of the library is provided with gqlplus,
therefore the distribution is self-contained (it does not depend on
readline being installed on the system).


## USAGE

To invoke the program, type
```
$ gqlplus
```
at the command line. You can also use any of the sqlplus command line
arguments.

Being a front-end for sqlplus, gqlplus needs the sqlplus executable to
run. gqlplus looks for the full path to the sqlplus binary in $SQLPLUS_BIN 
first, then for a binary called 'sqlplus' in $PATH, followed by
$ORACLE_HOME/bin, followed by the local directory.

## ENV VARIABLES
 - SQLPLUS_BIN: 
   This is meant to support Oracle instantclient installations on 64bit 
   that do not call the binary sqlplus but rather sqlplus64.

 - EMACS_MODE:
   If set, gqlplus is assumed to be launched from Emacs. 
   Emacs maps Ctrl-G to send SIGINT (instead of Ctrl-C). 
   So, during Emacs editing session we don't want to pass to it 
   sqlplus because it disrupts its' operation and does not make sense
   since that is not the interrupt we want to catch.

 - TMPDIR, TEMPDIR and TEMP:
   GQLPlus respects these ENV vars as pointers to a temporary directory.
   If not set, defaults to /tmp.

To get help on using gqlplus, as well as version number, type
```
$ gqlplus -h
```
Usage is nearly identical to sqlplus, with the following additions:

- cursor keys may be used to invoke and navigate previous gqlplus
  (sqlplus) commands. The command-line editing is identical to bash
  shell. By default, it uses Emacs key bindings; if you wish to use vi
  commands, create a file containing the following line:

  set editing-mode vi

  Name the file .inputrc and put it in your home directory. Note that
  if you use vi key bindings, you will be placed in the INSERT mode
  after invoking a previous command using the up-arrow cursor key;
  press Escape key to leave that mode and move around the line using
  vi commands.  See GNU Readline Library documentation for more
  details.

- gqlplus completes table and column names, similar to the way that
  bash and tcsh complete filenames (i.e., using a TAB key). In order
  to accomplish this, gqlplus has to read descriptions of all tables
  and views available to the current user, at startup. This processing
  may be somewhat lengthy (20, 30 seconds, perhaps even a few minutes for
  large databases). If this is found to be inconvenient, use the '-dc'
  (`disable completion') command-line argument to disable the column
  name completion feature (table name completion is always enabled
  since it does not cause a noticeable delay at startup).

  To interrupt a hanging gqlplus, use SIGQUIT signal (Ctrl-\ on most
  terminals).

- from within gqlplus, use '--!h' to display command history, and
  '--!r' to rescan tables (for the purpose of updating table- and
  column-name completion).


## BUGS

- Ctrl-C during a long query sometimes breaks synchronization between
  gqlplus and sqlplus, causing the messages to show up with
  delay. Restart the program to reset synchronization.

The following functionality cannot be implemented, to the best of my
knowledge, with a reasonable effort:

- running a SQL script which redefines the prompt 

- gqlplus hangs if started with the /nolog command-line switch in the
  presence of login.sql file which redefines the prompt

- ACCEPT command without a prompt is not fully supported. Therefore,
  if you use ACCEPT command, use a prompt.

- gqlplus hangs if glogin.sql and/or login.sql files contain the
  `pause' or `set pause on' statements.
  
  This issue reported by Mathias Weyland.


## LICENSE

gqlplus is licensed under the GNU General Public License. See file
LICENSE for details.


## AUTHORS

gqlplus is written and maintained by Ljubomir J. Buturovic of San
Francisco State University, San Francisco, California and Jess Portnoy. 


For all comments and problem reports drop a line:

ljubomir@sfsu.edu 

kernel01@gmail.com



