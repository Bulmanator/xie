#if !defined(XI_RENDERER_H_)
#define XI_RENDERER_H_

// @todo: add configuration parameters
//

typedef struct xiRenderer xiRenderer;

#define XI_RENDERER_INIT(name) xiRenderer *name(void *platform)
#define XI_RENDERER_SUBMIT(name) void name(xiRenderer *renderer)

typedef XI_RENDERER_INIT(xiRendererInit);
typedef XI_RENDERER_SUBMIT(xiRendererSubmit);

typedef struct xiRenderer {
    buffer commands;

    xiArena uniform;
} xiRenderer;

#endif  // XI_RENDERER_H_
