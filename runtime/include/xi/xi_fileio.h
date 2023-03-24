#if !defined(XI_FILEIO_H_)
#define XI_FILEIO_H_

enum xiDirectoryEntryType {
    XI_DIRECTORY_ENTRY_TYPE_FILE = 0,
    XI_DIRECTORY_ENTRY_TYPE_DIRECTORY
};

typedef struct xiDirectoryEntry {
    u32 type;
    u64 size;
    u64 time;

    string path;     // full path to entry
    string basename; // name of the base path segment without file extension
} xiDirectoryEntry;

typedef struct xiDirectoryList {
    u32 count;
    xiDirectoryEntry *entries;
} xiDirectoryList;

extern XI_API void xi_directory_list_get(xiArena *arena, xiDirectoryList *list, string path, b32 recursive);

// filter for only files in the supplied 'list', putting the resulting entries in 'out'
//
extern XI_API void xi_directory_list_filter_for_files(xiArena *arena,
        xiDirectoryList *out, xiDirectoryList *list);

// filter for directories only in the supplied 'list', putting the resulting entries in 'out'
//
extern XI_API void xi_directory_list_filter_for_directories(xiArena *arena,
        xiDirectoryList *out, xiDirectoryList *list);

// filter for files with a specific prefix in the filename in the supplied 'list', putting the
// the resulting entries in 'out'
//
extern XI_API void xi_directory_list_filter_for_prefix(xiArena *arena,
        xiDirectoryList *out, xiDirectoryList *list, string prefix);

// filter for files with a specific suffix in the basename in the supplied 'list', putting the
// the resulting entries in 'out'
//
extern XI_API void xi_directory_list_filter_for_suffix(xiArena *arena,
        xiDirectoryList *out, xiDirectoryList *list, string suffix);

// filter for files with a specific extension in the path in the supplied 'list', putting the
// the resulting entries in 'out'. this differs from suffix above as it is applied to the path not the
// basename of the directory entry
//
extern XI_API void xi_directory_list_filter_for_extension(xiArena *arena,
        xiDirectoryList *out, xiDirectoryList *list, string extension);

// the entry have the fully qualified path after which will be allocated in the arena provided
//
extern XI_API b32 xi_os_directory_entry_get_by_path(xiArena *arena, xiDirectoryEntry *entry, string path);

// create delete directories
//
// create directory will return true if the specified path already exists as a directory, otherwise
// if the path specifies a file that already exists it will return false
//
// if the path specifies a basename the function will attempt to create the directory within the working
// directory
//
//
extern XI_API b32  xi_os_directory_create(string path);
extern XI_API void xi_os_directory_delete(string path);

// open close file handles
//
enum xiFileAccessFlags {
    XI_FILE_ACCESS_FLAG_READ  = (1 << 0),
    XI_FILE_ACCESS_FLAG_WRITE = (1 << 1)
};

typedef struct xiFileHandle {
    b32   valid;
    void *os;
} xiFileHandle;

// opening a non-existent file as 'write' will create the file automatically, attempting to open a
// non-existent file as 'read' will result in an error
//
extern XI_API b32  xi_os_file_open(xiFileHandle *handle, string path, u32 access);
extern XI_API void xi_os_file_close(xiFileHandle *handle);

extern XI_API void xi_os_file_delete(string path);

extern XI_API b32 xi_os_file_read(xiFileHandle *handle, void *dst, uptr offset, uptr size);
extern XI_API b32 xi_os_file_write(xiFileHandle *handle, void *src, uptr offset, uptr size);

// resulting string will be zero sized if the file fails to open the file
//
// if the buffer doesn't have enough space, the only the remaining size will be loaded
//
extern XI_API string xi_file_read_all(xiArena *arena, string path);
extern XI_API string xi_file_read_all_buffer(buffer *b, string path);

#endif  // XI_FILEIO_H_
