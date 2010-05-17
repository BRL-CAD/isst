#include "tie.h"
#include "camera.h"

const char name[] = "myplugin";

fastf_t aval;

struct hitdata_s {
    render_t *render;
    fastf_t color;
    fastf_t a;
    int in;
    fastf_t prevhitdist;
};

static void *
hit(tie_ray_t *ray, tie_id_t *id, tie_tri_t *tri, void *ptr)
{
    struct hitdata_s *hd = (struct hitdata_s *)ptr;
    fastf_t dist;

    if(hd->in) {
	dist = aval * (id->dist - hd->prevhitdist);
	hd->color += dist;
	hd->a -= dist;
	hd->in = 0;
	if(hd->color > 1.0) {
	    hd->color = 1.0;
	    return ptr;
	}
	if(hd->a < 0)
	    return ptr;	/* end raytracing */
    } else {
	hd->in = 1;
	hd->prevhitdist = id->dist;
    }

    /* continue firing */
    return NULL;
}

void
adrt_plugin_work(render_t *r, tie_t *t, tie_ray_t *ray, TIE_3 *pixel)
{
    tie_id_t id;
    struct hitdata_s hitdata;
    adrt_mesh_t *mesh;
    hitdata.render = r;
    hitdata.a = 1.0;
    hitdata.in = 0;
    hitdata.color = 0;

    tie_work(t, ray, &id, hit, &hitdata);
    if(hitdata.in) {
	VSET(pixel->v, 1, 0, 0);
    } else
	VSET(pixel->v, hitdata.color, hitdata.color, hitdata.color);
}

void
adrt_plugin_free(render_t *r)
{
    return;
}

void
init(render_t *r, char *usr)
{
    aval = 0.1 * atof(usr);
    r->work = adrt_plugin_work;
    r->free = adrt_plugin_free;
}
