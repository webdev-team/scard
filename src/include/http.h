/*
 *  http.h
 *  scard
 *
 *  Created by Michel Depeige on 29/12/2014.
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

#ifndef __HTTP_H__
#define __HTTP_H__

/* prototypes */
void	http_cleanup(void);
int	http_init(struct event_base *base);

#endif
