/*
 * empathy-new-account-dialog.h - Source for EmpathyNewAccountDialog
 *
 * Copyright (C) 2011 - Collabora Ltd.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with This library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ___EMPATHY_NEW_ACCOUNT_DIALOG_H__
#define ___EMPATHY_NEW_ACCOUNT_DIALOG_H__

#include <gtk/gtk.h>

#include <libempathy/empathy-account-settings.h>

G_BEGIN_DECLS

#define EMPATHY_TYPE_NEW_ACCOUNT_DIALOG (empathy_new_account_dialog_get_type ())
#define EMPATHY_NEW_ACCOUNT_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EMPATHY_TYPE_NEW_ACCOUNT_DIALOG, EmpathyNewAccountDialog))
#define EMPATHY_NEW_ACCOUNT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), EMPATHY_TYPE_NEW_ACCOUNT_DIALOG, EmpathyNewAccountDialogClass))
#define EMPATHY_IS_NEW_ACCOUNT_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EMPATHY_TYPE_NEW_ACCOUNT_DIALOG))
#define EMPATHY_IS_NEW_ACCOUNT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EMPATHY_TYPE_NEW_ACCOUNT_DIALOG))
#define EMPATHY_NEW_ACCOUNT_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), EMPATHY_TYPE_NEW_ACCOUNT_DIALOG, EmpathyNewAccountDialogClass))

typedef struct _EmpathyNewAccountDialog EmpathyNewAccountDialog;
typedef struct _EmpathyNewAccountDialogClass EmpathyNewAccountDialogClass;
typedef struct _EmpathyNewAccountDialogPrivate EmpathyNewAccountDialogPrivate;

struct _EmpathyNewAccountDialog {
  GtkDialog parent;

  EmpathyNewAccountDialogPrivate *priv;
};

struct _EmpathyNewAccountDialogClass {
  GtkDialogClass parent_class;
};

GType empathy_new_account_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * empathy_new_account_dialog_new (GtkWindow *parent);

EmpathyAccountSettings * empathy_new_account_dialog_get_settings (
    EmpathyNewAccountDialog *self);

G_END_DECLS

#endif /* ___EMPATHY_NEW_ACCOUNT_DIALOG_H__ */
