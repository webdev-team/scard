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
#include "config.h"
#include "conf.h"
#include "main.h"
#include "tools.h"
#endif

/* prototypes */
conf_t	g_conf;

int	main(int argc, char *argv[])
{
	struct event_base	*sched;

	/* init the program, check opt, init some stuff... */
	checkopt(argc, argv);

	if (init(&sched) == ERROR)
		exit(ERROR);

	/* switch to daemon mode if needed */
	if (g_mode & DAEMON)
		daemonize();

	log_msg("[-] exiting\n");
	log_deinit();

	conf_erase(&g_conf);
	return(NOERROR);
}
