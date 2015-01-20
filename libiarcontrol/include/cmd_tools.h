/** 
 *
 *   Command line implementation.
 *
 *   Deriver from "Camera Control for STAR I."
 *
 *   (c) Copyright 2007 Federico Bareilles <fede@iar.unlp.edu.ar>,
 *   Instituto Argentino de Radio Astronomia (IAR).
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *     
 *   The author does NOT admit liability nor provide warranty for any
 *   of this software. This material is provided "AS-IS" in the hope
 *   that it may be useful for others.
 *
 **/

#ifndef _CMD_TOOLS_H
#define _CMD_TOOLS_H

#define _GNU_SOURCE

#include <unistd.h>

#include <readline/readline.h>

typedef int ct_fd_callback_t (int, void *);

extern int interactive_flag;

#define PRINTINT(fmt, args...) \
if(interactive_flag) {         \
        printf(fmt, ## args);  \
}

/* A structure which contains information on the commands this program
   can understand. */

typedef struct {
	char *name;		/* User printable name of the function. */
	int link;               /* Make link for this command. */
	int show;               /* Show or hide command. */
	rl_icpfunc_t *func;	/* Function to call to do the job. */
	char *param;            /* Parameters for this function.  */
	char *doc;		/* Documentation for this function.  */
} COMMAND;

/* The names of internal functions that actually do the manipulation. */
int com_help   __P((char *));
int com_quit   __P((char *));
int com_chdir  __P((char *));
int com_export __P((char *));

/* Example of base command structure: 

COMMAND commands[] = {
	{ "cd",       0, 0, com_chdir, "[path]", "Change working directory" },	
	{ "export",   0, 0, com_export, "name=value", "Change or add an environment variable" },	
	{ "help",     0, 0, com_help, "[cmd]", "Display this text" },
	{ "?",        0, 1, com_help, "[cmd]", "Synonym for `help'" },
	{ "quit",     0, 0, com_quit, "", "Quit ..." },
	{ "q",        0, 1, com_quit, "", "Quit :-)" },
	{ "test",     0, 0, com_test,"", "[!]" },
	{ (char *)NULL, 0, 0, (rl_icpfunc_t *)NULL, (char *) NULL, (char *)NULL }
};
*/


int cmd_tools_set_command(COMMAND *cmd);
int cmd_tools_set_prompt(char *prompt);
int cmd_tools_use_history(char *basename);
int cmd_tools_use_baner(int flag);

int cmd_tools_main_loop(int argc, char * const argv[]);

int cmd_tools_link_commands_to(char *program_name);
int cmd_tools_un_link_commands(void);

int cmd_tools_add_fd_callback(int fd, ct_fd_callback_t *fd_callback, 
			      void *data);

#endif  /* ifndef _CMD_TOOLS_H */
