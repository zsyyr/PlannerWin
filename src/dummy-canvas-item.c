/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * Copyright (C) 2009 Maurice van der Pot <griffon26@kfk4ever.com>
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
 * Free Software Foundation, Inc., 59 Temgle Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* Dummy item type for GnomeCanvas widget
 *
 * This dummy item can be used to manipulate the bounding box returned by the
 * get_bounds function of the canvas root. It has no visible shape, but is
 * included in bounding box calculations.
 *
 * Author: Maurice van der Pot <griffon26@kfk4ever.com>
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include <libgnomecanvas/gnome-canvas-util.h>
#include "dummy-canvas-item.h"


enum {
	PROP_0,
	PROP_X1,
	PROP_Y1,
	PROP_X2,
	PROP_Y2
};


static void gnome_canvas_dummy_class_init   (GnomeCanvasDummyClass *class);
static void gnome_canvas_dummy_init         (GnomeCanvasDummy      *dummy);
static void gnome_canvas_dummy_destroy      (GtkObject             *object);
static void gnome_canvas_dummy_set_property (GObject               *object,
					     guint                  param_id,
					     const GValue          *value,
					     GParamSpec            *pspec);
static void gnome_canvas_dummy_get_property (GObject               *object,
					     guint                  param_id,
					     GValue                *value,
					     GParamSpec            *pspec);

static void   gnome_canvas_dummy_update      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void   gnome_canvas_dummy_realize     (GnomeCanvasItem *item);
static void   gnome_canvas_dummy_unrealize   (GnomeCanvasItem *item);
static void   gnome_canvas_dummy_draw        (GnomeCanvasItem *item, GdkDrawable *drawable,
					     int x, int y, int width, int height);
static double gnome_canvas_dummy_point       (GnomeCanvasItem *item, double x, double y,
					     int cx, int cy, GnomeCanvasItem **actual_item);
static void   gnome_canvas_dummy_bounds      (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2);
static void   gnome_canvas_dummy_render      (GnomeCanvasItem *item, GnomeCanvasBuf *buf);


static GnomeCanvasItemClass *parent_class;


