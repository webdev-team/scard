/*
 *  eca.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libecasoundc/ecasoundc.h>
#include "main.h"
#include "eca.h"

/* prototypes */
int		eca_create_chains(void);
static void	eca_build_command(char *cmd, char *arg);

/* 
 * add audio input in a specified chain
 */

void	eca_add_input(char *source)
{
	char    **tmp = NULL;

	eca_build_command("c-select", source);
	tmp = hash_get(g_conf.record, source);
	assert(tmp != NULL);
	assert(tmp[0] != NULL);

	/*
	 * 0 is input (/dev/dsp, ...)
	 * 1 is format (16,2,48000, ...)
	 */

	eca_build_command("ai-add", tmp[0]);
}

/*
 * this is basicaly just a string helper function
 * with some basic checks.
 * 
 * it build the command to be send to eci_command()
 * and exec it through log_eci_command()
 */ 

static void	eca_build_command(char *cmd, char *arg)
{
	size_t	cl, al;

	cl = strlen(cmd);
	al = strlen(arg);
	assert(cl > 0);
	assert(al > 0);

	char vla[cl + 1 + al + 1];
	sprintf(vla, "%s %s", cmd, arg);
	log_eci_command(vla);
}	

/*
 * init ecasound and check if ecasound is ready
 * return -1 on error
 * return 0 on succes
 */ 

int	eca_init() {
	int	status;

	eci_init();
	status = eci_ready();

	if (status == 0)
		return ERROR;

	eca_create_chains();
	return NOERROR;
}

/*
 * cleanup ecasound stuff
 */

void	eca_cleanup() {
	eci_cleanup();
}

/*
 * create chains in ecasound
 * use source name as chain name
 */

int	eca_create_chains() {
	unsigned int	i = 0;

	while (g_sources[i] != NULL) {
		/* create chain */
		eca_build_command("c-add", g_sources[i]);

		/* add input */
		eca_add_input(g_sources[i]);

		i++;
	}
}

int	eca_check_status() {
	eci_command("engine-status");
	if (g_mode & DEBUG)
		log_msg("[e] ecasound engine status: %s\n", eci_last_string());

	return NOERROR;
}
