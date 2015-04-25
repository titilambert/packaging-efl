/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
#include "evas_common_private.h"
#include "evas_private.h"
#include "Evas_GL.h"

typedef struct _Evas_GL_TLS_data Evas_GL_TLS_data;

struct _Evas_GL
{
   DATA32      magic;
   Evas_Public_Data *evas;

   Eina_List  *contexts;
   Eina_List  *surfaces;
   Eina_Lock   lck;
   Eina_TLS    resource_key;
   Eina_List  *resource_list;
};

struct _Evas_GL_Context
{
   void    *data;
   Evas_GL_Context_Version version;
};

struct _Evas_GL_Surface
{
   void    *data;
};

struct _Evas_GL_TLS_data
{
   int error_state;
};

Evas_GL_TLS_data *
_evas_gl_internal_tls_get(Evas_GL *evas_gl)
{
   Evas_GL_TLS_data *tls_data;

   if (!evas_gl) return NULL;

   if (!(tls_data = eina_tls_get(evas_gl->resource_key)))
     {
        tls_data = (Evas_GL_TLS_data*) calloc(1, sizeof(Evas_GL_TLS_data));
        if (!tls_data)
          {
             ERR("Evas_GL: Could not set error!");
             return NULL;
          }
        tls_data->error_state = EVAS_GL_SUCCESS;

        if (eina_tls_set(evas_gl->resource_key, (void*)tls_data) == EINA_TRUE)
          {
             LKL(evas_gl->lck);
             evas_gl->resource_list = eina_list_prepend(evas_gl->resource_list, tls_data);
             LKU(evas_gl->lck);
             return tls_data;
          }
        else
          {
             ERR("Evas_GL: Failed setting TLS data!");
             free(tls_data);
             return NULL;
          }
     }

   return tls_data;
}

void
_evas_gl_internal_tls_destroy(Evas_GL *evas_gl)
{
   Evas_GL_TLS_data *tls_data;

   if (!evas_gl) return;

   if (!eina_tls_get(evas_gl->resource_key))
     {
        WRN("Destructor: TLS data was never set!");
        return;
     }

   LKL(evas_gl->lck);
   EINA_LIST_FREE(evas_gl->resource_list, tls_data)
     free(tls_data);

   if (evas_gl->resource_key)
     eina_tls_free(evas_gl->resource_key);
   evas_gl->resource_key = 0;
   LKU(evas_gl->lck);
}

void
_evas_gl_internal_error_set(Evas_GL *evas_gl, int error_enum)
{
   Evas_GL_TLS_data *tls_data;

   if (!evas_gl) return;

   tls_data = _evas_gl_internal_tls_get(evas_gl);
   if (!tls_data) return;

   tls_data->error_state = error_enum;
}

int
_evas_gl_internal_error_get(Evas_GL *evas_gl)
{
   Evas_GL_TLS_data *tls_data;

   if (!evas_gl) return EVAS_GL_NOT_INITIALIZED;

   tls_data = _evas_gl_internal_tls_get(evas_gl);
   if (!tls_data) return EVAS_GL_NOT_INITIALIZED;

   return tls_data->error_state;
}

EAPI Evas_GL *
evas_gl_new(Evas *e)
{
   Evas_GL *evas_gl;

   MAGIC_CHECK(e, Evas, MAGIC_EVAS);
   return NULL;
   MAGIC_CHECK_END();

   evas_gl = calloc(1, sizeof(Evas_GL));
   if (!evas_gl) return NULL;

   evas_gl->magic = MAGIC_EVAS_GL;
   evas_gl->evas = eo_data_ref(e, EVAS_CANVAS_CLASS);
   LKI(evas_gl->lck);

   if (!evas_gl->evas->engine.func->gl_context_create)
     {
        ERR("Evas GL engine not available.");
        free(evas_gl);
        return NULL;
     }

   // Initialize tls resource key
   if (eina_tls_new(&(evas_gl->resource_key)) == EINA_FALSE)
     {
        ERR("Error creating tls key");
        free(evas_gl);
        return NULL;
     }

   _evas_gl_internal_error_set(evas_gl, EVAS_GL_SUCCESS);
   return evas_gl;
}

