/*
 * Copyright (C) 2010 Collabora Ltd.
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Authors: Philip Withnall <philip.withnall@collabora.co.uk>
 */

#include "config.h"

#include <string.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <telepathy-glib/util.h>

#include <folks/folks.h>

#include <libempathy/empathy-individual-manager.h>
#include <libempathy/empathy-utils.h>

#include "empathy-individual-linker.h"
#include "empathy-individual-store.h"
#include "empathy-individual-view.h"
#include "empathy-individual-widget.h"
#include "empathy-persona-store.h"
#include "empathy-persona-view.h"

/**
 * SECTION:empathy-individual-linker
 * @title:EmpathyIndividualLinker
 * @short_description: A widget used to link together #FolksIndividual<!-- -->s
 * @include: libempathy-gtk/empathy-individual-linker.h
 *
 * #EmpathyIndividualLinker is a widget which allows selection of several
 * #FolksIndividual<!-- -->s to link together to form a single new individual.
 * The widget provides a preview of the linked individual.
 */

/**
 * EmpathyIndividualLinker:
 * @parent: parent object
 *
 * Widget which extends #GtkBin to provide a list of #FolksIndividual<!-- -->s
 * to link together.
 */

#define GET_PRIV(obj) EMPATHY_GET_PRIV (obj, EmpathyIndividualLinker)

typedef struct {
  EmpathyIndividualStore *individual_store; /* owned */
  EmpathyIndividualView *individual_view; /* child widget */
  GtkWidget *preview_widget; /* child widget */
  EmpathyPersonaStore *persona_store; /* owned */

  FolksIndividual *start_individual; /* owned, allow-none */
  FolksIndividual *new_individual; /* owned, allow-none */

  /* Stores the Individuals whose Personas have been added to the
   * new_individual */
  /* unowned Individual (borrowed from EmpathyIndividualStore) -> bool */
  GHashTable *changed_individuals;
} EmpathyIndividualLinkerPriv;

enum {
  PROP_START_INDIVIDUAL = 1,
};

G_DEFINE_TYPE (EmpathyIndividualLinker, empathy_individual_linker,
    GTK_TYPE_BIN);

static void
contact_toggle_cell_data_func (GtkTreeViewColumn *tree_column,
    GtkCellRenderer *cell,
    GtkTreeModel *tree_model,
    GtkTreeIter *iter,
    EmpathyIndividualLinker *self)
{
  EmpathyIndividualLinkerPriv *priv;
  FolksIndividual *individual;
  gboolean is_group, individual_added;

  priv = GET_PRIV (self);

  gtk_tree_model_get (tree_model, iter,
      EMPATHY_INDIVIDUAL_STORE_COL_IS_GROUP, &is_group,
      EMPATHY_INDIVIDUAL_STORE_COL_INDIVIDUAL, &individual,
      -1);

  individual_added = GPOINTER_TO_UINT (g_hash_table_lookup (
      priv->changed_individuals, individual));

  /* We don't want to show checkboxes next to the group rows.
   * All checkboxes should be sensitive except the checkbox for the start
   * individual, which should be permanently active and insensitive */
  g_object_set (cell,
      "visible", !is_group,
      "sensitive", individual != priv->start_individual,
      "active", individual_added || individual == priv->start_individual,
      NULL);

  tp_clear_object (&individual);
}

static void
link_individual (EmpathyIndividualLinker *self,
    FolksIndividual *individual)
{
  EmpathyIndividualLinkerPriv *priv = GET_PRIV (self);
  GList *new_persona_list;

  /* Add the individual to the link */
  g_hash_table_insert (priv->changed_individuals, individual,
      GUINT_TO_POINTER (TRUE));

  /* Add personas which are in @individual to priv->new_individual, appending
   * them to the list of personas.
   * This is rather slow. */
  new_persona_list = g_list_copy (folks_individual_get_personas (
      priv->new_individual));
  new_persona_list = g_list_concat (new_persona_list,
      g_list_copy (folks_individual_get_personas (individual)));
  folks_individual_set_personas (priv->new_individual, new_persona_list);
  g_list_free (new_persona_list);
}

static void
unlink_individual (EmpathyIndividualLinker *self,
    FolksIndividual *individual)
{
  EmpathyIndividualLinkerPriv *priv = GET_PRIV (self);
  GList *new_persona_list, *old_persona_list, *removing_personas, *l;

  /* Remove the individual from the link */
  g_hash_table_remove (priv->changed_individuals, individual);

  /* Remove personas which are in @individual from priv->new_individual.
   * This is rather slow. */
  old_persona_list = folks_individual_get_personas (priv->new_individual);
  removing_personas = folks_individual_get_personas (individual);
  new_persona_list = NULL;

  for (l = old_persona_list; l != NULL; l = l->next)
    {
      GList *removing = g_list_find (removing_personas, l->data);

      if (removing == NULL)
        new_persona_list = g_list_prepend (new_persona_list, l->data);
    }

  new_persona_list = g_list_reverse (new_persona_list);
  folks_individual_set_personas (priv->new_individual, new_persona_list);
  g_list_free (new_persona_list);
}

