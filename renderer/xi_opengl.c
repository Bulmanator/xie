#include "xi_opengl.h"

void GL_DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei len, const char *message, const void *user_data) {
    (void) source;
    (void) type;
    (void) id;
    (void) len;
    (void) message;
    (void) user_data;

    // @todo: do something more substantial with this!
    //

    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        // Assert(false && "opengl error!");
        //
        // Sharing screens via discord triggers the above assert because it decides to insert its
        // own compatability layer code and that is classed as a HIGH error when using OpenGL core
        //
    }
}

ExportFunc RENDERER_INIT(GL_Init) {
    B32 result = false;

    GL_Context *gl = GL_ContextCreate(renderer, platform);
    if (gl) {
        result = true;

        renderer->backend = gl;

        gl->DebugMessageCallback(GL_DebugCallback, gl);

        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // :speed real bad for release

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        glDisable(GL_DEPTH_TEST);
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

        U32 num_vertices = renderer->vertices.limit;
        U32 num_indices  = renderer->indices.limit;

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

        GLsizeiptr vbo_size = num_vertices * sizeof(Vertex3);

        gl->BindBuffer(GL_ARRAY_BUFFER, gl->vbo);
        gl->BufferStorage(GL_ARRAY_BUFFER, vbo_size, 0, buffer_flags);

        renderer->vertices.base  = gl->MapBufferRange(GL_ARRAY_BUFFER, 0, vbo_size, buffer_flags | GL_MAP_FLUSH_EXPLICIT_BIT);
        renderer->vertices.count = 0;
        renderer->vertices.limit = num_vertices;

        gl->BindBuffer(GL_ARRAY_BUFFER, 0);

        GLsizeiptr ebo_size = num_indices * sizeof(U16);

        gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->ebo);
        gl->BufferStorage(GL_ELEMENT_ARRAY_BUFFER, ebo_size, 0, buffer_flags);

        renderer->indices.base  = gl->MapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, ebo_size, buffer_flags | GL_MAP_FLUSH_EXPLICIT_BIT);
        renderer->indices.count = 0;
        renderer->indices.limit = num_indices;

        gl->BindBuffer(GL_ARRAY_BUFFER, 0);

        // setup uniform buffer
        //
        void *ubo_base = 0;
        U64   ubo_size = renderer->uniforms.limit;

        if (ubo_size == 0) { ubo_size = MB(16); }

        GLint alignment = 0;
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);

        // @todo: when we allow a configurable size for the uniform buffer, we will have
        // to align the size up to the offset alignment required by the driver
        //

        gl->GenBuffers(1, &gl->ubo);

        gl->BindBuffer(GL_UNIFORM_BUFFER, gl->ubo);
        gl->BufferStorage(GL_UNIFORM_BUFFER, ubo_size, 0, buffer_flags);

        ubo_base = gl->MapBufferRange(GL_UNIFORM_BUFFER, 0, ubo_size, buffer_flags | GL_MAP_FLUSH_EXPLICIT_BIT);

        renderer->ubo_globals_size = AlignUp(sizeof(ShaderGlobals), alignment);
        renderer->uniforms.used    = 0;
        renderer->uniforms.data    = cast(U8 *) ubo_base;
        renderer->uniforms.limit   = ubo_size;

        gl->GenProgramPipelines(1, &gl->pipeline);

        gl->shader_count = 32; // :configure
        gl->shaders      = M_ArenaPush(gl->arena, GLuint, gl->shader_count);

        if (GL_BaseShaderCompile(gl)) {
            if (renderer->command_buffer.limit == 0) {
                renderer->command_buffer.limit = MB(8);
            }

            renderer->command_buffer.used = 0;
            renderer->command_buffer.data = M_ArenaPush(gl->arena, U8, renderer->command_buffer.limit);
        }
        else {
            // this will de-init the arena associated with the context
            //
            GL_ContextDelete(gl);
            result = false;

            return result;
        }

        U32 level_count = 1;
        U32 dimension   = renderer->sprite_array.dimension;

        if (dimension == 0) {
            renderer->sprite_array.dimension = 256;
            level_count = 9;
        }
        else {
            // @todo: replace with clz or something...
            //
            while (dimension > 1) {
                dimension  >>= 1;
                level_count += 1;
            }
        }

        U32 width  = renderer->sprite_array.dimension;
        U32 height = width;

        U32 count = renderer->sprite_array.limit;
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

        if (renderer->texture_limit == 0) {
            // some arbitary number of textures
            //
            renderer->texture_limit = 128;
        }

        gl->textures = M_ArenaPush(gl->arena, GLuint, renderer->texture_limit);
        glGenTextures(renderer->texture_limit, gl->textures);

        // setup texture transfer queue
        //
        RendererTransferQueue *transfer_queue = &renderer->transfer_queue;

        if (transfer_queue->task_limit == 0) {
            transfer_queue->task_limit= 256;
        }
        else {
            transfer_queue->task_limit = U32_Pow2Next(transfer_queue->task_limit);
        }

        transfer_queue->tasks = M_ArenaPush(gl->arena, RendererTransferTask, transfer_queue->task_limit);

        if (transfer_queue->limit == 0) {
            transfer_queue->limit = MB(256);
        }

        gl->GenBuffers(1, &gl->transfer_buffer);
        gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, gl->transfer_buffer);

        gl->BufferStorage(GL_PIXEL_UNPACK_BUFFER, transfer_queue->limit, 0, buffer_flags);
        transfer_queue->base = gl->MapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, transfer_queue->limit, buffer_flags | GL_MAP_FLUSH_EXPLICIT_BIT);

        if (renderer->layer_offset == 0.0f) {
            renderer->layer_offset = 1.0f;
        }
    }

    return result;
}

