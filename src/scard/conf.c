/*
 *  conf.c
 *  scard
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

#ifdef	HAVE_CONFIG_H
#include	"config.h"
#endif
#include 	<assert.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<stdint.h>
#include	<stdlib.h>
#define _GNU_SOURCE
#include	<string.h>
#include	<unistd.h>
#include	<errno.h>
#include	"conf.h"
#include	"main.h"

#define	MAIN_INITIAL_CAPACITY	16
#define	SUB_INITIAL_CAPACITY	4
#define	SEC_NONE	0
#define SEC_GLOBAL	1
#define SEC_RECORD	2
#define SEC_HTTP	3

/* prototypes */
char	*conf_search(void);
void	conf_ini_check_parameter(char *str, hash_t *mem, int line);
short	conf_ini_check_section(char *str, int line);
int	conf_ini_read(FILE *f, conf_t *conf);
int	conf_ini_save(conf_t *mem, char *file);
int	conf_ini_save_section(hash_t *mem, FILE *out);

/*
 *	Remove the current configuration from memory
 *	usefull before quiting, or before reloading the conf.
 */

void	conf_erase(conf_t *conf)
{
	hash_text_erase(conf->global);
	hash_text_erase(conf->record);
	hash_text_erase(conf->http);
}

/*
 *	reinit configuration
 */

int	conf_reinit(conf_t *conf)
{
	conf_erase(conf);
	return conf_init(conf);
}

/*
 *	Dump the current configuration in stdout
 *	usefull for the test stuff program
 */

void	conf_dump(conf_t *conf)
{
	conf_dump_section(conf->global);
	conf_dump_section(conf->record);
	conf_dump_section(conf->http);
}

void	conf_dump_section(hash_t *mem)
{
	char		**data;
	struct record_s	*recs;
	unsigned int	c, i, j;

	printf("===\n");
	/* check if we have something */
	if (! mem)
		return ;

	recs = mem->records;
	for (i = 0, c = 0; c < mem->records_count; i++) {
		if (recs[i].hash != 0 && recs[i].key) {
			c++;
			printf("--- %s: ", recs[i].key);
			data = recs[i].value;
			for (j = 0; data[j] != NULL; j++)
				printf("\"%s\" ", data[j]);
			printf("\n");
		}
	}
}

/*
 *	Read the configuration from the file
 *	return 0 if no errors
 *	return -1 on errors
 */

int	conf_init(conf_t *conf)
{
	hash_t	*hash;

	conf->global = NULL;
	hash = hash_new(MAIN_INITIAL_CAPACITY);
	if (hash == NULL)
		return -1;
	conf->global = hash;
	conf->record = NULL;
	hash = hash_new(MAIN_INITIAL_CAPACITY);
	if (hash == NULL)
		return -1;
	conf->record = hash;
	conf->http = NULL;
	hash = hash_new(MAIN_INITIAL_CAPACITY);
	if (hash == NULL)
		return -1;
	conf->http = hash;

	return 0;
}

/*
 * search for existing configuration files
 * return an allocated string containing the path of the configuration file
 * on error: return NULL
 */

char	*conf_search(void)
{
	FILE	*conf;
	char	*tmp;
	char	*dup;

	/* if no file given, try env, then home directy, then system side */
	tmp = getenv(CONF_ENV);
	if (tmp != NULL) {
		dup = strdup(tmp);
		if (dup != NULL) {
			conf = fopen(dup, "r");
			if (conf) {
				fclose(conf);
				return dup;
			}
			free(dup);
		}
	}
#if !defined (_WIN32) && !defined (_WIN64)
	tmp = getenv("HOME");
	if (tmp != NULL) {
		dup = strdup(tmp);
		if (dup != NULL) {
			tmp = malloc((strlen(dup) + \
					strlen(CONF_HOME) + 1) * sizeof(*tmp));
			if (tmp != NULL) {
				strcpy(tmp, dup);
				strcpy(tmp + strlen(dup), CONF_HOME);
				conf = fopen(tmp, "r");
				free(tmp);
				if (conf) {
					fclose(conf);
					return dup;
				}
				free(dup);
			}
			else {
				free(dup);
			}
		}
	}
#endif
	dup = strdup(DEFAULT_CONF);
	if (dup == NULL) {
		log_err("[c] strdup() error: %s\n", strerror(errno));
		return NULL;
	}
	conf = fopen(dup, "r");
	if (conf) {
		fclose(conf);
		return dup;
	}
	return NULL;
}

int	conf_read(conf_t *mem, char *file)
{
	FILE	*conf;
	char	*found = NULL;
	int	error = 0;

	/* if no file given, try env, then home directy, then system side */
	if (file == NULL) {
		found = conf_search();
		if (found == NULL) {
			log_err("[c] cannot find any configuration file\n");
			return -1;
		}
	}

	if (file == NULL)
		file = found;
	conf = fopen(file, "r");
	if (conf == NULL) {
		log_err("[c] cannot open configuration file: %s\n", strerror(errno));
		return -1;
	}
	if (g_mode & VERBOSE)
		log_msg("[c] using configuration file: %s\n", file);

	error = conf_ini_read(conf, mem);
	fclose(conf);
	free(found);
	return 0;
}

