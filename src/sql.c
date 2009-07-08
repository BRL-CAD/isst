/*                          S Q L . C
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

/** @file sql.c
 *
 * Brief description
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mysql.h>

#include <bu.h>

#include "tie/tie.h"
#include "tie/adrt.h"

#include "isst.h"

#include "sql.h"

#define	ISST_MYSQL_USER		"adrt"
#define	ISST_MYSQL_PASS		"adrt"
#define ISST_MYSQL_DB		"adrt"

static int sql_connected = 0;

int
sql_connect(char *host)
{
	sql_connected = mysql_real_connect(&isst.mysql_db, bu_vls_addr(&isst.database), ISST_MYSQL_USER, ISST_MYSQL_PASS, ISST_MYSQL_DB, 0, 0, 0)?1:0;
	return sql_connected;
}

int
sql_verify(char *username, char *passwd){
	int uid = 1;
#ifndef HAX
	MYSQL_RES *res;
	MYSQL_ROW row;

	sprintf(query, "select (select password from user where username = '%s') = Password('%s')", isst.username, isst.password);
	if(mysql_query(&isst.mysql_db, query)) {
		gtk_label_set_text(GTK_LABEL (status), "Status: Database Nonexistent");
		mysql_close(&isst.mysql_db);
		return;
	}

	res = mysql_use_result(&isst.mysql_db);
	row = mysql_fetch_row(res);

	if(row[0] == NULL || !strcmp("NULL", row[0])) {
		gtk_label_set_text(GTK_LABEL (status), "Status: Authentication Failed");
		mysql_close(&isst.mysql_db);
		return;
	}
	
	sprintf(query, "select uid from user where username = '%s'", GTK_ENTRY (username)->text);
	mysql_query(&isst.mysql_db, query);

	res = mysql_use_result (&isst.mysql_db);
	row = mysql_fetch_row (res);
	uid = atoi (row[0]);
	mysql_free_result (res);
#endif
	return uid;
}

int
sql_close()
{
	mysql_close(&isst.mysql_db);
	return 0;
}

struct proj_s *
sql_projects(int uid)
{
	char query[BUFSIZ];
	MYSQL_RES *res;
	MYSQL_ROW row;
	struct proj_s *ret = NULL, *p;
	int i;

	sprintf (query, "select pid,name from project where uid='%d'", isst.uid);
	mysql_query (&isst.mysql_db, query);
	res = mysql_use_result (&isst.mysql_db);

	while ( row = mysql_fetch_row (res) ) {
		if(ret == NULL)
			ret = p = (struct proj_s *)malloc(sizeof(struct proj_s));
		else {
			p->next = (struct proj_s *)malloc(sizeof(struct proj_s));
			p = p->next;
		}
		p->id = atoi(row[0]);
		strncpy(p->name, row[1], 64);
		p->next = NULL;
	}
	mysql_free_result (res);
	return ret;
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
