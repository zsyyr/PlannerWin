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
#include <string.h>
#include <time.h>
#include <glib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libplanner/mrp-task.h>
#include "planner-marshal.h"
#include "planner-usage-model.h"

enum {
	RESOURCE_ADDED,
	RESOURCE_REMOVED,
	TASK_ADDED,
	TASK_REMOVED,
	ASSIGNMENT_ADDED,
	ASSIGNMENT_REMOVED,
	LAST_SIGNAL
};

struct _PlannerUsageModelPriv {
	MrpProject *project;
	GHashTable *resource2node;
	GHashTable *assign2node;
	GNode      *tree;
	gboolean    in_new;
};

static void usage_model_init                           (PlannerUsageModel      *model);
static void usage_model_class_init                     (PlannerUsageModelClass *klass);
static void usage_model_finalize                       (GObject                 *object);
static void usage_model_tree_model_init                (GtkTreeModelIface       *iface);
static void usage_model_resource_assignment_added_cb   (MrpResource             *res,
							 MrpAssignment           *assign,
							 PlannerUsageModel      *model);
static void usage_model_resource_assignment_removed_cb (MrpResource             *res,
							 MrpAssignment           *assign,
							 PlannerUsageModel      *model);
static void usage_model_resource_added_cb              (MrpProject              *project,
							 MrpResource             *resource,
							 PlannerUsageModel      *model);
static void usage_model_resource_removed_cb            (MrpProject              *project,
							 MrpResource             *resource,
							 PlannerUsageModel      *model);
static void usage_model_task_added_cb                  (MrpProject              *project,
							 MrpTask                 *task,
							 PlannerUsageModel      *model);
static void usage_model_task_removed_cb                (MrpProject              *project,
							 MrpTask                 *task,
							 PlannerUsageModel      *model);


static GObjectClass *parent_class;
static guint signals[LAST_SIGNAL];

GType
planner_usage_model_get_type (void)
{
	static GType type = 0;
	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerUsageModelClass),
			NULL,   /*base init */
			NULL,   /*base finalize */
			(GClassInitFunc) usage_model_class_init,
			NULL,   /*class finalize */
			NULL,   /*class data */
			sizeof (PlannerUsageModel),
			0,
			(GInstanceInitFunc) usage_model_init
		};
		static const GInterfaceInfo tree_model_info = {
			(GInterfaceInitFunc) usage_model_tree_model_init,
			NULL,
			NULL
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "PlannerUsageModel",
					       &info, 0);
		g_type_add_interface_static (type,
					     GTK_TYPE_TREE_MODEL,
					     &tree_model_info);
	}
	return type;
}

MrpProject *
planner_usage_model_get_project (PlannerUsageModel *model)
{
	return model->priv->project;
}

static void
usage_model_init (PlannerUsageModel *model)
{
	PlannerUsageModelPriv *priv;

	priv = g_new0 (PlannerUsageModelPriv, 1);
	model->priv = priv;

	priv->resource2node = g_hash_table_new (NULL, NULL);
	priv->assign2node = g_hash_table_new (NULL, NULL);

}

static void
usage_model_finalize (GObject *object)
{
	PlannerUsageModel *model = PLANNER_USAGE_MODEL (object);

	g_hash_table_destroy (model->priv->assign2node);
	g_hash_table_destroy (model->priv->resource2node);
	g_node_destroy (model->priv->tree);
	g_free (model->priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		G_OBJECT_CLASS (parent_class)->finalize (object);
	}
}

