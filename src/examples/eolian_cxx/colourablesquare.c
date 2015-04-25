#include <stdio.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Eo.h"

#include "colourable.eo.h"
#include "colourablesquare.eo.h"

#define MY_CLASS COLOURABLESQUARE_CLASS

static int _colourablesquare_impl_logdomain;

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_colourablesquare_impl_logdomain, __VA_ARGS__)

struct _ColourableSquare_Data
{
   int size;
};
typedef struct _ColourableSquare_Data  ColourableSquare_Data;

static void
_colourablesquare_size_constructor(Eo *obj, ColourableSquare_Data *self, int size)
{
   if(!_colourablesquare_impl_logdomain)
     {
        _colourablesquare_impl_logdomain
          = eina_log_domain_register
          ("colourablesquare", EINA_COLOR_LIGHTBLUE);
     }
   self->size = size;
   DBG("_colourablesquare_constructor(%d)\n", size);
   eo_do_super(obj, MY_CLASS, eo_constructor());
}

static int
_colourablesquare_size_get(Eo *obj EINA_UNUSED, ColourableSquare_Data *self)
{
   DBG("_colourablesquare_size_get() => %d\n", self->size);
   return self->size;
}

static void
_colourablesquare_size_print(Eo *obj EINA_UNUSED, ColourableSquare_Data *self)
{
   DBG("_colourablesquare_size_print() ==> %d\n", self->size);
}

static void
_colourablesquare_size_set(Eo *obj EINA_UNUSED, ColourableSquare_Data *self EINA_UNUSED, int size)
{
   DBG("_colourablesquare_size_set(%d)\n", size);
}

#include "colourablesquare.eo.c"
