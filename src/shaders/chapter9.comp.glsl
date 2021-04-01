#version 450 core

#define MAT_LAMBERTIAN  0
#define MAT_METAL       1

layout (local_size_x = 16, local_size_y = 16) in;
layout (rgba32f) uniform image2D image_data;
uniform vec2 resolution;
uniform uint samples;
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
    int material_id;
};

struct hit_record_t
{
    vec3 p;
    vec3 normal;
    float t;
    bool front_face;
    int material_id;
};

struct material_t
{
    int type;
    vec3 albedo;
    float metal_fuzz;
};

struct scene_t
{
    int num_spheres;
    int num_materials;
    sphere_t spheres[4];
    material_t materials[4];
};

vec3        ray_at(ray_t r, float t);
bool        sphere_hit(ray_t r, float t_min, float t_max, inout hit_record_t rec);
void        set_face_normal(ray_t r, vec3 outward_normal, inout hit_record_t rec);
bool        scene_hit(ray_t r, scene_t s, float t_min, float t_max, inout hit_record_t rec);
vec3        ray_trace(ray_t r, scene_t world, uint max_depth);
uint        f_randi(inout uint index);
float       f_randf(inout uint index);
ray_t       get_ray(float u, float v);
void        write_color(vec3 color, float samples_per_pixel);
vec3        random_in_unit_sphere(inout uint index);
vec3        random_unit_vector(inout uint index);
bool        scatter_lambertian(ray_t r_in, inout hit_record_t rec, material_t mat, out vec3 atten, out ray_t r_scattered);
bool        scatter_metal(ray_t r_in, inout hit_record_t rec, material_t mat, out vec3 atten, out ray_t r_scattered);

uint state = (gl_GlobalInvocationID.x * 1973 + gl_GlobalInvocationID.y * 9277);

void
main(void)
{
    memoryBarrier();

    scene_t scene;
    scene.num_spheres = 4;
    scene.num_materials = 4;

    // Ground
    scene.materials[0].type = MAT_LAMBERTIAN;
    scene.materials[0].albedo = vec3(0.8, 0.8, 0.0); 
    scene.spheres[0].center = vec3(0, -100.5, -1);
    scene.spheres[0].radius = 100;
    scene.spheres[0].material_id = 0;

    // Center
    scene.materials[1].type = MAT_LAMBERTIAN;
    scene.materials[1].albedo = vec3(0.7, 0.3, 0.3); 
    scene.spheres[1].center = vec3(0, 0, -1);
    scene.spheres[1].radius = 0.5;
    scene.spheres[1].material_id = 1;

    // Left
    scene.materials[2].type = MAT_METAL;
    scene.materials[2].albedo = vec3(0.8, 0.8, 0.8); 
    scene.materials[2].metal_fuzz = 0.3;
    scene.spheres[2].center = vec3(-1, 0, -1);
    scene.spheres[2].radius = 0.5;
    scene.spheres[2].material_id = 2;

    // Right
    scene.materials[3].type = MAT_METAL;
    scene.materials[3].albedo = vec3(0.8, 0.6, 0.2); 
    scene.materials[3].metal_fuzz = 1.0;
    scene.spheres[3].center = vec3(1, 0, -1);
    scene.spheres[3].radius = 0.5;
    scene.spheres[3].material_id = 3;



    ray_t ray;

    vec3 pixel_data = vec3(0.0);
    
    for (uint i = 0; i < samples; i++)
    {
        float u = ((gl_GlobalInvocationID.x + f_randf(state)) / resolution.x);
        float v = ((gl_GlobalInvocationID.y + f_randf(state)) / resolution.y);
        ray = get_ray(u, v);         
        pixel_data += ray_trace(ray, scene, 50);
    }

    write_color(pixel_data, samples);
    
    memoryBarrier();
}

vec3
ray_at(ray_t r, float t)
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
        rec.material_id = s.material_id;

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
ray_trace(ray_t r, scene_t world, uint max_depth)
{
    ray_t cur_ray = r;
    vec3 atten = vec3(0.0);
    vec3 color = vec3(1.0);
    hit_record_t rec;

    int i;
    for (i = 0; i < max_depth; i++)
    {
        ray_t scattered_ray;

        if (scene_hit(cur_ray, world, 0.001, 100000000000.0, rec))
        {
            if (world.materials[rec.material_id].type == MAT_LAMBERTIAN)
            {
                if (scatter_lambertian(cur_ray, rec, world.materials[rec.material_id],
                                       atten, scattered_ray))
                {
                    color *= atten;
                    cur_ray = scattered_ray;
                }
                else
                {
                    atten = vec3(0.0);
                    color *= atten;
                    break;
                }
            }
            else if (world.materials[rec.material_id].type == MAT_METAL)
            {
                if (scatter_metal(cur_ray, rec, world.materials[rec.material_id],
                                       atten, scattered_ray))
                {
                    color *= atten;
                    cur_ray = scattered_ray;
                }
                else
                {
                    atten = vec3(0.0);
                    color *= atten;
                    break;
                }
            }
        }
        else
        {
            vec3 unit_dir = normalize(r.direction);
            float t = 0.5 * (unit_dir.y + 1.0);
            vec3 c = (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0); 
            color *= c;
            break;
        }
    }

    if (i < 50)
        return color;
    else
        return vec3(0.0);   // exceeded iteration
}

uint
f_randi(inout uint index)
{
    uint x = index;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 15;
    index = x;

    return x;
}

float
f_randf(inout uint index)
{
    return (f_randi(index) & 0xffffff) / 16777216.0f;
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

void
write_color(vec3 color, float samples_per_pixel)
{
    float r = color.x;
    float g = color.y;
    float b = color.z;

    // Gamma correction
    float scale = 1.0 / samples_per_pixel; 
    r = sqrt(scale * r);
    g = sqrt(scale * g);
    b = sqrt(scale * b);
    
    imageStore(image_data,
               ivec2(gl_GlobalInvocationID.xy),
               vec4(r, g, b, 1.0));
}

vec3
random_in_unit_sphere(inout uint index)
{
    float z = f_randf(index) * 2.0 - 1.0;
    float t = f_randf(index) * 2.0 * 3.1415926;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float x = r * cos(t);
    float y = r * sin(t);
    vec3 res = vec3(x, y, z);
    //res *= pow(f_randf(index), 1.0 / 3.0);

    return res;
}

vec3
random_unit_vector(inout uint index)
{
    float z = f_randf(index) * 2.0 - 1.0;
    float t = f_randf(index) * 2.0 * 3.1415926;
    float r = sqrt(1.0 - z * z);
    float x = r * cos(t);
    float y = r * sin(t);

    return vec3(x, y, z);
}

bool
scatter_lambertian(ray_t r_in, inout hit_record_t rec,
                   material_t mat, out vec3 atten, out ray_t r_scattered)
{
    vec3 scatter_dir = rec.normal + random_unit_vector(state);
    r_scattered.origin = rec.p;
    r_scattered.direction = scatter_dir;
    atten = mat.albedo;

    // NOTE: Maybe add the near_zero() function here at some point
    // to catch degenerate rays.

    return true;
}

bool
scatter_metal(ray_t r_in, inout hit_record_t rec,
              material_t mat, out vec3 atten, out ray_t r_scattered)
{
    vec3 reflected = reflect(normalize(r_in.direction), rec.normal);
    r_scattered.origin = rec.p;
    r_scattered.direction = reflected + mat.metal_fuzz * random_in_unit_sphere(state);
    atten = mat.albedo;

    return (dot(r_scattered.direction, rec.normal) > 0);
}