static void
usage_model_class_init (PlannerUsageModelClass *klass)
{
	GObjectClass *object_class;
	object_class = (GObjectClass *) klass;
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = usage_model_finalize;
	signals[RESOURCE_ADDED] =
		g_signal_new ("resource-added",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, MRP_TYPE_RESOURCE);
	signals[RESOURCE_REMOVED] =
		g_signal_new ("resource-removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, MRP_TYPE_RESOURCE);
	signals[TASK_ADDED] =
		g_signal_new ("task-added",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, MRP_TYPE_TASK);
	signals[TASK_REMOVED] =
		g_signal_new ("task-removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, MRP_TYPE_TASK);

	signals[ASSIGNMENT_ADDED] =
		g_signal_new ("assignment-added",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, MRP_TYPE_ASSIGNMENT);
	signals[ASSIGNMENT_REMOVED] =
		g_signal_new ("assignment-removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, MRP_TYPE_ASSIGNMENT);
}

static int
usage_model_get_n_columns (GtkTreeModel *tree_model)
{
	return NUM_COLS;
}

static GType
usage_model_get_column_type (GtkTreeModel *tree_model, gint column)
{
	switch (column) {
	case COL_RESNAME:
		return G_TYPE_STRING;
	case COL_TASKNAME:
		return G_TYPE_STRING;
	case COL_RESOURCE:
		return MRP_TYPE_RESOURCE;
	case COL_ASSIGNMENT:
		return MRP_TYPE_ASSIGNMENT;
	default:
		return G_TYPE_INVALID;
	}
}

static gboolean
usage_model_get_iter (GtkTreeModel *tree_model,
		       GtkTreeIter *iter, GtkTreePath *path)
{
	PlannerUsageModel *model;
	GtkTreeIter parent;
	gint *indices;
	gint depth, i;

	model = PLANNER_USAGE_MODEL (tree_model);

	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);

	g_return_val_if_fail (depth > 0, FALSE);

	parent.stamp = model->stamp;
	parent.user_data = model->priv->tree;

	if (!gtk_tree_model_iter_nth_child
	    (tree_model, iter, &parent, indices[0])) {
		return FALSE;
	}

	for (i = 1; i < depth; i++) {
		parent = *iter;
		if (!gtk_tree_model_iter_nth_child
		    (tree_model, iter, &parent, indices[i])) {
			return FALSE;
		}
	}

	return TRUE;
}

static GtkTreePath *
usage_model_get_path_from_node (PlannerUsageModel *model, GNode *node)
{
	GtkTreePath *path;
	GNode *parent;
	GNode *child;
	gint i = 0;

	g_return_val_if_fail (PLANNER_IS_USAGE_MODEL (model), NULL);
	g_return_val_if_fail (node != NULL, NULL);

	parent = node->parent;

	if (parent == NULL && node == model->priv->tree) {
		return gtk_tree_path_new_root ();
	}

	g_assert (parent != NULL);

	if (parent == model->priv->tree) {
		path = gtk_tree_path_new ();
	} else {
		path = usage_model_get_path_from_node (model, parent);
	}
	child = g_node_first_child (parent);

	if (path == NULL) {
		return NULL;
	}

	if (child == NULL) {
		gtk_tree_path_free (path);
		return NULL;
	}

	for (; child; child = g_node_next_sibling (child)) {
		if (child == node) {
			break;
		}
		i++;
	}

	if (child == NULL) {
		gtk_tree_path_free (path);
		return NULL;
	}
	gtk_tree_path_append_index (path, i);
	return path;
}

GtkTreePath *
planner_usage_model_get_path_from_resource (PlannerUsageModel *model,
					     MrpResource *resource)
{
	GNode *node;

	g_return_val_if_fail (PLANNER_IS_USAGE_MODEL (model), NULL);
	g_return_val_if_fail (MRP_IS_RESOURCE (resource), NULL);

	node = g_hash_table_lookup (model->priv->resource2node, resource);

	return usage_model_get_path_from_node (PLANNER_USAGE_MODEL (model),
						node);
}

GtkTreePath *
planner_usage_model_get_path_from_assignment (PlannerUsageModel *model,
					      MrpAssignment     *assignment)
{
	GNode *node;

	g_return_val_if_fail (PLANNER_IS_USAGE_MODEL (model), NULL);
	g_return_val_if_fail (MRP_IS_ASSIGNMENT (assignment), NULL);

	node = g_hash_table_lookup (model->priv->assign2node, assignment);

	return usage_model_get_path_from_node (PLANNER_USAGE_MODEL (model),
					       node);
}

static GtkTreePath *
usage_model_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	GNode *node;

	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (iter->user_data != NULL, NULL);
	g_return_val_if_fail (iter->stamp ==
			      PLANNER_USAGE_MODEL (tree_model)->stamp, NULL);

	node = iter->user_data;

	return usage_model_get_path_from_node (PLANNER_USAGE_MODEL
						(tree_model), node);
}

static void
usage_model_get_value (GtkTreeModel *tree_model,
			GtkTreeIter *iter, gint column, GValue *value)
{
	GNode *node;
	MrpTask *task;
	MrpResource *resource;
	MrpAssignment *assign;
	gchar *str = NULL;

	g_return_if_fail (iter != NULL);
	node = iter->user_data;
	if (MRP_IS_ASSIGNMENT (node->data)) {
		assign = MRP_ASSIGNMENT (node->data);
		task = mrp_assignment_get_task (assign);
		resource = mrp_assignment_get_resource (assign);
	} else {
		if (MRP_IS_RESOURCE (node->data)) {
			resource = MRP_RESOURCE (node->data);
			assign = NULL;
			task = NULL;
		} else {
			resource = NULL;
			assign = NULL;
			task = NULL;
			g_warning ("Type mismatch (%s:%d)", __FILE__,
				   __LINE__);
		}
	}
	g_assert (resource != NULL);

	switch (column) {
	case COL_RESNAME:
		g_object_get (resource, "name", &str, NULL);
		if (str == NULL) {
			str = g_strdup ("");
		}
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, str);
		g_free (str);
		break;

	case COL_TASKNAME:
		if (task) {
			g_object_get (task, "name", &str, NULL);
		}
		if (str == NULL) {      /*Implicit: || task == NULL */
			str = g_strdup ("");
		}
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, str);
		g_free (str);
		break;

	case COL_RESOURCE:
		g_value_init (value, MRP_TYPE_RESOURCE);
		g_value_set_object (value, resource);
		break;

	case COL_ASSIGNMENT:
		g_value_init (value, MRP_TYPE_ASSIGNMENT);
		g_value_set_object (value, assign);
		break;

	default:
		g_warning ("Bad column %d requested", column);
	}
}

