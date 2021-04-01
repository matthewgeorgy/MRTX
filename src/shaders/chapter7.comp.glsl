#version 450 core

layout (local_size_x = 16, local_size_y = 16) in;
layout (rgba32f) uniform image2D image_data;
uniform vec2 resolution;
uniform mat4 view_matrix;
uniform mat4 proj_matrix;

mat4 inv_viewmat = inverse(view_matrix);
mat4 inv_projmat = inverse(proj_matrix);

struct ray_t
{
    vec3 origin;
    vec3 direction;
};

struct sphere_t
{
    vec3 center;
    float radius;
};

struct hit_record_t
{
    vec3 p;
    vec3 normal;
    float t;
    bool front_face;
};

struct scene_t
{
    int num_spheres;
    sphere_t spheres[2];
};

vec3        ray_at(ray_t r, float t);
bool        sphere_hit(ray_t r, float t_min, float t_max, inout hit_record_t rec);
void        set_face_normal(ray_t r, vec3 outward_normal, inout hit_record_t rec);
bool        scene_hit(ray_t r, scene_t s, float t_min, float t_max, inout hit_record_t rec);
vec3        ray_trace(ray_t r, scene_t world);
ray_t       get_ray(float u, float v);

void
main(void)
{
    memoryBarrier();
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);

    float u = gl_GlobalInvocationID.x / resolution.x;
    float v = gl_GlobalInvocationID.y / resolution.y;

    scene_t scene;
    scene.num_spheres = 2;
    scene.spheres[0].center = vec3(0, 0, -1);
    scene.spheres[0].radius = 0.5;
    scene.spheres[1].center = vec3(0, -100.5, -1);
    scene.spheres[1].radius = 100;

    ray_t ray;
    ray = get_ray(u, v);

    vec4 data = vec4(ray_trace(ray, scene), 1.0);

    imageStore(image_data, pixel, data);    
    memoryBarrier();
}

vec3
ray_at(ray_t r, in float t)
{
    return r.origin + t * r.direction;
}

bool
sphere_hit(ray_t r, sphere_t s, float t_min, float t_max, inout hit_record_t rec)
{
    vec3 oc = r.origin - s.center;
    float a = dot(r.direction, r.direction);
    float half_b = dot(oc, r.direction);
    float c = dot(oc, oc) - s.radius * s.radius;
    float disc = half_b * half_b - a * c;

    if (disc < 0)
        return false;
    else
    {
        float sqrtd = sqrt(disc);
        float root = (-half_b - sqrtd) / a;
        if (root < t_min || root > t_max)
        {
            root = (-half_b + sqrtd) / a;
            if (root < t_min || root > t_max)
                return false;
        }
        
        rec.t = root;
        rec.p = ray_at(r, rec.t);
        vec3 outward_normal = (rec.p - s.center) / s.radius;
        set_face_normal(r, outward_normal, rec);

        return true;
    }
}

void
set_face_normal(ray_t r, vec3 outward_normal, inout hit_record_t rec)
{
    rec.front_face = dot(r.direction, outward_normal) < 0;
    rec.normal = rec.front_face ? outward_normal : -outward_normal;
}

bool
scene_hit(ray_t r, scene_t s, float t_min, float t_max, inout hit_record_t rec)
{
    hit_record_t temp_rec;
    bool hit_anything = false;
    float closest_so_far = t_max;

    for (int i = 0; i < s.num_spheres; i++)
    {
        if (sphere_hit(r, s.spheres[i], t_min, closest_so_far, temp_rec))
        {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    return hit_anything;
}

vec3
ray_trace(ray_t r, scene_t world)
{
    hit_record_t rec;
    if (scene_hit(r, world, 0, 10000000.0, rec))
        return 0.5 * (rec.normal + vec3(1, 1, 1));
    vec3 unit_dir = normalize(r.direction);
    float t = 0.5 * (unit_dir.y + 1.0);
    return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0); 
}

ray_t
get_ray(float u, float v)
{
    u = u * 2.0 - 1.0;
    v = v * 2.0 - 1.0;

    vec4 clip_pos = vec4(u, v, -1.0, 1.0);
    vec4 view_pos = inv_projmat * clip_pos;

    vec3 dir = normalize(vec3(inv_viewmat * vec4(view_pos.x, view_pos.y, -1.0, 0.0)));

    vec4 origin = inv_viewmat * vec4(0.0, 0.0, 0.0, 1.0);
    origin.xyz /= origin.w;

    ray_t r;

    r.origin = origin.xyz;
    r.direction = dir;
    
    return r;
}
