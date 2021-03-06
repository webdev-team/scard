/*
 *  main.h
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

#ifndef __SCARD_H__
#define __SCARD_H__

/* global includes */
#include "conf.h"

/* from init.c */
int     init();

/* from log.c */
void    log_from_libevent(int level, char *str);
void    log_init(void);
void    log_cleanup(void);
void    log_msg(char *msg, ...);
void    log_err(char *msg, ...);

/* log macros */
#define log_eci_command(cmd)\
	if (g_mode & DEBUG) \
		log_msg(">>> %s\n", cmd);\
	eci_command(cmd);

/* misc .c */
void    sig_set_handlers(void);

/* global variables */
extern unsigned int	g_mode;		/* runing mode */
extern char		*g_file;	/* configuration file */
extern conf_t		g_conf;

/* sources struct */
typedef struct		sources {
	char	*name;
	char	*file;
}			sources_t;

extern sources_t	*g_sources;	/* sources list */

/* gmode defines */
#define	DAEMON		(1 << 0)
#define	VERBOSE		(1 << 1)
#define	USE_SSL		(1 << 2)
#define	STARTING	(1 << 3)
#define SHUTDOWN	(1 << 4)
#define DEBUG		(1 << 8)
#define	SYSLOG		(1 << 10)

/* errors code */
#define	NOERROR		0
#define	ERROR		-1
#define	NOSERVER	-2

/* others defines */
#define	BUFF_SIZE	4096
#ifndef VERSION
#define	VERSION		"0.0.1"
#endif

#endif
