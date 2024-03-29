/*
 * Copyright (C) 2011 Collabora Ltd.
 *
 * The code contained in this file is free software; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either version
 * 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#include "config.h"

#include <glib/gi18n.h>

#include <gtk/gtk.h>
#include <pulse/pulseaudio.h>

#include <telepathy-glib/telepathy-glib.h>

#include "empathy-call-utils.h"

#include <libempathy/empathy-gsettings.h>
#include <libempathy/empathy-request-util.h>

#define DEBUG_FLAG EMPATHY_DEBUG_OTHER
#include <libempathy/empathy-debug.h>

static const gchar *
get_error_display_message (GError *error)
{
  if (error->domain != TP_ERROR)
    return _("There was an error starting the call");

  switch (error->code)
    {
      case TP_ERROR_NETWORK_ERROR:
        return _("Network error");
      case TP_ERROR_NOT_CAPABLE:
        return _("The specified contact doesn't support calls");
      case TP_ERROR_OFFLINE:
        return _("The specified contact is offline");
      case TP_ERROR_INVALID_HANDLE:
        return _("The specified contact is not valid");
      case TP_ERROR_EMERGENCY_CALLS_NOT_SUPPORTED:
        return _("Emergency calls are not supported on this protocol");
      case TP_ERROR_INSUFFICIENT_BALANCE:
        return _("You don't have enough credit in order to place this call");
    }

  return _("There was an error starting the call");
}

static void
show_call_error (GError *error)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (NULL, 0,
      GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
      "%s", get_error_display_message (error));

  g_signal_connect_swapped (dialog, "response",
      G_CALLBACK (gtk_widget_destroy),
      dialog);

  gtk_widget_show (dialog);
}

GHashTable *
empathy_call_create_call_request (const gchar *contact,
    gboolean initial_audio,
    gboolean initial_video)
{
  return tp_asv_new (
    TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING,
      TP_IFACE_CHANNEL_TYPE_CALL,
    TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT,
      TP_HANDLE_TYPE_CONTACT,
    TP_PROP_CHANNEL_TARGET_ID, G_TYPE_STRING,
      contact,
    TP_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO, G_TYPE_BOOLEAN,
      initial_audio,
    TP_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO, G_TYPE_BOOLEAN,
      initial_video,
    NULL);
}

GHashTable *
empathy_call_create_streamed_media_request (const gchar *contact,
    gboolean initial_audio,
    gboolean initial_video)
{
  return tp_asv_new (
    TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING,
      TP_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
    TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT,
      TP_HANDLE_TYPE_CONTACT,
    TP_PROP_CHANNEL_TARGET_ID, G_TYPE_STRING,
      contact,
    TP_PROP_CHANNEL_TYPE_STREAMED_MEDIA_INITIAL_AUDIO, G_TYPE_BOOLEAN,
      initial_audio,
    TP_PROP_CHANNEL_TYPE_STREAMED_MEDIA_INITIAL_VIDEO, G_TYPE_BOOLEAN,
      initial_video,
    NULL);
}

static void
create_streamed_media_channel_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  GError *error = NULL;

  if (!tp_account_channel_request_create_channel_finish (
           TP_ACCOUNT_CHANNEL_REQUEST (source),
           result,
           &error))
    {
      DEBUG ("Failed to create StreamedMedia channel: %s", error->message);
      show_call_error (error);
      g_error_free (error);
    }
}

static void
create_call_channel_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  TpAccountChannelRequest *streamed_media_req = user_data;
  GError *error = NULL;

  if (tp_account_channel_request_create_channel_finish (
      TP_ACCOUNT_CHANNEL_REQUEST (source), result, &error))
    {
      g_clear_object (&streamed_media_req);
      return;
    }

  DEBUG ("Failed to create Call channel: %s", error->message);

  if (streamed_media_req != NULL)
    {
      DEBUG ("Let's try with an StreamedMedia channel");
      g_error_free (error);
      tp_account_channel_request_create_channel_async (streamed_media_req,
          EMPATHY_AV_BUS_NAME, NULL,
          create_streamed_media_channel_cb,
          NULL);
      return;
    }

  show_call_error (error);
}

/* Try to request a Call channel and fallback to StreamedMedia if that fails */
static void
call_new_with_streams (const gchar *contact,
    TpAccount *account,
    gboolean initial_audio,
    gboolean initial_video,
    gint64 timestamp)
{
  GHashTable *call_request;
  TpAccountChannelRequest *call_req, *streamed_media_req = NULL;
#ifdef HAVE_EMPATHY_AV
  GHashTable *streamed_media_request;
#endif

  /* Call */
  call_request = empathy_call_create_call_request (contact,
      initial_audio,
      initial_video);

  call_req = tp_account_channel_request_new (account, call_request, timestamp);

  g_hash_table_unref (call_request);

#ifdef HAVE_EMPATHY_AV
  /* StreamedMedia */
  streamed_media_request = empathy_call_create_streamed_media_request (
      contact, initial_audio, initial_video);

  streamed_media_req = tp_account_channel_request_new (account,
      streamed_media_request,
      timestamp);

  g_hash_table_unref (streamed_media_request);
#endif

  tp_account_channel_request_create_channel_async (call_req,
      EMPATHY_CALL_BUS_NAME, NULL, create_call_channel_cb, streamed_media_req);

  g_object_unref (call_req);
}

