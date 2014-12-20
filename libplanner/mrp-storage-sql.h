/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2003 Mikael Hallendal <micke@imendio.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MRP_STORAGE_SQL_H__
#define __MRP_STORAGE_SQL_H__

#include <glib-object.h>
#include "mrp-storage-module.h"
#include "mrp-types.h"
#include "mrp-project.h"

extern GType mrp_storage_sql_type;

#define MRP_TYPE_STORAGE_SQL		mrp_storage_sql_type
#define MRP_STORAGE_SQL(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MRP_TYPE_STORAGE_SQL, MrpStorageSQL))
#define MRP_STORAGE_SQL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MRP_TYPE_STORAGE_SQL, MrpStorageSQLClass))
#define MRP_IS_STORAGE_SQL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MRP_TYPE_STORAGE_SQL))
#define MRP_IS_STORAGE_SQL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MRP_TYPE_STORAGE_SQL))
#define MRP_STORAGE_SQL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MRP_TYPE_STORAGE_SQL, MrpStorageSQLClass))

typedef struct _MrpStorageSQL      MrpStorageSQL;
typedef struct _MrpStorageSQLClass MrpStorageSQLClass;

struct _MrpStorageSQL {
	MrpStorageModule  parent;
	MrpProject       *project;
};

struct _MrpStorageSQLClass {
	MrpStorageModuleClass parent_class;
};


void mrp_storage_sql_register_type (GTypeModule *module);

#endif /* __MRP_STORAGE_SQL_H__ */
