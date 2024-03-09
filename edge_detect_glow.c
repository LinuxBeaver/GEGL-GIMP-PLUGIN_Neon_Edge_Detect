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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * 2023 Beaver, Edge Detect that Glows

Test this filter without installing by pasting this GEGL Syntax inside Gimp's GEGL Graph filter

edge algorithm=sobel amount=2
id=1
bloom radius=0
color-to-alpha color=#000000
gimp:layer-mode opacity=1 layer-mode=overlay composite-space=lab   aux=[ color value=black  opacity value=2.5 ]
levels in-low=0.010
crop
denoise-dct sigma=2

bloom strength=3

end of syntax

 THIS FILTER WAS INSPIRED BY FILTERS I SAW INSIDE IN ADOBE PHOTOSHOP CS5

 */

#include "config.h"
#include <glib/gi18n-lib.h>


/*I renamed GEGL_edge_algo to meme_edge_algo to make a custom enum list name to prevent them from conflicting and crashing Gimp. If I didn't do this gimp would fail to startup

ORIGINAL ENUM LIST NAME

enum_start (gegl_edge_algo)
   enum_value (GEGL_EDGE_SOBEL,    "sobel",    N_("Sobel"))
   enum_value (GEGL_EDGE_PREWITT,  "prewitt",  N_("Prewitt compass"))
   enum_value (GEGL_EDGE_GRADIENT, "gradient", N_("Gradient"))
   enum_value (GEGL_EDGE_ROBERTS,  "roberts",  N_("Roberts"))
   enum_value (GEGL_EDGE_DIFFERENTIAL, "differential", N_("Differential"))
   enum_value (GEGL_EDGE_LAPLACE,  "laplace",  N_("Laplace"))
enum_end (GeglEdgeAlgo)
*/
 
#ifdef GEGL_PROPERTIES

enum_start (meme_edge_algo)
   enum_value (GEGL_EDGE_SOBEL,    "sobel",    N_("Sobel"))
   enum_value (GEGL_EDGE_PREWITT,  "prewitt",  N_("Prewitt compass"))
   enum_value (GEGL_EDGE_GRADIENT, "gradient", N_("Gradient"))
   enum_value (GEGL_EDGE_ROBERTS,  "roberts",  N_("Roberts"))
   enum_value (GEGL_EDGE_DIFFERENTIAL, "differential", N_("Differential"))
enum_end (memeEdgeAlgo)

/*Edge Detect "laplace" was removed from the enum lsit for glitching out and looking bad.*/

property_enum (algorithm, _("Algorithm"),
               memeEdgeAlgo, meme_edge_algo,
               GEGL_EDGE_SOBEL)
  description (_("Edge detection algorithm"))

enum_start (edge_detect_type)
  enum_value (EDGEDETECT,      "edgedetect",
              N_("Edge Detect Mode"))
  enum_value (GLOW,      "glow",
              N_("Original Color Glow Mode"))
enum_end (edgedetecttype)

property_enum (mode, _("Mode"),
    edgedetecttype, edge_detect_type,
    EDGEDETECT)
   description  (_("Edge Detect mode"))


property_double (amount, _("Edge Amount"), 2.0)
    description (_("Edge detection amount"))
    value_range (1.0, 10.0)
    ui_range    (1.0, 10.0)

property_double (glow, _("Glow Strength"), 0.0)
    description (_("Glow strength using gegl:bloom"))
    value_range (0.0, 30.0)
    ui_range    (0.0, 30.0)
ui_meta ("visible", "!mode {glow}" )

property_double (glow2, _("Glow Strength"), 0.0)
    description (_("Glow strength using gegl:bloom"))
    value_range (0.0, 50.0)
    ui_range    (0.0, 50.0)
ui_meta ("visible", "!mode {edgedetect}" )

property_double (hue, _("Hue Rotate"), 0.0)
    description (_("Rotate Hue of the Edge Detect"))
    value_range (-180, 180.0)
ui_meta ("visible", "!mode {glow}" )

property_double (desaturate, _("Desaturation"), 0.0)
    description (_("Desaturate Edge Detect to make it white"))
    value_range (-100.0, 0.0)
