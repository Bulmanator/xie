typedef struct xiDirectoryEntryBucket xiDirectoryEntryBucket;

typedef struct xiDirectoryEntryBucket {
    xi_u32 count;
    xiDirectoryEntry entries[128];

    xiDirectoryEntryBucket *next;
} xiDirectoryEntryBucket;

typedef struct xiDirectoryListBuilder {
    xiDirectoryEntryBucket base;

    xiDirectoryEntryBucket *head;
    xiDirectoryEntryBucket *tail;

    xi_u32 total;
} xiDirectoryListBuilder;

// os fileio
//
XI_INTERNAL void xi_os_directory_list_build(xiArena *arena,
        xiDirectoryListBuilder *builder, xi_string path, xi_b32 recursive);

XI_INTERNAL void xi_directory_list_builder_append(xiDirectoryListBuilder *builder, xiDirectoryEntry *entry) {
    xiDirectoryEntryBucket *bucket = builder->tail;

    XI_ASSERT(bucket != 0);

    bucket->entries[bucket->count] = *entry;

    bucket->count  += 1;
    builder->total += 1;

    if (bucket->count >= XI_ARRAY_SIZE(bucket->entries)) {
        xiArena *temp = xi_temp_get();

        xiDirectoryEntryBucket *next = xi_arena_push_type(temp, xiDirectoryEntryBucket);

        builder->tail->next = next;
        builder->tail = next;
    }
}

XI_INTERNAL void xi_directory_list_builder_flatten(xiArena *arena,
        xiDirectoryList *list, xiDirectoryListBuilder *builder)
{

    if (builder->total > 0) {
        list->count   = builder->total;
        list->entries = xi_arena_push_array(arena, xiDirectoryEntry, list->count);

        xi_u32 offset = 0;

        xiDirectoryEntryBucket *bucket = builder->head;
        while (bucket != 0) {
            xi_memory_copy(&list->entries[offset], bucket->entries, bucket->count * sizeof(xiDirectoryEntry));

            offset += bucket->count;
            bucket  = bucket->next;
        }
    }
    else {
        list->count   = 0;
        list->entries = 0;
    }
}

void xi_directory_list_get(xiArena *arena, xiDirectoryList *list, xi_string path, xi_b32 recursive) {
    xiDirectoryListBuilder builder = { 0 };

    builder.head  = &builder.base;
    builder.tail  = &builder.base;

    builder.total = 0;

    xi_os_directory_list_build(arena, &builder, path, recursive);

    xi_directory_list_builder_flatten(arena, list, &builder);
}

void xi_directory_list_filter_for_files(xiArena *arena, xiDirectoryList *out, xiDirectoryList *list) {
    xiDirectoryListBuilder builder = { 0 };
    builder.head  = &builder.base;
    builder.tail  = &builder.base;

    builder.total = 0;

    for (xi_u32 it = 0; it < list->count; ++it) {
        xiDirectoryEntry *entry = &list->entries[it];

        if (entry->type == XI_DIRECTORY_ENTRY_TYPE_FILE) {
            xi_directory_list_builder_append(&builder, entry);
        }
    }

    xi_directory_list_builder_flatten(arena, out, &builder);
}

void xi_directory_list_filter_for_directories(xiArena *arena, xiDirectoryList *out, xiDirectoryList *list) {
    xiDirectoryListBuilder builder = { 0 };
    builder.head  = &builder.base;
    builder.tail  = &builder.base;

    builder.total = 0;

    for (xi_u32 it = 0; it < list->count; ++it) {
        xiDirectoryEntry *entry = &list->entries[it];

        if (entry->type == XI_DIRECTORY_ENTRY_TYPE_DIRECTORY) {
            xi_directory_list_builder_append(&builder, entry);
        }
    }

    xi_directory_list_builder_flatten(arena, out, &builder);
}

void xi_directory_list_filter_for_prefix(xiArena *arena,
        xiDirectoryList *out, xiDirectoryList *list, xi_string prefix)
{
    xiDirectoryListBuilder builder = { 0 };
    builder.head  = &builder.base;
    builder.tail  = &builder.base;

    builder.total = 0;

    for (xi_u32 it = 0; it < list->count; ++it) {
        xiDirectoryEntry *entry = &list->entries[it];

        xi_string start = xi_str_prefix(entry->basename, prefix.count);

        if ((entry->type == XI_DIRECTORY_ENTRY_TYPE_FILE) && xi_str_equal(prefix, start)) {
            xi_directory_list_builder_append(&builder, entry);
        }
    }

    xi_directory_list_builder_flatten(arena, out, &builder);
}

void xi_directory_list_filter_for_suffix(xiArena *arena,
        xiDirectoryList *out, xiDirectoryList *list, xi_string suffix)
{
    xiDirectoryListBuilder builder = { 0 };
    builder.head  = &builder.base;
    builder.tail  = &builder.base;

    builder.total = 0;

    for (xi_u32 it = 0; it < list->count; ++it) {
        xiDirectoryEntry *entry = &list->entries[it];

        xi_string end = xi_str_suffix(entry->basename, suffix.count);

        if ((entry->type == XI_DIRECTORY_ENTRY_TYPE_FILE) && xi_str_equal(suffix, end)) {
            xi_directory_list_builder_append(&builder, entry);
        }
    }

    xi_directory_list_builder_flatten(arena, out, &builder);
}

void xi_directory_list_filter_for_extension(xiArena *arena,
        xiDirectoryList *out, xiDirectoryList *list, xi_string extension)
{
    xiDirectoryListBuilder builder = { 0 };
    builder.head  = &builder.base;
    builder.tail  = &builder.base;

    builder.total = 0;

    for (xi_u32 it = 0; it < list->count; ++it) {
        xiDirectoryEntry *entry = &list->entries[it];

        xi_string ext = xi_str_suffix(entry->path, extension.count);

        if ((entry->type == XI_DIRECTORY_ENTRY_TYPE_FILE) && xi_str_equal(extension, ext)) {
            xi_directory_list_builder_append(&builder, entry);
        }
    }

    xi_directory_list_builder_flatten(arena, out, &builder);
}

xi_string xi_file_read_all(xiArena *arena, xi_string path) {
    xi_string result = { 0 };

    xiArena *temp = xi_temp_get();

    xiDirectoryEntry entry;
    if (xi_os_directory_entry_get_by_path(temp, &entry, path)) {
        if (entry.type == XI_DIRECTORY_ENTRY_TYPE_FILE) {
            xiFileHandle handle;
            if (xi_os_file_open(&handle, path, XI_FILE_ACCESS_FLAG_READ)) {
                result.count = entry.size;
                result.data  = xi_arena_push_size(arena, result.count);

                xi_os_file_read(&handle, result.data, 0, result.count);
                xi_os_file_close(&handle);
            }
        }
    }

    return result;
}

xi_string xi_file_read_all_buffer(xi_buffer *b, xi_string path) {
    xi_string result = { 0 };

    xiArena *temp = xi_temp_get();

    xiDirectoryEntry entry;
    if (xi_os_directory_entry_get_by_path(temp, &entry, path)) {
        if (entry.type == XI_DIRECTORY_ENTRY_TYPE_FILE) {
            xiFileHandle handle;
            if (xi_os_file_open(&handle, path, XI_FILE_ACCESS_FLAG_READ)) {
                result.count = XI_MIN(b->limit - b->used, entry.size);
                result.data  = b->data + b->used;

                xi_os_file_read(&handle, result.data, 0, result.count);
                xi_os_file_close(&handle);

                b->used += result.count;
            }
        }
    }

    return result;
}
