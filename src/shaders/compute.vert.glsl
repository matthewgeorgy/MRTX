#version 450 core

vec2 vertices[4] = vec2[4](vec2(1.0, 1.0),
                           vec2(1.0, -1.0),
                           vec2(-1.0, 1.0),
                           vec2(-1.0, -1.0));
vec2 uvs[4] = vec2[4](vec2(1.0, 1.0),
                      vec2(1.0, 0.0),
                      vec2(0.0, 1.0),
                      vec2(0.0, 0.0));

out vec2 tex_coords;

void
main(void)
{
    vec2 pos = vertices[gl_VertexID];
    tex_coords = uvs[gl_VertexID];
    gl_Position = vec4(pos, 0.0, 1.0);
}
