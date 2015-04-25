#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Eo.h"
#include "eo_bench.h"
#include "class_simple.h"

static void
bench_eo_do_general(int request)
{
   int i;
   Eo *obj = eo_add(SIMPLE_CLASS, NULL);
   for (i = 0 ; i < request ; i++)
     {
        eo_do(obj, simple_a_set(i));
     }

   eo_unref(obj);
}

static const Eo_Class *cur_klass;

static void
_a_set(Eo *obj, void *class_data EINA_UNUSED, int a)
{
   eo_do_super(obj, cur_klass, simple_a_set(a));
}

static void
bench_eo_do_super(int request)
{
   static Eo_Class_Description class_desc = {
        EO_VERSION,
        "Simple2",
        EO_CLASS_TYPE_REGULAR,
        EO_CLASS_DESCRIPTION_NOOPS(),
        NULL,
        0,
        NULL,
        NULL
   };
   cur_klass = eo_class_new(&class_desc, SIMPLE_CLASS, NULL);

   int i;
   Eo *obj = eo_add(cur_klass, NULL);
   for (i = 0 ; i < request ; i++)
     {
        eo_do(obj, simple_a_set(i));
     }

   eo_unref(obj);
}

void eo_bench_eo_do(Eina_Benchmark *bench)
{
   eina_benchmark_register(bench, "various",
         EINA_BENCHMARK(bench_eo_do_general), 1000, 100000, 500);
   eina_benchmark_register(bench, "super",
         EINA_BENCHMARK(bench_eo_do_super), 1000, 100000, 500);
}