void
empathy_call_new_with_streams (const gchar *contact,
    TpAccount *account,
    gboolean initial_audio,
    gboolean initial_video,
    gint64 timestamp)
{
  call_new_with_streams (contact, account, initial_audio, initial_video,
      timestamp);
}

void
empathy_call_set_stream_properties (GstElement *element,
  gboolean echo_cancellation)
{
  GstStructure *props;
  GSettings *gsettings_call;
  gboolean echo_cancellation_setting;

  gsettings_call = g_settings_new (EMPATHY_PREFS_CALL_SCHEMA);

  echo_cancellation_setting = g_settings_get_boolean (gsettings_call,
      EMPATHY_PREFS_CALL_ECHO_CANCELLATION);

  DEBUG ("Echo cancellation: element allowed: %s, user enabled: %s",
    echo_cancellation ? " yes" : "no",
    echo_cancellation_setting ? " yes" : "no");


  props = gst_structure_new ("props",
      PA_PROP_MEDIA_ROLE, G_TYPE_STRING, "phone",
      NULL);

  if (echo_cancellation && echo_cancellation_setting)
    {
      gst_structure_set (props,
          "filter.want", G_TYPE_STRING, "echo-cancel",
          NULL);
    }

  g_object_set (element, "stream-properties", props, NULL);
  gst_structure_free (props);

  g_object_unref (gsettings_call);
}

/* Copied from telepathy-yell call-channel.c */
void
empathy_call_channel_send_video (TpCallChannel *self,
    gboolean send)
{
  GPtrArray *contents;
  gboolean found = FALSE;
  guint i;

  g_return_if_fail (TP_IS_CALL_CHANNEL (self));

  /* Loop over all the contents, if some of them a video set all their
   * streams to sending, otherwise request a video channel in case we want to
   * sent */
  contents = tp_call_channel_get_contents (self);
  for (i = 0 ; i < contents->len ; i++)
    {
      TpCallContent *content = g_ptr_array_index (contents, i);

      if (tp_call_content_get_media_type (content) ==
              TP_MEDIA_STREAM_TYPE_VIDEO)
        {
          GPtrArray *streams;
          guint j;

          found = TRUE;
          streams = tp_call_content_get_streams (content);
          for (j = 0; j < streams->len; j++)
            {
              TpCallStream *stream = g_ptr_array_index (streams, j);

              tp_call_stream_set_sending_async (stream, send, NULL, NULL);
            }
        }
    }

  if (send && !found)
    {
      tp_call_channel_add_content_async (self, "video",
          TP_MEDIA_STREAM_TYPE_VIDEO, TP_MEDIA_STREAM_DIRECTION_BIDIRECTIONAL,
          NULL, NULL);
    }
}

/* Copied from telepathy-yell call-channel.c */
TpSendingState
empathy_call_channel_get_video_state (TpCallChannel *self)
{
  TpSendingState result = TP_SENDING_STATE_NONE;
  GPtrArray *contents;
  guint i;

  g_return_val_if_fail (TP_IS_CALL_CHANNEL (self), TP_SENDING_STATE_NONE);

  contents = tp_call_channel_get_contents (self);
  for (i = 0 ; i < contents->len ; i++)
    {
      TpCallContent *content = g_ptr_array_index (contents, i);

      if (tp_call_content_get_media_type (content) ==
              TP_MEDIA_STREAM_TYPE_VIDEO)
        {
          GPtrArray *streams;
          guint j;

          streams = tp_call_content_get_streams (content);
          for (j = 0; j < streams->len; j++)
            {
              TpCallStream *stream = g_ptr_array_index (streams, j);
              TpSendingState state;

              state = tp_call_stream_get_local_sending_state (stream);
              if (state != TP_SENDING_STATE_PENDING_STOP_SENDING &&
                  state > result)
                result = state;
            }
        }
    }

  return result;
}
