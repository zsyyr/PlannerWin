/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Imendio AB
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PLANNER_USAGE_VIEW_H__
#define __PLANNER_USAGE_VIEW_H__

#include <gtk/gtk.h>
#include "planner-view.h"

#define PLANNER_TYPE_USAGE_VIEW	           (planner_usage_view_get_type ())
#define PLANNER_USAGE_VIEW(obj)	           (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_USAGE_VIEW, PlannerUsageView))
#define PLANNER_USAGE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), PLANNER_TYPE_USAGE_VIEW, PlannerUsageViewClass))
#define PLANNER_IS_USAGE_VIEW(obj)	   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_USAGE_VIEW))
#define PLANNER_IS_USAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_USAGE_VIEW))
#define PLANNER_USAGE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_USAGE_VIEW, PlannerUsageViewClass))

typedef struct _PlannerUsageView       PlannerUsageView;
typedef struct _PlannerUsageViewClass  PlannerUsageViewClass;
typedef struct _PlannerUsageViewPriv   PlannerUsageViewPriv;

struct _PlannerUsageView {
	PlannerView           parent;
	PlannerUsageViewPriv *priv;
};

struct _PlannerUsageViewClass {
	PlannerViewClass parent_class;
};

GType        planner_usage_view_get_type     (void) G_GNUC_CONST;
PlannerView *planner_usage_view_new          (void);

#endif /* __PLANNER_USAGE_VIEW_H__ */

