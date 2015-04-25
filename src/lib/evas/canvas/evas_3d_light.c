#include "evas_common_private.h"
#include "evas_private.h"

#define MY_CLASS EVAS_3D_LIGHT_CLASS

static Eina_Bool
_light_node_change_notify(const Eina_Hash *hash EINA_UNUSED, const void *key,
                        void *data EINA_UNUSED, void *fdata)
{
   Evas_3D_Node *n = *(Evas_3D_Node **)key;
   eo_do(n, evas_3d_object_change(EVAS_3D_STATE_NODE_LIGHT, (Evas_3D_Object *)fdata));
   return EINA_TRUE;
}

EOLIAN static void
_evas_3d_light_evas_3d_object_change_notify(Eo *obj, Evas_3D_Light_Data *pd, Evas_3D_State state EINA_UNUSED, Evas_3D_Object *ref EINA_UNUSED)
{
   if (pd->nodes)
     eina_hash_foreach(pd->nodes, _light_node_change_notify, obj);
}

EOLIAN static void
_evas_3d_light_evas_3d_object_update_notify(Eo *obj EINA_UNUSED, Evas_3D_Light_Data *pd EINA_UNUSED)
{

}

void
evas_3d_light_node_add(Evas_3D_Light *light, Evas_3D_Node *node)
{
   int count = 0;
   Evas_3D_Light_Data *pd = eo_data_scope_get(light, MY_CLASS);
   if (pd->nodes == NULL)
     {
        pd->nodes = eina_hash_pointer_new(NULL);

        if (pd->nodes == NULL)
          {
             ERR("Failed to create hash table.");
             return;
          }
     }
   else
     count = (int)(uintptr_t)eina_hash_find(pd->nodes, &node);

   eina_hash_set(pd->nodes, &node, (const void *)(uintptr_t)(count + 1));
}

void
evas_3d_light_node_del(Evas_3D_Light *light, Evas_3D_Node *node)
{
   int count = 0;
   Evas_3D_Light_Data *pd = eo_data_scope_get(light, MY_CLASS);
   if (pd->nodes == NULL)
     {
        ERR("No node to delete.");
        return;
     }

   count = (int)(uintptr_t)eina_hash_find(pd->nodes, &node);

   if (count == 1)
     eina_hash_del(pd->nodes, &node, NULL);
   else
     eina_hash_set(pd->nodes, &node, (const void *)(uintptr_t)(count - 1));
}


EAPI Evas_3D_Light *
evas_3d_light_add(Evas *e)
{
   MAGIC_CHECK(e, Evas, MAGIC_EVAS);
   return NULL;
   MAGIC_CHECK_END();
   Evas_Object *eo_obj = eo_add(MY_CLASS, e);
   return eo_obj;
}

EOLIAN static void
_evas_3d_light_eo_base_constructor(Eo *obj, Evas_3D_Light_Data *pd)
{
   eo_do_super(obj, MY_CLASS, eo_constructor());
   eo_do(obj, evas_3d_object_type_set(EVAS_3D_OBJECT_TYPE_LIGHT));
   evas_color_set(&pd->ambient, 0.0, 0.0, 0.0, 1.0);
   evas_color_set(&pd->diffuse, 1.0, 1.0, 1.0, 1.0);
   evas_color_set(&pd->specular, 1.0, 1.0, 1.0, 1.0);

   pd->spot_exp = 0.0;
   pd->spot_cutoff = 180.0;
   pd->spot_cutoff_cos = -1.0;

   pd->atten_const = 1.0;
   pd->atten_linear = 0.0;
   pd->atten_quad = 0.0;
}

EOLIAN static void
_evas_3d_light_eo_base_destructor(Eo *obj, Evas_3D_Light_Data *pd)
{
   if (pd->nodes)
     eina_hash_free(pd->nodes);
   eo_do_super(obj, MY_CLASS, eo_destructor());
}


EOLIAN static void
_evas_3d_light_directional_set(Eo *obj, Evas_3D_Light_Data *pd, Eina_Bool directional)
{
   if (pd->directional != directional)
     {
        pd->directional = directional;
        eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_ANY, NULL));
     }
}

EOLIAN static Eina_Bool
_evas_3d_light_directional_get(Eo *obj EINA_UNUSED, Evas_3D_Light_Data *pd)
{
   return pd->directional;
}

EOLIAN static void
_evas_3d_light_ambient_set(Eo *obj, Evas_3D_Light_Data *pd, Evas_Real r, Evas_Real g, Evas_Real b, Evas_Real a)
{
   pd->ambient.r = r;
   pd->ambient.g = g;
   pd->ambient.b = b;
   pd->ambient.a = a;

   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_LIGHT_AMBIENT, NULL));
}

EOLIAN static void
_evas_3d_light_ambient_get(Eo *obj EINA_UNUSED, Evas_3D_Light_Data *pd, Evas_Real *r, Evas_Real *g, Evas_Real *b, Evas_Real *a)
{
   if (r) *r = pd->ambient.r;
   if (g) *g = pd->ambient.g;
   if (b) *b = pd->ambient.b;
   if (a) *a = pd->ambient.a;
}

EOLIAN static void
_evas_3d_light_diffuse_set(Eo *obj, Evas_3D_Light_Data *pd, Evas_Real r, Evas_Real g, Evas_Real b, Evas_Real a)
{
   pd->diffuse.r = r;
   pd->diffuse.g = g;
   pd->diffuse.b = b;
   pd->diffuse.a = a;

   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_LIGHT_DIFFUSE, NULL));
}

