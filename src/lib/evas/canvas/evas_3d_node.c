#include "evas_common_private.h"
#include "evas_private.h"

#define MY_CLASS EVAS_3D_NODE_CLASS
#define MY_CLASS_NAME "Evas_3D_Node"

Evas_3D_Mesh_Frame *evas_3d_mesh_frame_find(Evas_3D_Mesh_Data *pd, int frame);

static inline Evas_3D_Node_Mesh *
_node_mesh_new(Evas_3D_Node *node, Evas_3D_Mesh *mesh)
{
   Evas_3D_Node_Mesh *nm = (Evas_3D_Node_Mesh *)malloc(sizeof(Evas_3D_Node_Mesh));

   if (nm == NULL)
     {
        ERR("Failed to allocate memory.");
        return NULL;
     }

   nm->node = node;
   nm->mesh = mesh;
   nm->frame = 0;

   return nm;
}

static inline void
_node_mesh_free(Evas_3D_Node_Mesh *nm)
{
   free(nm);
}

static void
_node_mesh_free_func(void *data)
{
   _node_mesh_free((Evas_3D_Node_Mesh *)data);
}

static Eina_Bool
_node_scene_root_change_notify(const Eina_Hash *hash EINA_UNUSED, const void *key,
                               void *data EINA_UNUSED, void *fdata)
{
   Evas_3D_Scene *s = *(Evas_3D_Scene **)key;
   eo_do(s, evas_3d_object_change(EVAS_3D_STATE_SCENE_ROOT_NODE, (Evas_3D_Object *)fdata));
   return EINA_TRUE;
}

static Eina_Bool
_node_scene_camera_change_notify(const Eina_Hash *hash EINA_UNUSED, const void *key,
                                 void *data EINA_UNUSED, void *fdata)
{
   Evas_3D_Scene *s = *(Evas_3D_Scene **)key;
   eo_do(s, evas_3d_object_change(EVAS_3D_STATE_SCENE_CAMERA_NODE, (Evas_3D_Object *)fdata));
   return EINA_TRUE;
}

EOLIAN static void
_evas_3d_node_evas_3d_object_change_notify(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_State state EINA_UNUSED , Evas_3D_Object *ref EINA_UNUSED)
{
   Eina_List    *l;
   Evas_3D_Node *n;

   /* Notify all scenes using this node that it has changed. */
   if (pd->scenes_root)
     eina_hash_foreach(pd->scenes_root, _node_scene_root_change_notify, obj);

   if (pd->scenes_camera)
     eina_hash_foreach(pd->scenes_camera, _node_scene_camera_change_notify, obj);

   /* Notify parent that a member has changed. */
   if (pd->parent)
    {
       eo_do(pd->parent, evas_3d_object_change(EVAS_3D_STATE_NODE_MEMBER, obj));
    }

   /* Notify members that the parent has changed. */
   EINA_LIST_FOREACH(pd->members, l, n)
     {
        eo_do(n, evas_3d_object_change(EVAS_3D_STATE_NODE_PARENT, obj));
     }
}

static Eina_Bool
_node_transform_update(Evas_3D_Node *node, void *data EINA_UNUSED)
{
   Evas_3D_Node_Data *pd = eo_data_scope_get(node, MY_CLASS);
   Eina_Bool transform_dirty = EINA_FALSE, parent_dirty = EINA_FALSE;

   eo_do(node,
         transform_dirty = evas_3d_object_dirty_get(EVAS_3D_STATE_NODE_TRANSFORM),
         parent_dirty = evas_3d_object_dirty_get(EVAS_3D_STATE_NODE_PARENT));

   if (transform_dirty || parent_dirty)
     {
        if (pd->parent)
          {
             Evas_3D_Node_Data *pdparent = eo_data_scope_get(pd->parent, MY_CLASS);
             const Evas_Vec3 *scale_parent = &pdparent->scale_world;
             const Evas_Vec4 *orientation_parent = &pdparent->orientation_world;

             /* Orienatation */
             if (pd->orientation_inherit)
               {
                  evas_vec4_quaternion_multiply(&pd->orientation_world,
                                                orientation_parent, &pd->orientation);
               }
             else
               {
                  pd->orientation_world = pd->orientation;
               }

             /* Scale */
             if (pd->scale_inherit)
               evas_vec3_multiply(&pd->scale_world, scale_parent, &pd->scale);
             else
               pd->scale_world = pd->scale;

             /* Position */
             if (pd->position_inherit)
               {
                  evas_vec3_multiply(&pd->position_world, &pd->position, scale_parent);
                  evas_vec3_quaternion_rotate(&pd->position_world, &pd->position_world,
                                              orientation_parent);
                  evas_vec3_add(&pd->position_world, &pd->position_world,
                                &pdparent->position_world);
               }
             else
               {
                  pd->position_world = pd->position;
               }
          }
        else
          {
             pd->position_world = pd->position;
             pd->orientation_world = pd->orientation;
             pd->scale_world = pd->scale;
          }

        if (pd->type == EVAS_3D_NODE_TYPE_CAMERA)
          {
             evas_mat4_inverse_build(&pd->data.camera.matrix_world_to_eye,
                                     &pd->position_world, &pd->orientation_world,
                                     &pd->scale_world);
          }
        else if (pd->type == EVAS_3D_NODE_TYPE_LIGHT)
          {
          }
        else if (pd->type == EVAS_3D_NODE_TYPE_MESH)
          {
             evas_mat4_build(&pd->data.mesh.matrix_local_to_world,
                             &pd->position_world, &pd->orientation_world, &pd->scale_world);
          }
/*
        if (pd->parent)
          {
             evas_mat4_nocheck_multiply(&pd->matrix_local_to_world,
                                        &pd->parent->matrix_local_to_world,
                                        &pd->matrix_local_to_parent);
          }
        else
          {
             evas_mat4_copy(&pd->matrix_local_to_world, &pd->matrix_local_to_parent);
          }*/
     }

   return EINA_TRUE;
}

