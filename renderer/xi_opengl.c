#include "xi_opengl.h"

extern XI_EXPORT XI_RENDERER_INIT(xi_opengl_init) {
    xiRenderer *result = 0;

    xiOpenGLContext *gl = gl_os_context_create(platform);
    if (gl) {
        result = (xiRenderer *) gl;

        // we were able to create an os specific opengl context, so continue with the os non-specific
        // initialisation
        //
        glGetIntegerv(GL_MAJOR_VERSION, &gl->info.major_version);
        glGetIntegerv(GL_MINOR_VERSION, &gl->info.minor_version);

        if (gl->info.srgb) {
            gl->texture_format = GL_SRGB8_ALPHA8;
            glEnable(GL_FRAMEBUFFER_SRGB);
        }
        else {
            gl->texture_format = GL_RGBA8;
        }

        gl->GenVertexArrays(1, &gl->vao);
        gl->GenBuffers(1, &gl->vbo);
        gl->GenBuffers(1, &gl->ebo);

        // @todo: allow configurable quad count :configure
        //
        u32 num_vertices = 4 * 1024;
        u32 num_indices  = 6 * 1024;

        // we just use persistently mapped buffers for the immeidate mode quad buffer
        // as it is easier to manage. i don't know if the opengl driver will copy
        // to gpu local memory when unmapping a buffer, if it doesn't we wouldn't even
        // see a performance increase from unmapping
        //
        // we manually flush the buffer ourselves so no need for GL_MAP_COHERENT_BIT
        //
        GLbitfield buffer_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;

        GLsizeiptr vbo_size = num_vertices * sizeof(vert3);

        gl->BindBuffer(GL_ARRAY_BUFFER, gl->vbo);
        gl->BufferStorage(GL_ARRAY_BUFFER, vbo_size, 0, buffer_flags);

        result->vertices.base  = gl->MapBufferRange(GL_ARRAY_BUFFER, 0, vbo_size, buffer_flags);
        result->vertices.count = 0;
        result->vertices.limit = num_vertices;

        gl->BindBuffer(GL_ARRAY_BUFFER, 0);

        GLsizeiptr ebo_size = num_indices * sizeof(u16);

        gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->ebo);
        gl->BufferStorage(GL_ELEMENT_ARRAY_BUFFER, ebo_size, 0, buffer_flags);

        result->indices.base  = gl->MapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, ebo_size, buffer_flags);
        result->indices.count = 0;
        result->indices.limit = num_indices;

        gl->BindBuffer(GL_ARRAY_BUFFER, 0);

        // setup uniform buffer
        //
        // :configure
        //
        void *ubo_base = 0;
        uptr  ubo_size = XI_MB(8);

        GLint alignment = 0;
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);

        // @todo: when we allow a configurable size for the uniform buffer, we will have
        // to align the size up to the offset alignment required by the driver
        //

        gl->GenBuffers(1, &gl->ubo);

        gl->BindBuffer(GL_UNIFORM_BUFFER, gl->ubo);
        gl->BufferStorage(GL_UNIFORM_BUFFER, ubo_size, 0, buffer_flags);

        ubo_base = gl->MapBufferRange(GL_UNIFORM_BUFFER, 0, ubo_size, buffer_flags);

        xi_arena_init_from(&result->uniforms, ubo_base, ubo_size);

        result->uniforms.default_alignment = alignment;
        result->uniforms.flags |= XI_ARENA_FLAG_STRICT_ALIGNMENT;

        gl->GenProgramPipelines(1, &gl->pipeline);

        gl->shader_count = 32; // :configure
        gl->shaders      = xi_arena_push_array(&gl->arena, GLuint, gl->shader_count);

        if (gl_base_shader_compile(gl)) {
            result->commands.limit = XI_MB(16); // :configure
            result->commands.used  = 0;
            result->commands.data  = xi_arena_push_size(&gl->arena, result->commands.limit);
        }
        else {
            // this will de-init the arena associated with the context
            //
            gl_os_context_delete(gl);
            result = 0;
        }
    }

    return result;
}