static gboolean
usage_model_iter_next (GtkTreeModel *model, GtkTreeIter *iter)
{
	GNode *node, *next;

	node = iter->user_data;

	next = g_node_next_sibling (node);

	if (next == NULL) {
		iter->user_data = NULL;
		return FALSE;
	}

	iter->user_data = next;
	iter->stamp = PLANNER_USAGE_MODEL (model)->stamp;
	return TRUE;
}

static gboolean
usage_model_iter_children (GtkTreeModel *tree_model,
			    GtkTreeIter *iter, GtkTreeIter *parent)
{
	GNode *node, *child;

	if (parent) {
		node = parent->user_data;
	} else {
		node = PLANNER_USAGE_MODEL (tree_model)->priv->tree;
	}

	child = g_node_first_child (node);

	if (child == NULL) {
		iter->user_data = NULL;
		return FALSE;
	}

	iter->user_data = child;
	iter->stamp = PLANNER_USAGE_MODEL (tree_model)->stamp;
	return TRUE;
}

static gboolean
usage_model_iter_has_child (GtkTreeModel *model, GtkTreeIter *iter)
{
	GNode *node;
	node = iter->user_data;
	return (g_node_n_children (node) > 0);
}

static gint
usage_model_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	GNode *node;
	if (iter) {
		node = iter->user_data;
	} else {
		node = PLANNER_USAGE_MODEL (tree_model)->priv->tree;
	}
	return g_node_n_children (node);
}

static gboolean
usage_model_iter_nth_child (GtkTreeModel *tree_model,
			     GtkTreeIter *iter,
			     GtkTreeIter *parent_iter, gint n)
{
	PlannerUsageModel *model;
	GNode *parent;
	GNode *child;

	g_return_val_if_fail (parent_iter == NULL
			      || parent_iter->user_data != NULL, FALSE);

	model = PLANNER_USAGE_MODEL (tree_model);

	if (parent_iter == NULL) {
		parent = model->priv->tree;
	} else {
		parent = parent_iter->user_data;
	}
	child = g_node_nth_child (parent, n);
	if (child) {
		iter->user_data = child;
		iter->stamp = model->stamp;
		return TRUE;
	} else {
		iter->user_data = NULL;
		return FALSE;
	}
}

static gboolean
usage_model_iter_parent (GtkTreeModel *tree_model,
			  GtkTreeIter *iter, GtkTreeIter *child)
{
	GNode *node;
	GNode *parent;
	node = child->user_data;
	parent = node->parent;
	if (parent == NULL) {
		iter->user_data = NULL;
		return FALSE;
	} else {
		iter->user_data = parent;
		iter->stamp = PLANNER_USAGE_MODEL (tree_model)->stamp;
		return TRUE;
	}
}

static void
usage_model_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_n_columns = usage_model_get_n_columns;
	iface->get_column_type = usage_model_get_column_type;
	iface->get_iter = usage_model_get_iter;
	iface->get_path = usage_model_get_path;
	iface->get_value = usage_model_get_value;
	iface->iter_next = usage_model_iter_next;
	iface->iter_children = usage_model_iter_children;
	iface->iter_has_child = usage_model_iter_has_child;
	iface->iter_n_children = usage_model_iter_n_children;
	iface->iter_nth_child = usage_model_iter_nth_child;
	iface->iter_parent = usage_model_iter_parent;
}