ui_meta ("visible", "!mode {glow}" )

property_int (smooth, _("Edge Smooth"), 5)
    description (_("Mean Curvature Smoothing"))
    value_range (0, 10)
    ui_range    (0, 10)
ui_meta ("visible", "!mode {edgedetect}" )

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     edge_detect_glow
#define GEGL_OP_C_SOURCE edge_detect_glow.c

#include "gegl-op.h"

typedef struct
{
 GeglNode *input;
 GeglNode *glowgraph;
 GeglNode *bloom;
 GeglNode *bloom2;
 GeglNode *edge;
 GeglNode *huechroma;
 GeglNode *mcb;
 GeglNode *luminance;
 GeglNode *output;
}State;

static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglProperties *o = GEGL_PROPERTIES (operation);

  State *state = o->user_data = g_malloc0 (sizeof (State));

  state->input    = gegl_node_get_input_proxy (gegl, "input");
  state->output   = gegl_node_get_output_proxy (gegl, "output");

#define glowstring \
" id=alphalock src-in aux=[  ref=alphalock id=1 bloom radius=0 color-to-alpha color=#000000 gimp:layer-mode opacity=1 layer-mode=overlay  composite-space=lab   aux=[ color value=black  opacity value=2.5 ] levels in-low=0.010 crop denoise-dct sigma=2 ] median-blur radius=0    "\
/*This long GEGL syntax string is below edge detect and makes the glow effect. It also uses a GEGL exclusive blend mode (src-in) and behaves like Gimp's lock alpha channel feature.*/ 

                state->glowgraph = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", glowstring,
                                  NULL);

  state->bloom = gegl_node_new_child (gegl,
                                  "operation", "gegl:bloom", 
                                  NULL);

  state->bloom2 = gegl_node_new_child (gegl,
                                  "operation", "gegl:bloom", 
                                  NULL);

  state->edge = gegl_node_new_child (gegl,
                                  "operation", "gegl:edge", 
                                  NULL);

  state->huechroma = gegl_node_new_child (gegl,
                                  "operation", "gegl:hue-chroma", 
                                  NULL);

  state->mcb = gegl_node_new_child (gegl,
                                  "operation", "gegl:mean-curvature-blur", "iterations", 5,
                                  NULL);

  state->luminance = gegl_node_new_child (gegl,
                                  "operation", "gimp:layer-mode", "layer-mode", 56, "blend-space", 2, 
                                  NULL);

 gegl_operation_meta_redirect (operation, "amount", state->edge, "amount"); 
 gegl_operation_meta_redirect (operation, "algorithm", state->edge, "algorithm"); 
 gegl_operation_meta_redirect (operation, "glow", state->bloom, "strength"); 
 gegl_operation_meta_redirect (operation, "glow2", state->bloom2, "strength"); 
 gegl_operation_meta_redirect (operation, "desaturate", state->huechroma, "chroma"); 
 gegl_operation_meta_redirect (operation, "hue", state->huechroma, "hue"); 
 gegl_operation_meta_redirect (operation, "smooth", state->mcb, "iterations"); 


} 

static void update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;

switch (o->mode) {
        break;
    case EDGEDETECT:
  gegl_node_link_many (state->input, state->edge, state->glowgraph, state->bloom, state->huechroma, state->output, NULL);
        break;
    case GLOW:
  gegl_node_link_many (state->input, state->luminance, state->mcb, state->bloom2, state->output, NULL);
  gegl_node_link_many (state->edge, state->glowgraph, state->bloom,  NULL);
    gegl_node_connect (state->luminance, "aux", state->bloom, "output");
        break;
    }
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
    "name",        "lb:edge",
    "title",       _("Neon Edge Detect"),
    "reference-hash", "karlmarxruinedhumanity33r41afproudonwasrightthough",
    "description", _("A better edge detect algorithm that optionally glows"),
    "gimp:menu-path", "<Image>/Filters/Edge-Detect",
    "gimp:menu-label", _("Neon Edge Detect"),
    NULL);
}

#endif