EAPI void
evas_gl_free(Evas_GL *evas_gl)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return;
   MAGIC_CHECK_END();

   // Delete undeleted surfaces
   while (evas_gl->surfaces)
     evas_gl_surface_destroy(evas_gl, evas_gl->surfaces->data);

   // Delete undeleted contexts
   while (evas_gl->contexts)
     evas_gl_context_destroy(evas_gl, evas_gl->contexts->data);

   // Destroy tls
   _evas_gl_internal_tls_destroy(evas_gl);

   eo_data_unref(evas_gl->evas->evas, evas_gl->evas);
   evas_gl->magic = 0;
   LKD(evas_gl->lck);
   free(evas_gl);
}

EAPI Evas_GL_Config *
evas_gl_config_new()
{
   Evas_GL_Config *cfg;

   cfg = calloc(1, sizeof(Evas_GL_Config));

   if (!cfg) return NULL;

   return cfg;
}

EAPI void
evas_gl_config_free(Evas_GL_Config *cfg)
{
   if (cfg) free(cfg);
}

EAPI Evas_GL_Surface *
evas_gl_surface_create(Evas_GL *evas_gl, Evas_GL_Config *config, int width, int height)
{
   Evas_GL_Surface *surf;

   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if (!config)
     {
        ERR("Invalid Config Pointer!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_CONFIG);
        return NULL;
     }

   if ((width <= 0) || (height <= 0))
     {
        ERR("Invalid surface dimensions: %d, %d", width, height);
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_PARAMETER);
        return NULL;
     }

   surf = calloc(1, sizeof(Evas_GL_Surface));

   if (!surf)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_ALLOC);
        return NULL;
     }

   surf->data = evas_gl->evas->engine.func->gl_surface_create(evas_gl->evas->engine.data.output, config, width, height);

   if (!surf->data)
     {
        ERR("Failed creating a surface from the engine.");
        free(surf);
        return NULL;
     }

   // Keep track of the surface creations
   LKL(evas_gl->lck);
   evas_gl->surfaces = eina_list_prepend(evas_gl->surfaces, surf);
   LKU(evas_gl->lck);

   return surf;
}

EAPI Evas_GL_Surface *
evas_gl_pbuffer_surface_create(Evas_GL *evas_gl, Evas_GL_Config *cfg,
                               int w, int h, const int *attrib_list)
{
   Evas_GL_Surface *surf;

   // Magic
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if (!cfg)
     {
        ERR("Invalid Config Pointer!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_CONFIG);
        return NULL;
     }

   if ((w <= 0) || (h <= 0))
     {
        ERR("Invalid surface dimensions: %d, %d", w, h);
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_PARAMETER);
        return NULL;
     }

   if (!evas_gl->evas->engine.func->gl_pbuffer_surface_create)
     {
        ERR("Engine does not support PBuffer!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_NOT_INITIALIZED);
        return NULL;
     }

   surf = calloc(1, sizeof(Evas_GL_Surface));
   if (!surf)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_ALLOC);
        return NULL;
     }

   surf->data = evas_gl->evas->engine.func->gl_pbuffer_surface_create
     (evas_gl->evas->engine.data.output, cfg, w, h, attrib_list);
   if (!surf->data)
     {
        ERR("Engine failed to create a PBuffer!");
        free(surf);
        return NULL;
     }

   // Keep track of the surface creations
   LKL(evas_gl->lck);
   evas_gl->surfaces = eina_list_prepend(evas_gl->surfaces, surf);
   LKU(evas_gl->lck);

   return surf;
}

