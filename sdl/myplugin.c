#include "tie.h"
#include "camera.h"

const char name[] = "myplugin";

void
adrt_plugin_work(render_t *r, tie_t *t, tie_ray_t *ray, TIE_3 *pixel)
{
    tie_id_t id;
    adrt_mesh_t *mesh;
    if ((mesh = (adrt_mesh_t *)tie_work(t, ray, &id, render_hit, NULL))) {
	pixel->v[0] = 1.0;
	pixel->v[1] = 0.0;
	pixel->v[2] = 1.0;
    }
}

void
adrt_plugin_free(render_t *r)
{
    return;
}

void
init(render_t *r, char *usr)
{
    r->work = adrt_plugin_work;
    r->free = adrt_plugin_free;
}
