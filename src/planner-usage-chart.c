/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio AB
 * Copyright (C) 2003 Benjamin BAYART <benjamin@sitadelle.com>
 * Copyright (C) 2003 Xavier Ordoquy <xordoquy@wanadoo.fr>
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

/* FIXME: This code needs a SERIOUS clean-up. */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <libplanner/mrp-task.h>
#include <libplanner/mrp-resource.h>
#include <libplanner/mrp-assignment.h>
#include <libplanner/mrp-group-assignment.h>
#include "planner-marshal.h"
#include "planner-usage-chart.h"
#include "planner-gantt-header.h"
#include "planner-gantt-background.h"
#include "planner-usage-model.h"
#include "planner-usage-row.h"
#include "planner-scale-utils.h"

/* Padding to the left and right of the contents of the gantt chart. */
#define PADDING 100.0

#define ZOOM_IN_LIMIT 12
#define ZOOM_OUT_LIMIT 0

#define DEFAULT_ZOOM_LEVEL 7
#define SCALE(n) (f*pow(2,(n)-19))
#define ZOOM(x) (log((x)/f)/log(2)+19)

/* Font width factor. */
static gdouble f = 1.0;

typedef struct _TreeNode TreeNode;
typedef void (*TreeFunc) (TreeNode *node, gpointer data);

struct _TreeNode {
	MrpResource      *resource;
	MrpAssignment    *assignment;
	GnomeCanvasItem  *item;
	TreeNode         *parent;
	TreeNode        **children;
	guint             num_children;
	guint             expanded:1;
};

typedef struct {
	gulong    id;
	gpointer instance;
} ConnectData;

struct _PlannerUsageChartPriv {
	GtkWidget       *header;
	GnomeCanvas     *canvas;

	GtkAdjustment   *hadjustment;
	GtkAdjustment   *vadjustment;

	GtkTreeModel    *model;
	TreeNode        *tree;
	PlannerUsageTree *view;

	GnomeCanvasItem *background;
	gdouble          zoom;
	gint             row_height;
	gdouble          height;

	mrptime          project_start;
	mrptime          last_time;

	gboolean         height_changed;
	guint            reflow_idle_id;
	GList           *signal_ids;
};

enum {
	PROP_0,
	PROP_HEADER_HEIGHT,
	PROP_ROW_HEIGHT,
	PROP_MODEL
};

enum {
	STATUS_UPDATED,
	LAST_SIGNAL
};


static void      usage_chart_class_init            (PlannerUsageChartClass *klass);
static void      usage_chart_init                  (PlannerUsageChart      *chart);
static void      usage_chart_finalize              (GObject                 *object);
static void      usage_chart_set_property          (GObject                 *object,
						     guint                    prop_id,
						     const GValue            *value,
						     GParamSpec              *spec);
static void      usage_chart_get_property          (GObject                 *object,
						     guint                    prop_id,
						     GValue                  *value,
						     GParamSpec              *spec);
static void      usage_chart_set_zoom              (PlannerUsageChart      *chart,
						     gdouble                  level);
static void      usage_chart_destroy               (GtkObject               *object);
static void      usage_chart_style_set             (GtkWidget               *widget,
						     GtkStyle                *prev_style);
static void      usage_chart_realize               (GtkWidget               *widget);
static void      usage_chart_map                   (GtkWidget               *widget);
static void      usage_chart_unrealize             (GtkWidget               *widget);
static void      usage_chart_size_allocate         (GtkWidget               *widget,
						     GtkAllocation           *allocation);
static void      usage_chart_set_adjustments       (PlannerUsageChart      *chart,
						     GtkAdjustment           *hadj,
						     GtkAdjustment           *vadj);
static void      usage_chart_reflow_now            (PlannerUsageChart      *chart);
static void      usage_chart_reflow                (PlannerUsageChart      *chart,
						     gboolean                 height_changed);
static gdouble   usage_chart_reflow_do             (PlannerUsageChart      *chart,
						     TreeNode                *root,
						     gdouble                  start_y);
static gboolean  usage_chart_reflow_idle           (PlannerUsageChart      *chart);
static gint      usage_chart_get_width             (PlannerUsageChart      *chart);
static TreeNode *usage_chart_tree_node_new         (void);
static void      usage_chart_tree_node_remove      (TreeNode                *node);
static void      usage_chart_remove_children       (PlannerUsageChart      *chart,
						     TreeNode                *node);
static void      usage_chart_tree_traverse         (TreeNode                *node,
						     TreeFunc                 func,
						     gpointer                 data);
static void      scale_func                         (TreeNode                *node,
						     gpointer                 data);
static void      usage_chart_set_scroll_region     (PlannerUsageChart      *chart,
						     gdouble                  x1,
						     gdouble                  y1,
						     gdouble                  x2,
						     gdouble                  y2);
static void      usage_chart_build_tree            (PlannerUsageChart      *chart);
static TreeNode *usage_chart_insert_resource       (PlannerUsageChart      *chart,
						     GtkTreePath             *path,
						     MrpResource             *resource);
static TreeNode *usage_chart_insert_assignment     (PlannerUsageChart      *chart,
						     GtkTreePath             *path,
						     MrpAssignment           *assign);
static TreeNode *usage_chart_insert_row            (PlannerUsageChart      *chart,
						     GtkTreePath             *path,
						     MrpResource             *resource,
						     MrpAssignment           *assign);
static void      usage_chart_tree_node_insert_path (TreeNode                *node,
						     GtkTreePath             *path,
						     TreeNode                *new_node);
