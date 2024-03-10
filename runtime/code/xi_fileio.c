typedef struct DirectoryEntryBucket DirectoryEntryBucket;
struct DirectoryEntryBucket {
    U32 count;
    DirectoryEntry entries[128];

    DirectoryEntryBucket *next;
};

typedef struct DirectoryListBuilder DirectoryListBuilder;
struct DirectoryListBuilder {
    DirectoryEntryBucket base;

    DirectoryEntryBucket *head;
    DirectoryEntryBucket *tail;

    U32 total;
};

// :OS_FileIO
//
FileScope void OS_DirectoryListBuild(M_Arena *arena, DirectoryListBuilder *builder, Str8 path, B32 recursive);

FileScope void DirectoryListBuilderAppend(DirectoryListBuilder *builder, DirectoryEntry entry) {
    DirectoryEntryBucket *bucket = builder->tail;

    Assert(bucket != 0);

    bucket->entries[bucket->count] = entry;

    bucket->count  += 1;
    builder->total += 1;

    if (bucket->count >= ArraySize(bucket->entries)) {
        M_Arena *temp = M_TempGet();

        DirectoryEntryBucket *next = M_ArenaPush(temp, DirectoryEntryBucket, 1);

        builder->tail->next = next;
        builder->tail = next;
    }
}

FileScope void DirectoryListBuilderFlatten(M_Arena *arena, DirectoryList *list, DirectoryListBuilder *builder) {
    if (builder->total > 0) {
        list->num_entries = builder->total;
        list->entries     = M_ArenaPush(arena, DirectoryEntry, list->num_entries);

        U32 offset = 0;

        DirectoryEntryBucket *bucket = builder->head;
        while (bucket != 0) {
            MemoryCopy(&list->entries[offset], bucket->entries, bucket->count * sizeof(DirectoryEntry));

            offset += bucket->count;
            bucket  = bucket->next;
        }
    }
    else {
        list->num_entries = 0;
        list->entries     = 0;
    }
}

DirectoryList DirectoryListGet(M_Arena *arena, Str8 path, B32 recursive) {
    DirectoryList result = { 0 };

    DirectoryListBuilder builder = { 0 };
    builder.head  = &builder.base;
    builder.tail  = &builder.base;

    builder.total = 0;

    OS_DirectoryListBuild(arena, &builder, path, recursive);

    DirectoryListBuilderFlatten(arena, &result, &builder);

    return result;
}

DirectoryList DirectoryListFilterForFiles(M_Arena *arena, DirectoryList *list) {
    DirectoryList result = { 0 };

    DirectoryListBuilder builder = { 0 };
    builder.head  = &builder.base;
    builder.tail  = &builder.base;

    builder.total = 0;

    for (U32 it = 0; it < list->num_entries; ++it) {
        if (list->entries[it].type == DIRECTORY_ENTRY_TYPE_FILE) {
            DirectoryListBuilderAppend(&builder, list->entries[it]);
        }
    }

    DirectoryListBuilderFlatten(arena, &result, &builder);

    return result;
}

DirectoryList DirectoryListFilterForDirectories(M_Arena *arena, DirectoryList *list) {
    DirectoryList result = { 0 };

    DirectoryListBuilder builder = { 0 };
    builder.head  = &builder.base;
    builder.tail  = &builder.base;

    builder.total = 0;

    for (U32 it = 0; it < list->num_entries; ++it) {
        if (list->entries[it].type == DIRECTORY_ENTRY_TYPE_DIRECTORY) {
            DirectoryListBuilderAppend(&builder, list->entries[it]);
        }
    }

    DirectoryListBuilderFlatten(arena, &result, &builder);

    return result;
}

DirectoryList DirectoryListFilterForPrefix(M_Arena *arena, DirectoryList *list, Str8 prefix) {
    DirectoryList result = { 0 };

    DirectoryListBuilder builder = { 0 };
    builder.head  = &builder.base;
    builder.tail  = &builder.base;

    builder.total = 0;

    for (U32 it = 0; it < list->num_entries; ++it) {
        DirectoryEntry *entry = &list->entries[it];

        Str8 start = Str8_Prefix(entry->basename, prefix.count);

        if ((entry->type == DIRECTORY_ENTRY_TYPE_FILE) && Str8_Equal(prefix, start, 0)) {
            DirectoryListBuilderAppend(&builder, list->entries[it]);
        }
    }

    DirectoryListBuilderFlatten(arena, &result, &builder);

    return result;
}

DirectoryList DirectoryListFilterForSuffix(M_Arena *arena, DirectoryList *list, Str8 suffix) {
    DirectoryList result = { 0 };

    DirectoryListBuilder builder = { 0 };
    builder.head  = &builder.base;
    builder.tail  = &builder.base;

    builder.total = 0;

    for (U32 it = 0; it < list->num_entries; ++it) {
        DirectoryEntry *entry = &list->entries[it];

        Str8 end = Str8_Suffix(entry->basename, suffix.count);

        if ((entry->type == DIRECTORY_ENTRY_TYPE_FILE) && Str8_Equal(suffix, end, 0)) {
            DirectoryListBuilderAppend(&builder, list->entries[it]);
        }
    }

    DirectoryListBuilderFlatten(arena, &result, &builder);

    return result;
}

Str8 FileReadAll(M_Arena *arena, Str8 path) {
    Str8 result = { 0 };

    M_Arena *temp = M_TempGet();

    DirectoryEntry entry;
    if (OS_DirectoryEntryGetByPath(temp, &entry, path)) {
        if (entry.type == DIRECTORY_ENTRY_TYPE_FILE) {
            FileHandle handle = OS_FileOpen(path, FILE_ACCESS_FLAG_READ);

            result.count = entry.size;
            result.data  = M_ArenaPush(arena, U8, result.count);

            OS_FileRead(&handle, result.data, 0, result.count);
            OS_FileClose(&handle);
        }
    }

    return result;
}

Str8 FileReadAllToBuffer(Buffer *buffer, Str8 path) {
    Str8 result = { 0 };

    M_Arena *temp = M_TempGet();

    DirectoryEntry entry;
    if (OS_DirectoryEntryGetByPath(temp, &entry, path)) {
        if (entry.type == DIRECTORY_ENTRY_TYPE_FILE) {
            FileHandle handle = OS_FileOpen(path, FILE_ACCESS_FLAG_READ);

            Assert(buffer->used <= buffer->limit);

            result.count = Min(buffer->limit - buffer->used, cast(S64) entry.size);
            result.data  = buffer->data + buffer->used;

            OS_FileRead(&handle, result.data, 0, result.count);
            OS_FileClose(&handle);

            buffer->used += result.count;
        }
    }

    return result;
}
