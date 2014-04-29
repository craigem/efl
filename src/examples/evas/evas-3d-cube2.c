#include <Ecore.h>
#include <Ecore_Evas.h>
#include <stdio.h>
#include <math.h>
#include <Evas.h>

#define  WIDTH          1024
#define  HEIGHT         1024

typedef struct _Scene_Data
{
   Evas_3D_Scene    *scene;
   Evas_3D_Node     *root_node;
   Evas_3D_Node     *camera_node;
   Evas_3D_Node     *light_node;
   Evas_3D_Node     *mesh_node;

   Evas_3D_Camera   *camera;
   Evas_3D_Light    *light;
   Evas_3D_Mesh     *mesh;
   Evas_3D_Material *material0;
   Evas_3D_Material *material1;

   Evas_3D_Texture  *texture0;
   Evas_3D_Texture  *texture1;
   Evas_3D_Texture  *texture_normal;
} Scene_Data;

Ecore_Evas       *ecore_evas  = NULL;
Evas             *evas        = NULL;
Evas_Object      *background  = NULL;
Evas_Object      *image       = NULL;

static const float cube_vertices[] =
{
   /* Front */
   -1.0,  1.0,  1.0,     0.0,  0.0,  1.0,     1.0, 0.0, 0.0, 1.0,     0.0,  1.0,
    1.0,  1.0,  1.0,     0.0,  0.0,  1.0,     1.0, 0.0, 0.0, 1.0,     1.0,  1.0,
   -1.0, -1.0,  1.0,     0.0,  0.0,  1.0,     1.0, 0.0, 0.0, 1.0,     0.0,  0.0,
    1.0, -1.0,  1.0,     0.0,  0.0,  1.0,     1.0, 0.0, 0.0, 1.0,     1.0,  0.0,

   /* Back */
    1.0,  1.0, -1.0,     0.0,  0.0, -1.0,     0.0, 0.0, 1.0, 1.0,     0.0,  1.0,
   -1.0,  1.0, -1.0,     0.0,  0.0, -1.0,     0.0, 0.0, 1.0, 1.0,     1.0,  1.0,
    1.0, -1.0, -1.0,     0.0,  0.0, -1.0,     0.0, 0.0, 1.0, 1.0,     0.0,  0.0,
   -1.0, -1.0, -1.0,     0.0,  0.0, -1.0,     0.0, 0.0, 1.0, 1.0,     1.0,  0.0,

   /* Left */
   -1.0,  1.0, -1.0,    -1.0,  0.0,  0.0,     0.0, 1.0, 0.0, 1.0,     0.0,  1.0,
   -1.0,  1.0,  1.0,    -1.0,  0.0,  0.0,     0.0, 1.0, 0.0, 1.0,     1.0,  1.0,
   -1.0, -1.0, -1.0,    -1.0,  0.0,  0.0,     0.0, 1.0, 0.0, 1.0,     0.0,  0.0,
   -1.0, -1.0,  1.0,    -1.0,  0.0,  0.0,     0.0, 1.0, 0.0, 1.0,     1.0,  0.0,

   /* Right */
    1.0,  1.0,  1.0,     1.0,  0.0,  0.0,     1.0, 1.0, 0.0, 1.0,     0.0,  1.0,
    1.0,  1.0, -1.0,     1.0,  0.0,  0.0,     1.0, 1.0, 0.0, 1.0,     1.0,  1.0,
    1.0, -1.0,  1.0,     1.0,  0.0,  0.0,     1.0, 1.0, 0.0, 1.0,     0.0,  0.0,
    1.0, -1.0, -1.0,     1.0,  0.0,  0.0,     1.0, 1.0, 0.0, 1.0,     1.0,  0.0,

   /* Top */
   -1.0,  1.0, -1.0,     0.0,  1.0,  0.0,     1.0, 0.0, 1.0, 1.0,     0.0,  1.0,
    1.0,  1.0, -1.0,     0.0,  1.0,  0.0,     1.0, 0.0, 1.0, 1.0,     1.0,  1.0,
   -1.0,  1.0,  1.0,     0.0,  1.0,  0.0,     1.0, 0.0, 1.0, 1.0,     0.0,  0.0,
    1.0,  1.0,  1.0,     0.0,  1.0,  0.0,     1.0, 0.0, 1.0, 1.0,     1.0,  0.0,

   /* Bottom */
    1.0, -1.0, -1.0,     0.0, -1.0,  0.0,     0.0, 1.0, 1.0, 1.0,     0.0,  1.0,
   -1.0, -1.0, -1.0,     0.0, -1.0,  0.0,     0.0, 1.0, 1.0, 1.0,     1.0,  1.0,
    1.0, -1.0,  1.0,     0.0, -1.0,  0.0,     0.0, 1.0, 1.0, 1.0,     0.0,  0.0,
   -1.0, -1.0,  1.0,     0.0, -1.0,  0.0,     0.0, 1.0, 1.0, 1.0,     1.0,  0.0,
};