static Eina_Bool
_node_item_update(Evas_3D_Node *node, void *data EINA_UNUSED)
{
   Evas_3D_Node_Data *pd = eo_data_scope_get(node, EVAS_3D_NODE_CLASS);
   if (pd->type == EVAS_3D_NODE_TYPE_CAMERA)
     {
        if (pd->data.camera.camera)
          {
             eo_do(pd->data.camera.camera, evas_3d_object_update());
          }
     }
   else if (pd->type == EVAS_3D_NODE_TYPE_LIGHT)
     {
        if (pd->data.light.light)
          {
             eo_do(pd->data.light.light, evas_3d_object_update());
          }
     }
   else if (pd->type == EVAS_3D_NODE_TYPE_MESH)
     {
        Eina_List *l;
        Evas_3D_Mesh *m;
        EINA_LIST_FOREACH(pd->data.mesh.meshes, l, m)
          {
             eo_do(m, evas_3d_object_update());
          }
     }

   return EINA_TRUE;
}

static Eina_Bool
_node_aabb_update(Evas_3D_Node *node, void *data EINA_UNUSED)
{
   Evas_3D_Node_Data *pd = eo_data_scope_get(node, EVAS_3D_NODE_CLASS);
   Eina_Bool transform_dirty = EINA_FALSE, mesh_geom_dirty = EINA_FALSE;
   Eina_Bool mesh_frame_dirty = EINA_FALSE, member_dirty = EINA_FALSE;
   Eina_Bool frame_found = EINA_FALSE, is_change_orientation = EINA_FALSE;
   Eina_Bool parent_dirty = EINA_FALSE;
   const Eina_List *m, *l;
   Evas_3D_Mesh *mesh;
   int frame, count, size, i, j;
   Evas_3D_Mesh_Frame *f;
   float minx, miny, minz, maxx, maxy, maxz, vxmin, vymin, vzmin, vxmax, vymax, vzmax;
   float *minmaxdata;
   Evas_Vec4 orientation = {0};
   Evas_Box3 box3;

   eo_do(node,
         transform_dirty = evas_3d_object_dirty_get(EVAS_3D_STATE_NODE_TRANSFORM),
         mesh_geom_dirty = evas_3d_object_dirty_get(EVAS_3D_STATE_NODE_MESH_GEOMETRY),
         mesh_frame_dirty = evas_3d_object_dirty_get(EVAS_3D_STATE_NODE_MESH_FRAME),
         parent_dirty = evas_3d_object_dirty_get(EVAS_3D_STATE_NODE_PARENT),
         member_dirty = evas_3d_object_dirty_get(EVAS_3D_STATE_NODE_MEMBER));

     if (transform_dirty ||
       mesh_geom_dirty ||
       mesh_frame_dirty ||
       member_dirty ||
       parent_dirty)
     {
        if (pd->type == EVAS_3D_NODE_TYPE_MESH)
          {

             if (pd->orientation_world.x || pd->orientation_world.y || pd->orientation_world.z)
               {
                  evas_vec4_set(&orientation, pd->orientation_world.x, pd->orientation_world.y, pd->orientation_world.z, pd->orientation_world.w);
                  is_change_orientation = EINA_TRUE;
               }

             eo_do (node, m = (Eina_List *)evas_3d_node_mesh_list_get());

             EINA_LIST_FOREACH(m, l, mesh)
               {
                  eo_do(node, frame = evas_3d_node_mesh_frame_get(mesh));
                  Evas_3D_Mesh_Data *mpd = eo_data_scope_get(mesh, EVAS_3D_MESH_CLASS);
                  f = evas_3d_mesh_frame_find(mpd, frame);

                  if (f)
                    {
                       i = 0, j = 0;

                       evas_box3_empty_set(&box3);
                       count = f->vertices[EVAS_3D_VERTEX_POSITION].element_count;
                       size =  f->vertices[EVAS_3D_VERTEX_POSITION].size;
                       if (!size) size = count * sizeof(float) * mpd->vertex_count;
                       minmaxdata = (float *)malloc(size);

                       if (!minmaxdata)
                         {
                            ERR("Not enough memory.");
                            return EINA_FALSE;
                         }

                       memcpy(minmaxdata, f->vertices[EVAS_3D_VERTEX_POSITION].data, size);

                       /*get current coordinates, set orientation and find min/max*/
                       if (is_change_orientation)
                         {
                            Evas_Vec3 rotate;
                            evas_vec3_set(&rotate, minmaxdata[0], minmaxdata[1], minmaxdata[2]);
                            evas_vec3_quaternion_rotate(&rotate, &rotate, &orientation);
                            vxmax = vxmin = minmaxdata[0] = rotate.x;
                            vymax = vymin = minmaxdata[1] = rotate.y;
                            vzmax = vzmin = minmaxdata[2] = rotate.z;
                         }
                       else
                         {
                            vxmax = vxmin = minmaxdata[0];
                            vymax = vymin = minmaxdata[1];
                            vzmax = vzmin = minmaxdata[2];
                         }

                       j += count;

                       if (is_change_orientation)
                         {
                            for (i = 1; i < mpd->vertex_count; ++i)
                              {
                                 Evas_Vec3 rotate;
                                 evas_vec3_set(&rotate, minmaxdata[j], minmaxdata[j + 1], minmaxdata[j + 2]);
                                 evas_vec3_quaternion_rotate(&rotate, &rotate, &orientation);
                                 minmaxdata[j] = rotate.x;
                                 minmaxdata[j + 1] = rotate.y;
                                 minmaxdata[j + 2] = rotate.z;
                                 vxmin > minmaxdata[j] ? vxmin = minmaxdata[j] : 0;
                                 vxmax < minmaxdata[j] ? vxmax = minmaxdata[j] : 0;
                                 vymin > minmaxdata[j + 1] ? vymin = minmaxdata[j + 1] : 0;
                                 vymax < minmaxdata[j + 1] ? vymax = minmaxdata[j + 1] : 0;
                                 vzmin > minmaxdata[j + 2] ? vzmin = minmaxdata[j + 2] : 0;
                                 vzmax < minmaxdata[j + 2] ? vzmax = minmaxdata[j + 2] : 0;
                                 j += count;
                              }
                         }
                       else
                         {
                            for (i = 1; i < mpd->vertex_count; ++i)
                              {
                                 vxmin > minmaxdata[j] ? vxmin = minmaxdata[j] : 0;
                                 vxmax < minmaxdata[j] ? vxmax = minmaxdata[j] : 0;
                                 vymin > minmaxdata[j + 1] ? vymin = minmaxdata[j + 1] : 0;
                                 vymax < minmaxdata[j + 1] ? vymax = minmaxdata[j + 1] : 0;
                                 vzmin > minmaxdata[j + 2] ? vzmin = minmaxdata[j + 2] : 0;
                                 vzmax < minmaxdata[j + 2] ? vzmax = minmaxdata[j + 2] : 0;
                                 j += count;
                              }
                         }

                       if (!frame_found)
                         {
                            evas_box3_empty_set(&pd->aabb);
                            evas_box3_empty_set(&pd->obb);
                            minx = vxmin;
                            miny = vymin;
                            minz = vzmin;
                            maxx = vxmax;
                            maxy = vymax;
                            maxz = vzmax;
                         }
                       else
                         {
                            minx > vxmin ? minx = vxmin : 0;
                            maxx < vxmax ? maxx = vxmax : 0;
                            miny > vymin ? miny = vymin : 0;
                            maxy < vymax ? maxy = vymax : 0;
                            minz > vzmin ? minz = vzmin : 0;
                            maxz < vzmax ? maxz = vzmax : 0;
                         }

                       frame_found = EINA_TRUE;
                       free(minmaxdata);
                       evas_box3_set(&box3, minx, miny, minz, maxx, maxy, maxz);
                       evas_box3_union(&pd->aabb, &pd->aabb, &box3);
                       evas_box3_union(&pd->obb, &pd->obb, &f->aabb);
                    }
               }

             if (frame_found)
               {
                  if ((pd->scale_world.x != 1 || pd->scale_world.y != 1 || pd->scale_world.z != 1))
                    {
                       Evas_Vec3 scale;
                       evas_vec3_set(&scale, pd->scale_world.x, pd->scale_world.y, pd->scale_world.z);
                       evas_vec3_multiply(&pd->obb.p0, &scale, &pd->obb.p0);
                       evas_vec3_multiply(&pd->obb.p1, &scale, &pd->obb.p1);
                       evas_vec3_multiply(&pd->aabb.p0, &scale, &pd->aabb.p0);
                       evas_vec3_multiply(&pd->aabb.p1, &scale, &pd->aabb.p1);
                    }
                  if (is_change_orientation)
                    {
                       evas_vec3_quaternion_rotate(&pd->obb.p0, &pd->obb.p0, &orientation);
                       evas_vec3_quaternion_rotate(&pd->obb.p1, &pd->obb.p1, &orientation);
                    }
                  if ((pd->position_world.x || pd->position_world.y || pd->position_world.z))
                    {
                       Evas_Vec3 position;
                       evas_vec3_set(&position, pd->position_world.x, pd->position_world.y, pd->position_world.z);
                       evas_vec3_add(&pd->obb.p0, &position, &pd->obb.p0);
                       evas_vec3_add(&pd->obb.p1, &position, &pd->obb.p1);
                       evas_vec3_add(&pd->aabb.p0, &position, &pd->aabb.p0);
                       evas_vec3_add(&pd->aabb.p1, &position, &pd->aabb.p1);
                    }
               }
          }
        else
          {
             Eina_List *current;
             Evas_3D_Node *datanode;

             /* Update AABB, OBB, bounding sphere of this node. */
             evas_box3_empty_set(&pd->aabb);
             evas_box3_empty_set(&pd->obb);

             EINA_LIST_FOREACH(pd->members, current, datanode)
               {
                  Evas_3D_Node_Data *datapd = eo_data_scope_get(datanode, EVAS_3D_NODE_CLASS);
                  evas_box3_union(&pd->obb, &pd->obb, &datapd->obb);
                  evas_box3_union(&pd->aabb, &pd->aabb, &datapd->aabb);
               }
          }
          evas_build_sphere(&pd->obb, &pd->bsphere);
     }

   return EINA_TRUE;
}

