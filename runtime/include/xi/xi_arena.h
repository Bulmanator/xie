#if !defined(XI_ARENA_H_)
#define XI_ARENA_H_

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(XI_ARENA_DEFAULT_ALIGNMENT)
    #define XI_ARENA_DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

enum xiArenaFlags {
    // signifies that the base of the arena has been virtually reserved and thus requires to be
    // committed/decommitted as the arena use grows/shrinks. if this flag is not present, the entire
    // arena range is assumed to be valid
    //
    XI_ARENA_FLAG_VIRTUAL_BASE = (1 << 0),

    // forces strict use of the arena's default alignment for all push/pop operations, the presence of
    // this flag makes push/push_aligned effectively the same
    //
    XI_ARENA_FLAG_STRICT_ALIGNMENT = (1 << 1)
};

typedef struct xiArena {
    void *base;
    uptr  size;
    uptr  offset;

    uptr last_offset;
    uptr default_alignment;

    uptr committed;
    u64  flags;
    u64  pad;
} xiArena;

extern XI_API void xi_arena_init_from(xiArena *arena, void *base, uptr size);
extern XI_API void xi_arena_init_virtual(xiArena *arena, uptr size);

extern XI_API void *xi_arena_push_aligned(xiArena *arena, uptr size, uptr alignment);
extern XI_API void *xi_arena_push(xiArena *arena, uptr size);

#define xi_arena_push_size_aligned(arena, size, alignment) xi_arena_push_aligned(arena, size, alignment)
#define xi_arena_push_size(arena, size) xi_arena_push(arena, size)

#define xi_arena_push_type_aligned(arena, type, alignment) xi_arena_push_aligned(arena, sizeof(type), alignment)
#define xi_arena_push_type(arena, type) xi_arena_push(arena, sizeof(type))

#define xi_arena_push_array_aligned(arena, type, count, alignment) xi_arena_push_aligned(arena, (count) * sizeof(type), alignment)
#define xi_arena_push_array(arena, type, count) xi_arena_push(arena, (count) * sizeof(type))

extern XI_API uptr xi_arena_offset_get(xiArena *arena);

extern XI_API void xi_arena_pop_to(xiArena *arena, uptr offset);
extern XI_API void xi_arena_pop_size(xiArena *arena, uptr size);
extern XI_API void xi_arena_pop_last(xiArena *arena);

#define xi_arena_pop_type(arena, type) xi_arena_pop_size(arena, sizeof(type))
#define xi_arena_pop_array(arena, type, count) xi_arena_pop_size(arena, (count) * sizeof(type))

extern XI_API void xi_arena_reset(xiArena *arena);

#if defined(__cplusplus)
}
#endif

#endif  // XI_ARENA_H_
