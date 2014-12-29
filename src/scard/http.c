/*
 *  http.c
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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include "main.h"
#include "http.h"

/* global variables needed for getopt */
struct	evhttp	*ev_httpd = NULL;

/* prototypes */
static void	http_dump_cb(struct evhttp_request *req, void *arg);

void	http_cleanup()
{
	if (ev_httpd != NULL)
		evhttp_free(ev_httpd);
}

/*
 * this is for debuging only
 * inspired by https://github.com/libevent/libevent/blob/master/sample/http-server.c
 * 
 * dump output on stderr only.
 */ 

static void	http_dump_cb(struct evhttp_request *req, void *arg)
{
	const char		*cmd;
	char			*line;
	struct evkeyvalq	*headers;
	struct evkeyval		*header;
	struct evbuffer		*buf;

	(void)arg;

	switch (evhttp_request_get_command(req)) {
		case EVHTTP_REQ_GET: cmd = "GET"; break;
		case EVHTTP_REQ_POST: cmd = "POST"; break;
		case EVHTTP_REQ_HEAD: cmd = "HEAD"; break;
		case EVHTTP_REQ_PUT: cmd = "PUT"; break;
		case EVHTTP_REQ_DELETE: cmd = "DELETE"; break;
		case EVHTTP_REQ_OPTIONS: cmd = "OPTIONS"; break;
		case EVHTTP_REQ_TRACE: cmd = "TRACE"; break;
		case EVHTTP_REQ_CONNECT: cmd = "CONNECT"; break;
		case EVHTTP_REQ_PATCH: cmd = "PATCH"; break;
		default: cmd = "unknown"; break;
	}

	fprintf(stderr, "[h] %s request on %s...\n[h] ... headers:\n",
		    cmd, evhttp_request_get_uri(req));

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header; header = header->next.tqe_next)
		fprintf(stderr, "<<< \t%s: %s\n", header->key, header->value);

	buf = evhttp_request_get_input_buffer(req);
	fprintf(stderr, "[h] ... data:\n");
	while ((line = evbuffer_readln(buf, NULL, EVBUFFER_EOL_ANY)) != NULL) {
		fprintf(stderr, "<<< \t%s\n", line);
		free(line);
	}

	evhttp_send_reply(req, 200, "OK", NULL);
}

/*
 * init libevent http engine
 */

int	http_init(struct event_base *base)
{
	assert(base != NULL);

	if (! hash_text_is_true(g_conf.http, "webgui"))
		return NOERROR;

	ev_httpd = evhttp_new(base);
	if (! ev_httpd)
		log_err("[h] evhttp_start() error\n");

	if (evhttp_bind_socket(ev_httpd, "0.0.0.0", 8080) == -1) {
		log_err("[h] cannot bind HTTP socket\n");
		return ERROR;
	}

	if (g_mode & VERBOSE)
		log_msg("[h] WebGUI started\n");

	/* register /dump URI on debug mode only */
	if (g_mode & DEBUG)
		evhttp_set_cb(ev_httpd, "/dump", http_dump_cb, NULL);

	return NOERROR;
}