static void      usage_chart_add_signal            (PlannerUsageChart      *chart,
						     gpointer                 instance,
						     gulong                   sig_id,
						     char                    *sig_name);
static void      usage_chart_disconnect_signals    (PlannerUsageChart      *chart);
static void      usage_chart_project_start_changed (MrpProject              *project,
						     GParamSpec              *spec,
						     PlannerUsageChart      *chart);
static void      usage_chart_root_finish_changed   (MrpTask                 *root,
						     GParamSpec              *spec,
						     PlannerUsageChart      *chart);
static void      show_hide_descendants              (TreeNode                *node,
						     gboolean                 show);
static void      collapse_descendants               (TreeNode                *node);
static TreeNode *usage_chart_tree_node_at_path     (TreeNode                *node,
						     GtkTreePath             *path);
static void      usage_chart_row_changed           (GtkTreeModel            *model,
						     GtkTreePath             *path,
						     GtkTreeIter             *iter,
						     gpointer                 data);
static void      usage_chart_row_inserted          (GtkTreeModel            *model,
						     GtkTreePath             *path,
						     GtkTreeIter             *iter,
						     gpointer                 data);
static void      usage_chart_row_deleted           (GtkTreeModel            *model,
						     GtkTreePath             *path,
						     gpointer                 data);


static guint signals[LAST_SIGNAL];
static GtkVBoxClass *parent_class = NULL;

GType
planner_usage_chart_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerUsageChartClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) usage_chart_class_init,
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (PlannerUsageChart),
			0,      /* n_preallocs */
			(GInstanceInitFunc) usage_chart_init
		};
		type = g_type_register_static (GTK_TYPE_VBOX,
					       "PlannerUsageChart", &info,
					       0);
	}
	return type;
}

static void
usage_chart_class_init (PlannerUsageChartClass * class)
{
	GObjectClass      *o_class;
	GtkObjectClass    *object_class;
	GtkWidgetClass    *widget_class;

	parent_class = g_type_class_peek_parent (class);

	o_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	o_class->set_property = usage_chart_set_property;
	o_class->get_property = usage_chart_get_property;
	o_class->finalize = usage_chart_finalize;

	object_class->destroy = usage_chart_destroy;

	widget_class->style_set = usage_chart_style_set;
	widget_class->realize = usage_chart_realize;
	widget_class->map = usage_chart_map;
	widget_class->unrealize = usage_chart_unrealize;
	widget_class->size_allocate = usage_chart_size_allocate;

	class->set_scroll_adjustments = usage_chart_set_adjustments;

	widget_class->set_scroll_adjustments_signal =
		g_signal_new ("set_scroll_adjustments",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (PlannerUsageChartClass,
					       set_scroll_adjustments), NULL,
			      NULL, planner_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT,
			      GTK_TYPE_ADJUSTMENT);

	signals[STATUS_UPDATED] =
		g_signal_new ("status_updated",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      0, NULL, NULL,
			      planner_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	/* Properties. */

	g_object_class_install_property (o_class,
					 PROP_MODEL,
					 g_param_spec_object ("model",
							      NULL,
							      NULL,
							      GTK_TYPE_TREE_MODEL,
							      G_PARAM_READWRITE));

	g_object_class_install_property (o_class,
					 PROP_HEADER_HEIGHT,
					 g_param_spec_int ("header-height",
							   NULL,
							   NULL,
							   0, G_MAXINT, 0,
							   G_PARAM_READWRITE));

	g_object_class_install_property (o_class,
					 PROP_ROW_HEIGHT,
					 g_param_spec_int ("row-height",
							   NULL,
							   NULL,
							   0, G_MAXINT, 0,
							   G_PARAM_READWRITE));

}

static void
usage_chart_header_date_hint_changed_cb (GtkWidget *header,
					 gchar *hint,
					 PlannerUsageChart *chart)
{
	planner_usage_chart_status_updated (chart, hint);
}

static void
usage_chart_init (PlannerUsageChart * chart)
{
	PlannerUsageChartPriv *priv;

	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (chart), FALSE);

	priv = g_new0 (PlannerUsageChartPriv, 1);
	chart->priv = priv;

	priv->tree = usage_chart_tree_node_new ();

	priv->zoom = DEFAULT_ZOOM_LEVEL;

	priv->height_changed = FALSE;
	priv->reflow_idle_id = 0;

	gtk_box_set_homogeneous (GTK_BOX (chart), FALSE);
	gtk_box_set_spacing (GTK_BOX (chart), 0);

	priv->header = g_object_new (PLANNER_TYPE_GANTT_HEADER,
				     "scale", SCALE (priv->zoom),
				     "zoom", priv->zoom, NULL);

	g_signal_connect (priv->header,
			  "date_hint_changed",
			  G_CALLBACK (usage_chart_header_date_hint_changed_cb),
			  chart);

	gtk_box_pack_start (GTK_BOX (chart), GTK_WIDGET (priv->header), FALSE,
			    TRUE,
			    0);

	priv->canvas = GNOME_CANVAS (gnome_canvas_new ());
	priv->canvas->close_enough = 5;
	gnome_canvas_set_center_scroll_region (priv->canvas, FALSE);

	/* Easiest way to get access to the chart from the canvas items. */
	g_object_set_data (G_OBJECT (priv->canvas), "chart", chart);
	gtk_box_pack_start (GTK_BOX (chart),
			    GTK_WIDGET (priv->canvas), TRUE, TRUE, 0);

	priv->row_height = -1;
	priv->height = -1;
	priv->project_start = MRP_TIME_INVALID;
	priv->last_time = MRP_TIME_INVALID;
	priv->background =
		gnome_canvas_item_new (gnome_canvas_root (priv->canvas),
				       PLANNER_TYPE_GANTT_BACKGROUND, "scale",
				       SCALE (priv->zoom), "zoom", priv->zoom,
				       NULL);
}

