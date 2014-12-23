/*
 *  signal.c
 *  scard
 *
 *  Created by Michel Depeige on 22/12/2014.
 *  Copyright (c) 2014 Michel Depeige.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 */

#define _GNU_SOURCE
#define _BSD_SOURCE
#define __BSD_VISIBLE 1
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "posix+bsd.h"
#include <event.h>
#include "main.h"
#include "conf.h"
#include "tools.h"

/* prototypes */
void            handler_debug(int sig);
void            handler_verbose(int sig);
void            sigchld_handler(int sig);

/*
 * Set usefull handlers
 */

void    sig_set_handlers()
{
        if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
                fatal("cannot register handler");
        if (signal(SIGUSR1, handler_verbose) == SIG_ERR)
                fatal("cannot register handler");
        if (signal(SIGUSR2, handler_debug) == SIG_ERR)
                fatal("cannot register handler");
}

/*
 * switch DEBUG mode on / off
 * implies VERBOSE mode
 */

void    handler_debug(int sig)
{
        sig = 0;

        if (g_mode & DEBUG) {
                g_mode &= DEBUG;
                g_mode &= VERBOSE;
                log_msg("[s] disabling debug mode");
        }
        else {
                g_mode |= DEBUG;
                g_mode |= VERBOSE;
                log_msg("[s] enabling debug mode");
        }
}

/*
 * switch VERBOSE mode on / off
 */

void    handler_verbose(int sig)
{
        sig = 0;

        if (g_mode & VERBOSE) {
                g_mode &= VERBOSE;
                log_msg("[s] disabling verbose mode");
        }
        else {
                g_mode |= VERBOSE;
                log_msg("[s] enabling verbose mode");
        }
}

/*
 * on SIGCHLD, log information about the child process
 */

void    sigchld_handler(int sig)
{
        int     status;
        int     pid;

        sig = 0;
        /* Wait without blocking */
        if ((pid = waitpid(-1, &status, WNOHANG)) < 0) {
                log_err("[s] waitpid failed\n");
                return ;
        }

        if (WIFEXITED(status)) {
                log_msg("[s] process %i exited, status %i\n",  pid, WEXITSTATUS(status));
        }
}
