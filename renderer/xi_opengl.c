#include "xi_opengl.h"

void xi_gl_debug_proc(GLenum source, GLenum type, GLuint id, GLenum severity,
        GLsizei len, const char *message, const void *user_data)
{
    (void) source;
    (void) type;
    (void) id;
    (void) len;
    (void) message;
    (void) user_data;

    OutputDebugString(message);
    OutputDebugString("\n");

    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        XI_ASSERT(false && "opengl error!");
    }
}

extern XI_EXPORT XI_RENDERER_INIT(xi_opengl_init) {
    xi_b32 result = false;

    xiOpenGLContext *gl = gl_os_context_create(renderer, platform);
    if (gl) {
        result = true;

        renderer->backend = gl;

        gl->DebugMessageCallback(xi_gl_debug_proc, gl);

        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

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

        xi_u32 num_vertices = renderer->vertices.limit;
        xi_u32 num_indices  = renderer->indices.limit;

        // you can "misalign" the number of vertices/indices to the number of quads meaning you
        // will run out of one before the other, but thats up to the user
        //
        if (num_vertices == 0) { num_vertices = 65536; } // 16k quads * 4 vertices
        if (num_indices  == 0) { num_indices  = 98304; } // 16k quads * 6 indices

        // @todo: maybe we should clamp the number of vertices/indices
        //

        // we just use persistently mapped buffers for the immeidate mode quad buffer
        // as it is easier to manage. i don't know if the opengl driver will copy
        // to gpu local memory when unmapping a buffer, if it doesn't we wouldn't even
        // see a performance increase from unmapping
        //
        // we manually flush the buffer ourselves so no need for GL_MAP_COHERENT_BIT
        //
        GLbitfield buffer_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;

        GLsizeiptr vbo_size = num_vertices * sizeof(xi_vert3);

        gl->BindBuffer(GL_ARRAY_BUFFER, gl->vbo);
        gl->BufferStorage(GL_ARRAY_BUFFER, vbo_size, 0, buffer_flags);

        renderer->vertices.base  = gl->MapBufferRange(GL_ARRAY_BUFFER, 0, vbo_size, buffer_flags | GL_MAP_FLUSH_EXPLICIT_BIT);
        renderer->vertices.count = 0;
        renderer->vertices.limit = num_vertices;

        gl->BindBuffer(GL_ARRAY_BUFFER, 0);

        GLsizeiptr ebo_size = num_indices * sizeof(xi_u16);

        gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->ebo);
        gl->BufferStorage(GL_ELEMENT_ARRAY_BUFFER, ebo_size, 0, buffer_flags);

        renderer->indices.base  = gl->MapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, ebo_size, buffer_flags | GL_MAP_FLUSH_EXPLICIT_BIT);
        renderer->indices.count = 0;
        renderer->indices.limit = num_indices;

        gl->BindBuffer(GL_ARRAY_BUFFER, 0);

        // setup uniform buffer
        //
        void *ubo_base = 0;
        xi_uptr ubo_size = renderer->uniforms.size;
        if (ubo_size == 0) { ubo_size = XI_MB(16); }

        GLint alignment = 0;
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);

        // @todo: when we allow a configurable size for the uniform buffer, we will have
        // to align the size up to the offset alignment required by the driver
        //

        gl->GenBuffers(1, &gl->ubo);

        gl->BindBuffer(GL_UNIFORM_BUFFER, gl->ubo);
        gl->BufferStorage(GL_UNIFORM_BUFFER, ubo_size, 0, buffer_flags);

        ubo_base = gl->MapBufferRange(GL_UNIFORM_BUFFER, 0, ubo_size, buffer_flags | GL_MAP_FLUSH_EXPLICIT_BIT);

        xi_arena_init_from(&renderer->uniforms, ubo_base, ubo_size);

        renderer->uniforms.default_alignment = alignment;
        renderer->uniforms.flags |= XI_ARENA_FLAG_STRICT_ALIGNMENT;

        gl->GenProgramPipelines(1, &gl->pipeline);

        gl->shader_count = 32; // :configure
        gl->shaders      = xi_arena_push_array(&gl->arena, GLuint, gl->shader_count);

        if (gl_base_shader_compile(gl)) {
            if (renderer->command_buffer.limit == 0) {
                renderer->command_buffer.limit = XI_MB(8);
            }

            renderer->command_buffer.used = 0;
            renderer->command_buffer.data = xi_arena_push_size(&gl->arena, renderer->command_buffer.limit);
        }
        else {
            // this will de-init the arena associated with the context
            //
            gl_os_context_delete(gl);
            result = false;
        }

        xi_u32 level_count = 1;
        xi_u32 dimension = renderer->sprite_array.dimension;
        if (dimension == 0) {
            renderer->sprite_array.dimension = 256;
            level_count = 9;
        }
        else {
            // @todo: replace with clz or something...
            //
            while (dimension > 1) {
                dimension >>= 1;
                level_count += 1;
            }
        }

        xi_u32 width  = renderer->sprite_array.dimension;
        xi_u32 height = width;

        xi_u32 count = renderer->sprite_array.limit;
        if (count == 0) {
            count = renderer->sprite_array.limit = 512;
        }

        // setup sprite array
        //
        glGenTextures(1, &gl->sprite_array);
        glBindTexture(GL_TEXTURE_2D_ARRAY, gl->sprite_array);

        gl->TexStorage3D(GL_TEXTURE_2D_ARRAY, level_count, gl->texture_format, width, height, count);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

        // setup texture transfer queue
        //
        xiRendererTransferQueue *transfer_queue = &renderer->transfer_queue;

        if (transfer_queue->max_tasks == 0) {
            transfer_queue->max_tasks = 256;
        }
        else {
            transfer_queue->max_tasks = xi_pow2_next_u32(transfer_queue->max_tasks);
        }

        transfer_queue->tasks = xi_arena_push_array(&gl->arena, xiRendererTransferTask, transfer_queue->max_tasks);

        if (transfer_queue->limit == 0) {
            transfer_queue->limit = XI_MB(256);
        }

        gl->GenBuffers(1, &gl->transfer_buffer);
        gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, gl->transfer_buffer);

        gl->BufferStorage(GL_PIXEL_UNPACK_BUFFER, transfer_queue->limit, 0, buffer_flags);

        transfer_queue->base = gl->MapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0,
            transfer_queue->limit, buffer_flags | GL_MAP_FLUSH_EXPLICIT_BIT);
    }

    return result;
}

