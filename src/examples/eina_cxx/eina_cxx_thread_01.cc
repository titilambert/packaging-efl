//Compile with:
//gcc -g eina_list_01.c -o eina_list_01 `pkg-config --cflags --libs eina`

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <Eina.hh>

#include <iterator>
#include <algorithm>

#include <iostream>

namespace eina = efl::eina;

void thread1(eina::mutex&)
{
}

int main()
{
   eina::eina_init eina_init;
   eina::eina_threads_init threads_init;

   eina::mutex m;
   eina::condition_variable c;

   eina::unique_lock<eina::mutex> l(m);

   eina::thread thread1(&::thread1, eina::ref(m));

   thread1.join();
}
