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
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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

/* globals */
char		**g_sources;

/* prototypes */
void		check_id(void);
int		init(struct event_base **sched);
static void	init_flags();
int		register_sources(hash_t *section, unsigned int count);
void		unregister_sources(void);

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
	unsigned int   	c, i;

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

	register_sources(section, c);
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
	log_init();
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

/*
 * This function register sources on global array
 */

int	register_sources(hash_t *section, unsigned int count)
{
	struct record_s	*recs;	
	unsigned int   	c, i;

	assert(section != NULL);
	assert(count > 0);

	g_sources = malloc((count + 1) * sizeof(char *));
	if (g_sources == NULL)
		log_err("[i] malloc() error: %s\n", strerror(errno));

	recs = section->records;
	for (i = 0, c = 0; c < section->records_count; i++) {
		if (recs[i].hash != 0 && recs[i].key) {
			g_sources[c] = (char *)recs[i].key;
			c++;
		}
	}
	g_sources[c] = NULL;

	if (g_mode & VERBOSE)
		log_msg("[i] %i source(s) registered\n", c);
	return NOERROR;
}

void	unregister_sources()
{
	free(g_sources);
}