static Eina_Bool
_node_update_done(Evas_3D_Node *obj, void *data EINA_UNUSED)
{
   //@FIXME
   Evas_3D_Object_Data *pdobject = eo_data_scope_get(obj, EVAS_3D_OBJECT_CLASS);
   memset(&pdobject->dirty[0], 0x00, sizeof(Eina_Bool) * EVAS_3D_STATE_MAX);
   return EINA_TRUE;
}

EOLIAN static void
_evas_3d_node_evas_3d_object_update_notify(Eo *obj, Evas_3D_Node_Data *pd EINA_UNUSED)
{
   /* Update transform. */
   evas_3d_node_tree_traverse(obj, EVAS_3D_TREE_TRAVERSE_LEVEL_ORDER, EINA_FALSE,
                              _node_transform_update, NULL);

   /* Update AABB. */
   evas_3d_node_tree_traverse(obj, EVAS_3D_TREE_TRAVERSE_POST_ORDER, EINA_FALSE,
                              _node_aabb_update, NULL);

   /* Update node item. */
   evas_3d_node_tree_traverse(obj, EVAS_3D_TREE_TRAVERSE_ANY_ORDER, EINA_FALSE,
                              _node_item_update, NULL);

   /* Mark all nodes in the tree as up-to-date. */
   evas_3d_node_tree_traverse(obj, EVAS_3D_TREE_TRAVERSE_ANY_ORDER, EINA_FALSE,
                              _node_update_done, NULL);
}

