#version 450 core

in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D image_data;

void
main(void)
{
    frag_color = texture(image_data, tex_coords);
}