static TreeNode *
usage_chart_tree_node_new (void)
{
	TreeNode *node;
	node = g_new0 (TreeNode, 1);
	node->expanded = TRUE;
	return node;
}

static void
usage_chart_set_property (GObject * object,
			   guint prop_id,
			   const GValue * value, GParamSpec * spec)
{
	PlannerUsageChart *chart;

	chart = PLANNER_USAGE_CHART (object);

	switch (prop_id) {
	case PROP_MODEL:
		planner_usage_chart_set_model (chart,
						g_value_get_object (value));
		break;
	case PROP_HEADER_HEIGHT:
		g_object_set (chart->priv->header, "height",
			      g_value_get_int (value), NULL);
		break;
	case PROP_ROW_HEIGHT:
		chart->priv->row_height = g_value_get_int (value);
		usage_chart_reflow (chart, TRUE);
		break;
	default:
		break;
	}
}

static void
usage_chart_get_property (GObject * object,
			   guint prop_id, GValue * value, GParamSpec * spec)
{
	PlannerUsageChart *chart;

	chart = PLANNER_USAGE_CHART (object);

	switch (prop_id) {
	case PROP_MODEL:
		g_value_set_object (value, G_OBJECT (chart->priv->model));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
		break;
	}
}

static void
usage_chart_set_zoom (PlannerUsageChart * chart, gdouble level)
{
	PlannerUsageChartPriv *priv;

	priv = chart->priv;
	priv->zoom = level;

	usage_chart_tree_traverse (priv->tree, scale_func, chart);

	g_object_set (priv->header,
		      "scale", SCALE (priv->zoom),
		      "zoom", priv->zoom,
		      NULL);

	gnome_canvas_item_set (GNOME_CANVAS_ITEM (priv->background),
			       "scale", SCALE (priv->zoom),
			       "zoom", priv->zoom,
			       NULL);

	usage_chart_reflow_now (chart);
}

static mrptime
usage_chart_get_center (PlannerUsageChart * chart)
{
	PlannerUsageChartPriv *priv;
	gint                    x1, width, x;

	priv = chart->priv;

	gnome_canvas_get_scroll_offsets (priv->canvas, &x1, NULL);

	width = GTK_WIDGET (priv->canvas)->allocation.width;

	x = x1 + width / 2 - PADDING;
	x += floor (priv->project_start * SCALE (priv->zoom) + 0.5);

	return floor (x / SCALE (priv->zoom) + 0.5);
}

static void
usage_chart_set_center (PlannerUsageChart *chart, mrptime t)
{
	PlannerUsageChartPriv *priv;
	gint                    x1, width, x;

	priv = chart->priv;

	x = floor (t * SCALE (priv->zoom) + 0.5);

	width = GTK_WIDGET (priv->canvas)->allocation.width;

	x1 = x - width / 2 + PADDING;
	x1 -= floor (priv->project_start * SCALE (priv->zoom) + 0.5);

	gnome_canvas_scroll_to (priv->canvas, x1, 0);
}

void
planner_usage_chart_status_updated (PlannerUsageChart *chart,
				     gchar              *message)
{
	g_return_if_fail (PLANNER_IS_USAGE_CHART (chart));

	g_signal_emit (chart, signals[STATUS_UPDATED], 0, message);
}

void
planner_usage_chart_zoom_in (PlannerUsageChart *chart)
{
	PlannerUsageChartPriv *priv;
	mrptime                 mt;

	g_return_if_fail (PLANNER_IS_USAGE_CHART (chart));

	priv = chart->priv;

	mt = usage_chart_get_center (chart);
	usage_chart_set_zoom (chart, priv->zoom + 1);
	usage_chart_set_center (chart, mt);
}

void
planner_usage_chart_zoom_out (PlannerUsageChart *chart)
{
	PlannerUsageChartPriv *priv;
	mrptime                 mt;

	g_return_if_fail (PLANNER_IS_USAGE_CHART (chart));

	priv = chart->priv;

	mt = usage_chart_get_center (chart);
	usage_chart_set_zoom (chart, priv->zoom - 1);
	usage_chart_set_center (chart, mt);
}

void
planner_usage_chart_can_zoom (PlannerUsageChart *chart,
			       gboolean           *in,
			       gboolean           *out)
{
	PlannerUsageChartPriv *priv;

	g_return_if_fail (PLANNER_IS_USAGE_CHART (chart));

	priv = chart->priv;
	if (in) {
		*in = (priv->zoom < ZOOM_IN_LIMIT);
	}
	if (out) {
		*out = (priv->zoom > ZOOM_OUT_LIMIT);
	}

}

void
planner_usage_chart_zoom_to_fit (PlannerUsageChart *chart)
{
	gdouble                 t;
	gdouble                 zoom;
	gdouble                 alloc;

	g_return_if_fail (PLANNER_IS_USAGE_CHART (chart));

	t = usage_chart_get_width (chart);
	if (t == -1) {
		return;
	}

	alloc = GTK_WIDGET (chart)->allocation.width - PADDING * 2;

	zoom = planner_scale_clamp_zoom (ZOOM (alloc / t));
	usage_chart_set_zoom (chart, zoom);
}

