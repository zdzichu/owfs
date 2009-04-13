/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Function naming scheme:
    OW -- Generic call to interaface
    LI -- LINK commands
    L1 -- 2480B commands
    FS -- filesystem commands
    UT -- utility functions

    Written 2003 Paul H Alfille
        Fuse code based on "fusexmp" {GPL} by Miklos Szeredi, mszeredi@inf.bme.hu
        Serial code based on "xt" {GPL} by David Querbach, www.realtime.bc.ca
        in turn based on "miniterm" by Sven Goldt, goldt@math.tu.berlin.de
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    Other portions based on Dallas Semiconductor Public Domain Kit,
    ---------------------------------------------------------------------------
    Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the "Software"),
        to deal in the Software without restriction, including without limitation
        the rights to use, copy, modify, merge, publish, distribute, sublicense,
        and/or sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following conditions:
        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
    OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
        Except as contained in this notice, the name of Dallas Semiconductor
        shall not be used except as stated in the Dallas Semiconductor
        Branding Policy.
    ---------------------------------------------------------------------------
    Implementation:
    25-05-2003 iButtonLink device
*/

#ifndef OW_SERVER_H				/* tedious wrapper */
#define OW_SERVER_H

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

struct request_packet {
	struct connection_in *owserver;
	ASCII *path;
	BYTE *data_value;
	size_t data_length;
	off_t data_offset;
	int error_code;
	int tokens;					/* for anti-loop work */
	BYTE *tokenstring;			/* List of tokens from owservers passed */
};

extern struct ow_global {
	uint32_t sg;
	int timeout_network;
	int timeout_server;
	int autoserver;
} ow_Global;

int ServerPresence(struct request_packet *rp);
int ServerRead(struct request_packet *rp);
int ServerWrite(struct request_packet *rp);
int ServerDir(void (*dirfunc) (void *, const char *), void *v, struct request_packet *rp);

#endif							/* OW_SERVER_H */
