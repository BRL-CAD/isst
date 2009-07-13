/*                 A D R T  - O P T I M I Z E . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2009 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

/** @file adrt-optimize.c
 *
 * Brief description
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <inttypes.h>
#include "tie/tie.h"

#define MYSQL_USER	"adrt"
#define MYSQL_PASS	"adrt"
#define MYSQL_DB	"adrt"

#define STATUS(_s) { \
    fprintf(stdout, "                                                \r"); \
    fprintf(stdout, "Status: %s\r", _s); \
    fflush(stdout); }

static void optimize(char *mysql_hostname, char *gid) {
    MYSQL mysql_db;
    MYSQL_RES *res;
    MYSQL_ROW row;
    uint32_t trinum, meshnum, vnum, gind, tnum, cache_size, i;
    uint8_t ftype;
    char *name, query[256], status[40];
    void *gdata, *cache;
    tie_t tie;
    TIE_3 *vlist, **tlist;

    /* Initialize mysql database */
    mysql_init(&mysql_db);

    if(!mysql_real_connect(&mysql_db, mysql_hostname, MYSQL_USER, MYSQL_PASS, MYSQL_DB, 0, 0, 0)) {
	printf("Connection failed\n");
	return;
    }

    /* Form query string */
    sprintf(query, "select name,trinum,meshnum,gdata from geometry where gid='%d'", atoi(gid));
    mysql_query(&mysql_db, query);
    res = mysql_use_result(&mysql_db);
    row = mysql_fetch_row(res);
    if(!row) {
	fprintf(stderr, "error: geometry id doesn't exist.\n");
	return;
    }

    name = (char *)row[0];
    trinum = atoi(row[1]);
    meshnum = atoi(row[2]);
    gdata = row[3];
    gind = 2; /* skip over endian */
    tlist = 0;
    tnum = 0;

    printf("Optimizing: %s (%d triangles)\n", name, trinum);

    /* Initialize TIE and Load Geometry. */
    tie_init(&tie, trinum, TIE_KDTREE_OPTIMAL);

    /* Load triangles from each mesh into tie. */
    for(i = 0; i < meshnum; i++) {
	sprintf(status, "Loading Geometry (mesh %d of %d)", i+1, meshnum);
	STATUS(status);

	{
	    uint8_t c;
	    char *mesh;

	    c = ((uint8_t *)gdata)[gind];
	    gind += 1;

	    mesh = &((uint8_t *)gdata)[gind];
	    gind += c;
	}

	memcpy(&vnum, &((uint8_t *)gdata)[gind], sizeof(uint32_t));
	gind += sizeof(uint32_t);   

	vlist = (TIE_3 *)&((uint8_t *)gdata)[gind];
	gind += vnum * sizeof(TIE_3);

	ftype = ((uint8_t *)gdata)[gind];
	gind += 1;

	if(ftype) {
	    uint32_t fnum, *flist, n;

	    memcpy(&fnum, &((uint8_t *)gdata)[gind], sizeof(uint32_t));
	    gind += sizeof(uint32_t);
	    flist = (uint32_t *)&((uint8_t *)gdata)[gind];
	    gind += fnum * 3 * sizeof(uint32_t);

	    if(fnum > tnum) {
		tnum = fnum;
		tlist = (TIE_3 **)realloc(tlist, 3 * tnum * sizeof(TIE_3 *));
	    }
	    for(n = 0; n < 3*fnum; n++)
		tlist[n] = &vlist[flist[n]];

	    tie_push(&tie, tlist, fnum, 0, 0);
	} else {
	    uint32_t n;
	    uint16_t fnum, *flist;

	    memcpy(&fnum, &((uint8_t *)gdata)[gind], sizeof(uint16_t));
	    gind += sizeof(uint16_t);
	    flist = (uint16_t *)&((uint8_t *)gdata)[gind];
	    gind += fnum * 3 * sizeof(uint16_t);

	    if(fnum > tnum) {
		tnum = fnum;
		tlist = (TIE_3 **)realloc(tlist, 3 * tnum * sizeof(TIE_3 *));
	    }

	    for(n = 0; n < 3*fnum; n++)
		tlist[n] = &vlist[flist[n]];

	    tie_push(&tie, tlist, fnum, 0, 0);
	}
    }

    /* Free the geometry data */
    mysql_free_result(res);

    STATUS("Optimizing");
    tie_prep(&tie);

    STATUS("Building Data");
    cache_size = tie_kdtree_cache_free(&tie, &cache);

    STATUS("Saving Data");
    {
	void *cache_escaped;
	uint32_t s;

	cache_escaped = malloc(2*cache_size);

	sprintf(cache_escaped, "update geometry set adata='");
	s = strlen(cache_escaped);
	s += mysql_real_escape_string(&mysql_db, &((uint8_t *)cache_escaped)[s], cache, cache_size);

	sprintf(query, "' where gid='%d'", atoi(gid));
	strcat(&((uint8_t *)cache_escaped)[s], query);
	s += strlen(query);

	mysql_real_query(&mysql_db, cache_escaped, s);
	sprintf(query, "update geometry set asize='%d' where gid='%d'", cache_size, atoi(gid));
	mysql_query(&mysql_db, query);

	free(cache_escaped);
    }

    sprintf(status, "Complete (%d bytes)", cache_size);
    STATUS(status);

    free(cache);
    tie_free(&tie);
    mysql_close(&mysql_db);

    printf("\n");
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
	fprintf(stderr, "Usage: adrt-optimize mysql_hostname geometry_id\n");
	return(1);
    }

    optimize(argv[1], argv[2]);

    return(0);
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
*/
