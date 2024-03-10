// os memory functions
//
FileScope U64 OS_AllocationGranularity();
FileScope U64 OS_PageSize();

FileScope void *OS_MemoryReserve(U64 size);
FileScope B32   OS_MemoryCommit(void *base, U64 size);
FileScope void  OS_MemoryDecommit(void *base, U64 size);
FileScope void  OS_MemoryRelease(void *base, U64 size);

//
// --------------------------------------------------------------------------------
// :Init
// --------------------------------------------------------------------------------
//
#if OS_SWITCH
    // switchbrew doesn't currently support generic virtual memory semantics, this means we have to
    // resort to using malloc/free for heap allocations.
    //
    // As this prevents us from reserving large regions of the address space and committing only what
    // we need, the maximum reserve size must be heavily limited as malloc is pretty much guaranteed to
    // fail with larger 'reserve' sizes like 64GiB
    //
    // while this works fine in theory, it has a massive glaring issue that you cannot push a single
    // allocation larger than this limit
    //
    // :switch_virtmem
    //
    #define M_ARENA_RESERVE_LIMIT MB(64)
#else
    #define M_ARENA_RESERVE_LIMIT TB(256)
#endif

M_Arena *M_ArenaAlloc(U64 reserve, M_ArenaFlags flags) {
    M_Arena *result = 0;

#if OS_SWITCH
    // :switch_virtmem arenas must grow otherwise would be limited by small sizes
    //
    flags &= ~M_ARENA_DONT_GROW;
#endif

    U64 granularity = OS_AllocationGranularity();
    U64 limit       = Clamp(M_ARENA_INITIAL_COMMIT, AlignUp(reserve, granularity), M_ARENA_RESERVE_LIMIT);
    U64 commit      = M_ARENA_INITIAL_COMMIT;

    void *base = OS_MemoryReserve(limit);
    if (base) {
        if (OS_MemoryCommit(base, commit)) {
            result = cast(M_Arena *) base;

            result->current     = result;
            result->prev        = 0;

            result->base        = 0;
            result->limit       = limit;
            result->offset      = M_ARENA_INITIAL_OFFSET;

            result->committed   = commit;

            result->last_offset = M_ARENA_INITIAL_OFFSET;

            result->flags       = flags;
        }
        else {
            OS_MemoryRelease(base, reserve);
        }
    }

    return result;
}

void M_ArenaReset(M_Arena *arena) {
    M_Arena *current = arena->current;
    while (current->prev != 0) {
        U8 *base = cast(U8 *) current;
        U64 size = current->limit;

        current = current->prev;

        OS_MemoryRelease(base, size);
    }

    Assert(current == arena);
    Assert(current->committed >= M_ARENA_INITIAL_COMMIT);

    // Decommit the region of memory that is no longer being used
    //
    U64 decommit_size = current->committed - M_ARENA_INITIAL_COMMIT;
    if (decommit_size != 0) {
        U8 *decommit_base = cast(U8 *) current + M_ARENA_INITIAL_COMMIT;
        OS_MemoryDecommit(decommit_base, decommit_size);

        current->committed = M_ARENA_INITIAL_COMMIT;
    }

    current->offset    = M_ARENA_INITIAL_OFFSET;
    arena->last_offset = M_ARENA_INITIAL_OFFSET;
}

void M_ArenaRelease(M_Arena *arena) {
    M_Arena *current = arena->current;
    while (current->prev != 0) {
        U8 *base = cast(U8 *) current;
        U64 size = current->limit;

        current = current->prev;

        OS_MemoryRelease(base, size);
    }

    Assert(current == arena);

    OS_MemoryRelease(current, current->limit);
}

//
// --------------------------------------------------------------------------------
// :Arena_Push
// --------------------------------------------------------------------------------
//