static void
toggle_individual_row (EmpathyIndividualLinker *self,
    GtkTreePath *path)
{
  EmpathyIndividualLinkerPriv *priv = GET_PRIV (self);
  FolksIndividual *individual;
  GtkTreeIter iter;
  GtkTreeModel *tree_model;
  gboolean individual_added;

  tree_model = GTK_TREE_MODEL (priv->individual_store);

  gtk_tree_model_get_iter (tree_model, &iter, path);
  gtk_tree_model_get (tree_model, &iter,
      EMPATHY_INDIVIDUAL_STORE_COL_INDIVIDUAL, &individual,
      -1);

  individual_added = GPOINTER_TO_UINT (g_hash_table_lookup (
      priv->changed_individuals, individual));

  /* Toggle the Individual's linked status */
  if (individual_added)
    unlink_individual (self, individual);
  else
    link_individual (self, individual);

  g_object_unref (individual);
}

static void
row_toggled_cb (GtkCellRendererToggle *cell_renderer,
    const gchar *path,
    EmpathyIndividualLinker *self)
{
  GtkTreePath *tree_path = gtk_tree_path_new_from_string (path);
  toggle_individual_row (self, tree_path);
  gtk_tree_path_free (tree_path);
}

static void
search_bar_activate_cb (EmpathyLiveSearch *search_bar,
    EmpathyIndividualLinker *self)
{
  EmpathyIndividualLinkerPriv *priv = GET_PRIV (self);
  GtkTreeSelection *selection;
  GList *rows, *l;

  /* Toggle the status of the selected individuals */
  selection = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (priv->individual_view));
  rows = gtk_tree_selection_get_selected_rows (selection, NULL);

  for (l = rows; l != NULL; l = l->next)
    {
      GtkTreePath *path = (GtkTreePath *) l->data;
      toggle_individual_row (self, path);
      gtk_tree_path_free (path);
    }

  g_list_free (rows);
}

static void
set_up (EmpathyIndividualLinker *self)
{
  EmpathyIndividualLinkerPriv *priv;
  EmpathyIndividualManager *individual_manager;
  GtkCellRenderer *cell_renderer;
  GtkPaned *paned;
  GtkWidget *label, *scrolled_window, *search_bar;
  GtkBox *vbox;
  EmpathyPersonaView *persona_view;

  priv = GET_PRIV (self);

  /* Layout panes */
  paned = GTK_PANED (gtk_hpaned_new ());

  /* Left column heading */
  vbox = GTK_BOX (gtk_vbox_new (FALSE, 6));
  label = gtk_label_new (_("Select contacts to link"));
  gtk_box_pack_start (vbox, label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  /* Individual selector */
  individual_manager = empathy_individual_manager_dup_singleton ();
  priv->individual_store = empathy_individual_store_new (individual_manager);
  g_object_unref (individual_manager);

  empathy_individual_store_set_show_protocols (priv->individual_store, FALSE);

  priv->individual_view = empathy_individual_view_new (priv->individual_store,
      EMPATHY_INDIVIDUAL_VIEW_FEATURE_NONE, EMPATHY_INDIVIDUAL_FEATURE_NONE);
  empathy_individual_view_set_show_offline (priv->individual_view, TRUE);

  /* Add a checkbox column to the selector */
  cell_renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (cell_renderer, "toggled", (GCallback) row_toggled_cb, self);
  gtk_tree_view_insert_column_with_data_func (
      GTK_TREE_VIEW (priv->individual_view), 0, NULL, cell_renderer,
      (GtkTreeCellDataFunc) contact_toggle_cell_data_func, self, NULL);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
      GTK_WIDGET (priv->individual_view));
  gtk_widget_show (GTK_WIDGET (priv->individual_view));

  gtk_box_pack_start (vbox, scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  /* Live search */
  search_bar = empathy_live_search_new (GTK_WIDGET (priv->individual_view));
  empathy_individual_view_set_live_search (priv->individual_view,
      EMPATHY_LIVE_SEARCH (search_bar));
  g_signal_connect (search_bar, "activate", (GCallback) search_bar_activate_cb,
      self);

  gtk_box_pack_end (vbox, search_bar, FALSE, TRUE, 0);

  gtk_paned_pack1 (paned, GTK_WIDGET (vbox), TRUE, FALSE);
  gtk_widget_show (GTK_WIDGET (vbox));

  /* Right column heading */
  vbox = GTK_BOX (gtk_vbox_new (FALSE, 6));
  label = gtk_label_new (_("New contact preview"));
  gtk_box_pack_start (vbox, label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  /* New individual preview */
  priv->preview_widget = empathy_individual_widget_new (priv->new_individual,
      EMPATHY_INDIVIDUAL_WIDGET_SHOW_DETAILS);
  gtk_box_pack_start (vbox, priv->preview_widget, FALSE, TRUE, 0);
  gtk_widget_show (priv->preview_widget);

  /* Persona list */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_SHADOW_IN);

  priv->persona_store = empathy_persona_store_new (priv->new_individual);
  empathy_persona_store_set_show_protocols (priv->persona_store, TRUE);
  persona_view = empathy_persona_view_new (priv->persona_store);
  empathy_persona_view_set_show_offline (persona_view, TRUE);

  gtk_container_add (GTK_CONTAINER (scrolled_window),
      GTK_WIDGET (persona_view));
  gtk_widget_show (GTK_WIDGET (persona_view));

  gtk_box_pack_start (vbox, scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  gtk_paned_pack2 (paned, GTK_WIDGET (vbox), TRUE, FALSE);
  gtk_widget_show (GTK_WIDGET (vbox));

  /* Add the panes to the bin */
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (paned));
  gtk_widget_show (GTK_WIDGET (paned));
}

static void
empathy_individual_linker_init (EmpathyIndividualLinker *self)
{
  EmpathyIndividualLinkerPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EMPATHY_TYPE_INDIVIDUAL_LINKER, EmpathyIndividualLinkerPriv);

  self->priv = priv;

  priv->changed_individuals = g_hash_table_new (NULL, NULL);

  set_up (self);
}