PlannerUsageModel *
planner_usage_model_new (MrpProject *project)
{
	PlannerUsageModel *model;
	PlannerUsageModelPriv *priv;

	GList *resources, *r;
	MrpResource *resource;

	model = PLANNER_USAGE_MODEL (g_object_new
				      (PLANNER_TYPE_USAGE_MODEL, NULL));
	priv = model->priv;
	priv->in_new = TRUE;

	priv->project = project;

	resources = mrp_project_get_resources (project);

	priv->tree = g_node_new (NULL);

	for (r = resources; r; r = r->next) {
		resource = r->data;
		usage_model_resource_added_cb (project, resource, model);
	}

	g_signal_connect_object (project,
				 "resource_added",
				 G_CALLBACK (usage_model_resource_added_cb),
				 model, 0);
	g_signal_connect_object (project,
				 "resource_removed",
				 G_CALLBACK
				 (usage_model_resource_removed_cb), model,
				 0);
	g_signal_connect_object (project, "task_inserted",
				 G_CALLBACK (usage_model_task_added_cb),
				 model, 0);
	g_signal_connect_object (project, "task_removed",
				 G_CALLBACK (usage_model_task_removed_cb),
				 model, 0);

	priv->in_new = FALSE;

	return model;
}

MrpAssignment *
planner_usage_model_get_assignment (PlannerUsageModel *model,
				     GtkTreeIter *iter)
{
	MrpAssignment *assign;

	g_return_val_if_fail (PLANNER_IS_USAGE_MODEL (model), NULL);

	assign = ((GNode *) iter->user_data)->data;

	if (assign == NULL) {
		g_warning ("Eeek");
		return NULL;
	} else {
		if (MRP_IS_ASSIGNMENT (assign)) {
			return assign;
		} else {
			return NULL;
		}
	}
}

MrpResource *
planner_usage_model_get_resource (PlannerUsageModel *model,
				   GtkTreeIter *iter)
{
	MrpResource *res;

	g_return_val_if_fail (PLANNER_IS_USAGE_MODEL (model), NULL);

	res = ((GNode *) iter->user_data)->data;

	if (res == NULL) {
		g_warning ("Eeek");
		return NULL;
	} else {
		if (MRP_IS_RESOURCE (res)) {
			return res;
		} else {
			return NULL;
		}
	}
}

gboolean
planner_usage_model_is_assignment (PlannerUsageModel *model,
				    GtkTreeIter *iter)
{
	g_return_val_if_fail (PLANNER_IS_USAGE_MODEL (model), FALSE);

	return MRP_IS_ASSIGNMENT (((GNode *) iter->user_data)->data);
}

gboolean
planner_usage_model_is_resource (PlannerUsageModel *model,
				  GtkTreeIter *iter)
{
	g_return_val_if_fail (PLANNER_IS_USAGE_MODEL (model), FALSE);

	return MRP_IS_RESOURCE (((GNode *) iter->user_data)->data);
}

MrpAssignment *
planner_usage_model_path_get_assignment (PlannerUsageModel *model,
					  GtkTreePath *path)
{
	GtkTreeIter iter;

	g_return_val_if_fail (PLANNER_IS_USAGE_MODEL (model), NULL);

	usage_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	return planner_usage_model_get_assignment (model, &iter);
}

MrpResource *
planner_usage_model_path_get_resource (PlannerUsageModel *model,
					GtkTreePath *path)
{
	GtkTreeIter iter;

	g_return_val_if_fail (PLANNER_IS_USAGE_MODEL (model), NULL);

	usage_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	return planner_usage_model_get_resource (model, &iter);
}

gboolean
planner_usage_model_path_is_resource (PlannerUsageModel *model,
				       GtkTreePath *path)
{
	GtkTreeIter iter;

	g_return_val_if_fail (PLANNER_IS_USAGE_MODEL (model), FALSE);

	usage_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	return planner_usage_model_is_resource (model, &iter);
}

gboolean
planner_usage_model_path_is_assignment (PlannerUsageModel *model,
					 GtkTreePath *path)
{
	GtkTreeIter iter;

	g_return_val_if_fail (PLANNER_IS_USAGE_MODEL (model), FALSE);

	usage_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	return planner_usage_model_is_assignment (model, &iter);
}

