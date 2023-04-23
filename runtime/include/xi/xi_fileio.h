#if !defined(XI_FILEIO_H_)
#define XI_FILEIO_H_

enum xiDirectoryEntryType {
    XI_DIRECTORY_ENTRY_TYPE_FILE = 0,
    XI_DIRECTORY_ENTRY_TYPE_DIRECTORY
};

typedef struct xiDirectoryEntry {
    xi_u32 type;
    xi_u64 size;
    xi_u64 time;

    xi_string path;     // full path to entry
    xi_string basename; // name of the base path segment without file extension
} xiDirectoryEntry;

typedef struct xiDirectoryList {
    xi_u32 count;
    xiDirectoryEntry *entries;
} xiDirectoryList;

extern XI_API void xi_directory_list_get(xiArena *arena, xiDirectoryList *list, xi_string path, xi_b32 recursive);

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
        xiDirectoryList *out, xiDirectoryList *list, xi_string prefix);

// filter for files with a specific suffix in the basename in the supplied 'list', putting the
// the resulting entries in 'out'
//
extern XI_API void xi_directory_list_filter_for_suffix(xiArena *arena,
        xiDirectoryList *out, xiDirectoryList *list, xi_string suffix);

// filter for files with a specific extension in the path in the supplied 'list', putting the
// the resulting entries in 'out'. this differs from suffix above as it is applied to the path not the
// basename of the directory entry
//
extern XI_API void xi_directory_list_filter_for_extension(xiArena *arena,
        xiDirectoryList *out, xiDirectoryList *list, xi_string extension);

// the entry have the fully qualified path after which will be allocated in the arena provided
//
extern XI_API xi_b32 xi_os_directory_entry_get_by_path(xiArena *arena, xiDirectoryEntry *entry, xi_string path);

// create delete directories
//
// create directory will return true if the specified path already exists as a directory, otherwise
// if the path specifies a file that already exists it will return false
//
// if the path specifies a basename the function will attempt to create the directory within the working
// directory
//
//
extern XI_API xi_b32 xi_os_directory_create(xi_string path);
extern XI_API void xi_os_directory_delete(xi_string path);

// open close file handles
//
// @todo: these status values should be more granular... did a read fail because 'invalid offset' or
// 'invalid size' etc..
//
enum xiFileHandleStatus {
    XI_FILE_HANDLE_STATUS_VALID = 0,
    XI_FILE_HANDLE_STATUS_FAILED_OPEN,
    XI_FILE_HANDLE_STATUS_FAILED_WRITE,
    XI_FILE_HANDLE_STATUS_FAILED_READ,
    XI_FILE_HANDLE_STATUS_CLOSED
};

enum xiFileAccessFlags {
    XI_FILE_ACCESS_FLAG_READ      = (1 << 0),
    XI_FILE_ACCESS_FLAG_WRITE     = (1 << 1),
    XI_FILE_ACCESS_FLAG_READWRITE = (XI_FILE_ACCESS_FLAG_READ | XI_FILE_ACCESS_FLAG_WRITE)
};

typedef struct xiFileHandle {
    xi_u32 status;
    void *os;
} xiFileHandle;

// specify this for the 'offset' parameter to file write function to append to the end of the file,
//
#define XI_FILE_OFFSET_APPEND ((xi_uptr) -1)

// opening a non-existent file as 'write' will create the file automatically, attempting to open a
// non-existent file as 'read' will result in an error
//
extern XI_API xi_b32 xi_os_file_open(xiFileHandle *handle, xi_string path, xi_u32 access);
extern XI_API void xi_os_file_close(xiFileHandle *handle);

extern XI_API void xi_os_file_delete(xi_string path);

extern XI_API xi_b32 xi_os_file_read(xiFileHandle *handle, void *dst, xi_uptr offset, xi_uptr size);
extern XI_API xi_b32 xi_os_file_write(xiFileHandle *handle, void *src, xi_uptr offset, xi_uptr size);

// resulting string will be zero sized if the file fails to open the file
//
// if the buffer doesn't have enough space, the only the remaining size will be loaded
//
extern XI_API xi_string xi_file_read_all(xiArena *arena, xi_string path);
extern XI_API xi_string xi_file_read_all_buffer(xi_buffer *b, xi_string path);

#endif  // XI_FILEIO_H_
