#if !defined(XI_OPENGL_H_)
#define XI_OPENGL_H_

#define RENDERER_BACKEND 1
typedef struct GL_Context RendererBackend;

#include <xi/xi.h>

#if OS_WIN32
    #define WIN32_LEAN_AND_MEAN 1
    #include <windows.h>

    #include <gl/gl.h>
    typedef uintptr_t GLsizeiptr;
#elif XI_OS_LINUX
    #include <GL/gl.h>
#endif

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
#define GL_PIXEL_UNPACK_BUFFER             0x88EC
#define GL_UNIFORM_BUFFER                  0x8A11
#define GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT 0x8A34
#define GL_FRAGMENT_SHADER                 0x8B30
#define GL_VERTEX_SHADER                   0x8B31
#define GL_COMPILE_STATUS                  0x8B81
#define GL_LINK_STATUS                     0x8B82
#define GL_PROGRAM_SEPARABLE               0x8258
#define GL_TEXTURE_2D_ARRAY                0x8C1A
#define GL_DEBUG_OUTPUT_SYNCHRONOUS        0x8242
#define GL_DEBUG_SEVERITY_HIGH             0x9146
#define GL_STREAM_DRAW                     0x88E0
#define GL_TEXTURE0                        0x84C0
#define GL_TEXTURE1                        0x84C1
#define GL_TEXTURE2                        0x84C2
#define GL_TEXTURE3                        0x84C3
#define GL_TEXTURE4                        0x84C4
#define GL_TEXTURE5                        0x84C5
#define GL_TEXTURE6                        0x84C6
#define GL_TEXTURE7                        0x84C7
#define GL_TEXTURE8                        0x84C8
#define GL_TEXTURE9                        0x84C9
#define GL_TEXTURE10                       0x84CA
#define GL_TEXTURE11                       0x84CB
#define GL_TEXTURE12                       0x84CC
#define GL_TEXTURE13                       0x84CD
#define GL_TEXTURE14                       0x84CE
#define GL_TEXTURE15                       0x84CF
#define GL_TEXTURE16                       0x84D0
#define GL_TEXTURE17                       0x84D1
#define GL_TEXTURE18                       0x84D2
#define GL_TEXTURE19                       0x84D3
#define GL_TEXTURE20                       0x84D4
#define GL_TEXTURE21                       0x84D5
#define GL_TEXTURE22                       0x84D6
#define GL_TEXTURE23                       0x84D7
#define GL_TEXTURE24                       0x84D8
#define GL_TEXTURE25                       0x84D9
#define GL_TEXTURE26                       0x84DA
#define GL_TEXTURE27                       0x84DB
#define GL_TEXTURE28                       0x84DC
#define GL_TEXTURE29                       0x84DD
#define GL_TEXTURE30                       0x84DE
#define GL_TEXTURE31                       0x84DF
#define GL_CLAMP_TO_EDGE                   0x812F
#define GL_TEXTURE_LOD_BIAS                0x8501

#define GL_MAP_WRITE_BIT                   0x0002
#define GL_MAP_PERSISTENT_BIT              0x0040
#define GL_MAP_FLUSH_EXPLICIT_BIT          0x0010

#define GL_VERTEX_SHADER_BIT               0x00000001
#define GL_FRAGMENT_SHADER_BIT             0x00000002

typedef void (APIENTRY *GLDEBUGPROC)(GLenum, GLenum, GLuint, GLenum, GLsizei, const char *, const void *);

// any function typedefs for extension functions to load dynamically
//
typedef void GL_glGenVertexArrays(GLsizei, GLuint *);
typedef void GL_glGenBuffers(GLsizei, GLuint *);
typedef void GL_glBindBuffer(GLenum, GLuint);
typedef void GL_glBufferStorage(GLenum, GLsizeiptr, const void *, GLbitfield);
typedef void GL_glGenProgramPipelines(GLsizei, GLuint *);
typedef void GL_glShaderSource(GLuint, GLsizei, const GLchar **, const GLint *);
typedef void GL_glCompileShader(GLuint);
typedef void GL_glGetShaderiv(GLuint, GLenum, GLint *);
typedef void GL_glProgramParameteri(GLuint, GLenum, GLint);
typedef void GL_glAttachShader(GLuint, GLuint);
typedef void GL_glLinkProgram(GLuint);
typedef void GL_glDetachShader(GLuint, GLuint);
typedef void GL_glGetProgramiv(GLuint, GLenum, GLint *);
typedef void GL_glGetProgramInfoLog(GLuint, GLsizei, GLsizei *, GLchar *);
typedef void GL_glDeleteProgram(GLuint);
typedef void GL_glGetShaderInfoLog(GLuint, GLsizei, GLsizei *, GLchar *);
typedef void GL_glDeleteShader(GLuint);
typedef void GL_glBindVertexArray(GLuint);
typedef void GL_glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
typedef void GL_glEnableVertexAttribArray(GLuint);
typedef void GL_glUseProgramStages(GLuint, GLbitfield, GLuint);
typedef void GL_glBindProgramPipeline(GLuint);
typedef void GL_glDrawElementsBaseVertex(GLenum, GLsizei, GLenum, void *, GLint);
typedef void GL_glFlushMappedBufferRange(GLenum, GLintptr, GLsizeiptr);
typedef void GL_glTexStorage3D(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);
typedef void GL_glTexSubImage3D(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void *);
typedef void GL_glTexImage3D(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
typedef void GL_glDebugMessageCallback(GLDEBUGPROC, const void *);
typedef void GL_glBufferSubData(GLenum, GLintptr, GLsizeiptr, const void *);
typedef void GL_glBufferData(GLenum, GLsizeiptr, GLvoid *, GLenum);
typedef void GL_glActiveTexture(GLenum);
typedef void GL_glBindBufferRange(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr);

typedef void *GL_glMapBufferRange(GLenum, GLintptr, GLsizeiptr, GLbitfield);

typedef GLuint GL_glCreateShader(GLenum);
typedef GLuint GL_glCreateProgram(void);

#define GL_FUNCTION_POINTER(name) GL_gl##name *name

typedef struct GL_Context GL_Context;
struct GL_Context {
    struct {
        B32 srgb;
        B32 multisample;

        GLint major_version;
        GLint minor_version;
    } info;

    M_Arena *arena;

    GLenum texture_format;
    GLuint transfer_buffer;

    GLuint sprite_array;

    GLuint *textures;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    GLuint ubo;

    U32 shader_count; // a shader contains a single stage
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
    GL_FUNCTION_POINTER(FlushMappedBufferRange);
    GL_FUNCTION_POINTER(DrawElementsBaseVertex);
    GL_FUNCTION_POINTER(TexStorage3D);
    GL_FUNCTION_POINTER(TexSubImage3D);
    GL_FUNCTION_POINTER(TexImage3D);
    GL_FUNCTION_POINTER(DebugMessageCallback);
    GL_FUNCTION_POINTER(BufferSubData);
    GL_FUNCTION_POINTER(BufferData);
    GL_FUNCTION_POINTER(ActiveTexture);
    GL_FUNCTION_POINTER(BindBufferRange);
};

#undef GL_FUNCTION_POINTER

// os specific calls
//
FileScope GL_Context *GL_ContextCreate(RendererContext *renderer, void *platform);
FileScope void        GL_ContextDelete(GL_Context *gl);

// generic opengl calls
//
FileScope B32 GL_BaseShaderCompile(GL_Context *gl);

#endif  // XI_OPENGL_H_
