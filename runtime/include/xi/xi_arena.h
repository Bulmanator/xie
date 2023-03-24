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

// arena initialisation
//
extern XI_API void xi_arena_init_from(xiArena *arena, void *base, uptr size);
extern XI_API void xi_arena_init_virtual(xiArena *arena, uptr size);

// release any virtual memory associated with the arena and reset all members to zero
//
extern XI_API void xi_arena_deinit(xiArena *arena);

// base arena push allocation calls
//
extern XI_API void *xi_arena_push_aligned(xiArena *arena, uptr size, uptr alignment);
extern XI_API void *xi_arena_push(xiArena *arena, uptr size);

// arena push macros, these are preferred to the base calls above
//
#define xi_arena_push_size_aligned(arena, size, alignment) xi_arena_push_aligned(arena, size, alignment)
#define xi_arena_push_size(arena, size) xi_arena_push(arena, size)

#define xi_arena_push_type_aligned(arena, type, alignment) (type *) xi_arena_push_aligned(arena, sizeof(type), alignment)
#define xi_arena_push_type(arena, type) (type *) xi_arena_push(arena, sizeof(type))

#define xi_arena_push_array_aligned(arena, type, count, alignment) (type *) xi_arena_push_aligned(arena, (count) * sizeof(type), alignment)
#define xi_arena_push_array(arena, type, count) (type *) xi_arena_push(arena, (count) * sizeof(type))

// base arena push copy allocation calls
// like the push calls above but take a source pointer to copy data to
//
extern XI_API void *xi_arena_push_copy_aligned(xiArena *arena, void *src, uptr size, uptr alignment);
extern XI_API void *xi_arena_push_copy(xiArena *arena, void *src, uptr size);

// arena push copy macros, like standard push calls these are preferred to the base calls
//
#define xi_arena_push_size_copy_aligned(arena, src, size, alignment) xi_arena_push_copy_aligned(arena, src, size, alignment)
#define xi_arena_push_size_copy(arena, src, size) xi_arena_push_copy(arena, src, size)

#define xi_arena_push_copy_type_aligned(arena, src, type, alignment) (type *) xi_arena_push_copy_aligned(arena, src, sizeof(type), alignment)
#define xi_arena_push_copy_type(arena, src, type) (type *) xi_arena_push_copy(arena, src, sizeof(type))

#define xi_arena_push_copy_array_aligned(arena, src, type, count, alignment) (type *) xi_arena_push_copy_aligned(arena, src, (count) * sizeof(type), alignment)
#define xi_arena_push_copy_array(arena, src, type, count) (type *) xi_arena_push_copy(arena, src, (count) * sizeof(type))

// get the current offset of the arena
//
extern XI_API uptr xi_arena_offset_get(xiArena *arena);

// pop allocation calls
//
extern XI_API void xi_arena_pop_to(xiArena *arena, uptr offset);
extern XI_API void xi_arena_pop_size(xiArena *arena, uptr size);
extern XI_API void xi_arena_pop_last(xiArena *arena);

// utility pop macros for dealing with typed removal
//
#define xi_arena_pop_type(arena, type) xi_arena_pop_size(arena, sizeof(type))
#define xi_arena_pop_array(arena, type, count) xi_arena_pop_size(arena, (count) * sizeof(type))

// reset call
//
extern XI_API void xi_arena_reset(xiArena *arena);

// thread-local temporary arena
//
extern XI_API xiArena *xi_temp_get();
extern XI_API void xi_temp_reset();

#if defined(__cplusplus)
}
#endif

#endif  // XI_ARENA_H_
