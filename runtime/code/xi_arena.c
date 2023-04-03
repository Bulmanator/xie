// os memory functions
//
XI_INTERNAL xi_uptr xi_os_allocation_granularity_get();
XI_INTERNAL xi_uptr xi_os_page_size_get();

XI_INTERNAL void *xi_os_virtual_memory_reserve(xi_uptr size);
XI_INTERNAL xi_b32 xi_os_virtual_memory_commit(void *base, xi_uptr size);
XI_INTERNAL void xi_os_virtual_memory_decommit(void *base, xi_uptr size);
XI_INTERNAL void xi_os_virtual_memory_release(void *base, xi_uptr size);

void xi_arena_init_from(xiArena *arena, void *base, xi_uptr size) {
    arena->base   = base;
    arena->size   = size;
    arena->offset = 0;

    arena->last_offset       = 0;
    arena->default_alignment = XI_ARENA_DEFAULT_ALIGNMENT;

    arena->committed = 0; // not used
    arena->flags     = 0;
    arena->pad       = 0;
}

void xi_arena_init_virtual(xiArena *arena, xi_uptr size) {
    xi_uptr granularity = xi_os_allocation_granularity_get();

    xi_uptr full_size = XI_ALIGN_UP(size, granularity);

    arena->base   = xi_os_virtual_memory_reserve(full_size);
    arena->size   = full_size;
    arena->offset = 0;

    arena->last_offset       = 0;
    arena->default_alignment = XI_ARENA_DEFAULT_ALIGNMENT;

    arena->committed = 0;
    arena->flags     = XI_ARENA_FLAG_VIRTUAL_BASE;
    arena->pad       = 0;
}

void xi_arena_deinit(xiArena *arena) {
    xi_b32 virtual = (arena->flags & XI_ARENA_FLAG_VIRTUAL_BASE) != 0;

    void *base   = arena->base;
    xi_uptr size = arena->size;

    XI_ASSERT(base != 0);

    arena->base   = 0;
    arena->size   = 0;
    arena->offset = 0;

    arena->last_offset       = 0;
    arena->default_alignment = 0;

    arena->committed = 0;
    arena->flags     = 0;

    if (virtual) {
        xi_os_virtual_memory_release(base, size);

        // we don't want to touch the arena after calling this in-case the user has passed us a pointer
        // to an arena stored inside its own reserved base
        //
    }
}

void *xi_arena_push_aligned(xiArena *arena, xi_uptr size, xi_uptr alignment) {
    void *result = 0;

    if (arena->flags & XI_ARENA_FLAG_STRICT_ALIGNMENT) {
        alignment = arena->default_alignment;
    }

    xi_uptr align_offset   = XI_ALIGN_UP(arena->offset, alignment) - arena->offset;
    xi_uptr allocation_end = arena->offset + align_offset + size;

    if (allocation_end <= arena->size) {
        // we have enough space to push the allocation
        //
        if (arena->flags & XI_ARENA_FLAG_VIRTUAL_BASE) {
            if (allocation_end > arena->committed) {
                // we haven't committed enough virtual memory so commit more
                //
                xi_uptr page_size = xi_os_page_size_get();

                void *commit_base   = (xi_u8 *) arena->base + arena->committed;
                xi_uptr commit_size = XI_ALIGN_UP(allocation_end - arena->committed, page_size);

                xi_os_virtual_memory_commit(commit_base, commit_size);

                arena->committed += commit_size;
            }
        }

        // :note for now we don't need to zero the memory as we are explicitly decommitting pages when
        // the arena is reset or an allocation is popped. on modern operating systems when decommitting
        // pages it guarantees us zero pages when re-committing/accessing them again
        //
        result = (xi_u8 *) arena->base + arena->offset + align_offset;

        arena->last_offset = arena->offset;
        arena->offset      = allocation_end;
    }

    XI_ASSERT(result != 0);
    XI_ASSERT(((xi_u64) result & (alignment - 1)) == 0);

    return result;
}

void *xi_arena_push(xiArena *arena, xi_uptr size) {
    void *result = xi_arena_push_aligned(arena, size, arena->default_alignment);
    return result;
}

void *xi_arena_push_copy_aligned(xiArena *arena, void *src, xi_uptr size, xi_uptr alignment) {
    void *result = xi_arena_push_aligned(arena, size, alignment);

    xi_memory_copy(result, src, size);
    return result;
}

void *xi_arena_push_copy(xiArena *arena, void *src, xi_uptr size) {
    void *result = xi_arena_push_aligned(arena, size, arena->default_alignment);

    xi_memory_copy(result, src, size);
    return result;
}

xi_uptr xi_arena_offset_get(xiArena *arena) {
    xi_uptr result = arena->offset;

    if (arena->flags & XI_ARENA_FLAG_STRICT_ALIGNMENT) {
        result = XI_ALIGN_UP(result, arena->default_alignment);
    }

    return result;
}

void xi_arena_pop_to(xiArena *arena, xi_uptr offset) {
    XI_ASSERT(offset <= arena->offset); // can't pop forward

    if (arena->flags & XI_ARENA_FLAG_VIRTUAL_BASE) {
        xi_uptr page_size    = xi_os_page_size_get();
        xi_uptr removed_size = arena->offset - offset;
        xi_uptr to_decommit  = XI_ALIGN_DOWN(removed_size, page_size);
        xi_uptr left_over    = removed_size - to_decommit;

        arena->committed -= to_decommit;

        xi_os_virtual_memory_decommit((xi_u8 *) arena->base + arena->committed, to_decommit);
        xi_memory_zero((xi_u8 *) arena->base + (arena->committed - left_over), left_over);
    }

    arena->offset      = offset;
    arena->last_offset = offset;
}

void xi_arena_pop_size(xiArena *arena, xi_uptr size) {
    XI_ASSERT(arena->offset >= size);

    xi_uptr new_offset = (arena->offset - size);
    xi_arena_pop_to(arena, new_offset);
}

void xi_arena_pop_last(xiArena *arena) {
    xi_arena_pop_to(arena, arena->last_offset);
}

void xi_arena_reset(xiArena *arena) {
    arena->last_offset = 0;
    arena->offset      = 0;

    if (arena->flags & XI_ARENA_FLAG_VIRTUAL_BASE) {
        if (arena->committed > 0) {
            xi_os_virtual_memory_decommit(arena->base, arena->committed);
            arena->committed = 0;
        }
    }
}

XI_GLOBAL XI_THREAD_VAR xiArena __tls_temp;

xiArena *xi_temp_get() {
    xiArena *result = &__tls_temp;
    if (!result->base) {
        xi_arena_init_virtual(result, XI_GB(16));
    }

    return result;
}

void xi_temp_reset() {
    if (__tls_temp.base) {
        xi_arena_reset(&__tls_temp);
    }
}


