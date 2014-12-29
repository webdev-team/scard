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
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "posix+bsd.h"
#include <event.h>
#include "main.h"
#include "conf.h"
#include "eca.h"
#include "env.h"
#include "http.h"

#if !defined(LIBEVENT_VERSION_NUMBER) || LIBEVENT_VERSION_NUMBER < 0x02001300
#error "This version of Libevent is not supported; Get 2.0.19-stable or later."
#endif

/* globals */
sources_t	*g_sources;

/* prototypes */
void		check_id(void);
int		init(struct event_base **base);
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
 * check recordpath presence and test if it's useable
 */

int 	check_recordpath()
{
	char		*path = NULL;
	struct stat	st;
	int		error;
	FILE		*f;

	path = hash_text_get_first(g_conf.global, "recordpath");
	if (path == NULL)
		return ERROR;

	/* check if it's a directory */
	error = stat(path, &st);
	if (error) {
		log_err("[i] stat() error: %s\n", strerror(errno));
		return ERROR;
	}

	if(! ((st.st_mode & S_IFDIR) != 0)) {
		log_err("[i] %s doesn't look like a directory\n");
		return ERROR;
	}

	/* 
	 * check if it's writeable
	 * test filename is path + / + .scard + XXXXXX
	 */
	char	test[strlen(path) + 1 + 6 + 6 + 1];
	sprintf(test, "%s/.scardXXXXXX", path);
	mktemp(test);

	f = fopen(test, "w");
	if (f == NULL) {
		log_err("[i] fopen() error: %s\n", strerror(errno));
		return ERROR;
	}

	fclose(f);
	unlink(test);

	return NOERROR;
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

int	init(struct event_base **base)
{
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

	check_recordpath();

	/* init libevents */
	*base = event_init();
	if (*base == NULL) {
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
	if (http_init(*base) == ERROR)
		log_err("[i] cannot init WebGUI\n");

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

	g_sources = malloc((count + 1) * sizeof(sources_t));
	if (g_sources == NULL)
		log_err("[i] malloc() error: %s\n", strerror(errno));

	recs = section->records;
	for (i = 0, c = 0; c < section->records_count; i++) {
		if (recs[i].hash != 0 && recs[i].key) {
			g_sources[c].name = (char *)recs[i].key;
			c++;
		}
	}
	g_sources[c].name = NULL;
	g_sources[c].file = NULL;

	if (g_mode & VERBOSE)
		log_msg("[i] %i source(s) registered\n", c);
	return NOERROR;
}

void	unregister_sources()
{
	int	i = 0;

	while (g_sources[i].name != NULL) {
		free(g_sources[i].file);
	}

	free(g_sources);
}