EOLIAN static void
_evas_3d_light_diffuse_get(Eo *obj EINA_UNUSED, Evas_3D_Light_Data *pd, Evas_Real *r, Evas_Real *g, Evas_Real *b, Evas_Real *a)
{
   if (r) *r = pd->diffuse.r;
   if (g) *g = pd->diffuse.g;
   if (b) *b = pd->diffuse.b;
   if (a) *a = pd->diffuse.a;
}

EOLIAN static void
_evas_3d_light_specular_set(Eo *obj, Evas_3D_Light_Data *pd, Evas_Real r, Evas_Real g, Evas_Real b, Evas_Real a)
{
   pd->specular.r = r;
   pd->specular.g = g;
   pd->specular.b = b;
   pd->specular.a = a;

   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_LIGHT_SPECULAR, NULL));
}

EOLIAN static void
_evas_3d_light_specular_get(Eo *obj EINA_UNUSED, Evas_3D_Light_Data *pd, Evas_Real *r, Evas_Real *g, Evas_Real *b, Evas_Real *a)
{
   if (r) *r = pd->specular.r;
   if (g) *g = pd->specular.g;
   if (b) *b = pd->specular.b;
   if (a) *a = pd->specular.a;
}

EOLIAN static void
_evas_3d_light_spot_exponent_set(Eo *obj, Evas_3D_Light_Data *pd, Evas_Real exponent)
{
   pd->spot_exp = exponent;
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_LIGHT_SPOT_EXP, NULL));
}

EOLIAN static Evas_Real
_evas_3d_light_spot_exponent_get(Eo *obj EINA_UNUSED, Evas_3D_Light_Data *pd)
{
   return pd->spot_exp;
}

EOLIAN static void
_evas_3d_light_spot_cutoff_set(Eo *obj, Evas_3D_Light_Data *pd, Evas_Real cutoff)
{
   pd->spot_cutoff = cutoff;
   pd->spot_cutoff_cos = cos(cutoff * M_PI / 180.0);
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_LIGHT_SPOT_CUTOFF, NULL));
}

EOLIAN static Evas_Real
_evas_3d_light_spot_cutoff_get(Eo *obj EINA_UNUSED, Evas_3D_Light_Data *pd)
{
   return pd->spot_cutoff;
}

EOLIAN static void
_evas_3d_light_attenuation_set(Eo *obj, Evas_3D_Light_Data *pd, Evas_Real constant, Evas_Real linear, Evas_Real quadratic)
{
   pd->atten_const = constant;
   pd->atten_linear = linear;
   pd->atten_quad = quadratic;
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_LIGHT_ATTENUATION, NULL));
}

EOLIAN static void
_evas_3d_light_attenuation_get(Eo *obj EINA_UNUSED, Evas_3D_Light_Data *pd, Evas_Real *constant, Evas_Real *linear, Evas_Real *quadratic)
{
   if (constant) *constant = pd->atten_const;
   if (linear) *linear = pd->atten_linear;
   if (quadratic) *quadratic = pd->atten_quad;
}

EOLIAN static void
_evas_3d_light_attenuation_enable_set(Eo *obj, Evas_3D_Light_Data *pd, Eina_Bool enable)
{
   if (pd->enable_attenuation != enable)
     {
        pd->enable_attenuation = enable;
        eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_LIGHT_ATTENUATION, NULL));
     }
}

EOLIAN static Eina_Bool
_evas_3d_light_attenuation_enable_get(Eo *obj EINA_UNUSED, Evas_3D_Light_Data *pd)
{
   return pd->enable_attenuation;
}

EOLIAN static void
_evas_3d_light_projection_matrix_set(Eo *obj, Evas_3D_Light_Data *pd,
                                         const Evas_Real *matrix)
{
   evas_mat4_array_set(&pd->projection, matrix);
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_LIGHT_PROJECTION, NULL));
}

EOLIAN static void
_evas_3d_light_projection_matrix_get(Eo *obj EINA_UNUSED,
                                         Evas_3D_Light_Data *pd,
                                         Evas_Real *matrix)
{
   memcpy(matrix, &pd->projection.m[0], sizeof(Evas_Real) * 16);
}

EOLIAN static void
_evas_3d_light_projection_perspective_set(Eo *obj, Evas_3D_Light_Data *pd,
                                              Evas_Real fovy, Evas_Real aspect,
                                              Evas_Real dnear, Evas_Real dfar)
{
   Evas_Real   xmax;
   Evas_Real   ymax;

   ymax = dnear * (Evas_Real)tan((double)fovy * M_PI / 360.0);
   xmax = ymax * aspect;

   evas_mat4_frustum_set(&pd->projection, -xmax, xmax, -ymax, ymax, dnear, dfar);
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_LIGHT_PROJECTION, NULL));
}

EOLIAN static void
_evas_3d_light_projection_frustum_set(Eo *obj, Evas_3D_Light_Data *pd,
                                          Evas_Real left, Evas_Real right,
                                          Evas_Real bottom, Evas_Real top,
                                          Evas_Real dnear, Evas_Real dfar)
{
   evas_mat4_frustum_set(&pd->projection, left, right, bottom, top, dnear, dfar);
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_LIGHT_PROJECTION, NULL));
}

EOLIAN static void
_evas_3d_light_projection_ortho_set(Eo *obj, Evas_3D_Light_Data *pd,
                                        Evas_Real left, Evas_Real right,
                                        Evas_Real bottom, Evas_Real top,
                                        Evas_Real dnear, Evas_Real dfar)
{
   evas_mat4_ortho_set(&pd->projection, left, right, bottom, top, dnear, dfar);
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_LIGHT_PROJECTION, NULL));
}


#include "canvas/evas_3d_light.eo.c"