EAPI void
evas_gl_surface_destroy(Evas_GL *evas_gl, Evas_GL_Surface *surf)
{
   // Magic
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return;
   MAGIC_CHECK_END();

   if (!surf)
     {
        ERR("Trying to destroy a NULL surface pointer!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_SURFACE);
        return;
     }

   // Call Engine's Surface Destroy
   evas_gl->evas->engine.func->gl_surface_destroy(evas_gl->evas->engine.data.output, surf->data);

   // Remove it from the list
   LKL(evas_gl->lck);
   evas_gl->surfaces = eina_list_remove(evas_gl->surfaces, surf);
   LKU(evas_gl->lck);

   // Delete the object
   free(surf);
   surf = NULL;
}

EAPI Evas_GL_Context *
evas_gl_context_version_create(Evas_GL *evas_gl, Evas_GL_Context *share_ctx,
                               Evas_GL_Context_Version version)
{
   Evas_GL_Context *ctx;

   // Magic
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if ((version != EVAS_GL_GLES_1_X) && (version != EVAS_GL_GLES_2_X))
     {
        ERR("Can not create an OpenGL-ES %d.x context (not supported).",
            (int) version);
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_PARAMETER);
        return NULL;
     }

   // Allocate a context object
   ctx = calloc(1, sizeof(Evas_GL_Context));
   if (!ctx)
     {
        ERR("Unable to create a Evas_GL_Context object");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_ALLOC);
        return NULL;
     }

   // Call engine->gl_create_context
   ctx->version = version;
   if (share_ctx)
     ctx->data = evas_gl->evas->engine.func->gl_context_create(evas_gl->evas->engine.data.output, share_ctx->data, version);
   else
     ctx->data = evas_gl->evas->engine.func->gl_context_create(evas_gl->evas->engine.data.output, NULL, version);

   // Set a few variables
   if (!ctx->data)
     {
        ERR("Failed creating a context from the engine.");
        free(ctx);
        return NULL;
     }

   // Keep track of the context creations
   LKL(evas_gl->lck);
   evas_gl->contexts = eina_list_prepend(evas_gl->contexts, ctx);
   LKU(evas_gl->lck);

   return ctx;
}

EAPI Evas_GL_Context *
evas_gl_context_create(Evas_GL *evas_gl, Evas_GL_Context *share_ctx)
{
   return evas_gl_context_version_create(evas_gl, share_ctx, EVAS_GL_GLES_2_X);
}

EAPI void
evas_gl_context_destroy(Evas_GL *evas_gl, Evas_GL_Context *ctx)
{

   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return;
   MAGIC_CHECK_END();

   if (!ctx)
     {
        ERR("Trying to destroy a NULL context pointer!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_CONTEXT);
        return;
     }

   // Call Engine's destroy
   evas_gl->evas->engine.func->gl_context_destroy(evas_gl->evas->engine.data.output, ctx->data);

   // Remove it from the list
   LKL(evas_gl->lck);
   evas_gl->contexts = eina_list_remove(evas_gl->contexts, ctx);
   LKU(evas_gl->lck);

   // Delete the object
   free(ctx);
   ctx = NULL;
}

EAPI Eina_Bool
evas_gl_make_current(Evas_GL *evas_gl, Evas_GL_Surface *surf, Evas_GL_Context *ctx)
{
   Eina_Bool ret;

   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   if ((surf) && (ctx))
     ret = (Eina_Bool)evas_gl->evas->engine.func->gl_make_current(evas_gl->evas->engine.data.output, surf->data, ctx->data);
   else if ((!surf) && (!ctx))
     ret = (Eina_Bool)evas_gl->evas->engine.func->gl_make_current(evas_gl->evas->engine.data.output, NULL, NULL);
   else
     {
        ERR("Bad match between surface: %p and context: %p", surf, ctx);
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_MATCH);
        return EINA_FALSE;
     }

   return ret;
}

EAPI Evas_GL_Context *
evas_gl_current_context_get(Evas_GL *evas_gl)
{
   Evas_GL_Context *comp;
   void *internal_ctx;
   Eina_List *li;

   // Magic
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if (!evas_gl->evas->engine.func->gl_current_context_get)
     {
        CRI("Can not get current context with this engine: %s",
            evas_gl->evas->engine.module->definition->name);
        return NULL;
     }

   internal_ctx = evas_gl->evas->engine.func->gl_current_context_get(evas_gl->evas->engine.data.output);
   if (!internal_ctx)
     return NULL;

   LKL(evas_gl->lck);
   EINA_LIST_FOREACH(evas_gl->contexts, li, comp)
     {
        if (comp->data == internal_ctx)
          {
             LKU(evas_gl->lck);
             return comp;
          }
     }

   ERR("The currently bound context could not be found.");
   LKU(evas_gl->lck);
   return NULL;
}