static const unsigned short cube_indices[] =
{
   /* Front */
   0,   1,  2,  2,  1,  3,

   /* Back */
   4,   5,  6,  6,  5,  7,

   /* Left */
   8,   9, 10, 10,  9, 11,

   /* Right */
   12, 13, 14, 14, 13, 15,

   /* Top */
   16, 17, 18, 18, 17, 19,

   /* Bottom */
   20, 21, 22, 22, 21, 23
};

static const unsigned int pixels0[] =
{
   0xff0000ff, 0xff0000ff, 0xffff0000, 0xffff0000,
   0xff0000ff, 0xff0000ff, 0xffff0000, 0xffff0000,
   0xff00ff00, 0xff00ff00, 0xff000000, 0xff000000,
   0xff00ff00, 0xff00ff00, 0xff000000, 0xff000000,
};

static const unsigned int pixels1[] =
{
   0xffff0000, 0xffff0000, 0xff00ff00, 0xff00ff00,
   0xffff0000, 0xffff0000, 0xff00ff00, 0xff00ff00,
   0xff0000ff, 0xff0000ff, 0xffffffff, 0xffffffff,
   0xff0000ff, 0xff0000ff, 0xffffffff, 0xffffffff,
};

static void
_on_delete(Ecore_Evas *ee EINA_UNUSED)
{
   ecore_main_loop_quit();
}

static void
_on_canvas_resize(Ecore_Evas *ee)
{
   int w, h;

   ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);

   evas_object_resize(background, w, h);
   evas_object_resize(image, w, h);
   evas_object_move(image, 0, 0);
}

static Eina_Bool
_animate_scene(void *data)
{
   static float angle = 0.0f;
   static int   frame = 0;
   static int   inc   = 1;
   Scene_Data  *scene = (Scene_Data *)data;

   angle += 0.2;

   evas_3d_node_orientation_angle_axis_set(scene->mesh_node, angle, 1.0, 1.0, 1.0);

   /* Rotate */
   if (angle > 360.0)
     angle -= 360.0f;

   frame += inc;

   if (frame >= 20)
     inc = -1;
   else if (frame <= 0)
     inc = 1;

   evas_3d_node_mesh_frame_set(scene->mesh_node, scene->mesh, frame);

   return EINA_TRUE;
}

static void
_camera_setup(Scene_Data *data)
{
   data->camera = evas_3d_camera_add(evas);
   evas_3d_camera_projection_perspective_set(data->camera, 60.0, 1.0, 2.0, 50.0);

   data->camera_node = evas_3d_node_add(evas, EVAS_3D_NODE_TYPE_CAMERA);
   evas_3d_node_camera_set(data->camera_node, data->camera);
   evas_3d_node_member_add(data->root_node, data->camera_node);
   evas_3d_node_position_set(data->camera_node, 0.0, 0.0, 10.0);
   evas_3d_node_look_at_set(data->camera_node, EVAS_3D_SPACE_PARENT, 0.0, 0.0, 0.0,
			    EVAS_3D_SPACE_PARENT, 0.0, 1.0, 0.0);
}

