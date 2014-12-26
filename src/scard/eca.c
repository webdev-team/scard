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
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <libecasoundc/ecasoundc.h>
#include "main.h"
#include "eca.h"

/* prototypes */
int		eca_create_chains(void);
static void	eca_build_command(char *cmd, char *arg);
int		eca_set_destination(char *source, char *dest);
void		eca_set_format(void);

/* 
 * add audio input in a specified chain
 */

void	eca_add_source(char *source)
{
	char    **tmp = NULL;

	eca_build_command("c-add", source);
	eca_build_command("c-select", source);
	tmp = hash_get(g_conf.record, source);
	assert(tmp != NULL);
	assert(tmp[0] != NULL);

	/*
	 * 0 is input (/dev/dsp, ...)
	 * 1 is codec (wav, mp3, etc...)
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
 * this will build the file path for ao-add
 * ie: recordpath + source + date + prefix + hour.format
 * ex: /tmp/channel1/22112010/record06.mp3
 */

char	*eca_build_output_path(char *source) {
	int		len;
	time_t		now;
	struct tm	*info;
	char		*path = NULL;
	char		*pos;
	char		*prefix = NULL;
	char		*extension;
	char		**tmp;

	now = time(NULL);
	if (now == -1) {
		log_err("[t] time() error: %s\n", strerror(errno));
		return NULL;
	}

	info = localtime(&now);
	if (info == NULL) {
		log_err("[t] localtime() error: %s\n", strerror(errno));
		return NULL;
	}

	prefix = hash_text_get_first(g_conf.global, "prefix");
	if (prefix == NULL)
		prefix = "";

	/*
	 * this part remove format parameter from format
	 * ex: mp3,128000 -> mp3
	 */

	assert(hash_text_count_data(g_conf.record, source) == 2);
	tmp = hash_get(g_conf.record, source);
	assert(tmp != NULL);
	assert(tmp[1] != NULL);

	extension = strdup(tmp[1]);
	if (extension == NULL) {
		log_err("[h] strdup() error: %s\n", strerror(errno));
		return NULL;
	}

	if ((pos = strchr(extension, ',')) != NULL)
		*pos = '\0';

	len = asprintf(&path, "%s/%s/%02d%02d%i/%s%02d.%s", 
			hash_text_get_first(g_conf.global, "recordpath"),
			source, info->tm_mday, info->tm_mon + 1, 
			info->tm_year + 1900, prefix, info->tm_hour, extension);

	if (len == -1)
		return NULL;

	free(extension);
	return(path);
}

/*
 * check if a specified path exists
 * create it if it doesn't.
 * return 0 on succes
 * return -1 on error
 */

int	eca_check_output_path(char *path)
{
	char		*dir, *p, *tmp;
	int		error;
	struct stat    	st;

	/* duplicate dir to do some stuff in it */
	tmp = strdup(path);
	if (tmp == NULL) {
		log_err("[c] strdup() error: %s\n", strerror(errno));	
		return ERROR;
	}

	dir = dirname(tmp);
	assert(dir != NULL);
	error = stat(dir, &st);
        if (! error) {
		if (S_ISDIR (st.st_mode) == 0) {
			log_err("[e] %s exists and is not a directory !\n");
			free(tmp);
			return ERROR;
		}
		else {
			free(tmp);
			return NOERROR;
		}
	}

	/* avoid leading garbage if any */
	p = tmp;	
	while (*p == '/')
	     p++;

	/* create parents dirs */
	while ((p = strchr(p, '/'))) {
		*p = '\0';
		if (stat(tmp, &st) != 0) {
			if (mkdir(tmp, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)) {
				log_err("[c] mkdir() error: %s\n", strerror(errno));	
				goto error;
			}
		}
		else if (S_ISDIR (st.st_mode) == 0) {
			log_err("[e] %s exists and is not a directory !\n");
			goto error;
		}

		*p++ = '/';
		while (*p == '/')
			p++;
	}

	/* create final dir */
	if (stat(tmp, &st) != 0) {
		if (mkdir(tmp, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)) {
			log_err("[c] mkdir() error: %s\n", strerror(errno));
			goto error;
		}
	}
	else if (S_ISDIR (st.st_mode) == 0) {
			log_err("[e] %s exists and is not a directory !\n");
			goto error;
	}
	else
		return NOERROR;

error:
	free(dir);
	return ERROR;
}

/*
 * init ecasound and check if ecasound is ready
 * return 0 on succes
 * return -1 on error
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
	char		*path;

	/* create chainsetup and set audio format */
	eca_build_command("cs-add", "scard");
	eca_set_format();
	while (g_sources[i] != NULL) {
		/* add input */
		eca_add_source(g_sources[i]);
		if ((path = eca_build_output_path(g_sources[i])) == NULL) 
			return ERROR;

		eca_set_destination(g_sources[i], path);
		free(path);
		i++;
	}

	return NOERROR;
}

int	eca_check_status() {
	eci_command("engine-status");
	if (g_mode & DEBUG)
		log_msg("[e] ecasound engine status: %s\n", eci_last_string());

	return NOERROR;
}

int	eca_set_destination(char *source, char *dest)
{
	eca_build_command("c-select", source);
	eca_check_output_path(dest);
	eca_build_command("ao-add", dest);
}

/*
 * set audio format for current chain
 */

void	eca_set_format() {
	char	*format;

	format = hash_text_get_first(g_conf.global, "format");
	if (format == NULL)
		return ;

	eca_build_command("cs-set-audio-format", format);
	if (g_mode & DEBUG)
		log_msg("[e] audio format set to: %s\n", format);
}