static b32 gl_shader_compile(xiOpenGLContext *gl, GLuint *handle, GLenum stage, string code) {
    b32 result = false;

    GLuint shader = gl->CreateShader(stage);
    if (shader) {
        GLint lengths[3] = { (GLint) shader_header.count, 0, (GLint) code.count };
        u8 *codes[3]     = { shader_header.data,  0, code.data  };

        if (stage == GL_VERTEX_SHADER) {
            lengths[1] = (GLint) vertex_defines.count;
            codes[1]   = vertex_defines.data;
        }
        else if (stage == GL_FRAGMENT_SHADER) {
            lengths[1] = (GLint) fragment_defines.count;
            codes[1]   = fragment_defines.data;
        }

        gl->ShaderSource(shader, XI_ARRAY_SIZE(codes), (const GLchar **) codes, lengths);
        gl->CompileShader(shader);

        GLint compiled = GL_FALSE;
        gl->GetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

        if (compiled) {
            GLuint program = gl->CreateProgram();

            gl->ProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);

            gl->AttachShader(program, shader);
            gl->LinkProgram(program);
            gl->DetachShader(program, shader);

            GLint linked = GL_FALSE;
            gl->GetProgramiv(program, GL_LINK_STATUS, &linked);

            if (linked) {
                *handle = program;
                result  = true;
            }
            else {
                char log[1024];
                gl->GetProgramInfoLog(program, sizeof(log), 0, log);

                // @todo: append to global log buffer
                //

                gl->DeleteProgram(program);
            }
        }
        else {
            char log[1024];
            gl->GetShaderInfoLog(shader, sizeof(log), 0, log);

            // @todo: append to global log buffer
            //
        }

        gl->DeleteShader(shader);
    }

    return result;
}

b32 gl_base_shader_compile(xiOpenGLContext *gl) {
    b32 result = false;

    string vertex_code =
        xi_str_wrap_const("void main() {"
                          "    gl_Position = v4(vertex_position, 1.0);"

                          "    fragment_uv     = vertex_uv;"
                          "    fragment_colour = vertex_colour;"
                          "}");

    if (gl_shader_compile(gl, &gl->base_vs, GL_VERTEX_SHADER, vertex_code)) {
        string fragment_code =
            xi_str_wrap_const("void main() {"
                              "    output_colour = fragment_colour;"
                              "}");

        result = gl_shader_compile(gl, &gl->base_fs, GL_FRAGMENT_SHADER, fragment_code);
    }

    return result;
}

static void xi_opengl_submit(xiOpenGLContext *gl) {
    xiRenderer *renderer = &gl->renderer;

    glViewport(0, 0, renderer->setup.window_dim.w, renderer->setup.window_dim.h);

    // setup buffers
    //
    gl->BindVertexArray(gl->vao);

    gl->BindBuffer(GL_ARRAY_BUFFER, gl->vbo);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->ebo);

    gl->VertexAttribPointer(0, 3, GL_FLOAT,         GL_FALSE, sizeof(vert3), (void *) XI_OFFSET_OF(vert3, p));
    gl->VertexAttribPointer(1, 2, GL_FLOAT,         GL_FALSE, sizeof(vert3), (void *) XI_OFFSET_OF(vert3, uv));
    gl->VertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(vert3), (void *) XI_OFFSET_OF(vert3, c));

    gl->EnableVertexAttribArray(0);
    gl->EnableVertexAttribArray(1);
    gl->EnableVertexAttribArray(2);

    // setup shader
    //
    gl->UseProgramStages(gl->pipeline, GL_VERTEX_SHADER_BIT,   gl->base_vs);
    gl->UseProgramStages(gl->pipeline, GL_FRAGMENT_SHADER_BIT, gl->base_fs);

    gl->BindProgramPipeline(gl->pipeline);

    buffer *commands = &renderer->commands;
    for (uptr offset = 0; offset < commands->used;) {
        u32 type = *(u32 *) (commands->data + offset);
        offset += sizeof(u32);

        switch (type) {
            case XI_RENDER_COMMAND_xiRenderCommandDraw: {
                xiRenderCommandDraw *draw = (xiRenderCommandDraw *) (commands->data + offset);
                offset += sizeof(xiRenderCommandDraw);

                void *index_offset = (void *) (draw->index_offset * sizeof(u16));
                u32   index_count  = draw->index_count;

                gl->DrawElementsBaseVertex(GL_TRIANGLES, index_count,
                        GL_UNSIGNED_SHORT, index_offset, draw->vertex_offset);
            }
            break;
        }
    }

    // reset immediate mode buffers for next frame
    //
    renderer->commands.used  = 0;
    renderer->vertices.count = 0;
    renderer->indices.count  = 0;

    xi_arena_reset(&renderer->uniforms);

    renderer->draw_call = 0;
}

#if XI_OS_WIN32
    #include "os/wgl.c"
#endif
