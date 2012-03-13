/*
 * empathy-calendar-button.h - Header for EmpathyCalendarButton
 * Copyright (C) 2012 Collabora Ltd.
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

#ifndef __EMPATHY_CALENDAR_BUTTON_H__
#define __EMPATHY_CALENDAR_BUTTON_H__

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _EmpathyCalendarButton EmpathyCalendarButton;
typedef struct _EmpathyCalendarButtonClass EmpathyCalendarButtonClass;
typedef struct _EmpathyCalendarButtonPriv EmpathyCalendarButtonPriv;

struct _EmpathyCalendarButtonClass {
    GtkBoxClass parent_class;
};

struct _EmpathyCalendarButton {
    GtkBox parent;
    EmpathyCalendarButtonPriv *priv;
};

GType empathy_calendar_button_get_type (void);

#define EMPATHY_TYPE_CALENDAR_BUTTON \
  (empathy_calendar_button_get_type ())
#define EMPATHY_CALENDAR_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EMPATHY_TYPE_CALENDAR_BUTTON, \
    EmpathyCalendarButton))
#define EMPATHY_CALENDAR_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EMPATHY_TYPE_CALENDAR_BUTTON, \
  EmpathyCalendarButtonClass))
#define EMPATHY_IS_CALENDAR_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EMPATHY_TYPE_CALENDAR_BUTTON))
#define EMPATHY_IS_CALENDAR_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EMPATHY_TYPE_CALENDAR_BUTTON))
#define EMPATHY_CALENDAR_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EMPATHY_TYPE_CALENDAR_BUTTON, \
  EmpathyCalendarButtonClass))

GtkWidget * empathy_calendar_button_new (void);

GDate * empathy_calendar_button_get_date (EmpathyCalendarButton *self);

void empathy_calendar_button_set_date (EmpathyCalendarButton *self,
    GDate *date);

G_END_DECLS

#endif /* #ifndef __EMPATHY_CALENDAR_BUTTON_H__*/