static void
get_property (GObject *object,
    guint param_id,
    GValue *value,
    GParamSpec *pspec)
{
  EmpathyIndividualLinkerPriv *priv;

  priv = GET_PRIV (object);

  switch (param_id)
    {
      case PROP_START_INDIVIDUAL:
        g_value_set_object (value, priv->start_individual);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
set_property (GObject *object,
    guint param_id,
    const GValue *value,
    GParamSpec *pspec)
{
  EmpathyIndividualLinkerPriv *priv;

  priv = GET_PRIV (object);

  switch (param_id)
    {
      case PROP_START_INDIVIDUAL:
        empathy_individual_linker_set_start_individual (
            EMPATHY_INDIVIDUAL_LINKER (object), g_value_get_object (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
  EmpathyIndividualLinkerPriv *priv = GET_PRIV (object);

  tp_clear_object (&priv->individual_store);
  tp_clear_object (&priv->persona_store);
  tp_clear_object (&priv->start_individual);
  tp_clear_object (&priv->new_individual);

  G_OBJECT_CLASS (empathy_individual_linker_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  EmpathyIndividualLinkerPriv *priv = GET_PRIV (object);

  g_hash_table_destroy (priv->changed_individuals);

  G_OBJECT_CLASS (empathy_individual_linker_parent_class)->finalize (object);
}

static void
size_request (GtkWidget *widget,
    GtkRequisition *requisition)
{
  GtkBin *bin = GTK_BIN (widget);
  GtkWidget *child;

  requisition->width =
      gtk_container_get_border_width (GTK_CONTAINER (widget)) * 2;
  requisition->height =
      gtk_container_get_border_width (GTK_CONTAINER (widget)) * 2;

  child = gtk_bin_get_child (bin);

  if (child && gtk_widget_get_visible (child))
    {
      GtkRequisition child_requisition;

      gtk_widget_size_request (child, &child_requisition);

      requisition->width += child_requisition.width;
      requisition->height += child_requisition.height;
    }
}

static void
size_allocate (GtkWidget *widget,
    GtkAllocation *allocation)
{
  GtkBin *bin = GTK_BIN (widget);
  GtkAllocation child_allocation;
  GtkWidget *child;

  gtk_widget_set_allocation (widget, allocation);

  child = gtk_bin_get_child (bin);

  if (child && gtk_widget_get_visible (child))
    {
      child_allocation.x = allocation->x +
          gtk_container_get_border_width (GTK_CONTAINER (widget));
      child_allocation.y = allocation->y +
          gtk_container_get_border_width (GTK_CONTAINER (widget));
      child_allocation.width = MAX (allocation->width -
          gtk_container_get_border_width (GTK_CONTAINER (widget)) * 2, 0);
      child_allocation.height = MAX (allocation->height -
          gtk_container_get_border_width (GTK_CONTAINER (widget)) * 2, 0);

      gtk_widget_size_allocate (child, &child_allocation);
    }
}

static void
empathy_individual_linker_class_init (EmpathyIndividualLinkerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->dispose = dispose;
  object_class->finalize = finalize;

  widget_class->size_request = size_request;
  widget_class->size_allocate = size_allocate;

  /**
   * EmpathyIndividualLinker:start-individual:
   *
   * The #FolksIndividual to link other individuals to. This individual is
   * selected by default in the list of individuals, and cannot be unselected.
   * This ensures that empathy_individual_linker_get_linked_personas() will
   * always return at least one persona to link.
   */
  g_object_class_install_property (object_class, PROP_START_INDIVIDUAL,
      g_param_spec_object ("start-individual",
          "Start Individual",
          "The #FolksIndividual to link other individuals to.",
          FOLKS_TYPE_INDIVIDUAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (object_class, sizeof (EmpathyIndividualLinkerPriv));
}

/**
 * empathy_individual_linker_new:
 * @start_individual: (allow-none): the #FolksIndividual to link to, or %NULL
 *
 * Creates a new #EmpathyIndividualLinker.
 *
 * Return value: a new #EmpathyIndividualLinker
 */
GtkWidget *
empathy_individual_linker_new (FolksIndividual *start_individual)
{
  g_return_val_if_fail (start_individual == NULL ||
      FOLKS_IS_INDIVIDUAL (start_individual), NULL);

  return g_object_new (EMPATHY_TYPE_INDIVIDUAL_LINKER,
      "start-individual", start_individual,
      NULL);
}

/**
 * empathy_individual_linker_get_start_individual:
 * @self: an #EmpathyIndividualLinker
 *
 * Get the value of #EmpathyIndividualLinker:start-individual.
 *
 * Return value: (transfer none): the start individual for linking, or %NULL
 */
FolksIndividual *
empathy_individual_linker_get_start_individual (EmpathyIndividualLinker *self)
{
  g_return_val_if_fail (EMPATHY_IS_INDIVIDUAL_LINKER (self), NULL);

  return GET_PRIV (self)->start_individual;
}

/**
 * empathy_individual_linker_set_start_individual:
 * @self: an #EmpathyIndividualLinker
 * @individual: (allow-none): the start individual, or %NULL
 *
 * Set the value of #EmpathyIndividualLinker:start-individual to @individual.
 */
void
empathy_individual_linker_set_start_individual (EmpathyIndividualLinker *self,
    FolksIndividual *individual)
{
  EmpathyIndividualLinkerPriv *priv;

  g_return_if_fail (EMPATHY_IS_INDIVIDUAL_LINKER (self));
  g_return_if_fail (individual == NULL || FOLKS_IS_INDIVIDUAL (individual));

  priv = GET_PRIV (self);

  tp_clear_object (&priv->start_individual);
  tp_clear_object (&priv->new_individual);
  g_hash_table_remove_all (priv->changed_individuals);

  if (individual != NULL)
    {
      priv->start_individual = g_object_ref (individual);
      priv->new_individual = folks_individual_new (
          folks_individual_get_personas (individual));
      empathy_individual_view_set_store (priv->individual_view,
          priv->individual_store);
    }
  else
    {
      priv->start_individual = NULL;
      priv->new_individual = NULL;

      /* We only display Individuals in the individual view if we have a
       * new_individual to link them into */
      empathy_individual_view_set_store (priv->individual_view, NULL);
    }

  empathy_individual_widget_set_individual (
      EMPATHY_INDIVIDUAL_WIDGET (priv->preview_widget), priv->new_individual);
  empathy_persona_store_set_individual (priv->persona_store,
      priv->new_individual);

  g_object_notify (G_OBJECT (self), "start-individual");
}

/**
 * empathy_individual_linker_get_linked_personas:
 * @self: an #EmpathyIndividualLinker
 *
 * Return a list of the #FolksPersona<!-- -->s which comprise the linked
 * individual currently displayed in the widget.
 *
 * The return value is guaranteed to contain at least one element.
 *
 * Return value: (transfer none) (element-type Folks.Persona): a list of
 * #FolksPersona<!-- -->s to link together
 */
GList *
empathy_individual_linker_get_linked_personas (EmpathyIndividualLinker *self)
{
  EmpathyIndividualLinkerPriv *priv;
  GList *personas;

  g_return_val_if_fail (EMPATHY_IS_INDIVIDUAL_LINKER (self), NULL);

  priv = GET_PRIV (self);

  if (priv->new_individual == NULL)
    return NULL;

  personas = folks_individual_get_personas (priv->new_individual);
  g_assert (personas != NULL);
  return personas;
}