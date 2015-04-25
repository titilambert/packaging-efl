#ifndef SIMPLE_H
#define SIMPLE_H

typedef struct
{
   int a;
} Simple_Public_Data;

EAPI void simple_a_set(int a);
EAPI int simple_a_get(void);
EAPI Eina_Bool simple_a_print(void);
EAPI Eina_Bool simple_class_hi_print(void);
EAPI void simple_recursive(int n);
EAPI void simple_pure_virtual(void);
EAPI void simple_no_implementation(void);

extern const Eo_Event_Description _EV_A_CHANGED;
#define EV_A_CHANGED (&(_EV_A_CHANGED))

#define SIMPLE_CLASS simple_class_get()
const Eo_Class *simple_class_get(void);

#endif