GType
gnome_canvas_dummy_get_type (void)
{
	static GType dummy_type;

	if (!dummy_type) {
		static const GTypeInfo object_info = {
			sizeof (GnomeCanvasDummyClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gnome_canvas_dummy_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,			/* class_data */
			sizeof (GnomeCanvasDummy),
			0,			/* n_preallocs */
			(GInstanceInitFunc) gnome_canvas_dummy_init,
			NULL			/* value_table */
		};

		dummy_type = g_type_register_static (GNOME_TYPE_CANVAS_ITEM, "GnomeCanvasDummy",
						     &object_info, 0);
	}

	return dummy_type;
}

static void
gnome_canvas_dummy_class_init (GnomeCanvasDummyClass *class)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = g_type_class_peek_parent (class);

	gobject_class->set_property = gnome_canvas_dummy_set_property;
	gobject_class->get_property = gnome_canvas_dummy_get_property;

        g_object_class_install_property
                (gobject_class,
                 PROP_X1,
                 g_param_spec_double ("x1", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y1,
                 g_param_spec_double ("y1", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_X2,
                 g_param_spec_double ("x2", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y2,
                 g_param_spec_double ("y2", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	object_class->destroy = gnome_canvas_dummy_destroy;

	item_class->update = gnome_canvas_dummy_update;
	item_class->realize = gnome_canvas_dummy_realize;
	item_class->unrealize = gnome_canvas_dummy_unrealize;
	item_class->draw = gnome_canvas_dummy_draw;
	item_class->point = gnome_canvas_dummy_point;
	item_class->bounds = gnome_canvas_dummy_bounds;

	item_class->render = gnome_canvas_dummy_render;
}

static void
gnome_canvas_dummy_init (GnomeCanvasDummy *dummy)
{
	dummy->x1 = 0.0;
	dummy->y1 = 0.0;
	dummy->x2 = 0.0;
	dummy->y2 = 0.0;
}

static void
gnome_canvas_dummy_destroy (GtkObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_DUMMY (object));

	/* remember, destroy can be run multiple times! */

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* Computes the bounding box of the dummy object.
 */
static void
get_bounds (GnomeCanvasDummy *dummy, double *bx1, double *by1, double *bx2, double *by2)
{
	GnomeCanvasItem *item;
	gdouble          wx1, wy1, wx2, wy2;
	gint             cx1, cy1, cx2, cy2;

	item = GNOME_CANVAS_ITEM(dummy);

	wx1 = dummy->x1;
	wy1 = dummy->y1;
	wx2 = dummy->x2;
	wy2 = dummy->y2;

	gnome_canvas_item_i2w (item, &wx1, &wy1);
	gnome_canvas_item_i2w (item, &wx2, &wy2);
	gnome_canvas_w2c (item->canvas, wx1, wy1, &cx1, &cy1);
	gnome_canvas_w2c (item->canvas, wx2, wy2, &cx2, &cy2);

	*bx1 = cx1;
	*by1 = cy1;
	*bx2 = cx2;
	*by2 = cy2;
}

/* Computes the bounding box of the dummy, in canvas coordinates.
 */
static void
get_bounds_canvas (GnomeCanvasDummy *dummy, double *bx1, double *by1, double *bx2, double *by2, double affine[6])
{
	ArtDRect bbox_world;
	ArtDRect bbox_canvas;

	get_bounds (dummy, &bbox_world.x0, &bbox_world.y0, &bbox_world.x1, &bbox_world.y1);

	art_drect_affine_transform (&bbox_canvas, &bbox_world, affine);
	/* include 1 pixel of fudge */
	*bx1 = bbox_canvas.x0 - 1;
	*by1 = bbox_canvas.y0 - 1;
	*bx2 = bbox_canvas.x1 + 1;
	*by2 = bbox_canvas.y1 + 1;
}

static void
gnome_canvas_dummy_set_property (GObject              *object,
				guint                 param_id,
				const GValue         *value,
				GParamSpec           *pspec)
{
	GnomeCanvasItem *item;
	GnomeCanvasDummy *dummy;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_DUMMY (object));

	item = GNOME_CANVAS_ITEM (object);
	dummy = GNOME_CANVAS_DUMMY (object);

	switch (param_id) {
	case PROP_X1:
		dummy->x1 = g_value_get_double (value);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_Y1:
		dummy->y1 = g_value_get_double (value);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_X2:
		dummy->x2 = g_value_get_double (value);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_Y2:
		dummy->y2 = g_value_get_double (value);
		gnome_canvas_item_request_update (item);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_canvas_dummy_get_property (GObject              *object,
				guint                 param_id,
				GValue               *value,
				GParamSpec           *pspec)
{
	GnomeCanvasDummy *dummy;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_DUMMY (object));

	dummy = GNOME_CANVAS_DUMMY (object);

	switch (param_id) {
	case PROP_X1:
		g_value_set_double (value,  dummy->x1);
		break;

	case PROP_Y1:
		g_value_set_double (value,  dummy->y1);
		break;

	case PROP_X2:
		g_value_set_double (value,  dummy->x2);
		break;

	case PROP_Y2:
		g_value_set_double (value,  dummy->y2);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_canvas_dummy_render (GnomeCanvasItem *item,
			     GnomeCanvasBuf *buf)
{
}

static void
gnome_canvas_dummy_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasDummy *dummy;
	double x1, y1, x2, y2;

	dummy = GNOME_CANVAS_DUMMY (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	if (item->canvas->aa) {
		g_assert(FALSE);
	} else {
		get_bounds_canvas (dummy, &x1, &y1, &x2, &y2, affine);
		gnome_canvas_update_bbox (item, x1, y1, x2, y2);
	}
}

static void
gnome_canvas_dummy_realize (GnomeCanvasItem *item)
{
	if (parent_class->realize)
		(* parent_class->realize) (item);
}

static void
gnome_canvas_dummy_unrealize (GnomeCanvasItem *item)
{
	if (parent_class->unrealize)
		(* parent_class->unrealize) (item);
}

static void
gnome_canvas_dummy_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			int x, int y, int width, int height)
{
}

static double
gnome_canvas_dummy_point (GnomeCanvasItem *item, double x, double y,
			 int cx, int cy, GnomeCanvasItem **actual_item)
{
	*actual_item = item;

	/* Always return a distance > 0 so we don't receive events */
	return 1.0;
}

static void
gnome_canvas_dummy_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GnomeCanvasDummy *dummy;

	dummy = GNOME_CANVAS_DUMMY (item);

	get_bounds (dummy, x1, y1, x2, y2);
}
