#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

uptr xi_os_allocation_granularity_get() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    uptr result = info.dwAllocationGranularity;
    return result;
}

uptr xi_os_page_size_get() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    uptr result = info.dwPageSize;
    return result;
}

void *xi_os_virtual_memory_reserve(uptr size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
    return result;
}

b32 xi_os_virtual_memory_commit(void *base, uptr size) {
    b32 result = VirtualAlloc(base, size, MEM_COMMIT, PAGE_READWRITE) != 0;
    return result;
}

void xi_os_virtual_memory_decommit(void *base, uptr size) {
    VirtualFree(base, size, MEM_DECOMMIT);
}

void xi_os_virtual_memory_release(void *base, uptr size) {
    (void) size; // :note not needed by VirtualFree when releasing memory

    VirtualFree(base, 0, MEM_RELEASE);
}
