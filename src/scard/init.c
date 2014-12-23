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
#include "eca.h"
#include "env.h"

#if !defined(LIBEVENT_VERSION_NUMBER) || LIBEVENT_VERSION_NUMBER < 0x02001300
#error "This version of Libevent is not supported; Get 2.0.19-stable or later."
#endif

/* prototypes */
void		check_id(void);
int		init(struct event_base **sched);
static void	init_flags();

/* check if we are runing as root */
void	check_id()
{
	if (getuid() == 0 || getgid() == 0)
		fprintf(stderr, "[i] warning: running as root !\n");
}

/*
 * check if sources specified in the configuration file are somewhat coherent
 * this means something like name = device, format
 */

int	check_sources(hash_t *section)
{
	struct record_s	*recs;	
	unsigned int   	c, i, j;

	if (! section)
		return ERROR;

	recs = section->records;
	for (i = 0, c = 0; c < section->records_count; i++) {
		if (recs[i].hash != 0 && recs[i].key) {
			c++;
			if (g_mode & DEBUG)
				log_msg("[i] source found: %s\n", recs[i].key);
			if (hash_text_count_data(section, (char *)recs[i].key) != 2) {
				log_err("[i] incorrect source setup for %s\n",
					recs[i].key);
				return ERROR;
			}
		}
	}

	if (g_mode & VERBOSE)
		log_msg("[i] %i source(s) registered\n", c);

	return NOERROR;
}

/*
 * main init
 */

int	init(struct event_base **sched)
{
	char	**tmp = NULL;

	conf_init(&g_conf);
	if (conf_read(&g_conf, g_file) == -1)
		return ERROR;

	/* check for basic stuff */
	check_id();
	env_check_ecasound();
	if (check_sources(g_conf.record) == ERROR) {
		fprintf(stderr, "[i] source configuration error, exiting\n");
		return ERROR;
	}

	/* init libevents */
	*sched = event_init();
	if (*sched == NULL) {
		fprintf(stderr, "[i] cannot init libevent, exiting\n");
		return ERROR;
	}
	if (g_mode & VERBOSE) {
		printf("[i] using libevent %s, mechanism: %s\n",
			event_get_version(), event_get_method());
	}

	/* init ecasound */
	if (eca_init() == ERROR) {
		fprintf(stderr, "[i] cannot init libecasound, exiting\n");
		return ERROR;
	}

        /* set handlers */
        sig_set_handlers();

	/* misc init */
	init_flags();

	g_mode |= STARTING;

	log_msg("[i] scard version %s started.\n", VERSION);
	return NOERROR;
}

/* set global glags */
static void	init_flags()
{
        /* syslog logging flag */
        if (hash_text_is_true(g_conf.global, "syslog"))
                g_mode |= SYSLOG;
}