static void
_light_setup(Scene_Data *data)
{
   data->light = evas_3d_light_add(evas);
   evas_3d_light_ambient_set(data->light, 0.2, 0.2, 0.2, 1.0);
   evas_3d_light_diffuse_set(data->light, 1.0, 1.0, 1.0, 1.0);
   evas_3d_light_specular_set(data->light, 1.0, 1.0, 1.0, 1.0);

   data->light_node = evas_3d_node_add(evas, EVAS_3D_NODE_TYPE_LIGHT);
   evas_3d_node_light_set(data->light_node, data->light);
   evas_3d_node_member_add(data->root_node, data->light_node);
   evas_3d_node_position_set(data->light_node, 0.0, 0.0, 10.0);
   evas_3d_node_look_at_set(data->light_node, EVAS_3D_SPACE_PARENT, 0.0, 0.0, 0.0,
			    EVAS_3D_SPACE_PARENT, 0.0, 1.0, 0.0);
}

static void
_mesh_setup(Scene_Data *data)
{
   /* Setup material. */
   data->material0 = evas_3d_material_add(evas);
   data->material1 = evas_3d_material_add(evas);

   evas_3d_material_enable_set(data->material0, EVAS_3D_MATERIAL_AMBIENT, EINA_TRUE);
   evas_3d_material_enable_set(data->material0, EVAS_3D_MATERIAL_DIFFUSE, EINA_TRUE);
   evas_3d_material_enable_set(data->material0, EVAS_3D_MATERIAL_SPECULAR, EINA_TRUE);
   evas_3d_material_enable_set(data->material0, EVAS_3D_MATERIAL_NORMAL, EINA_TRUE);

   evas_3d_material_color_set(data->material0, EVAS_3D_MATERIAL_AMBIENT, 0.2, 0.2, 0.2, 1.0);
   evas_3d_material_color_set(data->material0, EVAS_3D_MATERIAL_DIFFUSE, 0.8, 0.8, 0.8, 1.0);
   evas_3d_material_color_set(data->material0, EVAS_3D_MATERIAL_SPECULAR, 1.0, 1.0, 1.0, 1.0);
   evas_3d_material_shininess_set(data->material0, 100.0);

   evas_3d_material_enable_set(data->material1, EVAS_3D_MATERIAL_AMBIENT, EINA_TRUE);
   evas_3d_material_enable_set(data->material1, EVAS_3D_MATERIAL_DIFFUSE, EINA_TRUE);
   evas_3d_material_enable_set(data->material1, EVAS_3D_MATERIAL_SPECULAR, EINA_TRUE);
   evas_3d_material_enable_set(data->material1, EVAS_3D_MATERIAL_NORMAL, EINA_TRUE);

   evas_3d_material_color_set(data->material1, EVAS_3D_MATERIAL_AMBIENT, 0.2, 0.2, 0.2, 1.0);
   evas_3d_material_color_set(data->material1, EVAS_3D_MATERIAL_DIFFUSE, 0.8, 0.8, 0.8, 1.0);
   evas_3d_material_color_set(data->material1, EVAS_3D_MATERIAL_SPECULAR, 1.0, 1.0, 1.0, 1.0);
   evas_3d_material_shininess_set(data->material1, 100.0);

   data->texture0 = evas_3d_texture_add(evas);
   data->texture1 = evas_3d_texture_add(evas);
   data->texture_normal = evas_3d_texture_add(evas);

   evas_3d_texture_data_set(data->texture0, EVAS_3D_COLOR_FORMAT_RGBA,
                            EVAS_3D_PIXEL_FORMAT_8888, 4, 4, &pixels0[0]);
   evas_3d_texture_data_set(data->texture1, EVAS_3D_COLOR_FORMAT_RGBA,
                            EVAS_3D_PIXEL_FORMAT_8888, 4, 4, &pixels1[0]);
   evas_3d_texture_file_set(data->texture_normal, "normal_lego.png", NULL);

   evas_3d_material_texture_set(data->material0, EVAS_3D_MATERIAL_DIFFUSE, data->texture0);
   evas_3d_material_texture_set(data->material1, EVAS_3D_MATERIAL_DIFFUSE, data->texture1);
   evas_3d_material_texture_set(data->material1, EVAS_3D_MATERIAL_NORMAL, data->texture_normal);

   /* Setup mesh. */
   data->mesh = evas_3d_mesh_add(evas);
   evas_3d_mesh_vertex_count_set(data->mesh, 24);
   evas_3d_mesh_frame_add(data->mesh, 0);

   evas_3d_mesh_frame_vertex_data_set(data->mesh, 0, EVAS_3D_VERTEX_POSITION,
				      12 * sizeof(float), &cube_vertices[ 0]);
   evas_3d_mesh_frame_vertex_data_set(data->mesh, 0, EVAS_3D_VERTEX_NORMAL,
				      12 * sizeof(float), &cube_vertices[ 3]);
   evas_3d_mesh_frame_vertex_data_set(data->mesh, 0, EVAS_3D_VERTEX_COLOR,
				      12 * sizeof(float), &cube_vertices[ 6]);
   evas_3d_mesh_frame_vertex_data_set(data->mesh, 0, EVAS_3D_VERTEX_TEXCOORD,
				      12 * sizeof(float), &cube_vertices[10]);

   evas_3d_mesh_index_data_set(data->mesh, EVAS_3D_INDEX_FORMAT_UNSIGNED_SHORT,
                               36, &cube_indices[0]);
   evas_3d_mesh_vertex_assembly_set(data->mesh, EVAS_3D_VERTEX_ASSEMBLY_TRIANGLES);

   evas_3d_mesh_shade_mode_set(data->mesh, EVAS_3D_SHADE_MODE_NORMAL_MAP);

   evas_3d_mesh_frame_material_set(data->mesh, 0, data->material0);

   evas_3d_mesh_frame_add(data->mesh, 20);
   evas_3d_mesh_frame_material_set(data->mesh, 20, data->material1);

   data->mesh_node = evas_3d_node_add(evas, EVAS_3D_NODE_TYPE_MESH);
   evas_3d_node_member_add(data->root_node, data->mesh_node);
   evas_3d_node_mesh_add(data->mesh_node, data->mesh);
}