static void
_node_free(Evas_3D_Object *obj)
{
   Evas_3D_Node_Data *pd = eo_data_scope_get(obj, MY_CLASS);

   if (pd->members)
     {
        Eina_List *l;
        Evas_3D_Node *n;

        EINA_LIST_FOREACH(pd->members, l, n)
          {
             eo_unref(n);
          }

        eina_list_free(pd->members);
     }

   if (pd->data.mesh.meshes)
     {
        Eina_List *l;
        Evas_3D_Mesh *m;

        EINA_LIST_FOREACH(pd->data.mesh.meshes, l, m)
          {
             //evas_3d_mesh_node_del(m, node);
             eo_unref(m);
          }

        eina_list_free(pd->data.mesh.meshes);
     }

   if (pd->data.mesh.node_meshes)
     eina_hash_free(pd->data.mesh.node_meshes);

   if (pd->scenes_root)
     eina_hash_free(pd->scenes_root);

   if (pd->scenes_camera)
     eina_hash_free(pd->scenes_camera);

   //free(node);
}

void
evas_3d_node_scene_root_add(Evas_3D_Node *node, Evas_3D_Scene *scene)
{
   int count = 0;
   Evas_3D_Node_Data *pd = eo_data_scope_get(node, MY_CLASS);
   if (pd->scenes_root == NULL)
     {
        pd->scenes_root = eina_hash_pointer_new(NULL);

        if (pd->scenes_root == NULL)
          {
             ERR("Failed to create hash table.");
             return;
          }
     }
   else
     count = (int)(uintptr_t)eina_hash_find(pd->scenes_root, &scene);

   eina_hash_set(pd->scenes_root, &scene, (const void *)(uintptr_t)(count + 1));
}

void
evas_3d_node_scene_root_del(Evas_3D_Node *node, Evas_3D_Scene *scene)
{
   int count = 0;
   Evas_3D_Node_Data *pd = eo_data_scope_get(node, MY_CLASS);
   if (pd->scenes_root == NULL)
     {
        ERR("No scene to delete.");
        return;
     }

   count = (int)(uintptr_t)eina_hash_find(pd->scenes_root, &scene);

   if (count == 1)
     eina_hash_del(pd->scenes_root, &scene, NULL);
   else
     eina_hash_set(pd->scenes_root, &scene, (const void *)(uintptr_t)(count - 1));
}

void
evas_3d_node_scene_camera_add(Evas_3D_Node *node, Evas_3D_Scene *scene)
{
   int count = 0;
   Evas_3D_Node_Data *pd = eo_data_scope_get(node, MY_CLASS);
   if (pd->scenes_camera == NULL)
     {
        pd->scenes_camera = eina_hash_pointer_new(NULL);

        if (pd->scenes_camera == NULL)
          {
             ERR("Failed to create hash table.");
             return;
          }
     }
   else
     count = (int)(uintptr_t)eina_hash_find(pd->scenes_camera, &scene);

   eina_hash_set(pd->scenes_camera, &scene, (const void *)(uintptr_t)(count + 1));
}

void
evas_3d_node_scene_camera_del(Evas_3D_Node *node, Evas_3D_Scene *scene)
{
   int count = 0;
   Evas_3D_Node_Data *pd = eo_data_scope_get(node, MY_CLASS);
   if (pd->scenes_camera == NULL)
     {
        ERR("No scene to delete.");
        return;
     }

   count = (int)(uintptr_t)eina_hash_find(pd->scenes_camera, &scene);

   if (count == 1)
     eina_hash_del(pd->scenes_camera, &scene, NULL);
   else
     eina_hash_set(pd->scenes_camera, &scene, (const void *)(uintptr_t)(count - 1));
}

void
evas_3d_node_traverse(Evas_3D_Node *from, Evas_3D_Node *to, Evas_3D_Node_Traverse_Type type,
                      Eina_Bool skip, Evas_3D_Node_Func func, void *data)
{
   Eina_List *nodes = NULL, *n;
   Evas_3D_Node *node = NULL;

   if (from == NULL || func == NULL)
     goto error;

   if (type == EVAS_3D_NODE_TRAVERSE_DOWNWARD)
     {
        if (to == NULL)
          goto error;

        node = to;

        do {
             nodes = eina_list_prepend(nodes, (const void *)node);

             if (node == from)
               break;
             Evas_3D_Node_Data *pdnode = eo_data_scope_get(node, MY_CLASS);
             node = pdnode->parent;

             if (node == NULL)
               goto error;
        } while (1);
     }
   else if (type == EVAS_3D_NODE_TRAVERSE_UPWARD)
     {
        node = from;

        do {
             nodes = eina_list_append(nodes, (const void *)node);

             if (node == to)
               break;
             Evas_3D_Node_Data *pdnode = eo_data_scope_get(node, MY_CLASS);
             node = pdnode->parent;

             if (node == NULL)
               {
                  if (to == NULL)
                    break;

                  goto error;
               }
        } while (1);
     }

   EINA_LIST_FOREACH(nodes, n, node)
     {
        if (!func(node, data) && skip)
          break;
     }

   eina_list_free(nodes);
   return;

error:
   ERR("Node traverse error.");

   if (nodes)
     eina_list_free(nodes);
}

