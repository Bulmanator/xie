#if !defined(XI_FILEIO_H_)
#define XI_FILEIO_H_

typedef U32 DirectoryEntryType;
enum DirectoryEntryType {
    DIRECTORY_ENTRY_TYPE_FILE = 0,
    DIRECTORY_ENTRY_TYPE_DIRECTORY
};

typedef struct DirectoryEntry DirectoryEntry;
struct DirectoryEntry {
    DirectoryEntryType type;

    U64 size;
    U64 time;

    Str8 path;
    Str8 basename; // with file extension
};

typedef struct DirectoryList DirectoryList;
struct DirectoryList {
    U32 num_entries;
    DirectoryEntry *entries;
};

Func DirectoryList DirectoryListGet(M_Arena *arena, Str8 path, B32 recursive);

// Filter entries in 'list'
//
Func DirectoryList DirectoryListFilterForFiles(M_Arena *arena, DirectoryList *list);
Func DirectoryList DirectoryListFilterForDirectories(M_Arena *arena, DirectoryList *list);
Func DirectoryList DirectoryListFilterForPrefix(M_Arena *arena, DirectoryList *list, Str8 suffix);
Func DirectoryList DirectoryListFilterForSuffix(M_Arena *arena, DirectoryList *list, Str8 prefix);

Func B32 OS_DirectoryEntryGetByPath(M_Arena *arena, DirectoryEntry *entry, Str8 path);

// create delete directories
//
// create directory will return true if the specified path already exists as a directory, otherwise
// if the path specifies a file that already exists it will return false
//
// if the path specifies a basename the function will attempt to create the directory within the working
// directory
//
//
Func B32  OS_DirectoryCreate(Str8 path);
Func void OS_DirectoryDelete(Str8 path);

// open close file handles
//
// @todo: these status values should be more granular... did a read fail because 'invalid offset' or
// 'invalid size' etc..
//
typedef U32 FileHandleStatus;
enum FileHandleStatus {
    FILE_HANDLE_STATUS_VALID = 0,
    FILE_HANDLE_STATUS_FAILED_OPEN,
    FILE_HANDLE_STATUS_FAILED_WRITE,
    FILE_HANDLE_STATUS_FAILED_READ,
    FILE_HANDLE_STATUS_CLOSED
};

typedef U32 FileAccessFlags;
enum FileAccessFlags {
    FILE_ACCESS_FLAG_READ      = (1 << 0),
    FILE_ACCESS_FLAG_WRITE     = (1 << 1),
    FILE_ACCESS_FLAG_READWRITE = (FILE_ACCESS_FLAG_READ | FILE_ACCESS_FLAG_WRITE)
};

typedef struct FileHandle FileHandle;
struct FileHandle {
    FileHandleStatus status;
    U64 v;
};

// specify this for the 'offset' parameter to file write function to append to the end of the file,
//
#define FILE_OFFSET_APPEND ((U64) -1)

// opening a non-existent file as 'write' will create the file automatically, attempting to open a
// non-existent file as 'read' will result in an error
//
Func FileHandle OS_FileOpen(Str8 path, FileAccessFlags access);
Func void OS_FileClose(FileHandle *handle);

Func void OS_FileDelete(Str8 path);

Func B32 OS_FileRead(FileHandle *handle, void *dst, U64 offset, U64 size);
Func B32 OS_FileWrite(FileHandle *handle, void *src, U64 offset, U64 size);

// resulting string will be zero sized if the file fails to open the file
//
// if the buffer doesn't have enough space, the only the remaining size will be loaded
//
Func Str8 FileReadAll(M_Arena *arena, Str8 path);
Func Str8 FileReadAllToBuffer(Buffer *buffer, Str8 path);

#endif  // XI_FILEIO_H_