static void
usage_model_resource_assignment_added_cb (MrpResource *res,
					   MrpAssignment *assign,
					   PlannerUsageModel *model)
{
	GNode *res_node;
	GNode *assign_node;
	GtkTreePath *path;
	GtkTreeIter iter;

	res_node = g_hash_table_lookup (model->priv->resource2node, res);
	assign_node = g_node_new (assign);
	g_node_append (res_node, assign_node);
	g_hash_table_insert (model->priv->assign2node, assign, assign_node);

	path = usage_model_get_path_from_node (model, assign_node);
	usage_model_get_iter (GTK_TREE_MODEL (model), &iter, path);
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
	gtk_tree_path_free (path);

	if (g_node_n_children (res_node) == 1) {
		path = usage_model_get_path_from_node (model, res_node);
		usage_model_get_iter (GTK_TREE_MODEL (model), &iter, path);
		gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (model),
						      path, &iter);
		gtk_tree_path_free (path);
	}
}

static void
usage_model_resource_assignment_removed_cb (MrpResource *res,
					     MrpAssignment *assign,
					     PlannerUsageModel *model)
{
	GNode *res_node;
	GNode *assign_node;
	GtkTreePath *path;
	GtkTreeIter iter;

	res_node = g_hash_table_lookup (model->priv->resource2node, res);
	assign_node = g_hash_table_lookup (model->priv->assign2node, assign);

	path = usage_model_get_path_from_node (model, assign_node);
	usage_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	g_hash_table_remove (model->priv->assign2node, assign);
	g_node_unlink (assign_node);
	g_node_destroy (assign_node);

	gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
	gtk_tree_path_free (path);

	if (g_node_n_children (res_node) == 0) {
		path = usage_model_get_path_from_node (model, res_node);
		usage_model_get_iter (GTK_TREE_MODEL (model), &iter, path);
		gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (model),
						      path, &iter);
		gtk_tree_path_free (path);
	}
}

static void
usage_model_resource_added_cb (MrpProject *project,
				MrpResource *resource,
				PlannerUsageModel *model)
{
	PlannerUsageModelPriv *priv;
	GList *tasks, *t;
	GNode *rnode;
	MrpAssignment *assign;
	GtkTreePath *path;
	GtkTreeIter iter;

	priv = model->priv;

	g_signal_connect_object (resource,
				 "assignment_added",
				 G_CALLBACK
				 (usage_model_resource_assignment_added_cb),
				 model, 0);
	g_signal_connect_object (resource, "assignment_removed",
				 G_CALLBACK
				 (usage_model_resource_assignment_removed_cb),
				 model, 0);
	rnode = g_node_new (resource);
	g_node_append (priv->tree, rnode);
	g_hash_table_insert (priv->resource2node, resource, rnode);

	path = usage_model_get_path_from_node (model, rnode);
	usage_model_get_iter (GTK_TREE_MODEL (model), &iter, path);
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
	gtk_tree_path_free (path);

	tasks = mrp_resource_get_assignments (resource);
	for (t = tasks; t; t = t->next) {
		assign = MRP_ASSIGNMENT (t->data);
		usage_model_resource_assignment_added_cb (resource, assign,
							   model);
		/*
		 *tnode = g_node_new(assign);
		 *g_node_append(rnode,tnode);
		 *g_hash_table_insert (priv->assign2node, assign, tnode);
		 *
		 *path = usage_model_get_path
		 */
	}
}

static void
usage_model_resource_removed_cb (MrpProject *project,
				  MrpResource *resource,
				  PlannerUsageModel *model)
{
	PlannerUsageModelPriv *priv;
	GNode *node;
	GtkTreePath *path;
	/*GtkTreeIter           iter; */

	priv = model->priv;

	g_signal_handlers_disconnect_by_func (resource,
					      usage_model_resource_assignment_added_cb,
					      model);
	g_signal_handlers_disconnect_by_func (resource,
					      usage_model_resource_assignment_removed_cb,
					      model);

	/*
	 *Look the resource in the tree
	 */
	node = g_hash_table_lookup (priv->resource2node, resource);
	g_hash_table_remove (priv->resource2node, resource);

	/*
	 *Remove the resource.
	 */
	path = usage_model_get_path_from_node (model, node);
	g_node_destroy (node);
	/*usage_model_get_iter(GTK_TREE_MODEL(model),&iter,path); */
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
	gtk_tree_path_free (path);
}

static void
usage_model_task_added_cb (MrpProject *project,
			    MrpTask *task, PlannerUsageModel *model)
{
	g_signal_emit (model, signals[TASK_ADDED], 0, task);
}

static void
usage_model_task_removed_cb (MrpProject *project,
			      MrpTask *task, PlannerUsageModel *model)
{
	g_signal_emit (model, signals[TASK_REMOVED], 0, task);
}

