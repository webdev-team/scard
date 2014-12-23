/*
 *  env.c
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
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "main.h"
#include "env.h"

/*
 * check for ECASOUND environement variable.
 * set it to 'ecasound' if not set and nothing in configuration
 * otherwhise set it to the configuration value
 *
 * return -1 on error
 * return 0 on succes
 */ 

void	env_check_ecasound() {
	char	*value;

	value = getenv(ECASOUND);
	if (value != NULL) {
		if (g_mode & DEBUG)
			log_msg("[+] ECASOUND already set to %s, leaving as it\n", value);

		return ;
	}

	/* check for configuration value */	
	value = hash_text_get_first(g_conf.global, "ecasound");
	if (value == NULL) {
		if (g_mode & DEBUG)
			log_msg("[+] ECASOUND env not set, setting ECASOUND=ecasound\n");		
		
		if (putenv("ECASOUND=ecasound") == -1)
			log_err("[-] cannot set ECASOUND: %s\n", strerror(errno));

		return ;
        }

	if (g_mode & DEBUG)
		log_msg("[+] ECASOUND env not set, setting ECASOUND=%s\n", value);	
	if (setenv(ECASOUND, value, 1) == -1)
		log_err("[-] setenv() error: %s\n", strerror(errno));	
}