void *M_ArenaPushFrom(M_Arena *arena, U64 size, M_ArenaFlags flags, U64 alignment) {
    void *result = 0;

    M_Arena *current = arena->current;

    U64 last_offset = current->base + current->offset;

    U64 offset = AlignUp(current->offset, alignment);
    U64 end    = offset + size;

    B32 growable = (arena->flags & M_ARENA_DONT_GROW) == 0;

    if (end > current->limit && growable) {
        // Allocate a new chunk as we have run out of space on the current arena
        //
        U64 reserve = M_ARENA_DEFAULT_RESERVE;
        if (size >= reserve) {
            reserve = size + M_ARENA_INITIAL_OFFSET;
        }

        Assert(reserve <= M_ARENA_RESERVE_LIMIT);

        M_Arena *next = M_ArenaAlloc(reserve, arena->flags);
        if (next != 0) {
            next->prev = current;
            next->base = current->base + current->limit;

            arena->current = current = next;

            offset = AlignUp(current->offset, alignment);
            end    = offset + size;
        }
    }

    if (end > current->committed) {
        // Commit more space
        //
        Assert(current->committed <= current->limit);

        U64 commit_size = AlignUp(end - current->committed, M_ARENA_COMMIT_BLOCK_SIZE);
        if ((current->committed + commit_size) > current->limit) {
            commit_size = current->limit - current->committed;
        }

        if (commit_size != 0) {
            U8 *commit_base = cast(U8 *) current + current->committed;

            if (OS_MemoryCommit(commit_base, commit_size)) {
                current->committed += commit_size;
            }
        }
    }

    if (end <= current->committed) {
        result = cast(U8 *) current + offset;
        current->offset = end;

        arena->last_offset = last_offset;

        if ((flags & M_ARENA_NO_ZERO) == 0) {
            // @todo: remove this flag, zero on pop. we can rely on the virtual memory system to zero
            // our memory for freshly committed pages. only when memory is re-used will we have to
            // zero memory reuse directly from an arena can only occur if a Pop call has happened, thus
            // we can decommit pages within a specified threshold (guaranteeing zero pages again on commit)
            // and any portion that isn't decommitted within this range can be set to zero like below
            //
            // this is significantly more efficient for larger sized pushes
            //
            MemoryZero(result, size);
        }
    }

    return result;
}

void *M_ArenaPushCopyFrom(M_Arena *arena, void *src, U64 size, M_ArenaFlags flags, U64 alignment) {
    void *result = M_ArenaPushFrom(arena, size, flags | M_ARENA_NO_ZERO, alignment);
    MemoryCopy(result, src, size);

    return result;
}

//
// --------------------------------------------------------------------------------
// :Arena_Pop
// --------------------------------------------------------------------------------
//

void M_ArenaPopTo(M_Arena *arena, U64 offset) {
    U64 end = Max(offset, M_ARENA_INITIAL_OFFSET);

    M_Arena *current = arena->current;
    while (current->base >= end) {
        U8 *base = cast(U8 *) current;
        U64 size = current->limit;

        current = current->prev;

        OS_MemoryRelease(base, size);
    }

    Assert(current != 0);

    end = end - current->base;
    current->offset = end;
}

void M_ArenaPopSize(M_Arena *arena, U64 size) {
    M_Arena *current = arena->current;

    Assert(current->offset >= M_ARENA_INITIAL_OFFSET);
    Assert(size <= (current->offset - M_ARENA_INITIAL_OFFSET));

    U64 new_offset = current->base + (current->offset - size);
    M_ArenaPopTo(arena, new_offset);
}

void M_ArenaPopLast(M_Arena *arena) {
    M_ArenaPopTo(arena, arena->last_offset);
}

U64 M_ArenaOffset(M_Arena *arena) {
    M_Arena *current = arena->current;

    U64 result = current->base + current->offset;
    return result;
}

GlobalVar ThreadVar M_Arena *__tls_temp;

M_Arena *M_TempGet() {
    M_Arena *result = __tls_temp;
    if (!result) {
        result = __tls_temp = M_ArenaAlloc(M_TEMP_ARENA_LIMIT, 0);
    }

    return result;
}

void M_TempReset() {
    if (__tls_temp != 0) {
        M_ArenaReset(__tls_temp);
    }
}