void
evas_3d_node_tree_traverse(Evas_3D_Node *root, Evas_3D_Tree_Traverse_Type type,
                           Eina_Bool skip, Evas_3D_Node_Func func, void *data)
{
   Eina_List *nodes = NULL, *l;
   Evas_3D_Node *node = NULL, *n, *last;

   if (root == NULL || func == NULL)
     return;

   if (type == EVAS_3D_TREE_TRAVERSE_LEVEL_ORDER)
     {
        /* Put the root node in the queue. */
        nodes = eina_list_append(nodes, root);

        while (eina_list_count(nodes) > 0)
          {
             /* Dequeue a node. */
             node = eina_list_data_get(nodes);
             Evas_3D_Node_Data *pdnode = eo_data_scope_get(node, MY_CLASS);

             nodes = eina_list_remove_list(nodes, nodes);

             /* Call node function on the node. */
             if (func(node, data) || !skip)
               {
                  /* Enqueue member nodes. */
                  EINA_LIST_FOREACH(pdnode->members, l, n)
                    {
                       nodes = eina_list_append(nodes, n);
                    }
               }
          }
     }
   else if (type == EVAS_3D_TREE_TRAVERSE_PRE_ORDER)
     {
        /* Put the root node in the stack. */
        nodes = eina_list_append(nodes, root);

        while (eina_list_count(nodes) > 0)
          {
             /* Pop a node from the stack. */
             node = eina_list_data_get(nodes);
             Evas_3D_Node_Data *pdnode = eo_data_scope_get(node, MY_CLASS);
             nodes = eina_list_remove_list(nodes, nodes);

             /* Call node function on the node. */
             if (func(node, data) || !skip)
               {
                  /* Push member nodes into the stack. */
                  EINA_LIST_REVERSE_FOREACH(pdnode->members, l, n)
                    {
                       nodes = eina_list_prepend(nodes, n);
                    }
               }
          }
     }
   else if (type == EVAS_3D_TREE_TRAVERSE_POST_ORDER)
     {
        if (skip)
          {
             ERR("Using skip with post order traversal has no effect.");
             return;
          }

        /* Put the root node in the stack. */
        nodes = eina_list_append(nodes, root);
        last = NULL;

        while (eina_list_count(nodes) > 0)
          {
             /* Peek a node from the stack. */
             node = eina_list_data_get(nodes);
             Evas_3D_Node_Data *pdnode = eo_data_scope_get(node, MY_CLASS);
             if (eina_list_count(pdnode->members) == 0)
               {
                  /* The peeked node is a leaf node,
                   * so visit it and pop from the stack. */
                  func(node, data);
                  nodes = eina_list_remove_list(nodes, nodes);

                  /* Save the last visited node. */
                  last = node;
               }
             else
               {
                  /* If the peeked node is not a leaf node,
                   * there can be only two possible cases.
                   *
                   * 1. the parent of the last visited node.
                   * 2. a sibling of the last visited node.
                   *
                   * If the last visited node is a direct child of the peeked node,
                   * we have no unvisted child nodes for the peeked node, so we have to visit
                   * the peeked node and pop from the stack.
                   *
                   * Otherwise it should be a sibling of the peeked node, so we have to push
                   * its childs into the stack. */
                   Evas_3D_Node_Data *pdlast;
                   if (last )
                     pdlast= eo_data_scope_get(last, MY_CLASS);
                  if (last && pdlast->parent == node)
                    {
                       /* Visit the node as it doesn't have any unvisited child node. */
                       func(node, data);
                       nodes = eina_list_remove_list(nodes, nodes);

                       /* Save the last visited node. */
                       last = node;
                    }
                  else
                    {
                       /* Push child nodes into the stack. */
                       EINA_LIST_REVERSE_FOREACH(pdnode->members, l, n)
                         {
                            nodes = eina_list_prepend(nodes, n);
                         }
                    }
               }
          }
     }

   if (nodes != NULL)
     eina_list_free(nodes);
}

Eina_Bool
_node_is_visible(Evas_3D_Node *node EINA_UNUSED, Evas_3D_Node *camera_node EINA_UNUSED)
{
   /* TODO: */
   return EINA_TRUE;
}

Eina_Bool
evas_3d_node_mesh_collect(Evas_3D_Node *node, void *data)
{
   Evas_3D_Scene_Public_Data *scene_data = (Evas_3D_Scene_Public_Data *)data;
   Evas_3D_Node_Data *pd = eo_data_scope_get(node, MY_CLASS);
   if (!_node_is_visible(node, scene_data->camera_node))
     {
        /* Skip entire sub-tree of this node. */
        return EINA_FALSE;
     }

   if (pd->type == EVAS_3D_NODE_TYPE_MESH)
     scene_data->mesh_nodes = eina_list_append(scene_data->mesh_nodes, node);

   return EINA_TRUE;
}

Eina_Bool
evas_3d_node_light_collect(Evas_3D_Node *node, void *data)
{
   Evas_3D_Scene_Public_Data *scene_data = (Evas_3D_Scene_Public_Data *)data;
   Evas_3D_Node_Data *pd = eo_data_scope_get(node, MY_CLASS);
   if (pd->type == EVAS_3D_NODE_TYPE_LIGHT)
     scene_data->light_nodes = eina_list_append(scene_data->light_nodes, node);

   return EINA_TRUE;
}

EAPI Evas_3D_Node *
evas_3d_node_add(Evas *e, Evas_3D_Node_Type type)
{
   MAGIC_CHECK(e, Evas, MAGIC_EVAS);
   return NULL;
   MAGIC_CHECK_END();
   Evas_Object *eo_obj = eo_add(MY_CLASS, e, evas_3d_node_constructor(type));
   return eo_obj;
}

EOLIAN static void
_evas_3d_node_constructor(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_Node_Type type)
{
   eo_do(obj, evas_3d_object_type_set(EVAS_3D_OBJECT_TYPE_NODE));

   evas_vec3_set(&pd->position, 0.0, 0.0, 0.0);
   evas_vec4_set(&pd->orientation, 0.0, 0.0, 0.0, 1.0);
   evas_vec3_set(&pd->scale, 1.0, 1.0, 1.0);

   evas_vec3_set(&pd->position_world, 0.0, 0.0, 0.0);
   evas_vec4_set(&pd->orientation_world, 0.0, 0.0, 0.0, 1.0);
   evas_vec3_set(&pd->scale_world, 1.0, 1.0, 1.0);

   pd->position_inherit = EINA_TRUE;
   pd->orientation_inherit = EINA_TRUE;
   pd->scale_inherit = EINA_TRUE;
   pd->data.mesh.node_meshes = 0;

   evas_box3_set(&pd->aabb, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

   pd->type = type;

   if (type == EVAS_3D_NODE_TYPE_MESH)
     {
        pd->data.mesh.node_meshes = eina_hash_pointer_new(_node_mesh_free_func);
        if (pd->data.mesh.node_meshes == NULL)
          {
             ERR("Failed to create node mesh table.");
             _node_free(obj);
          }
     }
}

EOLIAN static Evas_3D_Node_Type
_evas_3d_node_type_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd)
{
   return pd->type;
}

