#define _CRT_SECURE_NO_WARNINGS
#include <glad.h>
#include <glfw3.h>
#define MMATH_IMPL
#include <mmath.h>
#include "..\inc\gl_loadshader.hpp"
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include <nuklear.h>
#include <nuklear_glfw_gl3.h>

#define SCR_WIDTH   1600
#define SCR_HEIGHT  900
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024
#define UNREFERENCED_PARAMETER(__x)	__x

void framebuffer_size_callback(GLFWwindow *window, s32 width, s32 height);
void mouse_callback(GLFWwindow *window, f64 x_pos, f64 y_pos);
void process_input(GLFWwindow *window, f32 delta_time);
/* u32 load_shader(const char *cs_path); */
void reset_camera(void);

struct camera_t
{
    vec3_t lookfrom;
    vec3_t lookat;
    vec3_t up;
};

camera_t cam;
vec2_t window_size = {SCR_WIDTH, SCR_HEIGHT};
u32 samples = 1;
b32 sample_change = TRUE;
b32 nuklear_control;
f32 cam_speed = 5.0f;

int
main()
{
    /////////////////////////////////////////////////////////////////////////
    // GLFW SETUP

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "test", NULL, NULL);
    if (!window)
    {
        printf("failed to create window!\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    glfwSetFramebufferSizeCallback(window, &framebuffer_size_callback);
    glfwSetCursorPosCallback(window, &mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("failed to initialize glad!\n");
        glfwTerminate();
        return -1;
    }

    /////////////////////////////////////////////////////////////////////////
    // SHADER SETUP

    u32 render_shader = load_shader("..\\src\\shaders\\compute.vert", "..\\src\\shaders\\compute.frag");
    u32 shaders[] =
    {
		load_shader("..\\src\\shaders\\chapter7.comp"),
		load_shader("..\\src\\shaders\\chapter8.comp"),
		load_shader("..\\src\\shaders\\chapter9.comp"),
		load_shader("..\\src\\shaders\\chapter10.comp"),
		load_shader("..\\src\\shaders\\chapter11.comp"),
		load_shader("..\\src\\shaders\\hollow glass ball.comp"),
		load_shader("..\\src\\shaders\\checkered texture.comp"),
		load_shader("..\\src\\shaders\\lamp.comp"),
		load_shader("..\\src\\shaders\\plane.comp")
    };

    u32 comp_shader_index = 0;
    u32 comp_shader = shaders[0];

    /////////////////////////////////////////////////////////////////////////
    // OBJECTS

    u32 vao;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);

    u32 texture_data;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture_data);
    glTextureStorage2D(texture_data, 1, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT);
    glBindImageTexture(0, texture_data, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    /////////////////////////////////////////////////////////////////////////
    // NUKLEAR + MATRIX + CAMERA SETUP

    reset_camera();
    cam.up.x = 0;
    cam.up.y = 1;
    cam.up.z = 0;

    mat4_t view;
    mat4_t proj = mat4_perspective(70.0f, (f32)SCR_WIDTH / (f32)SCR_HEIGHT, 0.1f, 100.0f);

    f32 current_frame;
    f32 last_frame = 0.0f;
    f32 delta_time;

    struct nk_context *ctx;
    struct nk_glfw glfw = {0};
    struct nk_font_atlas *atlas;

    ctx = nk_glfw3_init(&glfw, window, NK_GLFW3_INSTALL_CALLBACKS);
    nk_glfw3_font_stash_begin(&glfw, &atlas);
    nk_glfw3_font_stash_end(&glfw);

    /////////////////////////////////////////////////////////////////////////
    // RENDER LOOP

    while (!glfwWindowShouldClose(window))
    {
        current_frame = (f32)glfwGetTime();
        delta_time = current_frame - last_frame;
        last_frame = current_frame;

        process_input(window, delta_time);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(vao);
        if (sample_change)
        {
            view = mat4_lookat(cam.lookfrom,
            vec3_add(cam.lookfrom, cam.lookat),
            cam.up);
            glUseProgram(comp_shader);
            glUniform1ui(glGetUniformLocation(comp_shader, "samples"),
                         samples);
            glUniform2f(glGetUniformLocation(comp_shader, "resolution"),
                        window_size.x, window_size.y);
            glUniformMatrix4fv(glGetUniformLocation(comp_shader, "view_matrix"),
                                1, GL_FALSE, m_cast(view));
            glUniformMatrix4fv(glGetUniformLocation(comp_shader, "proj_matrix"),
                                1, GL_FALSE, m_cast(proj));
            glDispatchCompute(SCR_WIDTH / 16, SCR_HEIGHT / 16, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            glBindTextureUnit(0, texture_data);

            if (samples > 1)
                sample_change = FALSE;
        }
        glBindTexture(GL_TEXTURE_2D, texture_data);
        glUseProgram(render_shader);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // NUKLEAR
        nk_glfw3_new_frame(&glfw);
        if (nk_begin(ctx, "Demo", nk_rect(50, 50, 275, 275),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                     NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            // Stats
            nk_layout_row_static(ctx, 20, 130, 1);
            nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "Frametime: %.3f ms", delta_time * 1000.0f);
            nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "FPS: %f", 1.0f / delta_time);

            // Camera
            nk_layout_row_static(ctx, 20, 250, 1);
            nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "Pos: x: %.2f y: %.2f z: %.2f", cam.lookfrom.x,
                                                                               cam.lookfrom.y,
                                                                               cam.lookfrom.z);
            nk_layout_row_static(ctx, 20, 250, 1);
            nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "Look: x: %.2f y: %.2f z: %.2f", cam.lookat.x,
                                                                                cam.lookat.y,
                                                                                cam.lookat.z);
            nk_layout_row_begin(ctx, NK_STATIC, 30, 5);
            {
                nk_layout_row_push(ctx, 50);
                nk_label(ctx, "Speed:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 150);
                nk_slider_float(ctx, 0, &cam_speed, 10.0f, 1.0f);
            } nk_layout_row_end(ctx);

            // Prev / Next Buttons 
            nk_layout_row_begin(ctx, NK_STATIC, 30, 5);
            {
                nk_layout_row_push(ctx, 100);
                if (nk_button_label(ctx, "Prev"))
                {
                    if (comp_shader_index == 0)
                        comp_shader_index = 8;
                    else
                        comp_shader_index--;

                    reset_camera();
                    comp_shader = shaders[comp_shader_index];
                }
                nk_layout_row_push(ctx, 100);
                if (nk_button_label(ctx, "Next"))
                {
                    if (comp_shader_index == 8)
                        comp_shader_index = 0;
                    else
                        comp_shader_index++;

                    reset_camera();
                    comp_shader = shaders[comp_shader_index];
                }
            } nk_layout_row_end(ctx);
            nk_layout_row_static(ctx, 20, 80, 1);
            nk_value_uint(ctx, "Scene", comp_shader_index + 1);

            nk_layout_row_static(ctx, 20, 80, 1);
            if (nk_button_label(ctx, "Reset"))
            {
                reset_camera();
            }
			nk_layout_row_static(ctx, 10, 200, 1);
			nk_text(ctx, "To increase sampling, press R", 50, NK_LEFT);
        } nk_end(ctx);

        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}

