#if !defined(XI_OPENGL_H_)
#define XI_OPENGL_H_

#include <xi/xi.h>

#if XI_OS_WIN32
    #define WIN32_LEAN_AND_MEAN 1
    #include <windows.h>

    #include <gl/gl.h>
#endif

typedef uintptr_t GLsizeiptr;
typedef intptr_t  GLintptr;
typedef char      GLchar;

// any defines from glcorearb.h we need
//
#define GL_MAJOR_VERSION                   0x821B
#define GL_MINOR_VERSION                   0x821C
#define GL_FRAMEBUFFER_SRGB                0x8DB9
#define GL_SRGB8_ALPHA8                    0x8C43
#define GL_ARRAY_BUFFER                    0x8892
#define GL_ELEMENT_ARRAY_BUFFER            0x8893
#define GL_MAP_WRITE_BIT                   0x0002
#define GL_MAP_PERSISTENT_BIT              0x0040
#define GL_UNIFORM_BUFFER                  0x8A11
#define GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT 0x8A34
#define GL_FRAGMENT_SHADER                 0x8B30
#define GL_VERTEX_SHADER                   0x8B31
#define GL_COMPILE_STATUS                  0x8B81
#define GL_LINK_STATUS                     0x8B82
#define GL_PROGRAM_SEPARABLE               0x8258
#define GL_VERTEX_SHADER_BIT               0x00000001
#define GL_FRAGMENT_SHADER_BIT             0x00000002

// any function typedefs for extension functions to load dynamically
//
typedef void xiOpenGL_glGenVertexArrays(GLsizei, GLuint *);
typedef void xiOpenGL_glGenBuffers(GLsizei, GLuint *);
typedef void xiOpenGL_glBindBuffer(GLenum, GLuint);
typedef void xiOpenGL_glBufferStorage(GLenum, GLsizeiptr, const void *, GLbitfield);
typedef void xiOpenGL_glGenProgramPipelines(GLsizei, GLuint *);
typedef void xiOpenGL_glShaderSource(GLuint, GLsizei, const GLchar **, const GLint *);
typedef void xiOpenGL_glCompileShader(GLuint);
typedef void xiOpenGL_glGetShaderiv(GLuint, GLenum, GLint *);
typedef void xiOpenGL_glProgramParameteri(GLuint, GLenum, GLint);
typedef void xiOpenGL_glAttachShader(GLuint, GLuint);
typedef void xiOpenGL_glLinkProgram(GLuint);
typedef void xiOpenGL_glDetachShader(GLuint, GLuint);
typedef void xiOpenGL_glGetProgramiv(GLuint, GLenum, GLint *);
typedef void xiOpenGL_glGetProgramInfoLog(GLuint, GLsizei, GLsizei *, GLchar *);
typedef void xiOpenGL_glDeleteProgram(GLuint);
typedef void xiOpenGL_glGetShaderInfoLog(GLuint, GLsizei, GLsizei *, GLchar *);
typedef void xiOpenGL_glDeleteShader(GLuint);
typedef void xiOpenGL_glBindVertexArray(GLuint);
typedef void xiOpenGL_glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
typedef void xiOpenGL_glEnableVertexAttribArray(GLuint);
typedef void xiOpenGL_glUseProgramStages(GLuint, GLbitfield, GLuint);
typedef void xiOpenGL_glBindProgramPipeline(GLuint);
typedef void xiOpenGL_glDrawElementsBaseVertex(GLenum, GLsizei, GLenum, void *, GLint);

typedef void *xiOpenGL_glMapBufferRange(GLenum, GLintptr, GLsizeiptr, GLbitfield);

typedef GLuint xiOpenGL_glCreateShader(GLenum);
typedef GLuint xiOpenGL_glCreateProgram(void);

#define GL_FUNCTION_POINTER(name) xiOpenGL_gl##name *name

typedef struct xiOpenGLContext {
    xiRenderer renderer;

    struct {
        b32 srgb;
        b32 multisample;

        GLint major_version;
        GLint minor_version;
    } info;

    xiArena arena;

    GLenum texture_format;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    GLuint ubo;

    u32 shader_count; // a shader contains a single stage
    GLuint *shaders;

    GLuint base_vs;
    GLuint base_fs;

    GLuint pipeline;

    GL_FUNCTION_POINTER(GenVertexArrays);
    GL_FUNCTION_POINTER(GenBuffers);
    GL_FUNCTION_POINTER(BindBuffer);
    GL_FUNCTION_POINTER(BufferStorage);
    GL_FUNCTION_POINTER(MapBufferRange);
    GL_FUNCTION_POINTER(GenProgramPipelines);
    GL_FUNCTION_POINTER(CreateShader);
    GL_FUNCTION_POINTER(ShaderSource);
    GL_FUNCTION_POINTER(CompileShader);
    GL_FUNCTION_POINTER(GetShaderiv);
    GL_FUNCTION_POINTER(CreateProgram);
    GL_FUNCTION_POINTER(ProgramParameteri);
    GL_FUNCTION_POINTER(AttachShader);
    GL_FUNCTION_POINTER(LinkProgram);
    GL_FUNCTION_POINTER(DetachShader);
    GL_FUNCTION_POINTER(GetProgramiv);
    GL_FUNCTION_POINTER(GetProgramInfoLog);
    GL_FUNCTION_POINTER(DeleteProgram);
    GL_FUNCTION_POINTER(GetShaderInfoLog);
    GL_FUNCTION_POINTER(DeleteShader);
    GL_FUNCTION_POINTER(BindVertexArray);
    GL_FUNCTION_POINTER(VertexAttribPointer);
    GL_FUNCTION_POINTER(EnableVertexAttribArray);
    GL_FUNCTION_POINTER(UseProgramStages);
    GL_FUNCTION_POINTER(BindProgramPipeline);
    GL_FUNCTION_POINTER(DrawElementsBaseVertex);
} xiOpenGLContext;

#undef GL_FUNCTION_POINTER

// base shader code
//
// @todo: this messes with the define for xi_str_wrap_const and makes it kind of a pain to use elsewhere
// in c so this should be moved to the general compile function
//
static const string shader_header =
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

static const string vertex_defines =
                  xi_str_wrap_const("layout(location = 0) in v3 vertex_position;"
                                    "layout(location = 1) in v2 vertex_uv;"
                                    "layout(location = 2) in v4 vertex_colour;"

                                    // this has to be re-declared if you are using GL_PROGRAM_SEPARABLE
                                    //
                                    "out gl_PerVertex {"
                                    "    vec4  gl_Position;"
                                    "    float gl_PointSize;"
                                    "    float gl_ClipDistance[];"
                                    "};"

                                    "layout(location = 0) out v2 fragment_uv;"
                                    "layout(location = 1) out v4 fragment_colour;");

static const string fragment_defines =
                  xi_str_wrap_const("layout(location = 0) in v2 fragment_uv;"
                                    "layout(location = 1) in v4 fragment_colour;"

                                    "layout(location = 0) out v4 output_colour;"

                                    "layout(binding = 1) uniform sampler2D image;");

// os specific calls
//
static xiOpenGLContext *gl_os_context_create(void *platform);
static void gl_os_context_delete(xiOpenGLContext *gl);

// generic opengl calls
//
static b32 gl_base_shader_compile(xiOpenGLContext *gl);

#endif  // XI_OPENGL_H_