EOLIAN static void
_evas_3d_node_member_add(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_Node *member)
{
   if (obj == member)
     {
        ERR("Failed to add a member node (adding to itself).");
        return;
     }
   Evas_3D_Node_Data *pdmember = eo_data_scope_get(member, MY_CLASS);
   if (pdmember->parent == obj)
     return;

   if (pdmember->parent)
     {
        /* Detaching from previous parent. */
         Evas_3D_Node_Data *pdmemberparent = eo_data_scope_get(pdmember->parent, MY_CLASS);
        pdmemberparent->members = eina_list_remove(pdmemberparent->members, member);

        /* Mark changed. */
        eo_do(pdmember->parent, evas_3d_object_change(EVAS_3D_STATE_NODE_MEMBER, NULL));
     }
   else
     {
        /* Should get a new reference. */
        eo_ref(member);
     }

   /* Add the member node. */
   pd->members = eina_list_append(pd->members, (const void *)member);
   pdmember->parent = obj;

   /* Mark changed. */
   eo_do(member, evas_3d_object_change(EVAS_3D_STATE_NODE_PARENT, NULL));
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_MEMBER, NULL));
}

EOLIAN static void
_evas_3d_node_member_del(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_Node *member)
{
   Evas_3D_Node_Data *pdmember = eo_data_scope_get(member, MY_CLASS);
   if (pdmember->parent != obj)
     {
        ERR("Failed to delete a member node (not a member of the given node)");
        return;
     }

   /* Delete the member node. */
   pd->members = eina_list_remove(pd->members, member);
   pdmember->parent = NULL;

   /* Mark modified object as changed. */
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_MEMBER, NULL));
   eo_do(member, evas_3d_object_change(EVAS_3D_STATE_NODE_PARENT, NULL));

   /* Decrease reference count. */
   eo_unref(member);
}

EOLIAN static Evas_3D_Node *
_evas_3d_node_parent_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd)
{
   return pd->parent;
}

EOLIAN static const Eina_List *
_evas_3d_node_member_list_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd)
{
   return pd->members;
}

EOLIAN static void
_evas_3d_node_position_set(Eo *obj, Evas_3D_Node_Data *pd, Evas_Real x, Evas_Real y, Evas_Real z)
{
   pd->position.x = x;
   pd->position.y = y;
   pd->position.z = z;

   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_TRANSFORM, NULL));
}

EOLIAN static void
_evas_3d_node_orientation_set(Eo *obj, Evas_3D_Node_Data *pd, Evas_Real x, Evas_Real y, Evas_Real z, Evas_Real w)
{
   pd->orientation.x = x;
   pd->orientation.y = y;
   pd->orientation.z = z;
   pd->orientation.w = w;

   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_TRANSFORM, NULL));
}

EOLIAN static void
_evas_3d_node_orientation_angle_axis_set(Eo *obj, Evas_3D_Node_Data *pd,
                                        Evas_Real angle, Evas_Real x, Evas_Real y, Evas_Real z)
{
   Evas_Real half_angle = 0.5 * DEGREE_TO_RADIAN(angle);
   Evas_Real s = sin(half_angle);
   Evas_Vec3 axis;

   evas_vec3_set(&axis, x, y, z);
   evas_vec3_normalize(&axis, &axis);

   pd->orientation.w = cos(half_angle);
   pd->orientation.x = s * axis.x;
   pd->orientation.y = s * axis.y;
   pd->orientation.z = s * axis.z;

   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_TRANSFORM, NULL));
}

EOLIAN static void
_evas_3d_node_scale_set(Eo *obj, Evas_3D_Node_Data *pd, Evas_Real x, Evas_Real y, Evas_Real z)
{
   pd->scale.x = x;
   pd->scale.y = y;
   pd->scale.z = z;

   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_TRANSFORM, NULL));
}

EOLIAN static void
_evas_3d_node_position_get(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_Space space,
                          Evas_Real *x, Evas_Real *y, Evas_Real *z)
{
   if (space == EVAS_3D_SPACE_LOCAL)
     {
        if (x) *x = 0.0;
        if (y) *y = 0.0;
        if (z) *z = 0.0;
     }
   else if (space == EVAS_3D_SPACE_PARENT)
     {
        if (x) *x = pd->position.x;
        if (y) *y = pd->position.y;
        if (z) *z = pd->position.z;
     }
   else if (space == EVAS_3D_SPACE_WORLD)
     {
        eo_do(obj, evas_3d_object_update());

        if (x) *x = pd->position_world.x;
        if (y) *y = pd->position_world.y;
        if (z) *z = pd->position_world.z;
     }
}

EOLIAN static void
_evas_3d_node_orientation_get(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_Space space,
                             Evas_Real *x, Evas_Real *y, Evas_Real *z, Evas_Real *w)
{
   if (space == EVAS_3D_SPACE_LOCAL)
     {
        if (x) *x = 0.0;
        if (y) *y = 0.0;
        if (z) *z = 0.0;
        if (w) *w = 0.0;
     }
   else if (space == EVAS_3D_SPACE_PARENT)
     {
        if (x) *x = pd->orientation.x;
        if (y) *y = pd->orientation.y;
        if (z) *z = pd->orientation.z;
        if (w) *w = pd->orientation.w;
     }
   else if (space == EVAS_3D_SPACE_WORLD)
     {
        eo_do(obj, evas_3d_object_update());

        if (x) *x = pd->orientation_world.x;
        if (y) *y = pd->orientation_world.y;
        if (z) *z = pd->orientation_world.z;
        if (w) *w = pd->orientation_world.w;
     }

}