/* load an init file */
int	conf_ini_read(FILE *f, conf_t *conf)
{
	char		tmp[BUFF_SIZE];
	short		section = SEC_NONE;
	unsigned int	line = 0;
	char		*dup;

	while (fgets(tmp, BUFF_SIZE, f) != NULL) {
		/* skip comments */
		if (tmp[0] == '#' || tmp[0] == '\r' || tmp[0] == '\n' || \
		    tmp[0] == ';' || tmp[0] == '\0')
			goto next;
		/* check for sections */
		if (tmp[0] == '[') {
			section = conf_ini_check_section(tmp, line);
			goto next;
		}

		switch (section) {
			case SEC_GLOBAL: {
				dup = strdup(tmp);
				if (dup == NULL) {
					log_err("[c] strdup() error: %s\n", strerror(errno));
					break;
				}
				conf_ini_check_parameter(dup, conf->global, line);
				free(dup);
				break;
			}
			case SEC_RECORD: {
				dup = strdup(tmp);
				if (dup == NULL) {
					log_err("[c] strdup() error: %s\n", strerror(errno));
					break;
				}
				conf_ini_check_parameter(dup, conf->record, line);
				free(dup);
				break;
			}
			case SEC_HTTP: {
				dup = strdup(tmp);
				if (dup == NULL) {
					log_err("[c] strdup() error: %s\n", strerror(errno));
					break;
				}
				conf_ini_check_parameter(dup, conf->http, line);
				free(dup);
				break;
			}
			default: {
				log_msg("[c] line %i outside known section: %s", line, tmp);
			}
		}
next:
		line++;
	}
	return 0;
}

/*
 * check if an entry is valid$
 * if valid : insert entry in hash table
 * if not : print a warning
 * return nothing
 */

void	conf_ini_check_parameter(char *str, hash_t *mem, int line)
{
	char		*data[MAX_TOKENS];
	char		*key, *pos, *token;
#ifdef HAVE_STRTOK_R
	char		*saveptr;
#endif
	unsigned short	i, skip;
	size_t		len;

	/* remove comments */
	str = strsep(&str, ";");

	/* split key and data */
	key = strsep(&str, " =\t");
	if (str == NULL) {
		log_err("[c] invalid parameter declaration line %i: %s", line, key);
		return ;
	}

	/* save , between "" */
	for (i = 0, skip = 0; str[i] != '\0' ; i++) {
		if (str[i] == '"')
			skip = (++skip % 2);
		if (skip && (str[i] == ','))
			str[i] = ';';
	}

	/* split data */
	for (i = 0; i < (MAX_TOKENS - 1); i++, str = NULL) {
		#ifdef HAVE_STRTOK_R
		token = strtok_r(str, "=\t\r,\n", &saveptr);
		#else
		token = strtok(str, "=\t\r,\n");
		#endif
		if (token == NULL)
			break;

		/* skip spaces before token */
		while (*token == ' ')
			token++;

		/* remove spaces after token */
		len = strlen(token);
		while (--len && (token[len] == ' '))
			token[len] = '\0';

		data[i] = token;
	}
	data[i] = NULL;

	/* consistency checks */
	if (i < 1) {
		log_err("[c] missing data or empty line %i\n", line);
		return ;
	}

	/* insert data */
	for (i = 0; data[i] != NULL; i++) {
		/* remove leading and trailing '"' */
		len = strlen(data[i]);
		if (len > 2) {
			if (data[i][len - 1] == '"')
				data[i][len - 1] = '\0';
			if (data[i][0] == '"')
				data[i]++;
		}

		/* restore ',' */
		while (1)  {
			pos = strchr(data[i], ';');
			if (pos == '\0')
				break;
			*pos = ',';
		}

		hash_text_insert(mem, key, data[i]);
	}
}

/*
 * check if a section is valid
 * return: the section id.
 */

short	conf_ini_check_section(char *str, int line)
{
	char	*sections[] = { NULL, "global", "record", "http"};
	short	i;
	char	*end;

	str++;
	end = strchr(str, ']');
	*end = '\0';
	assert(end != NULL);
	for (i = 1; i < 4; i++) {
		if (strcmp(str, sections[i]) == 0) {
			return i;
		}
	}

	/* if we are here, then there is a syntax error */
	log_err("[c] invalid section declaration line %i: %s\n", line, str);
	return SEC_NONE;
}

/*
 *	save the current configuration to the specified file
 *	usefeull for a gui, unused otherwise
 */

int	conf_ini_save(conf_t *mem, char *file)
{
	FILE		*out;

	out = fopen(file, "w+");
	if (!out)
		return -1;

	fwrite((void *)"[global]\n", 9, 1, out);
	conf_ini_save_section(mem->global, out);
	fwrite((void *)"\n", 1, 1, out);
	fwrite((void *)"[record]\n", 9, 1, out);
	conf_ini_save_section(mem->record, out);
	fwrite((void *)"\n", 1, 1, out);
	fwrite((void *)"[http]\n", 9, 1, out);
	conf_ini_save_section(mem->http, out);
	fwrite((void *)"\n", 1, 1, out);
	fclose(out);

	return 0;
}

int	conf_ini_save_section(hash_t *mem, FILE *out)
{
	char		**data;
	struct record_s	*recs;
	unsigned int	c, i, j;

	if (! mem)
		return -1;

	recs = mem->records;
	for (i = 0, c = 0; c < mem->records_count; i++) {
		if (recs[i].hash != 0 && recs[i].key) {
			c++;
			fwrite((void *)recs[i].key, strlen(recs[i].key), 1, out);
			fwrite((void *)" = ", 3, 1, out);
			data = recs[i].value;
			for (j = 0; data[j] != NULL; j++) {
				fwrite((void *)data[j], strlen(data[j]), 1, out);
				if (data[j + 1] != NULL)
					fwrite((void *)", ", 2, 1, out);
			}
			fwrite((void *)"\n", 1, 1, out);
		}
	}

	return 0;
}
