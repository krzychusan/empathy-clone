/*
 * empathy-sanity-cleaning.c
 * Code automatically called when starting a specific version of Empathy for
 * the first time doing misc cleaning.
 *
 * Copyright (C) 2012 Collabora Ltd.
 * @author Guillaume Desmottes <guillaume.desmottes@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "empathy-sanity-cleaning.h"

#include <telepathy-glib/telepathy-glib.h>

#include <libempathy/empathy-gsettings.h>

#define DEBUG_FLAG EMPATHY_DEBUG_OTHER
#include <libempathy/empathy-debug.h>

/*
 * This number has to be increased each time a new task is added or modified.
 *
 * If the number stored in gsettings is lower than it, all the tasks will
 * be executed.
 */
#define SANITY_CLEANING_NUMBER 2

static void
account_update_parameters_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  GError *error = NULL;
  TpAccount *account = TP_ACCOUNT (source);

  if (!tp_account_update_parameters_finish (account, result, NULL, &error))
    {
      DEBUG ("Failed to update parameters of account '%s': %s",
          tp_account_get_path_suffix (account), error->message);

      g_error_free (error);
      return;
    }

  tp_account_reconnect_async (account, NULL, NULL);
}

/* Make sure XMPP accounts don't have a negative priority (bgo #671452) */
static void
fix_xmpp_account_priority (TpAccountManager *am)
{
  GList *accounts, *l;

  accounts = tp_account_manager_get_valid_accounts (am);
  for (l = accounts; l != NULL; l = g_list_next (l))
    {
      TpAccount *account = l->data;
      GHashTable *params;
      gint priority;

      if (tp_strdiff (tp_account_get_protocol (account), "jabber"))
        continue;

      params = (GHashTable *) tp_account_get_parameters (account);
      if (params == NULL)
        continue;

      priority = tp_asv_get_int32 (params, "priority", NULL);
      if (priority >= 0)
        continue;

      DEBUG ("Resetting XMPP priority of account '%s' to 0",
          tp_account_get_path_suffix (account));

      params = tp_asv_new (
          "priority", G_TYPE_INT, 0,
          NULL);

      tp_account_update_parameters_async (account, params, NULL,
          account_update_parameters_cb, NULL);

      g_hash_table_unref (params);
    }

  g_list_free (accounts);
}

static void
set_facebook_account_fallback_server (TpAccountManager *am)
{
  GList *accounts, *l;

  accounts = tp_account_manager_get_valid_accounts (am);
  for (l = accounts; l != NULL; l = g_list_next (l))
    {
      TpAccount *account = l->data;
      GHashTable *params;
      gchar *fallback_servers[] = {
          "chat.facebook.com:443",
          NULL };

      if (tp_strdiff (tp_account_get_service (account), "facebook"))
        continue;

      params = (GHashTable *) tp_account_get_parameters (account);
      if (params == NULL)
        continue;

      if (tp_asv_get_strv (params, "fallback-servers") != NULL)
        continue;

      DEBUG ("Setting chat.facebook.com:443 as a fallback on account '%s'",
          tp_account_get_path_suffix (account));

      params = tp_asv_new (
          "fallback-servers", G_TYPE_STRV, fallback_servers,
          NULL);

      tp_account_update_parameters_async (account, params, NULL,
          account_update_parameters_cb, NULL);

      g_hash_table_unref (params);
    }

  g_list_free (accounts);
}

static void
run_sanity_cleaning_tasks (TpAccountManager *am)
{
  DEBUG ("Starting sanity cleaning tasks");

  fix_xmpp_account_priority (am);
  set_facebook_account_fallback_server (am);
}

static void
am_prepare_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  GError *error = NULL;
  TpAccountManager *am = TP_ACCOUNT_MANAGER (source);

  if (!tp_proxy_prepare_finish (am, result, &error))
    {
      DEBUG ("Failed to prepare account manager: %s", error->message);
      g_error_free (error);
      return;
    }

  run_sanity_cleaning_tasks (am);
}

void empathy_sanity_checking_run_if_needed (void)
{
  GSettings *settings;
  guint number;
  TpAccountManager *am;

  settings = g_settings_new (EMPATHY_PREFS_SCHEMA);
  number = g_settings_get_uint (settings, EMPATHY_PREFS_SANITY_CLEANING_NUMBER);

  if (number == SANITY_CLEANING_NUMBER)
    goto out;

  am = tp_account_manager_dup ();

  tp_proxy_prepare_async (am, NULL, am_prepare_cb, NULL);

  g_settings_set_uint (settings, EMPATHY_PREFS_SANITY_CLEANING_NUMBER,
      SANITY_CLEANING_NUMBER);

  g_object_unref (am);
out:
  g_object_unref (settings);
}
