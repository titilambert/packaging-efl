#ifndef SIMPLE_H
#define SIMPLE_H

typedef struct
{
   int cb_count;
} Simple_Public_Data;

EAPI void simple_a_set(int a);

extern const Eo_Event_Description _EV_A_CHANGED;
#define EV_A_CHANGED (&(_EV_A_CHANGED))

#define SIMPLE_CLASS simple_class_get()
const Eo_Class *simple_class_get(void);

#endif
