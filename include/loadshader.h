#ifndef LOADSHADER_H
#define LOADSHADER_H
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>

typedef enum
{
    VERTEX,
    FRAGMENT,
	COMPUTE,
    PROGRAM
} shader_enum;

void 
check_compile_errors(u32 data, 
                     shader_enum type,
                     const char *filename)
{
    b32     success;
    char    info_log[1024];

    if (type == PROGRAM)
    {
        glGetProgramiv(data, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(data, 1024, NULL, info_log);
            printf("PROGRAM LINKING ERROR OF TYPE:\n %s\n\n", info_log);
        }
    }
    else if (type == VERTEX || type == FRAGMENT|| type == COMPUTE)
    {
        glGetShaderiv(data, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(data, 1024, NULL, info_log);
            printf("SHADER COMPILATION ERROR OF TYPE: %s\n %s\n\n", filename, info_log);
        }
    }
    else
    {
        printf("INVALID SHADER TYPE\n");
    }
}

u32 
load_shader(const char *vs_path, 
            const char *fs_path)
{
    FILE    *fptr;
    s32     file_len;
    GLchar  *shader_source;
        
    // VERTEX SHADER
    fptr = fopen(vs_path, "rb");
    fseek(fptr, 0, SEEK_END);
    file_len = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    shader_source = (GLchar *)malloc(file_len + 1);
    fread(shader_source, 1, file_len, fptr);
    shader_source[file_len] = 0;
    fclose(fptr);

    u32 vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &shader_source, NULL);
    glCompileShader(vertex);
    check_compile_errors(vertex, VERTEX, vs_path);
    free(shader_source);

    // FRAGMENT SHADER
    fptr = fopen(fs_path, "rb");
    fseek(fptr, 0, SEEK_END);
    file_len = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    shader_source = (GLchar *)malloc(file_len + 1);
    fread(shader_source, 1, file_len, fptr);
    shader_source[file_len] = 0;
    fclose(fptr);

    u32 fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &shader_source, NULL);
    glCompileShader(fragment);
    check_compile_errors(fragment, FRAGMENT, fs_path);
    free(shader_source);

    // SHADER PROGRAM
    u32 program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    check_compile_errors(program, PROGRAM, vs_path);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

#endif // LOADSHADER_H
