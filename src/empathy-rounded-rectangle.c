/*
 * empathy-rounded-rectangle.c - Source for EmpathyRoundedRectangle
 * Copyright (C) 2011 Collabora Ltd.
 * @author Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
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

#include <math.h>

#include <clutter/clutter.h>

#include "empathy-rounded-rectangle.h"

G_DEFINE_TYPE (EmpathyRoundedRectangle,
    empathy_rounded_rectangle,
    CLUTTER_TYPE_CAIRO_TEXTURE)

struct _EmpathyRoundedRectanglePriv
{
  guint width, height;
  ClutterColor border_color;
  guint border_width;
};

static void
empathy_rounded_rectangle_paint (EmpathyRoundedRectangle *self)
{
  guint width, height;
  guint tmp_alpha;
  cairo_t *cr;

#define RADIUS (height / 8.)

  width = self->priv->width;
  height = self->priv->height;

  /* compute the composited opacity of the actor taking into
   * account the opacity of the color set by the user */
  tmp_alpha = clutter_actor_get_paint_opacity (CLUTTER_ACTOR (self))
            * self->priv->border_color.alpha
            / 255;

  cr = clutter_cairo_texture_create (CLUTTER_CAIRO_TEXTURE (self));

  cairo_set_source_rgba (cr,
      self->priv->border_color.red,
      self->priv->border_color.green,
      self->priv->border_color.blue,
      tmp_alpha);

  cairo_set_line_width (cr, self->priv->border_width);

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  /* make room for the portion of the border drawn on the outside */
  cairo_translate (cr, self->priv->border_width/2.0, self->priv->border_width/2.0);

  cairo_new_sub_path (cr);
  cairo_arc (cr, width - RADIUS, RADIUS, RADIUS,
      -M_PI/2.0, 0);
  cairo_arc (cr, width - RADIUS, height - RADIUS, RADIUS,
      0, M_PI/2.0);
  cairo_arc (cr, RADIUS, height - RADIUS, RADIUS,
      M_PI/2.0, M_PI);
  cairo_arc (cr, RADIUS, RADIUS, RADIUS,
      M_PI, -M_PI/2.0);
  cairo_close_path (cr);

  cairo_stroke (cr);
  cairo_destroy (cr);

#undef RADIUS
}

static void
empathy_rounded_rectangle_init (EmpathyRoundedRectangle *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EMPATHY_TYPE_ROUNDED_RECTANGLE, EmpathyRoundedRectanglePriv);

  self->priv->border_width = 1;
}

static void
empathy_rounded_rectangle_class_init (EmpathyRoundedRectangleClass *klass)
{
  g_type_class_add_private (klass, sizeof (EmpathyRoundedRectanglePriv));
}

static void
empathy_rounded_rectangle_update_surface_size (EmpathyRoundedRectangle *self)
{
  clutter_cairo_texture_set_surface_size (CLUTTER_CAIRO_TEXTURE (self),
      self->priv->width + self->priv->border_width,
      self->priv->height + self->priv->border_width);
}

ClutterActor *
empathy_rounded_rectangle_new (guint width,
    guint height)
{
  EmpathyRoundedRectangle *self;

  self = EMPATHY_ROUNDED_RECTANGLE (g_object_new (EMPATHY_TYPE_ROUNDED_RECTANGLE, NULL));

  self->priv->width = width;
  self->priv->height = height;

  empathy_rounded_rectangle_update_surface_size (self);
  empathy_rounded_rectangle_paint (self);

  return CLUTTER_ACTOR (self);
}

void
empathy_rounded_rectangle_set_border_width (EmpathyRoundedRectangle *self,
    guint border_width)
{
  self->priv->border_width = border_width;

  empathy_rounded_rectangle_update_surface_size (self);
  empathy_rounded_rectangle_paint (self);
}

void
empathy_rounded_rectangle_set_border_color (EmpathyRoundedRectangle *self,
    const ClutterColor *color)
{
  self->priv->border_color = *color;

  empathy_rounded_rectangle_paint (self);
}
