/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Credit to Øyvind Kolås (pippin) for major GEGL contributions
 * 2026, beaver, Neon

styles color-fill=#ffffff color-policy=solidcolor 

enableoutline=true  outline=1 outline-color=#ff0000 outline-blur=3 

shadow-color=#ff84ee shadow-opacity=1 shadow-x=0 shadow-y=0

enableinnerglow=true ig-value=#ff84ee ig-radius=1 ig-grow-radius=6 ig-opacity=1

gaussian-blur std-dev-x=4 std-dev-y=4 abyss-policy=none

dropshadow x=0 y=0 grow-radius=2 radius=15 color=#ff0000 opacity=0.2

id=1 overlay srgb=true aux=[ ref=1 opacity value=0.12 ]
 



]

end of syntax
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES


property_double (blur, _("Blur"), 3.5)
  value_range   (3.0, 7.0)
  ui_range      (3.0, 7.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_("Apply a blur on what should be an alpha defined shape"))

property_int (thin_thick, _("Thin or thick"), 0)
  value_range   (-5, 25)
  ui_range      (-5, 10)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_("Thin or thicken what should be the alpha defined shape. Negative thins, positive thickens."))


property_double (glow_opacity, _("Glow opacity"), 0.05)
  value_range   (0.0, 0.5)
  ui_range      (0.0, 0.2)
  ui_steps      (0.01, 0.10)
  description    (_("Glow opacity which will also enable or disable the shadow glow effect"))
 