static void
_scene_setup(Scene_Data *data)
{
   data->scene = evas_3d_scene_add(evas);
   evas_3d_scene_size_set(data->scene, WIDTH, HEIGHT);
   evas_3d_scene_background_color_set(data->scene, 0.0, 0.0, 0.0, 0.0);

   data->root_node = evas_3d_node_add(evas, EVAS_3D_NODE_TYPE_NODE);

   _camera_setup(data);
   _light_setup(data);
   _mesh_setup(data);

   evas_3d_scene_root_node_set(data->scene, data->root_node);
   evas_3d_scene_camera_node_set(data->scene, data->camera_node);
}

int
main(void)
{
   Scene_Data data;

   if (!ecore_evas_init())
     return 0;

   ecore_evas = ecore_evas_new(NULL, 10, 10, WIDTH, HEIGHT, NULL);

   if (!ecore_evas)
     return 0;

   ecore_evas_callback_delete_request_set(ecore_evas, _on_delete);
   ecore_evas_callback_resize_set(ecore_evas, _on_canvas_resize);
   ecore_evas_show(ecore_evas);

   evas = ecore_evas_get(ecore_evas);

   _scene_setup(&data);

   /* Add a background rectangle objects. */
   background = evas_object_rectangle_add(evas);
   evas_object_color_set(background, 0, 0, 0, 255);
   evas_object_move(background, 0, 0);
   evas_object_resize(background, WIDTH, HEIGHT);
   evas_object_show(background);

   /* Add an image object for 3D scene rendering. */
   image = evas_object_image_filled_add(evas);
   evas_object_move(image, 0, 0);
   evas_object_resize(image, WIDTH, HEIGHT);
   evas_object_show(image);

   /* Set the image object as render target for 3D scene. */
   evas_object_image_scene_set(image, data.scene);

   /* Add animation timer callback. */
   ecore_timer_add(0.01, _animate_scene, &data);

   /* Enter main loop. */
   ecore_main_loop_begin();

   ecore_evas_free(ecore_evas);
   ecore_evas_shutdown();

   return 0;
}