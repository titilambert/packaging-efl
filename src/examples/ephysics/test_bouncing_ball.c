#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ephysics_test.h"

static Eina_Bool
_on_keydown(void *data, Evas_Object *obj __UNUSED__, Evas_Object *src __UNUSED__, Evas_Callback_Type type, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   EPhysics_Body *body = data;

   if (type != EVAS_CALLBACK_KEY_UP)
     return EINA_FALSE;

   if (strcmp(ev->key, "Up") == 0)
     ephysics_body_central_impulse_apply(body, 0, -300, 0);
   else if (strcmp(ev->key, "Down") == 0)
     ephysics_body_central_impulse_apply(body, 0, 300, 0);
   else if (strcmp(ev->key, "Right") == 0)
     ephysics_body_central_impulse_apply(body, 300, 0, 0);
   else if (strcmp(ev->key, "Left") == 0)
     ephysics_body_central_impulse_apply(body, -300, 0, 0);

   return EINA_TRUE;
}

static void
_world_populate(Test_Data *test_data)
{
   Evas_Object *sphere, *shadow;
   EPhysics_Body *fall_body;

   shadow = elm_layout_add(test_data->win);
   elm_layout_file_set(
      shadow, PACKAGE_DATA_DIR "/" EPHYSICS_TEST_THEME ".edj", "shadow-ball");
   evas_object_move(shadow, WIDTH / 3, FLOOR_Y);
   evas_object_resize(shadow, 70, 3);
   evas_object_show(shadow);
   test_data->evas_objs = eina_list_append(test_data->evas_objs, shadow);

   sphere = elm_image_add(test_data->win);
   elm_image_file_set(
      sphere, PACKAGE_DATA_DIR "/" EPHYSICS_TEST_THEME ".edj", "big-blue-ball");
   evas_object_move(sphere, WIDTH / 3, HEIGHT / 8);
   evas_object_resize(sphere, 70, 70);
   evas_object_show(sphere);
   test_data->evas_objs = eina_list_append(test_data->evas_objs, sphere);

   fall_body = ephysics_body_sphere_add(test_data->world);
   ephysics_body_evas_object_set(fall_body, sphere, EINA_TRUE);
   ephysics_body_restitution_set(fall_body, 0.95);
   ephysics_body_friction_set(fall_body, 0.1);
   ephysics_body_event_callback_add(fall_body, EPHYSICS_CALLBACK_BODY_UPDATE,
                                    update_object_cb, shadow);
   test_data->bodies = eina_list_append(test_data->bodies, fall_body);
   test_data->data = fall_body;
}

static void
_restart(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Test_Data *test_data = data;

   DBG("Restart pressed");
   elm_object_event_callback_del(test_data->win, _on_keydown, test_data->data);
   test_clean(test_data);
   _world_populate(test_data);
   elm_object_event_callback_add(test_data->win, _on_keydown, test_data->data);
}

void
test_bouncing_ball(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EPhysics_Body *boundary;
   EPhysics_World *world;
   Test_Data *test_data;

   if (!ephysics_init())
     return;

   test_data = test_data_new();
   test_win_add(test_data, "Bouncing Ball", EINA_TRUE);

   elm_layout_signal_callback_add(test_data->layout, "restart", "test-theme",
                                  _restart, test_data);
   elm_object_signal_emit(test_data->layout, "borders,show", "ephysics_test");
   elm_object_signal_emit(test_data->layout, "arrows,show", "ephysics_test");

   world = ephysics_world_new();
   ephysics_world_light_all_bodies_set(world, EINA_TRUE);
   ephysics_world_render_geometry_set(world, 50, 40, -50,
                                      WIDTH - 100, FLOOR_Y - 40, DEPTH);
   test_data->world = world;

   boundary = ephysics_body_bottom_boundary_add(test_data->world);
   ephysics_body_restitution_set(boundary, 0.65);
   ephysics_body_friction_set(boundary, 4);

   boundary = ephysics_body_right_boundary_add(test_data->world);
   ephysics_body_restitution_set(boundary, 0.4);
   ephysics_body_friction_set(boundary, 3);

   boundary = ephysics_body_left_boundary_add(test_data->world);
   ephysics_body_restitution_set(boundary, 0.4);
   ephysics_body_friction_set(boundary, 3);

   ephysics_body_top_boundary_add(test_data->world);

   _world_populate(test_data);
   elm_object_event_callback_add(test_data->win, _on_keydown, test_data->data);
}