property_double (glow_radius, _("Glow blur radius"), 12.0)
  value_range   (10.0, 45)
  ui_range      (10.0, 45.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description    (_("The glow’s blur range"))

property_double (glow_grow_radius, _("Glow grow radius"), 8.0)
  value_range   (0.0, 20.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description    (_("The distance to expand the glow before blurring"))


property_double (ig_radius, _("Inner Glowꞌs blur radius"), 3.5)
  value_range   (0.0, 20.0)
  ui_range      (0.0, 10.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_("Blur control of the inner glow"))


property_double (ig_grow_radius, _("Inner glowꞌs grow radius"), 3)
  value_range   (1, 10)
  ui_range      (1, 10)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_("The distance to expand the inner glow before blurring"))


property_double (ig_opacity, _("Inner glowꞌs opacity"), 0.7)
  value_range   (0.0, 1.00)
  ui_steps      (0.01, 0.10)
  description (_("Opacity of the inner glow"))
	

property_double  (ig_treatment, _("Inner glowꞌs unmodified pixel fix"), 60)
  value_range (50, 85)
  description (_("Cover pixels that inner glow might miss"))



property_int  (x_shift, _("X pixel shift"), 0)
  value_range (0, 3)
  description (_("Horizontal pixel shifting"))

property_int  (y_shift, _("Y pixel shift"), 0)
  value_range (0, 3)
  description (_("Veritical pixel shifting"))

property_seed (seed, _("Random seed of noise"), rand)


property_double  (lightness, _("Lightness"), 5)
  value_range (0, 15.0)
  description (_("Lightness adjustment"))

property_double  (saturation, _("Saturation"), 1.0)
  value_range (1.0, 1.5)
  description (_("Saturation boost"))


property_int  (hue, _("Hue rotation"), 0)
  value_range (-180, 180)
  description (_("Hue rotation to change color"))

/*Properties go here*/

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     neonlights
#define GEGL_OP_C_SOURCE neonlights.c

#include "gegl-op.h"

/*starred nodes go inside typedef struct */

typedef struct
{
 GeglNode *input;
 GeglNode *styles;
 GeglNode *gaussian;
 GeglNode *dropshadow;
 GeglNode *thin;
 GeglNode *saturation;
 GeglNode *thegraph;
 GeglNode *huelight;
 GeglNode *xshift;
 GeglNode *yshift;
 GeglNode *normal;
 GeglNode *opacity;
 GeglNode *idref;
 GeglNode *output;
}State;

static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglProperties *o = GEGL_PROPERTIES (operation);
   GeglColor *red1 = gegl_color_new ("#ff0000");
   GeglColor *red2 = gegl_color_new ("#ff0000");
   GeglColor *purple1 = gegl_color_new ("#ff84ee");
   GeglColor *purple2 = gegl_color_new ("#ff84ee");
   GeglColor *white = gegl_color_new ("#ffffff");
  State *state = o->user_data = g_malloc0 (sizeof (State));

/*new child node list is here, this is where starred nodes get defined

 state->newchildname = gegl_node_new_child (gegl, "operation", "lb:name", NULL);*/
  state->input    = gegl_node_get_input_proxy (gegl, "input");
 state->dropshadow = gegl_node_new_child (gegl, "operation", "gegl:dropshadow", "x", 0.0, "y", 0.0, "radius", 15.0, "color", red1,  NULL);
 state->styles = gegl_node_new_child (gegl, "operation", "gegl:styles", "enableoutline", TRUE, "outline", 3.0, "outline-color", red2, "color-policy", 2, "color-fill", white, "shadow-color", purple1, "shadow-x", 0.0, "shadow-y", 0.0, "enableinnerglow", TRUE, "ig-value", purple2,   NULL);
 state->gaussian = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", "std-dev-x", 4.0, "std-dev-y", 4.0, "abyss-policy", 0, "clip-extent", 0, NULL);
#define geglssyntax \
" id=1 overlay srgb=true aux=[ ref=1 opacity value=0.12 ]  "\

 state->thegraph = gegl_node_new_child (gegl, "operation", "gegl:gegl", "string", geglssyntax,  NULL);
 state->huelight = gegl_node_new_child (gegl, "operation", "gegl:hue-chroma", NULL);
 state->saturation = gegl_node_new_child (gegl, "operation", "gegl:saturation", NULL);
 state->yshift = gegl_node_new_child (gegl, "operation", "gegl:shift", "shift", 0, "direction", 1, NULL);
 state->xshift = gegl_node_new_child (gegl, "operation", "gegl:shift", "shift", 0, "direction", 0, NULL);
 state->normal = gegl_node_new_child (gegl, "operation", "gegl:over",  NULL);
 state->idref = gegl_node_new_child (gegl, "operation", "gegl:nop",  NULL);
 state->opacity = gegl_node_new_child (gegl, "operation", "gegl:opacity", "value", 0.6,  NULL);
 state->thin = gegl_node_new_child (gegl, "operation", "gegl:median-blur", "radius", 0, "alpha-percentile", 100.0, "abyss-policy", 0, NULL);
  state->output   = gegl_node_get_output_proxy (gegl, "output");

  gegl_operation_meta_redirect (operation, "glow_opacity",   state->styles, "shadow_opacity");
  gegl_operation_meta_redirect (operation, "glow_grow_radius",   state->styles, "shadow_grow_radius");
  gegl_operation_meta_redirect (operation, "glow_radius",  state->styles, "shadow_radius");
  gegl_operation_meta_redirect (operation, "ig_grow_radius",   state->styles, "ig_grow_radius");
  gegl_operation_meta_redirect (operation, "ig_radius",   state->styles, "ig_radius");
  gegl_operation_meta_redirect (operation, "ig_opacity",   state->styles, "ig_opacity");
  gegl_operation_meta_redirect (operation, "ig_treatment",  state->styles, "ig_treatment");
  gegl_operation_meta_redirect (operation, "hue",  state->huelight, "hue");
  gegl_operation_meta_redirect (operation, "blur",  state->gaussian, "std-dev-x");
  gegl_operation_meta_redirect (operation, "blur",  state->gaussian, "std-dev-y");
  gegl_operation_meta_redirect (operation, "blur",  state->gaussian, "std-dev-y");
  gegl_operation_meta_redirect (operation, "lightness",  state->huelight, "lightness");
  gegl_operation_meta_redirect (operation, "thin_thick",  state->thin, "radius");
  gegl_operation_meta_redirect (operation, "saturation",  state->saturation, "scale");
  gegl_operation_meta_redirect (operation, "x_shift",  state->xshift, "shift");
  gegl_operation_meta_redirect (operation, "y_shift",  state->yshift, "shift");
  gegl_operation_meta_redirect (operation, "seed",  state->xshift, "seed");
  gegl_operation_meta_redirect (operation, "seed",  state->yshift, "seed");



/*meta redirect property to new child orders go here

 gegl_operation_meta_redirect (operation, "propertyname", state->newchildname,  "originalpropertyname");
*/

}

static void
update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;

  gegl_node_link_many (state->input, state->thin, state->styles, state->gaussian, state->dropshadow, state->thegraph, state->huelight,  state->saturation, state->idref, state->normal, state->output,  NULL);
 gegl_node_connect (state->normal, "aux", state->opacity, "output");
  gegl_node_link_many (state->idref, state->xshift, state->yshift, state->opacity,  NULL);
/*optional connect from and too is here
  gegl_node_connect (state->blendmode, "aux", state->lastnodeinlist, "output"); */

}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",        "lb:neon",
    "title",       _("Neon"),
    "reference-hash", "agovernmentthathasnorighttoexist",
    "description", _("Make alpha defined shapes a glowing neon"
                     ""),
    "gimp:menu-path", "<Image>/Filters/Text Styling",
    "gimp:menu-label", _("Neon..."),
    NULL);
}

#endif