XI_INTERNAL xi_b32 gl_shader_compile(xiOpenGLContext *gl, GLuint *handle, GLenum stage, xi_string code) {
    xi_b32 result = false;

    const xi_string shader_header =
                  xi_str_wrap_const("#version 440 core\n"
                                    "#define f32 float\n"
                                    "#define f64 double\n"

                                    "#define v2u uvec2\n"
                                    "#define v2s ivec2\n"

                                    "#define v2 vec2\n"
                                    "#define v3 vec3\n"
                                    "#define v4 vec4\n"

                                    "#define m2x2 mat2\n"
                                    "#define m4x4 mat4\n"

                                   "layout(row_major, binding = 0) uniform xiGlobals {"
                                   "    m4x4 transform;"
                                   "    v4   camera_position;"
                                   "    f32  time;"
                                   "    f32  dt;"
                                   "    f32  unused0;"
                                   "    f32  unused1;"
                                   "};");

    const xi_string vertex_defines =
                  xi_str_wrap_const("layout(location = 0) in v3 vertex_position;"
                                    "layout(location = 1) in v3 vertex_uv;"
                                    "layout(location = 2) in v4 vertex_colour;"

                                    // this has to be re-declared if you are using GL_PROGRAM_SEPARABLE
                                    //
                                    "out gl_PerVertex {"
                                    "    vec4  gl_Position;"
                                    "    float gl_PointSize;"
                                    "    float gl_ClipDistance[];"
                                    "};"

                                    "layout(location = 0) out v3 fragment_uv;"
                                    "layout(location = 1) out v4 fragment_colour;");

    const xi_string fragment_defines =
                  xi_str_wrap_const("layout(location = 0) in v3 fragment_uv;"
                                    "layout(location = 1) in v4 fragment_colour;"

                                    "layout(location = 0) out v4 output_colour;"

                                    "layout(binding = 0) uniform sampler2DArray image;");


    GLuint shader = gl->CreateShader(stage);
    if (shader) {
        GLint lengths[3] = { (GLint) shader_header.count, 0, (GLint) code.count };
        xi_u8 *codes[3]  = { shader_header.data,  0, code.data  };

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

xi_b32 gl_base_shader_compile(xiOpenGLContext *gl) {
    xi_b32 result = false;

    xi_string vertex_code =
        xi_str_wrap_const("void main() {"
                          "    gl_Position = transform * v4(vertex_position, 1.0);"

                          "    fragment_uv     = vertex_uv;"
                          "    fragment_colour = vertex_colour;"
                          "}");

    if (gl_shader_compile(gl, &gl->base_vs, GL_VERTEX_SHADER, vertex_code)) {
        xi_string fragment_code =
            xi_str_wrap_const("void main() {"
                              "    output_colour = texture(image, fragment_uv) * fragment_colour;"
                              "}");

        result = gl_shader_compile(gl, &gl->base_fs, GL_FRAGMENT_SHADER, fragment_code);
    }

    return result;
}

XI_INTERNAL void gl_textures_upload(xiOpenGLContext *gl, xiRenderer *renderer) {
    gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, gl->transfer_buffer);

    xiRendererTransferQueue *transfer_queue = &renderer->transfer_queue;

    while (transfer_queue->task_count != 0) {
        xiRendererTransferTask *task = &transfer_queue->tasks[transfer_queue->first_task];
        if (task->state == XI_RENDERER_TRANSFER_TASK_STATE_LOADED) {
            gl->FlushMappedBufferRange(GL_PIXEL_UNPACK_BUFFER, task->offset, task->size);

            // we can upload
            //
            GLsizei width  = task->texture.width;
            GLsizei height = task->texture.height;

            GLsizei index  = task->texture.index;

            if (xi_renderer_texture_is_sprite(renderer, task->texture)) {
                glBindTexture(GL_TEXTURE_2D_ARRAY, gl->sprite_array);

                // base layer
                //
                gl->TexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index,
                        width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void *) task->offset);

                // mipmaps
                //
                GLint level = 1;
                xi_uptr offset = task->offset + (4 * width * height);
                while (width > 1 || height > 1) {
                    width  >>= 1;
                    height >>= 1;

                    if (width  == 0) { width  = 1; }
                    if (height == 0) { height = 1; }

                    gl->TexSubImage3D(GL_TEXTURE_2D_ARRAY, level, 0, 0, index,
                            width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void *) offset);

                    offset += (4 * width * height);
                    level  += 1;
                }

                XI_ASSERT((task->offset + task->size) == offset);
            }
            else {
                // @incomplete: bind the texture handle from the non-sprite set
                //
                index = 0;
            }
        }
        else if (task->state == XI_RENDERER_TRANSFER_TASK_STATE_PENDING) {
            // we encountered one that has been queued but hasn't finished loading from disk
            // yet so break here and it will continue next frame
            //
            break;
        }

        transfer_queue->first_task += 1;
        transfer_queue->first_task &= (transfer_queue->max_tasks - 1);

        transfer_queue->task_count -= 1;

        transfer_queue->read_offset = task->offset;
    }

    gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

