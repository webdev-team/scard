/*
 *  init.c
 *  iyell
 *
 *  Created by Michel Depeige on 22/12/2014.
 *  Copyright (c) 2007 Michel Depeige.
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "posix+bsd.h"
#include <event.h>
#include "main.h"
#include "conf.h"

#if !defined(LIBEVENT_VERSION_NUMBER) || LIBEVENT_VERSION_NUMBER < 0x02001300
#error "This version of Libevent is not supported; Get 2.0.19-stable or later."
#endif

/* prototypes */
void		check_id(void);
int		init(struct event_base **bot);
static void	init_flags();

/* functions */
void	check_id()
{
	if (getuid() == 0 || getgid() == 0)
		fprintf(stderr, "[i] warning: running as root !\n");
}

/*
 * main init
 */

int	init(struct event_base **bot)
{
	char	**tmp = NULL;

	/* check for basic stuff */
	check_id();

	/* init libevents */
	*bot = event_init();
	if (*bot == NULL) {
		fprintf(stderr, "[i] cannot init libevent, exiting\n");
		return ERROR;
	}
	if (g_mode & VERBOSE) {
		printf("[i] using libevent %s, mechanism: %s\n",
			event_get_version(), event_get_method());
	}

	log_msg("[i] scard version %s started.\n", VERSION);
	return NOERROR;
}

/* set global glags */
static void	init_flags()
{

}
