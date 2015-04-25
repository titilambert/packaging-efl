#include "evas_common_private.h"
#include "evas_private.h"

EOLIAN void
_evas_object_name_set(Eo *eo_obj, Evas_Object_Protected_Data *obj, const char *name)
{
   if (obj->name)
     {
        if (obj->layer && obj->layer->evas && obj->layer->evas->name_hash)
          eina_hash_del(obj->layer->evas->name_hash, obj->name, eo_obj);
        free(obj->name);
     }
   if (!name) obj->name = NULL;
   else
     {
        obj->name = strdup(name);
        if (obj->layer && obj->layer->evas && obj->layer->evas->name_hash)
          eina_hash_add(obj->layer->evas->name_hash, obj->name, eo_obj);
     }
}

EOLIAN const char *
_evas_object_name_get(Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj)
{
   return obj->name;
}

EOLIAN Evas_Object*
_evas_canvas_object_name_find(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, const char *name)
{
   if (!name) return NULL;
   else return (Evas_Object *)eina_hash_find(e->name_hash, name);
}

static Evas_Object *
_priv_evas_object_name_child_find(const Evas_Object *eo_obj, const char *name, int recurse)
{
   const Eina_Inlist *lst;
   Evas_Object_Protected_Data *child;

   if (!eo_isa(eo_obj, EVAS_OBJECT_SMART_CLASS)) return NULL;
   lst = evas_object_smart_members_get_direct(eo_obj);
   EINA_INLIST_FOREACH(lst, child)
     {
        if (child->delete_me) continue;
        if (!child->name) continue;
        if (!strcmp(name, child->name)) return child->object;
        if (recurse != 0)
          {
             if ((eo_obj = _priv_evas_object_name_child_find(child->object, name, recurse - 1)))
               return (Evas_Object *)eo_obj;
          }
     }
   return NULL;
}

EOLIAN Evas_Object *
_evas_object_name_child_find(Eo *eo_obj, Evas_Object_Protected_Data *obj EINA_UNUSED, const char *name, int recurse)
{
   return (!name ?  NULL : _priv_evas_object_name_child_find(eo_obj, name, recurse));
}