EOLIAN static void
_evas_3d_node_scale_get(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_Space space,
                       Evas_Real *x, Evas_Real *y, Evas_Real *z)
{
   if (space == EVAS_3D_SPACE_LOCAL)
     {
        if (x) *x = 0.0;
        if (y) *y = 0.0;
        if (z) *z = 0.0;
     }
   else if (space == EVAS_3D_SPACE_PARENT)
     {
        if (x) *x = pd->scale.x;
        if (y) *y = pd->scale.y;
        if (z) *z = pd->scale.z;
     }
   else if (space == EVAS_3D_SPACE_WORLD)
     {
        eo_do(obj, evas_3d_object_update());

        if (x) *x = pd->scale_world.x;
        if (y) *y = pd->scale_world.y;
        if (z) *z = pd->scale_world.z;
     }
}

EOLIAN static void
_evas_3d_node_position_inherit_set(Eo *obj, Evas_3D_Node_Data *pd, Eina_Bool inherit)
{
   pd->position_inherit = inherit;
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_TRANSFORM, NULL));
}

EOLIAN static void
_evas_3d_node_orientation_inherit_set(Eo *obj, Evas_3D_Node_Data *pd, Eina_Bool inherit)
{
   pd->orientation_inherit = inherit;
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_TRANSFORM, NULL));
}

EOLIAN static void
_evas_3d_node_scale_inherit_set(Eo *obj, Evas_3D_Node_Data *pd, Eina_Bool inherit)
{
   pd->scale_inherit = inherit;
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_TRANSFORM, NULL));
}

EOLIAN static Eina_Bool
_evas_3d_node_position_inherit_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd)
{
   return pd->position_inherit;
}

EOLIAN static Eina_Bool
_evas_3d_node_orientation_inherit_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd)
{
   return pd->orientation_inherit;
}

EOLIAN static Eina_Bool
_evas_3d_node_scale_inherit_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd)
{
   return pd->scale_inherit;
}

EOLIAN static void
_evas_3d_node_look_at_set(Eo *obj, Evas_3D_Node_Data *pd,
                         Evas_3D_Space target_space, Evas_Real tx, Evas_Real ty, Evas_Real tz,
                         Evas_3D_Space up_space, Evas_Real ux, Evas_Real uy, Evas_Real uz)
{
   Evas_Vec3   target;
   Evas_Vec3   up;
   Evas_Vec3   x, y, z;

   /* Target position in parent space. */
   if (target_space == EVAS_3D_SPACE_LOCAL)
     {
        ERR("TODO:");
        return;
     }
   else if (target_space == EVAS_3D_SPACE_PARENT)
     {
        evas_vec3_set(&target, tx, ty, tz);
     }
   else if (target_space == EVAS_3D_SPACE_WORLD)
     {
        ERR("TODO:");
        return;
     }
   else
     {
        ERR("Invalid coordinate space.");
        return;
     }

   if (up_space == EVAS_3D_SPACE_LOCAL)
     {
       evas_vec3_set(&up, ux, uy, uz);
        //ERR("TODO:");
        //return;
     }
   else if (up_space == EVAS_3D_SPACE_PARENT)
     {
        evas_vec3_set(&up, ux, uy, uz);
     }
   else if (up_space == EVAS_3D_SPACE_WORLD)
     {
        ERR("TODO:");
        return;
     }
   else
     {
        ERR("Invalid coordinate space.");
        return;
     }

   /* From now on, everything takes place in parent space. */
   evas_vec3_subtract(&z, &pd->position, &target);
   evas_vec3_normalize(&z, &z);

   evas_vec3_cross_product(&x, &up, &z);
   evas_vec3_normalize(&x, &x);

   evas_vec3_cross_product(&y, &z, &x);
   evas_vec3_normalize(&y, &y);

   /* Below matrix to quaternion conversion code taken from
    * http://fabiensanglard.net/doom3_documentation/37726-293748.pdf
    * When any license issue occurs, use ken shoemake's algorithm instead.
    */

   if (x.x + y.y + z.z > 0.0)
     {
        Evas_Real t = x.x + y.y + z.z + 1.0;
        Evas_Real s = evas_reciprocal_sqrt(t) * 0.5;

        pd->orientation.w = s * t;
        pd->orientation.z = (x.y - y.x) * s;
        pd->orientation.y = (z.x - x.z) * s;
        pd->orientation.x = (y.z - z.y) * s;
     }
   else if (x.x > y.y && x.x > z.z)
     {
        Evas_Real t = x.x - y.y - z.z + 1.0;
        Evas_Real s = evas_reciprocal_sqrt(t) * 0.5;

        pd->orientation.x = s * t;
        pd->orientation.y = (x.y + y.x) * s;
        pd->orientation.z = (z.x + x.z) * s;
        pd->orientation.w = (y.z - z.y) * s;
     }
   else if (y.y > z.z)
     {
        Evas_Real t = -x.x + y.y - z.z + 1.0;
        Evas_Real s = evas_reciprocal_sqrt(t) * 0.5;

        pd->orientation.y = s * t;
        pd->orientation.x = (x.y + y.x) * s;
        pd->orientation.w = (z.x - x.z) * s;
        pd->orientation.z = (y.z + z.y) * s;
     }
   else
     {
        Evas_Real t = -x.x - y.y + z.z + 1.0;
        Evas_Real s = evas_reciprocal_sqrt(t) * 0.5;

        pd->orientation.z = s * t;
        pd->orientation.w = (x.y - y.x) * s;
        pd->orientation.x = (z.x + x.z) * s;
        pd->orientation.y = (y.z + z.y) * s;
     }

   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_TRANSFORM, NULL));
}

