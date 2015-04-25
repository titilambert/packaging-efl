/* EINA - EFL data type library
 * Copyright (C) 2008 Cedric Bail
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
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "eina_suite.h"
#include "Eina.h"

static Eina_Array *_modules;

static void
_mempool_init(void)
{
   eina_init();
   /* force modules to be loaded in case they are not installed */
   _modules = eina_module_list_get(NULL,
                                   PACKAGE_BUILD_DIR "/src/modules",
                                   EINA_TRUE,
                                   NULL,
                                   NULL);
   eina_module_list_load(_modules);
}

static void
_mempool_shutdown(void)
{
   unsigned int i;
   Eina_Array_Iterator it;
   Eina_Module *module;	
   eina_module_list_free(_modules);
   if (_modules)
     {
        EINA_ARRAY_ITER_NEXT(_modules, i, module, it)
          free(module);
        eina_array_free(_modules);
     }
   eina_shutdown();
}

static void
_eina_mempool_test(Eina_Mempool *mp, Eina_Bool with_realloc, Eina_Bool with_gc)
{
   int *tbl[512];
   int i;

        fail_if(!mp);

   for (i = 0; i < 512; ++i)
     {
        tbl[i] = eina_mempool_malloc(mp, sizeof (int));
        fail_if(!tbl[i]);
        *tbl[i] = i;
     }

   for (i = 0; i < 512; ++i)
        fail_if(*tbl[i] != i);

   for (i = 0; i < 256; ++i)
        eina_mempool_free(mp, tbl[i]);

   if (with_realloc)
      fail_if(eina_mempool_realloc(mp, tbl[500], 25) == NULL);
   else
      fail_if(eina_mempool_realloc(mp, tbl[500], 25) != NULL);

   if (with_gc)
     {
        eina_mempool_gc(mp);
        eina_mempool_statistics(mp);
     }

   eina_mempool_del(mp);
}

#ifdef EINA_BUILD_CHAINED_POOL
START_TEST(eina_mempool_chained_mempool)
{
   Eina_Mempool *mp;

   _mempool_init();

   mp = eina_mempool_add("chained_mempool", "test", NULL, sizeof (int), 256);
   _eina_mempool_test(mp, EINA_FALSE, EINA_FALSE);

   _mempool_shutdown();
}
END_TEST
#endif

#ifdef EINA_BUILD_PASS_THROUGH
START_TEST(eina_mempool_pass_through)
{
   Eina_Mempool *mp;

   _mempool_init();

   mp = eina_mempool_add("pass_through", "test", NULL, sizeof (int), 8, 0);
   _eina_mempool_test(mp, EINA_TRUE, EINA_FALSE);

   _mempool_shutdown();
}
END_TEST
#endif

#ifdef EINA_BUILD_FIXED_BITMAP
START_TEST(eina_mempool_fixed_bitmap)
{
   Eina_Mempool *mp;

   _mempool_init();

   mp = eina_mempool_add("fixed_bitmap", "test", NULL, sizeof (int));
   _eina_mempool_test(mp, EINA_FALSE, EINA_FALSE);

   _mempool_shutdown();
}
END_TEST
#endif

#ifdef EINA_BUILD_EMEMOA_FIXED
START_TEST(eina_mempool_ememoa_fixed)
{
   Eina_Mempool *mp;

   _mempool_init();

   mp = eina_mempool_add("ememoa_fixed", "test", NULL, sizeof (int), 8, 0);
   _eina_mempool_test(mp, EINA_FALSE, EINA_TRUE);

   _mempool_shutdown();
}
END_TEST
#endif

#ifdef EINA_BUILD_EMEMOA_UNKNOWN
START_TEST(eina_mempool_ememoa_unknown)
{
   Eina_Mempool *mp;

   _mempool_init();

   mp = eina_mempool_add("ememoa_unknown",
                         "test",
                         NULL,
                         0,
                         2,
                         sizeof (int),
                         8,
                         sizeof (int) * 2,
                         8);
   _eina_mempool_test(mp, EINA_TRUE, EINA_TRUE);

   _mempool_shutdown();
}
END_TEST
#endif

void
eina_test_mempool(TCase *tc)
{
#ifdef EINA_BUILD_CHAINED_POOL
   tcase_add_test(tc, eina_mempool_chained_mempool);
#endif
#ifdef EINA_BUILD_PASS_THROUGH
   tcase_add_test(tc, eina_mempool_pass_through);
#endif
#ifdef EINA_BUILD_FIXED_BITMAP
   tcase_add_test(tc, eina_mempool_fixed_bitmap);
#endif
#ifdef EINA_BUILD_EMEMOA_FIXED
   tcase_add_test(tc, eina_mempool_ememoa_fixed);
#endif
#ifdef EINA_BUILD_EMEMOA_UNKNOWN
   tcase_add_test(tc, eina_mempool_ememoa_unknown);
#endif
}


