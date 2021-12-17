#ifndef GL_LOADSHADER_H
#define GL_LOADSHADER_H

#include <stdio.h>
#include <stdlib.h>
#include <types.h>

u32		load_shader(const char *vs_path, const char *fs_path);
u32		load_shader(const char *cs_path);
char	*load_source(const char *path);
void 	check_compile_errors(u32 data, b32 is_program, const char* filename);

// Define everything here since this file is only being used once in the whole
// program (main.cpp)

u32
load_shader(const char *vs_path,
			const char *fs_path)
{
	u32			vertex,
				fragment,
				program;
	char		*shader_src;


	// Vertex
	shader_src = load_source(vs_path);
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &shader_src, NULL);
	glCompileShader(vertex);
	check_compile_errors(vertex, 0, vs_path);
	free(shader_src);

	// Fragment
	shader_src = load_source(fs_path);
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &shader_src, NULL);
	glCompileShader(fragment);
	check_compile_errors(fragment, 0, vs_path);
	free(shader_src);

	// Program
	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);
	check_compile_errors(program, 1, NULL);

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return program;
}

u32
load_shader(const char *cs_path)
{
	u32			compute,
				program;
	char		*shader_src;


	shader_src = load_source(cs_path);
	compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &shader_src, NULL);
    glCompileShader(compute);
    check_compile_errors(compute, 0, cs_path);
    free(shader_src);

    program = glCreateProgram();
    glAttachShader(program, compute);
    glLinkProgram(program);
    check_compile_errors(program, 1, NULL);

	glDeleteShader(compute);

	return program;
}

char *
load_source(const char *path)
{
	FILE		*fptr;
	s32			file_len;
	char		*src;


	fptr = fopen(path, "rb");
	fseek(fptr, 0, SEEK_END);
	file_len = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);
	src = (char *)malloc(file_len + 1);
	fread(src, 1, file_len, fptr);
	src[file_len] = 0;
	fclose(fptr);

	return src;
}

void 
check_compile_errors(u32 data, 
                     b32 is_program,
                     const char* filename)
{
    s32 		success;
    char		info_log[1024];


    if (is_program)
    {
        glGetProgramiv(data, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(data, 1024, NULL, info_log);
            printf("PROGRAM LINKING ERROR OF TYPE:\n %s\n\n", info_log);
        }
    }
    else
    {
        glGetShaderiv(data, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(data, 1024, NULL, info_log);
            printf("SHADER COMPILATION ERROR OF TYPE: %s\n %s\n\n", filename, info_log);
        }
    }
}

#endif GL_LOADSHADER_H