XI_INTERNAL void xi_opengl_submit(xiRenderer *renderer) {
    xiOpenGLContext *gl = renderer->backend;

    gl_textures_upload(gl, renderer);

    glViewport(0, 0, renderer->setup.window_dim.w, renderer->setup.window_dim.h);

    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    // setup buffers
    //
    gl->BindVertexArray(gl->vao);

    gl->BindBuffer(GL_ARRAY_BUFFER, gl->vbo);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->ebo);

    gl->VertexAttribPointer(0, 3, GL_FLOAT,         GL_FALSE, sizeof(xi_vert3), (void *) XI_OFFSET_OF(xi_vert3, p));
    gl->VertexAttribPointer(1, 3, GL_FLOAT,         GL_FALSE, sizeof(xi_vert3), (void *) XI_OFFSET_OF(xi_vert3, uv));
    gl->VertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(xi_vert3), (void *) XI_OFFSET_OF(xi_vert3, c));

    gl->EnableVertexAttribArray(0);
    gl->EnableVertexAttribArray(1);
    gl->EnableVertexAttribArray(2);

    // setup shader
    //
    gl->UseProgramStages(gl->pipeline, GL_VERTEX_SHADER_BIT,   gl->base_vs);
    gl->UseProgramStages(gl->pipeline, GL_FRAGMENT_SHADER_BIT, gl->base_fs);

    gl->BindProgramPipeline(gl->pipeline);

    gl->FlushMappedBufferRange(GL_ARRAY_BUFFER, 0, renderer->vertices.count * sizeof(xi_vert3));
    gl->FlushMappedBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, renderer->indices.count * sizeof(xi_u16));

    //
    // @todo: flush ubo and texture buffers
    //

    gl->ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, gl->sprite_array);

    xi_uptr globals_size = XI_ALIGN_UP(sizeof(xiShaderGlobals), renderer->uniforms.default_alignment);

    xi_buffer *command_buffer = &renderer->command_buffer;
    for (xi_uptr offset = 0; offset < command_buffer->used;) {
        xi_u32 type = *(xi_u32 *) (command_buffer->data + offset);
        offset += sizeof(xi_u32);

        switch (type) {
            case XI_RENDER_COMMAND_xiRenderCommandDraw: {
                xiRenderCommandDraw *draw = (xiRenderCommandDraw *) (command_buffer->data + offset);
                offset += sizeof(xiRenderCommandDraw);

                gl->BindBufferRange(GL_UNIFORM_BUFFER, 0, gl->ubo, draw->ubo_offset, globals_size);

                void *index_offset = (void *) (draw->index_offset * sizeof(xi_u16));
                xi_u32 index_count = draw->index_count;

                gl->DrawElementsBaseVertex(GL_TRIANGLES, index_count,
                        GL_UNSIGNED_SHORT, index_offset, draw->vertex_offset);
            }
            break;
        }
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    // reset immediate mode buffers for next frame
    //
    renderer->vertices.count = 0;
    renderer->indices.count  = 0;

    renderer->command_buffer.used = 0;

    xi_arena_reset(&renderer->uniforms);

    renderer->draw_call = 0;
}

#if XI_OS_WIN32
    #include "os/wgl.c"
#endif
