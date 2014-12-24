/*
 *  main.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <event.h>
#ifdef HAVE_CONFIG_H
#include <stdlib.h>
#include <event.h>
#include "config.h"
#include "conf.h"
#include "eca.h"
#include "main.h"
#include "tools.h"
#endif

/* globals */
conf_t	g_conf;

/* prototypes */
void	watchdog(evutil_socket_t fd, short what, void *arg);

int	main(int argc, char *argv[])
{
	struct event_base	*base;
	struct event		*ev;
	struct timeval		timeout;

	/* init the program, check opt, init some stuff... */
	checkopt(argc, argv);

	if (init(&base) == ERROR)
		exit(ERROR);

	/* switch to daemon mode if needed */
	if (g_mode & DAEMON)
		daemonize();

	/* event loop */
	ev = evtimer_new(base, watchdog, ev);
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

	ev = event_new(base, -1, EV_PERSIST, watchdog, NULL);	
	event_add(ev, &timeout);
	event_base_dispatch(base);

	eca_cleanup();
	log_msg("[-] exiting\n");
	log_cleanup();

	conf_erase(&g_conf);
	return(NOERROR);
}

void	watchdog(evutil_socket_t fd, short what, void *arg)
{
	eca_check_status();
}