EOLIAN static void
_evas_3d_node_camera_set(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_Camera *camera)
{
   if (pd->type != EVAS_3D_NODE_TYPE_CAMERA)
     {
        ERR("Node type mismatch.");
        return;
     }

   if (pd->data.camera.camera == camera)
     return;

   if (pd->data.camera.camera)
     {
        /* Detach previous camera object. */
        evas_3d_camera_node_del(pd->data.camera.camera, obj);
        eo_unref(pd->data.camera.camera);
     }

   pd->data.camera.camera = camera;
   eo_ref(camera);

   /* Register change notification on the camera for this node. */
   evas_3d_camera_node_add(camera, obj);

   /* Mark changed. */
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_CAMERA, NULL));
}

EOLIAN static Evas_3D_Camera *
_evas_3d_node_camera_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd)
{
   return pd->data.camera.camera;
}

EOLIAN static void
_evas_3d_node_light_set(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_Light *light)
{
   if (pd->type != EVAS_3D_NODE_TYPE_LIGHT)
     {
        ERR("Node type mismatch.");
        return;
     }

   if (pd->data.light.light == light)
     return;

   if (pd->data.light.light)
     {
        /* Detach previous light object. */
        evas_3d_light_node_del(pd->data.light.light, obj);
        eo_unref(pd->data.light.light);
     }

   pd->data.light.light = light;
   eo_ref(light);

   /* Register change notification on the light for this node. */
   evas_3d_light_node_add(light, obj);

   /* Mark changed. */
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_LIGHT, NULL));
}

EOLIAN static Evas_3D_Light *
_evas_3d_node_light_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd)
{
   return pd->data.light.light;
}

EOLIAN static void
_evas_3d_node_mesh_add(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_Mesh *mesh)
{
   Evas_3D_Node_Mesh *nm = NULL;

   if (pd->type != EVAS_3D_NODE_TYPE_MESH)
     {
        ERR("Node type mismatch.");
        return;
     }

   if (eina_hash_find(pd->data.mesh.node_meshes, &mesh) != NULL)
     {
        ERR("The mesh is already added to the node.");
        return;
     }

   if ((nm = _node_mesh_new(obj, mesh)) == NULL)
     {
        ERR("Failed to create node mesh.");
        return;
     }

   /* TODO: Find node mesh and add if it does not exist. */
   if (!eina_hash_add(pd->data.mesh.node_meshes, &mesh, nm))
     {
        ERR("Failed to add a mesh to mesh table.");
        _node_mesh_free(nm);
        return;
     }

   pd->data.mesh.meshes = eina_list_append(pd->data.mesh.meshes, mesh);
   eo_ref(mesh);

   /* Register change notification. */
   evas_3d_mesh_node_add(mesh, obj);

   /* Mark changed. */
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_MESH_GEOMETRY, NULL));
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_MESH_MATERIAL, NULL));
}

EOLIAN static void
_evas_3d_node_mesh_del(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_Mesh *mesh)
{
   if (pd->type != EVAS_3D_NODE_TYPE_MESH)
     {
        ERR("Node type mismatch.");
        return;
     }

   if (!eina_hash_del(pd->data.mesh.node_meshes, &mesh, NULL))
     {
        ERR("The given mesh doesn't belong to this node.");
        return;
     }

   pd->data.mesh.meshes = eina_list_remove(pd->data.mesh.meshes, mesh);
   evas_3d_mesh_node_del(mesh, obj);
   eo_unref(mesh);

   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_MESH_GEOMETRY, NULL));
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_MESH_MATERIAL, NULL));
}

EOLIAN static const Eina_List *
_evas_3d_node_mesh_list_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd)
{
   return pd->data.mesh.meshes;
}

EOLIAN static void
_evas_3d_node_mesh_frame_set(Eo *obj, Evas_3D_Node_Data *pd, Evas_3D_Mesh *mesh, int frame)
{
   Evas_3D_Node_Mesh *nm = NULL;

   if (pd->type != EVAS_3D_NODE_TYPE_MESH)
     {
        ERR("Node type mismatch.");
        return;
     }

   if ((nm = eina_hash_find(pd->data.mesh.node_meshes, &mesh)) == NULL)
     {
        ERR("The given mesh doesn't belongs to this node.");
        return;
     }

   nm->frame = frame;
   eo_do(obj, evas_3d_object_change(EVAS_3D_STATE_NODE_MESH_FRAME, NULL));
}

EOLIAN static int
_evas_3d_node_mesh_frame_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd, Evas_3D_Mesh *mesh)
{
   Evas_3D_Node_Mesh *nm = NULL;

   if (pd->type != EVAS_3D_NODE_TYPE_MESH)
     {
        ERR("Node type mismatch.");
        return 0;
     }

   if ((nm = eina_hash_find(pd->data.mesh.node_meshes, &mesh)) == NULL)
     {
        ERR("The given mesh doesn't belongs to this node.");
        return 0;
     }

   return nm->frame;
}

EOLIAN static void
_evas_3d_node_bounding_box_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd, Evas_Real *x, Evas_Real *y, Evas_Real *z, Evas_Real *x2, Evas_Real *y2, Evas_Real *z2)
{
   eo_do(obj, evas_3d_object_update());
   if (x) *x = pd->aabb.p0.x;
   if (y) *y = pd->aabb.p0.y;
   if (z) *z = pd->aabb.p0.z;
   if (x2) *x2 = pd->aabb.p1.x;
   if (y2) *y2 = pd->aabb.p1.y;
   if (z2) *z2 = pd->aabb.p1.z;
}

EOLIAN static void
_evas_3d_node_bounding_sphere_get(Eo *obj EINA_UNUSED, Evas_3D_Node_Data *pd, Evas_Real *x, Evas_Real *y, Evas_Real *z, Evas_Real *r)
{
   eo_do(obj, evas_3d_object_update());
   if (x) *x = pd->bsphere.center.x;
   if (y) *y = pd->bsphere.center.y;
   if (z) *z = pd->bsphere.center.z;
   if (r) *r = pd->bsphere.radius;
}

#include "canvas/evas_3d_node.eo.c"
