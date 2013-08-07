#include "tie.h"
#include "camera.h"

const char name[] = "myplugin";

enum {
    HIT_OUT,
    HIT_IN,
    HIT_OVERLAP
};

fastf_t aval;

struct hitdata_s {
    render_t *render;
    fastf_t color;
    fastf_t a;
    int in;
    fastf_t prevhitdist;
    adrt_mesh_t *prevhitmesh;
};

static void *
hit(struct tie_ray_s *UNUSED(ray), struct tie_id_s *id, struct tie_tri_s *tri, void *ptr)
{
    struct hitdata_s *hd = (struct hitdata_s *)ptr;
    fastf_t dist;
    adrt_mesh_t *mesh = tri->ptr;

    if(hd->in == HIT_IN) {
	if( mesh != hd->prevhitmesh ) {
	    /*
	    bu_log("Overlap issue: %x(%s) %x(%s) (%f %f : %f)\n", 
		    hd->prevhitmesh, hd->prevhitmesh->name,
		    mesh, mesh->name,
		    1000*hd->prevhitdist, 1000*id->dist, 1000*(id->dist-hd->prevhitdist));
	    */
	    hd->in = HIT_OVERLAP;
	    return NULL;
	}

	dist = aval * (id->dist - hd->prevhitdist);
	hd->color += dist;
	hd->a -= dist;
	hd->in = HIT_OUT;
	if(hd->color > 1.0) {
	    hd->color = 1.0;
	    return ptr;
	}
	if(hd->a < 0)
	    return ptr;	/* end raytracing */
    } else {
	hd->in = HIT_IN;
	hd->prevhitdist = id->dist;
	hd->prevhitmesh = mesh;
    }

    /* continue firing */
    return NULL;
}

void
adrt_plugin_work(struct render_s *render, struct tie_s *tie, struct tie_ray_s *ray, vect_t *pixel)
{
    struct tie_id_s id;
    struct hitdata_s hitdata;
    hitdata.render = render;
    hitdata.a = 1.0;
    hitdata.in = 0;
    hitdata.color = 0;

    tie_work(tie, ray, &id, hit, &hitdata);
    VSETALL(*pixel, hitdata.color);
#if 0
    switch(hitdata.in) {
	case HIT_IN:
	    VSET(pixel->v, 1, 0, 0);
	    break;
	case HIT_OUT:
	    VSETALL(pixel->v, hitdata.color);
	    break;
	case HIT_OVERLAP:
	    VSET(pixel->v, 0, 1, 0);
	    break;
    }
#endif
}

static void
adrt_plugin_free(render_t *UNUSED(r))
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
