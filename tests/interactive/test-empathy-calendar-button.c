/*
 * Copyright (C) 2012 Collabora Ltd.
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
 *
 * Authors: Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#include "config.h"

#include <libempathy-gtk/empathy-calendar-button.h>

static void
date_changed_cb (EmpathyCalendarButton *button,
    GDate *date,
    gpointer user_data)
{
  if (date == NULL)
    {
      g_print ("date changed: none\n");
    }
  else
    {
      gchar buffer[128];

      g_date_strftime (buffer, 128, "%x", date);
      g_print ("date changed: %s\n", buffer);
    }
}

int
main (int argc,
    char **argv)
{
  GtkWidget *win, *button;
  GDate *date;

  gtk_init (&argc, &argv);

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  g_signal_connect_swapped (win, "destroy",
      G_CALLBACK (gtk_main_quit), NULL);

  button = empathy_calendar_button_new ();

  date = g_date_new_dmy (30, 11, 1984);
  empathy_calendar_button_set_date (EMPATHY_CALENDAR_BUTTON (button), date);
  g_date_free (date);

  g_signal_connect (button, "date-changed",
      G_CALLBACK (date_changed_cb), NULL);

  gtk_container_add (GTK_CONTAINER (win), button);
  gtk_widget_show_all (win);

  gtk_main ();
  return 0;
}