FileScope B32 GL_ShaderCompile(GL_Context *gl, GLuint *handle, GLenum stage, Str8 code) {
    B32 result = false;

    const Str8 shader_header =
                  Str8_Literal("#version 440 core\n"
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
                               "    m4x4  transform;"
                               "    v4    camera_position;"
                               "    f32   time;"
                               "    f32   dt;"
                               "    uvec2 window_dim;"
                               "};");

    const Str8 vertex_defines =
                  Str8_Literal("layout(location = 0) in v3 vertex_position;"
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

    const Str8 fragment_defines =
                  Str8_Literal("layout(location = 0) in v3 fragment_uv;"
                               "layout(location = 1) in v4 fragment_colour;"

                               "layout(location = 0) out v4 output_colour;"

                               "layout(binding = 0) uniform sampler2DArray image;");


    GLuint shader = gl->CreateShader(stage);
    if (shader) {
        GLint lengths[3] = { (GLint) shader_header.count, 0, (GLint) code.count };
        U8    *codes[3]  = {         shader_header.data,  0,         code.data  };

        if (stage == GL_VERTEX_SHADER) {
            lengths[1] = (GLint) vertex_defines.count;
              codes[1] =         vertex_defines.data;
        }
        else if (stage == GL_FRAGMENT_SHADER) {
            lengths[1] = (GLint) fragment_defines.count;
              codes[1] =         fragment_defines.data;
        }

        gl->ShaderSource(shader, ArraySize(codes), cast(const GLchar **) codes, lengths);
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

B32 GL_BaseShaderCompile(GL_Context *gl) {
    B32 result = false;

    Str8 vertex_code =
        Str8_Literal("void main() {"
                     "    gl_Position = transform * v4(vertex_position, 1.0);"

                     "    fragment_uv     = vertex_uv;"
                     "    fragment_colour = vertex_colour;"
                     "}");

    if (GL_ShaderCompile(gl, &gl->base_vs, GL_VERTEX_SHADER, vertex_code)) {
        Str8 fragment_code =
            Str8_Literal("void main() {"
                         "    output_colour = texture(image, fragment_uv) * fragment_colour;"
                         "}");

        result = GL_ShaderCompile(gl, &gl->base_fs, GL_FRAGMENT_SHADER, fragment_code);
    }

    return result;
}

FileScope void GL_TexturesUpload(GL_Context *gl, RendererContext *renderer) {
    gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, gl->transfer_buffer);

    RendererTransferQueue *transfer_queue = &renderer->transfer_queue;

    while (transfer_queue->task_count != 0) {
        RendererTransferTask *task = &transfer_queue->tasks[transfer_queue->first_task];
        if (task->state == RENDERER_TRANSFER_TASK_STATE_LOADED) {
            gl->FlushMappedBufferRange(GL_PIXEL_UNPACK_BUFFER, task->offset, task->size);

            // we can upload
            //
            GLsizei width  = task->texture.width;
            GLsizei height = task->texture.height;

            GLsizei index  = task->texture.index;

            if (RendererTextureIsSprite(renderer, task->texture)) {
                glBindTexture(GL_TEXTURE_2D_ARRAY, gl->sprite_array);

                // base layer
                //
                gl->TexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index,
                        width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, cast(void *) task->offset);

                // mipmaps
                //
                GLint level = 1;

                U64 offset = task->offset + (4 * width * height);
                while (width > 1 || height > 1) {
                    width  >>= 1;
                    height >>= 1;

                    if (width  == 0) { width  = 1; }
                    if (height == 0) { height = 1; }

                    gl->TexSubImage3D(GL_TEXTURE_2D_ARRAY, level, 0, 0, index,
                            width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, cast(void *) offset);

                    offset += (4 * width * height);
                    level  += 1;
                }

                Assert((task->offset + task->size) == offset);
            }
            else {
                Assert(index < cast(GLsizei) renderer->texture_limit);

                // just a straight texture copy as these are larger non-sprite textures they
                // do not have mip-mapping
                //
                // This is hacked together, tried to add mip-mapping to non-sprite textures. this should
                // be removed
                //
                glBindTexture(GL_TEXTURE_2D_ARRAY, gl->textures[index]);

                GLuint level_count = 1;

#if 0
                GLsizei mipw = width;
                GLsizei miph = height;

                while (mipw > 1 || miph > 1) {
                    mipw >>= 1;
                    miph >>= 1;

                    level_count += 1;
                }
#endif

                gl->TexStorage3D(GL_TEXTURE_2D_ARRAY, level_count, gl->texture_format, width, height, 1);

                gl->TexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1,
                        GL_RGBA, GL_UNSIGNED_BYTE, cast(void *) task->offset);

#if 0
                GLint level = 1;

                U64 offset = task->offset + (4 * width * height);
                while (width > 1  || height > 1) {
                    width  >>= 1;
                    height >>= 1;

                    if (width  == 0) { width  = 1; }
                    if (height == 0) { height = 1; }

                    gl->TexSubImage3D(GL_TEXTURE_2D_ARRAY, level, 0, 0, 0, width, height, 1,
                            GL_RGBA, GL_UNSIGNED_BYTE, cast(void *) offset);

                    offset += (4 * width * height);
                    level  += 1;
                }

                Assert(offset == (task->offset + task->size));
#endif

                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
            }
        }
        else if (task->state == RENDERER_TRANSFER_TASK_STATE_PENDING) {
            // we encountered one that has been queued but hasn't finished loading from disk
            // yet so break here and it will continue next frame
            //
            break;
        }

        transfer_queue->first_task += 1;
        transfer_queue->first_task &= (transfer_queue->task_limit - 1);

        transfer_queue->task_count -= 1;

        transfer_queue->read_offset = task->offset;
    }

    gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

FileScope void GL_Submit(RendererContext *renderer) {
    GL_Context *gl = renderer->backend;

    GL_TexturesUpload(gl, renderer);

    glViewport(0, 0, renderer->setup.window_dim.w, renderer->setup.window_dim.h);

    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // setup buffers
    //
    gl->BindVertexArray(gl->vao);

    gl->BindBuffer(GL_ARRAY_BUFFER,         gl->vbo);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->ebo);
    gl->BindBuffer(GL_UNIFORM_BUFFER,       gl->ubo);

    gl->VertexAttribPointer(0, 3, GL_FLOAT,         GL_FALSE, sizeof(Vertex3), cast(void *) OffsetTo(Vertex3, p));
    gl->VertexAttribPointer(1, 3, GL_FLOAT,         GL_FALSE, sizeof(Vertex3), cast(void *) OffsetTo(Vertex3, uv));
    gl->VertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Vertex3), cast(void *) OffsetTo(Vertex3, c));

    gl->EnableVertexAttribArray(0);
    gl->EnableVertexAttribArray(1);
    gl->EnableVertexAttribArray(2);

    // setup shader
    //
    gl->UseProgramStages(gl->pipeline, GL_VERTEX_SHADER_BIT,   gl->base_vs);
    gl->UseProgramStages(gl->pipeline, GL_FRAGMENT_SHADER_BIT, gl->base_fs);

    gl->BindProgramPipeline(gl->pipeline);

    gl->FlushMappedBufferRange(GL_ARRAY_BUFFER,         0, renderer->vertices.count * sizeof(Vertex3));
    gl->FlushMappedBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, renderer->indices.count  * sizeof(U16));
    gl->FlushMappedBufferRange(GL_UNIFORM_BUFFER,       0, renderer->uniforms.used);

    gl->BindBuffer(GL_UNIFORM_BUFFER, 0);

    gl->ActiveTexture(GL_TEXTURE0);

    U64 globals_size = renderer->ubo_globals_size;

    Buffer *command_buffer = &renderer->command_buffer;
    for (S64 offset = 0; offset < command_buffer->used;) {
        RenderCommandType type = *cast(RenderCommandType *) (command_buffer->data + offset);
        offset += sizeof(RenderCommandType);

        switch (type) {
            case RENDER_COMMAND_RenderCommandDraw: {
                RenderCommandDraw *draw = cast(RenderCommandDraw *) (command_buffer->data + offset);
                offset += sizeof(RenderCommandDraw);

                if (RendererTextureIsSprite(renderer, draw->texture)) {
                    glBindTexture(GL_TEXTURE_2D_ARRAY, gl->sprite_array);
                }
                else {
                    glBindTexture(GL_TEXTURE_2D_ARRAY, gl->textures[draw->texture.index]);
                }

                gl->BindBufferRange(GL_UNIFORM_BUFFER, 0, gl->ubo, draw->ubo_offset, globals_size);

                void *index_offset = cast(void *) (draw->index_offset * sizeof(U16));
                U32   index_count  = draw->index_count;

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
    renderer->uniforms.used       = 0;

    renderer->layer     = 0;
    renderer->draw_call = 0;
}

#if OS_WIN32
    #include "os/wgl.c"
#elif OS_LINUX
    #include "os/sdl2_gl.c"
#endif