gdouble
planner_usage_chart_get_zoom (PlannerUsageChart *chart)
{
	g_return_val_if_fail (PLANNER_IS_USAGE_CHART (chart), 0);

	return chart->priv->zoom;
}

static gint
usage_chart_get_width (PlannerUsageChart *chart)
{
	if (chart->priv->project_start == MRP_TIME_INVALID ||
	    chart->priv->last_time == MRP_TIME_INVALID) {
		return -1;
	}

	return chart->priv->last_time - chart->priv->project_start;
}

static void
usage_chart_finalize (GObject *object)
{
	PlannerUsageChart *chart;

	chart = PLANNER_USAGE_CHART (object);

	g_free (chart->priv->tree);
	g_free (chart->priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(*G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
usage_chart_destroy (GtkObject *object)
{
	PlannerUsageChart *chart;

	chart = PLANNER_USAGE_CHART (object);

	if (chart->priv->model != NULL) {
		usage_chart_disconnect_signals (chart);
		g_object_unref (chart->priv->model);
		chart->priv->model = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy) {
		(*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
	}
}

static void
usage_chart_style_set (GtkWidget *widget, GtkStyle *prev_style)
{
	PlannerUsageChart     *chart;
	PlannerUsageChartPriv *priv;
	GtkWidget              *canvas;
	PangoContext           *context;
	PangoFontMetrics       *metrics;

	if (GTK_WIDGET_CLASS (parent_class)->style_set) {
		GTK_WIDGET_CLASS (parent_class)->style_set (widget,
							    prev_style);
	}

	chart = PLANNER_USAGE_CHART (widget);
	priv = chart->priv;

	canvas = GTK_WIDGET (priv->canvas);

	context = gtk_widget_get_pango_context (canvas);
	metrics = pango_context_get_metrics (context,
					     canvas->style->font_desc, NULL);
	f = 0.2 * pango_font_metrics_get_approximate_char_width (metrics) /
		PANGO_SCALE;
	pango_font_metrics_unref(metrics);
}

static void
usage_chart_realize (GtkWidget *widget)
{
	PlannerUsageChart     *chart;
	PlannerUsageChartPriv *priv;
	GtkWidget              *canvas;
	GtkStyle               *style;
	GdkColormap            *colormap;

	chart = PLANNER_USAGE_CHART (widget);
	priv = chart->priv;

	canvas = GTK_WIDGET (priv->canvas);

	if (GTK_WIDGET_CLASS (parent_class)->realize) {
		GTK_WIDGET_CLASS (parent_class)->realize (widget);
	}

	/* Set the background to white. */
	style = gtk_style_copy (canvas->style);
	colormap = gtk_widget_get_colormap (canvas);
	gdk_color_white (colormap, &style->bg[GTK_STATE_NORMAL]);
	gtk_widget_set_style (canvas, style);
	gtk_style_unref (style);

	usage_chart_set_zoom (chart, priv->zoom);
}

static void
usage_chart_unrealize (GtkWidget *widget)
{
	if (GTK_WIDGET_CLASS (parent_class)->unrealize) {
		GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
	}
}

static void
usage_chart_map (GtkWidget * widget)
{
	PlannerUsageChart *chart;

	chart = PLANNER_USAGE_CHART (widget);

	if (GTK_WIDGET_CLASS (parent_class)->map) {
		GTK_WIDGET_CLASS (parent_class)->map (widget);
	}

	/* FIXME: Workaround for problem when changing the project length from
	 * other views. Need to fix this for real.
	 */
	usage_chart_set_zoom (chart, chart->priv->zoom);

	chart->priv->height_changed = TRUE;
	usage_chart_reflow_now (chart);
}

static void
usage_chart_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	PlannerUsageChart *chart;

	GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

	chart = PLANNER_USAGE_CHART (widget);

	if (GTK_WIDGET_MAPPED (chart)) {
		usage_chart_reflow_now (chart);
	}
}

static void
usage_chart_set_adjustments (PlannerUsageChart *chart,
			      GtkAdjustment      *hadj,
			      GtkAdjustment      *vadj)
{
	PlannerUsageChartPriv *priv;

	gboolean need_adjust = FALSE;

	g_return_if_fail (hadj == NULL || GTK_IS_ADJUSTMENT (hadj));
	g_return_if_fail (vadj == NULL || GTK_IS_ADJUSTMENT (vadj));

	priv = chart->priv;

	if (hadj == NULL) {
		hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
	}
	if (vadj == NULL) {
		vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
	}
	if (priv->hadjustment && (priv->hadjustment != hadj)) {
		g_object_unref (priv->hadjustment);
	}
	if (priv->vadjustment && (priv->vadjustment != hadj)) {
		g_object_unref (priv->vadjustment);
	}

	if (priv->hadjustment != hadj) {
		priv->hadjustment = hadj;
		g_object_ref (priv->hadjustment);
		gtk_object_sink (GTK_OBJECT (priv->hadjustment));
		gtk_widget_set_scroll_adjustments (priv->header, hadj, NULL);
		need_adjust = TRUE;
	}
	if (priv->vadjustment != vadj) {
		priv->vadjustment = vadj;
		g_object_ref (priv->vadjustment);
		gtk_object_sink (GTK_OBJECT (priv->vadjustment));
		need_adjust = TRUE;
	}

	if (need_adjust) {
		gtk_widget_set_scroll_adjustments (GTK_WIDGET (priv->canvas),
						   hadj, vadj);
	}
}

static gboolean
node_is_visible (TreeNode *node)
{
	g_return_val_if_fail (node->parent != NULL, FALSE);

	while (node->parent) {
		if (!node->parent->expanded) {
			return FALSE;
		}
		node = node->parent;
	}

	return TRUE;
}

static gdouble
usage_chart_reflow_do (PlannerUsageChart *chart,
			TreeNode           *root,
			gdouble             start_y)
{
	gdouble   row_y;
	TreeNode *node;
	guint     i;
	gint      row_height;

	if (root->children == NULL) {
		return start_y;
	}

	node = root->children[0];
	row_y = start_y;

	row_height = chart->priv->row_height;
	if (row_height == -1) {
		row_height = 23;
	}
	for (i = 0; i < root->num_children; i++) {
		node = root->children[i];
		if (node_is_visible (node)) {
			g_object_set (node->item,
				      "y", row_y,
				      "height", (double) row_height, NULL);
			row_y += row_height;

			if (node->children != NULL) {
				row_y += usage_chart_reflow_do (chart, node,
								 row_y);
			}
		}
	}

	return row_y - start_y;
}

static gboolean
usage_chart_reflow_idle (PlannerUsageChart *chart)
{
	PlannerUsageChartPriv *priv;
	mrptime                 t1, t2;
	gdouble                 x1, y1, x2, y2;
	gdouble                 height, width;
	gdouble                 bx1, bx2;
	GtkAllocation           allocation;

	priv = chart->priv;

	if (priv->height_changed || priv->height == -1) {
		height = usage_chart_reflow_do (chart, priv->tree, 0);
		priv->height = height;
	} else {
		height = priv->height;
	}

	allocation = GTK_WIDGET (priv->canvas)->allocation;

	t1 = priv->project_start;
	t2 = priv->last_time;

	x1 = t1 * SCALE (priv->zoom) - PADDING;
	x2 = t2 * SCALE (priv->zoom) + PADDING;

	y2 = height;
	y1 = 0;

	width = MAX (x2 - x1, allocation.width - 1.0);
	height = MAX (y2 - y1, allocation.height - 1.0);

	gnome_canvas_item_get_bounds (priv->canvas->root,
				      &bx1, NULL,
				      &bx2, NULL);

	bx2 += PADDING;

	width = MAX (width, bx2 - bx1);

	x2 = x1 + width;

	usage_chart_set_scroll_region (chart,
					x1,
					y1,
					x2,
					y1 + height);

	if (x1 > -1 && x2 > -1) {
		g_object_set (priv->header,
			      "x1", x1,
			      "x2", x2,
			      NULL);
	}

	priv->height_changed = FALSE;
	priv->reflow_idle_id = 0;

	return FALSE;
}

static void
usage_chart_reflow_now (PlannerUsageChart *chart)
{
	if (!GTK_WIDGET_MAPPED (chart)) {
		return;
	}

	usage_chart_reflow_idle (chart);
}

static void
usage_chart_reflow (PlannerUsageChart *chart, gboolean height_changed)
{
	if (!GTK_WIDGET_MAPPED (chart)) {
		return;
	}

	chart->priv->height_changed |= height_changed;
	if (chart->priv->reflow_idle_id != 0) {
		return;
	}

	chart->priv->reflow_idle_id =
		g_idle_add ((GSourceFunc) usage_chart_reflow_idle, chart);
}

static void
usage_chart_tree_traverse (TreeNode *node, TreeFunc func, gpointer data)
{
	gint      i;
	TreeNode *child;

	for (i = 0; i < node->num_children; i++) {
		child = node->children[i];
		usage_chart_tree_traverse (child, func, data);
	}

	func (node, data);
}

static void
scale_func (TreeNode *node, gpointer data)
{
	PlannerUsageChart     *chart;
	PlannerUsageChartPriv *priv;

	chart = PLANNER_USAGE_CHART (data);
	priv = chart->priv;

	if (node->item) {
		gnome_canvas_item_set (GNOME_CANVAS_ITEM (node->item),
				       "scale", SCALE (priv->zoom),
				       "zoom", priv->zoom,
				       NULL);
	}
}

static void
usage_chart_set_scroll_region (PlannerUsageChart *chart,
				gdouble             x1,
				gdouble             y1,
				gdouble             x2,
				gdouble             y2)
{
	GnomeCanvas *canvas;
	gdouble      ox1, oy1, ox2, oy2;

	canvas = chart->priv->canvas;

	gnome_canvas_get_scroll_region (canvas, &ox1, &oy1, &ox2, &oy2);
	if (ox1 == x1 && oy1 == y1 && ox2 == x2 && oy2 == y2) {
		return;
	}

	gnome_canvas_set_scroll_region (canvas, x1, y1, x2, y2);
}

GtkWidget *
planner_usage_chart_new (void)
{
	return planner_usage_chart_new_with_model (NULL);
}

GtkWidget *
planner_usage_chart_new_with_model (GtkTreeModel *model)
{
	PlannerUsageChart *chart;

	chart = g_object_new (PLANNER_TYPE_USAGE_CHART, NULL);

	if (model) {
		planner_usage_chart_set_model (chart, model);
	}

	return GTK_WIDGET (chart);
}

void
planner_usage_chart_set_model (PlannerUsageChart *chart,
				GtkTreeModel       *model)
{
	PlannerUsageChartPriv *priv;
	MrpProject             *project;
	MrpTask                *root;
	gulong                  signal_id;

	g_return_if_fail (PLANNER_IS_USAGE_CHART (chart));

	priv = chart->priv;

	if (model == priv->model) {
		return;
	}

	if (priv->model) {
		usage_chart_disconnect_signals (chart);
		g_object_unref (priv->model);
	}

	priv->model = model;
	if (model) {
		g_object_ref (model);
		usage_chart_build_tree (chart);
		project =
			planner_usage_model_get_project (PLANNER_USAGE_MODEL
							  (model));
		root = mrp_project_get_root_task (project);
		g_object_set (priv->background, "project", project, NULL);

		signal_id = g_signal_connect (project,
					      "notify::project-start",
					      G_CALLBACK
					      (usage_chart_project_start_changed),
					      chart);
		usage_chart_add_signal (chart, project, signal_id,
					 "notify::project-start");

		g_signal_connect (root,
				  "notify::finish",
				  G_CALLBACK
				  (usage_chart_root_finish_changed),
				  chart);

		signal_id = g_signal_connect (model,
					      "row-changed",
					      G_CALLBACK
					      (usage_chart_row_changed),
					      chart);
		usage_chart_add_signal (chart, model, signal_id,
					 "row-changed");

		signal_id = g_signal_connect (model,
					      "row-inserted",
					      G_CALLBACK
					      (usage_chart_row_inserted),
					      chart);
		usage_chart_add_signal (chart, model, signal_id,
					 "row-inserted");

		signal_id = g_signal_connect (model,
					      "row-deleted",
					      G_CALLBACK
					      (usage_chart_row_deleted),
					      chart);
		usage_chart_add_signal (chart, model, signal_id,
					 "row-deleted");

		/*
		 * signal_id = g_signal_connect(model,
		 * "row-reordered",
		 * G_CALLBACK (usage_chart_row_reordered),
		 * chart);
		 * usage_chart_add_signal (chart, model, signal_id,"row-reordered");
		 */

		priv->project_start = mrp_project_get_project_start (project);
		g_object_set (priv->background,
			      "project-start", priv->project_start,
			      NULL);

		priv->last_time = mrp_task_get_finish (root);

		priv->height_changed = TRUE;
		usage_chart_reflow_now (chart);

	}
	g_object_notify (G_OBJECT (chart), "model");
}

GtkTreeModel *
planner_usage_chart_get_model (PlannerUsageChart *chart)
{
	g_return_val_if_fail (PLANNER_IS_USAGE_CHART (chart), NULL);

	return chart->priv->model;
}


static void
usage_chart_build_tree (PlannerUsageChart * chart)
{
	PlannerUsageChartPriv *priv;
	GtkTreeIter             iter;
	GtkTreePath            *path;
	GtkTreeIter             child;

	priv = chart->priv;

	path = gtk_tree_path_new_root ();
	if (!gtk_tree_model_get_iter (chart->priv->model, &iter, path)) {
		gtk_tree_path_free (path);
		return;
	}

	gtk_tree_path_free (path);

	do {
		MrpResource *res;

		res = planner_usage_model_get_resource (PLANNER_USAGE_MODEL
							 (priv->model),
							 &iter);

		path = gtk_tree_model_get_path (priv->model, &iter);
		usage_chart_insert_resource (chart, path, res);
		gtk_tree_path_free (path);
		if (gtk_tree_model_iter_children (priv->model, &child, &iter)) {
			do {
				MrpAssignment *assign;
				assign = planner_usage_model_get_assignment
					(PLANNER_USAGE_MODEL (priv->model),
					 &child);
				path = gtk_tree_model_get_path (priv->model,
								&child);
				usage_chart_insert_assignment (chart,
							       path,
							       assign);
				gtk_tree_path_free (path);
			} while (gtk_tree_model_iter_next
				 (priv->model, &child));
		}
	} while (gtk_tree_model_iter_next (priv->model, &iter));

	/* FIXME: free paths used as keys. */
}

static TreeNode *
usage_chart_insert_row (PlannerUsageChart *chart,
			 GtkTreePath        *path,
			 MrpResource        *resource,
			 MrpAssignment      *assign)
{
	PlannerUsageChartPriv *priv;
	GnomeCanvasItem        *item;
	TreeNode               *tree_node;

	priv = chart->priv;

	item = gnome_canvas_item_new (gnome_canvas_root (priv->canvas),
				      PLANNER_TYPE_USAGE_ROW,
				      "resource", resource,
				      "assignment", assign,
				      "scale", SCALE (priv->zoom),
				      "zoom", priv->zoom,
				      NULL);

	tree_node = usage_chart_tree_node_new ();
	tree_node->item = item;
	tree_node->resource = resource;
	tree_node->assignment = assign;
	usage_chart_tree_node_insert_path (priv->tree, path, tree_node);
	return tree_node;
}

static TreeNode *
usage_chart_insert_resource (PlannerUsageChart *chart,
			      GtkTreePath        *path,
			      MrpResource        *resource)
{
	TreeNode *node;

	node = usage_chart_insert_row (chart, path, resource, NULL);
	node->expanded = 0;

	return node;
}

static TreeNode *
usage_chart_insert_assignment (PlannerUsageChart *chart,
				GtkTreePath        *path,
				MrpAssignment      *assign)
{
	return usage_chart_insert_row (chart, path, NULL, assign);
}

static void
usage_chart_tree_node_insert_path (TreeNode    *node,
				    GtkTreePath *path,
				    TreeNode    *new_node)
{
	gint *indices, i, depth;

	depth = gtk_tree_path_get_depth (path);
	indices = gtk_tree_path_get_indices (path);

	for (i = 0; i < depth - 1; i++) {
		node = node->children[indices[i]];
	}

	node->num_children++;
	node->children =
		g_realloc (node->children,
			   sizeof (gpointer) * node->num_children);

	if (node->num_children - 1 == indices[i]) {
		/* Don't need to move if the new node is at the end. */
	} else {
		memmove (node->children + indices[i] + 1,
			 node->children + indices[i],
			 sizeof (gpointer) * (node->num_children -
					      indices[i] - 1));
	}

	node->children[indices[i]] = new_node;
	new_node->parent = node;
}

static void
usage_chart_project_start_changed (MrpProject         *project,
				    GParamSpec         *spec,
				    PlannerUsageChart *chart)
{
	mrptime t;

	t = mrp_project_get_project_start (project);

	chart->priv->project_start = t;
	g_object_set (chart->priv->background, "project-start", t, NULL);

	usage_chart_reflow_now (chart);
}

static void
usage_chart_root_finish_changed (MrpTask            *root,
				  GParamSpec         *spec,
				  PlannerUsageChart *chart)
{
	chart->priv->last_time = mrp_task_get_finish (root);
	usage_chart_reflow (chart, FALSE);
}

static void
usage_chart_add_signal (PlannerUsageChart *chart,
			 gpointer            instance,
			 gulong              sig_id,
			 gchar              *sig_name)
{
	ConnectData *data;

	data = g_new0 (ConnectData, 1);
	data->instance = instance;
	data->id = sig_id;
	chart->priv->signal_ids = g_list_prepend (chart->priv->signal_ids, data);
}

static void
usage_chart_disconnect_signals (PlannerUsageChart *chart)
{
	GList       *l;
	ConnectData *data;

	for (l = chart->priv->signal_ids; l; l = l->next) {
		data = l->data;
		g_signal_handler_disconnect (data->instance, data->id);
		g_free (data);
	}

	g_list_free (chart->priv->signal_ids);
	chart->priv->signal_ids = NULL;
}

static void
show_hide_descendants (TreeNode *node, gboolean show)
{
	gint i;

	for (i = 0; i < node->num_children; i++) {
		planner_usage_row_set_visible (PLANNER_USAGE_ROW
						(node->children[i]->item),
						show);
		if (!show || (show && node->children[i]->expanded)) {
			show_hide_descendants (node->children[i], show);
		}
	}
}

static void
expand_descendants (TreeNode *node)
{
	gint i;

	for (i = 0; i < node->num_children; i++) {
		node->children[i]->expanded = TRUE;
		expand_descendants (node->children[i]);
	}
}

static void
collapse_descendants (TreeNode *node)
{
	gint i;

	for (i = 0; i < node->num_children; i++) {
		node->children[i]->expanded = FALSE;
		collapse_descendants (node->children[i]);
	}
}

void
planner_usage_chart_expand_row (PlannerUsageChart *chart,
				 GtkTreePath        *path)
{
	TreeNode *node;

	g_return_if_fail (PLANNER_IS_USAGE_CHART (chart));

	node = usage_chart_tree_node_at_path (chart->priv->tree, path);
	if (node) {
		node->expanded = TRUE;
		show_hide_descendants (node, TRUE);
		usage_chart_reflow (chart, TRUE);
	}
}

void
planner_usage_chart_collapse_row (PlannerUsageChart *chart,
				   GtkTreePath        *path)
{
	TreeNode *node;

	g_return_if_fail (PLANNER_IS_USAGE_CHART (chart));

	node = usage_chart_tree_node_at_path (chart->priv->tree, path);
	if (node) {
		node->expanded = FALSE;
		collapse_descendants (node);
		show_hide_descendants (node, FALSE);
		usage_chart_reflow (chart, TRUE);
	}
}

void
planner_usage_chart_expand_all (PlannerUsageChart *chart)
{
	g_return_if_fail (PLANNER_IS_USAGE_CHART (chart));

	expand_descendants (chart->priv->tree);
	show_hide_descendants (chart->priv->tree, TRUE);
	usage_chart_reflow (chart, TRUE);
}

void
planner_usage_chart_collapse_all (PlannerUsageChart *chart)
{
	TreeNode *node;
	int       i;

	g_return_if_fail (PLANNER_IS_USAGE_CHART (chart));

	node = chart->priv->tree;
	for (i = 0; i < node->num_children; i++) {
		node->children[i]->expanded = FALSE;
		collapse_descendants (node->children[i]);
		show_hide_descendants (node->children[i], FALSE);
	}

	usage_chart_reflow (chart, TRUE);
}

static TreeNode *
usage_chart_tree_node_at_path (TreeNode *node, GtkTreePath *path)
{
	gint *indices, i, depth;

	depth = gtk_tree_path_get_depth (path);
	indices = gtk_tree_path_get_indices (path);
	for (i = 0; i < depth; i++) {
		node = node->children[indices[i]];
	}

	return node;
}

static void
usage_chart_row_changed (GtkTreeModel *model,
			  GtkTreePath  *path,
			  GtkTreeIter  *iter,
			  gpointer      data)
{
}

static void
usage_chart_row_inserted (GtkTreeModel *model,
			   GtkTreePath  *path,
			   GtkTreeIter  *iter,
			   gpointer      data)
{
	PlannerUsageChart     *chart;
	gboolean                free_path = FALSE;
	gboolean                free_iter = FALSE;
	MrpResource            *res;
	MrpAssignment          *assign;
	TreeNode               *node = NULL;

	chart = data;

	if (path == NULL) {
		path = gtk_tree_model_get_path (model, iter);
		free_path = TRUE;
	}
	else if (iter == NULL) {
		iter = g_new0 (GtkTreeIter, 1);
		free_iter = TRUE;
		gtk_tree_model_get_iter (model, iter, path);
	}

	res = planner_usage_model_get_resource (PLANNER_USAGE_MODEL (model),
						 iter);
	assign = planner_usage_model_get_assignment (PLANNER_USAGE_MODEL
						      (model), iter);
	if (res) {
		node = usage_chart_insert_resource (chart, path, res);
	}

	if (assign && !node) {
		node = usage_chart_insert_assignment (chart, path, assign);
	}

	usage_chart_reflow (chart, TRUE);

	if (free_path) {
		gtk_tree_path_free (path);
	}

	if (free_iter) {
		g_free (iter);
	}

}
static void
usage_chart_row_deleted (GtkTreeModel *model,
			  GtkTreePath  *path,
			  gpointer      data)
{
	PlannerUsageChart     *chart;
	PlannerUsageChartPriv *priv;
	TreeNode               *node;

	chart = PLANNER_USAGE_CHART (data);
	priv = chart->priv;

	node = usage_chart_tree_node_at_path (priv->tree, path);
	usage_chart_tree_node_remove (node);
	usage_chart_remove_children (chart, node);
	/*
	 * g_return_if_fail (path != NULL || iter != NULL);
	 * fprintf(stderr,"On degage une ligne\n");
	 * if (path == NULL) {
	 * path = gtk_tree_model_get_path(model,iter);
	 * free_path=TRUE;
	 * } else if (iter == NULL) {
	 * iter = g_new0(GtkTreeIter,1);
	 * free_iter = TRUE;
	 * gtk_tree_model_get_iter(model,iter,path);
	 * }
	 * res = planner_usage_model_get_resource(PLANNER_USAGE_MODEL(model),iter);
	 * assign = planner_usage_model_get_assignment(PLANNER_USAGE_MODEL(model),iter);
	 * if (res) {
	 * usage_chart_remove_resource(chart,path,res);
	 * }
	 * if (assign) {
	 * usage_chart_remove_assignment(chart,path,assign);
	 * }
	 */
	usage_chart_reflow (chart, TRUE);
	/*
	 * if (free_path) {
	 * gt_tree_path_free(path);
	 * }
	 * if (free_iter) {
	 * g_free(iter);
	 * }
	 */
}

/*
static void	usage_chart_row_reordered		(GtkTreeModel	*model,
							 GtkTreePath	*path,
							 GtkTreeIter	*iter,
							 gpointer	 data)
{}

static void
usage_chart_resource_assignment_added			(MrpResource	*res,
							 MrpAssignment	*assign,
							 PlannerUsageChart	*chart)
{
	g_return_if_fail(MRP_IS_RESOURCE(res));
	g_return_if_fail(MRP_IS_ASSIGNMENT(assign));
	g_return_if_fail(PLANNER_IS_USAGE_CHART(chart));
	// So, res is added an assignment...
}
*/

static void
usage_chart_tree_node_remove (TreeNode *node)
{
	TreeNode *parent;
	gint      i, pos;

	parent = node->parent;
	pos = -1;
	for (i = 0; i < parent->num_children; i++) {
		if (parent->children[i] == node) {
			pos = i;
			break;
		}
	}
	g_assert (pos != -1);

	memmove (parent->children + pos,
		 parent->children + pos + 1,
		 sizeof (gpointer) * (parent->num_children - pos - 1));
	parent->num_children--;
	parent->children =
		g_realloc (parent->children,
			   sizeof (gpointer) * parent->num_children);
	node->parent = NULL;
}

static void
usage_chart_remove_children (PlannerUsageChart *chart, TreeNode *node)
{
	gint i;

	for (i = 0; i < node->num_children; i++) {
		usage_chart_remove_children (chart, node->children[i]);
	}

	gtk_object_destroy (GTK_OBJECT (node->item));
	node->item = NULL;

	node->assignment = NULL;
	node->resource = NULL;

	g_free (node->children);
	node->children = NULL;

	g_free (node);
}

void
planner_usage_chart_setup_root_task (PlannerUsageChart *chart)
{
	PlannerUsageChartPriv *priv;
	MrpProject             *project;
	MrpTask                *root;

	g_return_if_fail (PLANNER_IS_USAGE_CHART (chart));

	priv = chart->priv;

	project = planner_usage_model_get_project (PLANNER_USAGE_MODEL (priv->model));
	root = mrp_project_get_root_task (project);

	g_signal_connect (root,
			  "notify::finish",
			  G_CALLBACK
			  (usage_chart_root_finish_changed),
			  chart);
}

PlannerUsageTree *
planner_usage_chart_get_view (PlannerUsageChart *chart)
{
	g_return_val_if_fail (PLANNER_IS_USAGE_CHART (chart), NULL);

	return chart->priv->view;
}

void
planner_usage_chart_set_view (PlannerUsageChart *chart,
			      PlannerUsageTree  *view)
{
	g_return_if_fail (PLANNER_IS_USAGE_TREE (view));

/*	g_print ("View for Usage configured\n");  */

	chart->priv->view = view;
}
