void xi_arena_init_from(xiArena *arena, void *base, uptr size) {
    arena->base   = base;
    arena->size   = size;
    arena->offset = 0;

    arena->last_offset       = 0;
    arena->default_alignment = XI_ARENA_DEFAULT_ALIGNMENT;

    arena->committed = 0; // not used
    arena->flags     = 0;
    arena->pad       = 0;
}

void xi_arena_init_virtual(xiArena *arena, uptr size) {
    uptr granularity = xi_os_allocation_granularity_get();

    uptr full_size = XI_ALIGN_UP(size, granularity);

    arena->base   = xi_os_virtual_memory_reserve(full_size);
    arena->size   = full_size;
    arena->offset = 0;

    arena->last_offset       = 0;
    arena->default_alignment = XI_ARENA_DEFAULT_ALIGNMENT;

    arena->committed = 0;
    arena->flags     = XI_ARENA_FLAG_VIRTUAL_BASE;
    arena->pad       = 0;
}

void *xi_arena_push_aligned(xiArena *arena, uptr size, uptr alignment) {
    void *result = 0;

    if (arena->flags & XI_ARENA_FLAG_STRICT_ALIGNMENT) {
        alignment = arena->default_alignment;
    }

    uptr align_offset   = XI_ALIGN_UP(arena->offset, alignment) - arena->offset;
    uptr allocation_end = arena->offset + align_offset + size;

    if (allocation_end <= arena->size) {
        // we have enough space to push the allocation
        //
        if (arena->flags & XI_ARENA_FLAG_VIRTUAL_BASE) {
            if (allocation_end > arena->committed) {
                // we haven't committed enough virtual memory so commit more
                //
                uptr page_size = xi_os_page_size_get();

                void *commit_base = (u8 *) arena->base + arena->committed;
                uptr  commit_size = XI_ALIGN_UP(allocation_end - arena->committed, page_size);

                xi_os_virtual_memory_commit(commit_base, commit_size);

                arena->committed += commit_size;
            }
        }

        // :note for now we don't need to zero the memory as we are explicitly decommitting pages when
        // the arena is reset or an allocation is popped. on modern operating systems when decommitting
        // pages it guarantees us zero pages when re-committing/accessing them again
        //
        result = (u8 *) arena->base + arena->offset + align_offset;

        arena->last_offset = arena->offset;
        arena->offset      = allocation_end;
    }

    return result;
}

void *xi_arena_push(xiArena *arena, uptr size) {
    void *result = xi_arena_push_aligned(arena, size, arena->default_alignment);
    return result;
}

uptr xi_arena_offset_get(xiArena *arena) {
    uptr result = arena->offset;
    return result;
}

void xi_arena_pop_to(xiArena *arena, uptr offset) {
    if (arena->flags & XI_ARENA_FLAG_STRICT_ALIGNMENT) {
        offset = XI_ALIGN_UP(offset, arena->default_alignment);
    }

    if (arena->flags & XI_ARENA_FLAG_VIRTUAL_BASE) {
        uptr page_size    = xi_os_page_size_get();
        uptr removed_size = arena->offset - offset;
        uptr to_decommit  = XI_ALIGN_DOWN(removed_size, page_size);
        uptr left_over    = removed_size - to_decommit;

        arena->committed -= to_decommit;

        xi_os_virtual_memory_decommit((u8 *)arena->base + arena->committed, to_decommit);
        xi_memory_zero((u8 *)arena->base + (arena->committed - left_over), left_over);
    }

    arena->offset      = offset;
    arena->last_offset = offset;
}

void xi_arena_pop_size(xiArena *arena, uptr size) {
    XI_ASSERT(arena->offset >= size);

    uptr new_offset = (arena->offset - size);
    xi_arena_pop_to(arena, new_offset);
}

void xi_arena_pop_last(xiArena *arena) {
    xi_arena_pop_to(arena, arena->last_offset);
}

void xi_arena_reset(xiArena *arena) {
    arena->last_offset = 0;
    arena->offset      = 0;

    if (arena->flags & XI_ARENA_FLAG_VIRTUAL_BASE) {
        xi_os_virtual_memory_decommit(arena->base, arena->committed);
        arena->committed = 0;
    }
}


