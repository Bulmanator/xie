#if !defined(XI_ARENA_H_)
#define XI_ARENA_H_

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(M_TEMP_ARENA_LIMIT)
    #define M_TEMP_ARENA_LIMIT GB(8)
#endif

#define M_ARENA_DEFAULT_RESERVE   MB(32)
#define M_ARENA_INITIAL_COMMIT    MB(2)
#define M_ARENA_COMMIT_BLOCK_SIZE KB(64)
#define M_ARENA_INITIAL_OFFSET    64

typedef U32 M_ArenaFlags;
enum {
    // Arenas will grow by default as they run out of space, this flag will prevent that
    // from happening and will be limited to their initial size
    //
    // :switch_virtmem
    //
    M_ARENA_DONT_GROW = (1 << 0),

    // Used per allocation, prevents the system from setting the allocation to zero
    //
    M_ARENA_NO_ZERO   = (1 << 1)
};

typedef struct M_Arena M_Arena;
struct M_Arena {
    M_Arena *current;
    M_Arena *prev;

    U64 base;
    U64 limit;
    U64 offset;

    U64 committed;

    U64 last_offset;

    M_ArenaFlags flags;
    U32 unused;
};

StaticAssert(sizeof(M_Arena) == 64);

//
// --------------------------------------------------------------------------------
// :Arena_Init
// --------------------------------------------------------------------------------
//

Func M_Arena *M_ArenaAlloc(U64 reserve, M_ArenaFlags flags);

Func void M_ArenaReset(M_Arena *arena);
Func void M_ArenaRelease(M_Arena *arena);

Func U64 M_ArenaOffset(M_Arena *arena); // Current global offset of the arena

//
// --------------------------------------------------------------------------------
// :Arena_Push
// --------------------------------------------------------------------------------
//

Func void *M_ArenaPushFrom(M_Arena *arena, U64 size, M_ArenaFlags flags, U64 alignment);
Func void *M_ArenaPushCopyFrom(M_Arena *arena, void *src, U64 size, M_ArenaFlags flags, U64 alignment);

#define M_ArenaPush(...)     M_ArenaPushExpand((__VA_ARGS__, M_ArenaPush5, M_ArenaPush4, M_ArenaPush3, M_ArenaPush2))(__VA_ARGS__)
#define M_ArenaPushCopy(...) M_ArenaPushCopyExpand((__VA_ARGS__, M_ArenaPushCopy6, M_ArenaPushCopy5, M_ArenaPushCopy4, M_ArenaPushCopy3))(__VA_ARGS__)

// Arena push expansion macros
//
#define M_ArenaPushExpand(args) M_ArenaPushBase args
#define M_ArenaPushBase(a, b, c, d, e, f, ...) f

#define M_ArenaPush2(arena, T)          (T *) M_ArenaPushFrom((arena),       sizeof(T), 0, AlignOf(T))
#define M_ArenaPush3(arena, T, n)       (T *) M_ArenaPushFrom((arena), (n) * sizeof(T), 0, AlignOf(T))
#define M_ArenaPush4(arena, T, n, f)    (T *) M_ArenaPushFrom((arena), (n) * sizeof(T), f, AlignOf(T))
#define M_ArenaPush5(arena, T, n, f, a) (T *) M_ArenaPushFrom((arena), (n) * sizeof(T), f, a)

// Arena push copy expansion macros
//
#define M_ArenaPushCopyExpand(args) M_ArenaPushCopyBase args
#define M_ArenaPushCopyBase(a, b, c, d, e, f, g, ...) g

#define M_ArenaPushCopy3(arena, src, T)          (T *) M_ArenaPushCopyFrom((arena), src,       sizeof(T), 0, AlignOf(T))
#define M_ArenaPushCopy4(arena, src, T, n)       (T *) M_ArenaPushCopyFrom((arena), src, (n) * sizeof(T), 0, AlignOf(T))
#define M_ArenaPushCopy5(arena, src, T, n, f)    (T *) M_ArenaPushCopyFrom((arena), src, (n) * sizeof(T), f, AlignOf(T))
#define M_ArenaPushCopy6(arena, src, T, n, f, a) (T *) M_ArenaPushCopyFrom((arena), src, (n) * sizeof(T), f, a)

//
// --------------------------------------------------------------------------------
// :Arena_Pop
// --------------------------------------------------------------------------------
//

Func void M_ArenaPopTo(M_Arena *arena, U64 offset);
Func void M_ArenaPopSize(M_Arena *arena, U64 size);
Func void M_ArenaPopLast(M_Arena *arena);

#define M_ArenaPop(...) M_ArenaPopExpand((__VA_ARGS__, M_ArenaPop3, M_ArenaPop2))(__VA_ARGS__)

#define M_ArenaPopExpand(args) M_ArenaPopBase args
#define M_ArenaPopBase(a, b, c, d, ...) d

#define M_ArenaPop2(arena, T)    M_ArenaPopSize((arena),       sizeof(T))
#define M_ArenaPop3(arena, T, n) M_ArenaPopSize((arena), (n) * sizeof(T))

//
// --------------------------------------------------------------------------------
// :Temporary_Arena
// --------------------------------------------------------------------------------
//
// @todo: rework this to use stack-based temporary arenas instead
//

Func M_Arena *M_TempGet();
Func void     M_TempReset();

#if defined(__cplusplus)
}
#endif

#endif  // XI_ARENA_H_