EAPI Evas_GL_Surface *
evas_gl_current_surface_get(Evas_GL *evas_gl)
{
   Evas_GL_Surface *comp;
   void *internal_sfc;
   Eina_List *li;

   // Magic
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if (!evas_gl->evas->engine.func->gl_current_surface_get)
     {
        CRI("Can not get current surface with this engine: %s",
            evas_gl->evas->engine.module->definition->name);
        return NULL;
     }

   internal_sfc = evas_gl->evas->engine.func->gl_current_surface_get(evas_gl->evas->engine.data.output);
   if (!internal_sfc)
     return NULL;

   LKL(evas_gl->lck);
   EINA_LIST_FOREACH(evas_gl->surfaces, li, comp)
     {
        if (comp->data == internal_sfc)
          {
             LKU(evas_gl->lck);
             return comp;
          }
     }

   ERR("The currently bound surface could not be found.");
   LKU(evas_gl->lck);
   return NULL;
}

EAPI const char *
evas_gl_string_query(Evas_GL *evas_gl, int name)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return "";
   MAGIC_CHECK_END();

   return evas_gl->evas->engine.func->gl_string_query(evas_gl->evas->engine.data.output, name);
}

EAPI Evas_GL_Func
evas_gl_proc_address_get(Evas_GL *evas_gl, const char *name)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   return (Evas_GL_Func)evas_gl->evas->engine.func->gl_proc_address_get(evas_gl->evas->engine.data.output, name);
}

EAPI Eina_Bool
evas_gl_native_surface_get(Evas_GL *evas_gl, Evas_GL_Surface *surf, Evas_Native_Surface *ns)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   if (!surf)
     {
        ERR("Invalid surface!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_SURFACE);
        return EINA_FALSE;
     }

   if (!ns)
     {
        ERR("Invalid input parameters!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_PARAMETER);
        return EINA_FALSE;
     }

   return (Eina_Bool)evas_gl->evas->engine.func->gl_native_surface_get(evas_gl->evas->engine.data.output, surf->data, ns);
}


EAPI Evas_GL_API *
evas_gl_api_get(Evas_GL *evas_gl)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   return (Evas_GL_API*)evas_gl->evas->engine.func->gl_api_get(evas_gl->evas->engine.data.output, EVAS_GL_GLES_2_X);
}

EAPI Evas_GL_API *
evas_gl_context_api_get(Evas_GL *evas_gl, Evas_GL_Context *ctx)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if (!ctx)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_CONTEXT);
        return NULL;
     }

   return (Evas_GL_API*)evas_gl->evas->engine.func->gl_api_get(evas_gl->evas->engine.data.output, ctx->version);
}

EAPI int
evas_gl_rotation_get(Evas_GL *evas_gl)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return 0;
   MAGIC_CHECK_END();

   if (!evas_gl->evas->engine.func->gl_rotation_angle_get)
     return 0;

   return evas_gl->evas->engine.func->gl_rotation_angle_get(evas_gl->evas->engine.data.output);
}

EAPI int
evas_gl_error_get(Evas_GL *evas_gl)
{
   int err;

   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return EVAS_GL_NOT_INITIALIZED;
   MAGIC_CHECK_END();

   if ((err = _evas_gl_internal_error_get(evas_gl)) != EVAS_GL_SUCCESS)
     goto end;

   if (!evas_gl->evas->engine.func->gl_error_get)
     err = EVAS_GL_NOT_INITIALIZED;
   else
     err = evas_gl->evas->engine.func->gl_error_get(evas_gl->evas->engine.data.output);

end:
   /* Call to evas_gl_error_get() should set error to EVAS_GL_SUCCESS */
   _evas_gl_internal_error_set(evas_gl, EVAS_GL_SUCCESS);
   return err;
}

EAPI Eina_Bool
evas_gl_surface_query(Evas_GL *evas_gl, Evas_GL_Surface *surface, int attribute, void *value)
{
   if (!evas_gl) return EINA_FALSE;
   if (!surface)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_SURFACE);
        return EINA_FALSE;
     }

   if (!evas_gl->evas->engine.func->gl_surface_query)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_NOT_INITIALIZED);
        return EINA_FALSE;
     }

   if (!value)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_PARAMETER);
        return EINA_FALSE;
     }

   return evas_gl->evas->engine.func->gl_surface_query
         (evas_gl->evas->engine.data.output, surface->data, attribute, value);
}

// Internal function - called from evas_gl_core.c
EAPI void *
_evas_gl_native_context_get(Evas_GL_Context *ctx)
{
   if (!ctx) return NULL;
   return ctx->data;
}