void
framebuffer_size_callback(GLFWwindow *window,
                          s32 width,
                          s32 height)
{
	UNREFERENCED_PARAMETER(window);
    glViewport(0, 0, width, height);
    window_size.x = (f32)width;
    window_size.y = (f32)height;
}

void
process_input(GLFWwindow *window,
             f32 delta_time)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (sample_change)
    {
        f32 speed = cam_speed * delta_time;

        if (glfwGetKey(window, GLFW_KEY_W))
            cam.lookfrom = vec3_add(cam.lookfrom, vec3_scal(cam.lookat, speed));
        if (glfwGetKey(window, GLFW_KEY_S))
            cam.lookfrom = vec3_sub(cam.lookfrom, vec3_scal(cam.lookat, speed));
        if (glfwGetKey(window, GLFW_KEY_A))
        {
            vec3_t temp = vec3_cross(cam.lookat, cam.up);
            temp = vec3_normalize(temp);
            temp = vec3_scal(temp, speed);
            cam.lookfrom = vec3_sub(cam.lookfrom, temp);
        }
        if (glfwGetKey(window, GLFW_KEY_D))
        {
            vec3_t temp = vec3_cross(cam.lookat, cam.up);
            temp = vec3_normalize(temp);
            temp = vec3_scal(temp, speed);
            cam.lookfrom = vec3_add(cam.lookfrom, temp);
        }
    }

    if (glfwGetKey(window, GLFW_KEY_Q))
    {
        nuklear_control = FALSE;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    if (glfwGetKey(window, GLFW_KEY_E))
    {
        nuklear_control = TRUE;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if (glfwGetKey(window, GLFW_KEY_R))
    {
        samples = 600;
    }
}

void
mouse_callback(GLFWwindow *window,
               f64 x_pos,
               f64 y_pos)
{
    static b32 first_mouse = TRUE;
    static f32 yaw = -90.0f;
    static f32 pitch = 0.0f;
    static f64 last_x = SCR_WIDTH / 2;
    static f64 last_y = SCR_HEIGHT / 2;

    if (!nuklear_control)
    {
        if (first_mouse)
        {
            last_x = x_pos;
            last_y = y_pos;
            first_mouse = FALSE;
        }

        f32 x_offset = (f32)(x_pos - last_x);
        f32 y_offset = (f32)(last_y - y_pos);
        last_x = x_pos;
        last_y = y_pos;

        const f32 sens = 0.1f;
        x_offset *= sens;
        y_offset *= sens;

        yaw += x_offset;
        pitch += y_offset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        vec3_t direction;
        direction.x = m_cos(m_rads(yaw)) * m_cos(m_rads(pitch));
        direction.y = m_sin(m_rads(pitch));
        direction.z = m_sin(m_rads(yaw)) * m_cos(m_rads(pitch));

        cam.lookat = vec3_normalize(direction);
        samples = 1;
        sample_change = TRUE;
    }
    else
    {
        first_mouse = TRUE;
    }
}

void
reset_camera(void)
{
    cam.lookfrom.x = 0;
    cam.lookfrom.y = 0;
    cam.lookfrom.z = 1;

    cam.lookat.x = 0;
    cam.lookat.y = 0;
    cam.lookat.z = -1;
}
