/* Native Windows host services for MSYS2/MinGW. Ada byte strings use the
   process ANSI code page, as do the CRT file APIs used by Ada.Text_IO. All
   handles/snapshots are owned by one call or by a limited Search_Guard. */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include "adart.h"
#include "adahost.h"
#include "adacalendar.h"
#include <windows.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define HOST_SHARE (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
#define WINDOWS_EPOCH_TICKS UINT64_C(116444736000000000)

static void hostError(DWORD error)
{
    switch (error) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_NAME:
    case ERROR_BAD_PATHNAME:
    case ERROR_INVALID_DRIVE:
    case ERROR_FILENAME_EXCED_RANGE:
    case ERROR_DIRECTORY:
        __ada_raise(ADA_NAME_ERROR);
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:
        __ada_raise(ADA_STORAGE_ERROR);
        break;
    case ERROR_READ_FAULT:
    case ERROR_WRITE_FAULT:
    case ERROR_CRC:
    case ERROR_IO_DEVICE:
        __ada_raise(ADA_DEVICE_ERROR);
        break;
    default:
        __ada_raise(ADA_USE_ERROR);
        break;
    }
}

static int missing(DWORD error)
{
    return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND || error == ERROR_DIRECTORY;
}

static wchar_t* allocateWide(size_t length)
{
    if (length > SIZE_MAX / sizeof(wchar_t) - 1) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    wchar_t* result = malloc((length + 1) * sizeof *result);
    if (result == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
    }
    return result;
}

static wchar_t* wideCopy(const wchar_t* text, size_t length)
{
    wchar_t* result = allocateWide(length);
    if (result != NULL) {
        memcpy(result, text, length * sizeof *result);
        result[length] = 0;
    }
    return result;
}

static wchar_t* wideString(const char* text, int length, int allowEmpty, const AdaException* error)
{
    if (length < 0 || (!allowEmpty && length == 0) || (length > 0 && memchr(text, 0, (size_t)length))) {
        __ada_raise(error);
        return NULL;
    }
    if (length == 0) {
        return wideCopy(L"", 0);
    }
    int count = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, text, length, NULL, 0);
    if (count == 0) {
        __ada_raise(error);
        return NULL;
    }
    wchar_t* result = allocateWide((size_t)count);
    if (result != NULL) {
        if (MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, text, length, result, count) == 0) {
            free(result);
            __ada_raise(error);
            return NULL;
        }
        result[count] = 0;
    }
    return result;
}

static char* narrowString(const wchar_t* text)
{
    /* Best-fit substitutions would change the identity of a path or variable.
       UTF-8's conversion flags differ from legacy ANSI code pages. */
    BOOL substituted = FALSE;
    int utf8 = GetACP() == CP_UTF8;
    DWORD flags = utf8 ? WC_ERR_INVALID_CHARS : WC_NO_BEST_FIT_CHARS;
    BOOL* usedDefault = utf8 ? NULL : &substituted;
    int count = WideCharToMultiByte(CP_ACP, flags, text, -1, NULL, 0, NULL, usedDefault);
    if (count == 0 || substituted) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return NULL;
    }
    char* result = malloc((size_t)count);
    if (result == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    if (WideCharToMultiByte(CP_ACP, flags, text, -1, result, count, NULL, usedDefault) == 0 || substituted) {
        free(result);
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return NULL;
    }
    return result;
}

static void stringResult(void* result, const wchar_t* text)
{
    char* value = narrowString(text);
    if (value != NULL) {
        __ada_array_result(result, value, 1, (int64_t)strlen(value), 1);
        free(value);
    }
}

static wchar_t* environmentName(const char* name, int length)
{
    if (length <= 0 || memchr(name, '=', (size_t)length)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return NULL;
    }
    return wideString(name, length, 0, ADA_CONSTRAINT_ERROR);
}

void __ada_host_env_value(void* result, const char* name, int length)
{
    wchar_t* key = environmentName(name, length);
    if (key == NULL) {
        return;
    }
    DWORD capacity = 1;
    for (;;) {
        wchar_t* value = allocateWide(capacity - 1);
        if (value == NULL) {
            break;
        }
        SetLastError(ERROR_SUCCESS);
        DWORD count = GetEnvironmentVariableW(key, value, capacity);
        DWORD error = GetLastError();
        if (count == 0) {
            if (error == ERROR_SUCCESS) {
                stringResult(result, L"");
            } else {
                __ada_raise(error == ERROR_NOT_ENOUGH_MEMORY ? ADA_STORAGE_ERROR : ADA_CONSTRAINT_ERROR);
            }
        } else if (count < capacity) {
            stringResult(result, value);
        }
        free(value);
        if (count < capacity) {
            break;
        }
        capacity = count;
    }
    free(key);
}

int __ada_host_env_exists(const char* name, int length)
{
    if (length <= 0 || memchr(name, '=', (size_t)length) || memchr(name, 0, (size_t)length)) {
        return 0;
    }
    wchar_t* key = environmentName(name, length);
    if (key == NULL) {
        return 0;
    }
    wchar_t value;
    SetLastError(ERROR_SUCCESS);
    DWORD count = GetEnvironmentVariableW(key, &value, 1);
    DWORD error = GetLastError();
    free(key);
    if (count != 0 || error == ERROR_SUCCESS) {
        return 1;
    }
    if (error != ERROR_ENVVAR_NOT_FOUND) {
        hostError(error);
    }
    return 0;
}

static void setEnvironment(const wchar_t* name, const wchar_t* value)
{
    /* Keep MinGW's CRT environment in step for getenv/_spawn callers. The
       CRT removes empty values, so publish the native empty value afterwards.
       As on POSIX, callers must serialize mutations against readers. */
    int status = _wputenv_s(name, value == NULL ? L"" : value);
    if (status != 0) {
        __ada_raise(status == ENOMEM ? ADA_STORAGE_ERROR : ADA_CONSTRAINT_ERROR);
        return;
    }
    if (!SetEnvironmentVariableW(name, value)) {
        DWORD error = GetLastError();
        if (value != NULL || error != ERROR_ENVVAR_NOT_FOUND) {
            __ada_raise(error == ERROR_NOT_ENOUGH_MEMORY ? ADA_STORAGE_ERROR : ADA_CONSTRAINT_ERROR);
        }
    }
}

void __ada_host_env_set(const char* name, int length, const char* value, int valueLength)
{
    wchar_t* key = environmentName(name, length);
    if (key == NULL) {
        return;
    }
    wchar_t* text = wideString(value, valueLength, 1, ADA_CONSTRAINT_ERROR);
    if (text != NULL) {
        setEnvironment(key, text);
    }
    free(text);
    free(key);
}

void __ada_host_env_clear(const char* name, int length)
{
    wchar_t* key = environmentName(name, length);
    if (key != NULL) {
        setEnvironment(key, NULL);
        free(key);
    }
}

static wchar_t* environmentBlock(void)
{
    wchar_t* block = GetEnvironmentStringsW();
    if (block == NULL) {
        hostError(GetLastError());
    }
    return block;
}

void __ada_host_env_clear_all(void)
{
    wchar_t* block = environmentBlock();
    if (block == NULL) {
        return;
    }
    for (const wchar_t* entry = block; *entry; entry += wcslen(entry) + 1) {
        /* =C: entries are Windows' private per-drive working directories,
           not user environment variables; they cannot be named by this API. */
        if (*entry == L'=') {
            continue;
        }
        const wchar_t* equal = wcschr(entry, L'=');
        if (equal == NULL) {
            __ada_raise(ADA_PROGRAM_ERROR);
            break;
        }
        wchar_t* key = wideCopy(entry, (size_t)(equal - entry));
        if (key == NULL) {
            break;
        }
        setEnvironment(key, NULL);
        free(key);
        if (__ada_task_context()->exception != NULL) {
            break;
        }
    }
    FreeEnvironmentStringsW(block);
}

int __ada_host_env_count(void)
{
    wchar_t* block = environmentBlock();
    if (block == NULL) {
        return 0;
    }
    int count = 0;
    for (const wchar_t* entry = block; *entry; entry += wcslen(entry) + 1) {
        if (*entry != L'=') {
            if (count == INT_MAX) {
                __ada_raise(ADA_CONSTRAINT_ERROR);
                break;
            }
            ++count;
        }
    }
    FreeEnvironmentStringsW(block);
    return count;
}

static void environmentEntry(void* result, int index, int value)
{
    wchar_t* block = environmentBlock();
    if (block == NULL) {
        return;
    }
    int found = 0;
    for (wchar_t* entry = block; *entry; entry += wcslen(entry) + 1) {
        if (*entry == L'=' || index < 0) {
            continue;
        }
        if (index-- == 0) {
            wchar_t* equal = wcschr(entry, L'=');
            if (value) {
                stringResult(result, equal == NULL ? L"" : equal + 1);
            } else {
                wchar_t* key = wideCopy(entry, equal == NULL ? wcslen(entry) : (size_t)(equal - entry));
                if (key != NULL) {
                    stringResult(result, key);
                    free(key);
                }
            }
            found = 1;
            break;
        }
    }
    FreeEnvironmentStringsW(block);
    if (!found) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
}

void __ada_host_env_name(void* result, int index)
{
    environmentEntry(result, index, 0);
}

void __ada_host_env_entry_value(void* result, int index)
{
    environmentEntry(result, index, 1);
}

static int separator(wchar_t value)
{
    return value == L'\\' || value == L'/';
}

static int driveLetter(wchar_t value)
{
    return (value >= L'a' && value <= L'z') || (value >= L'A' && value <= L'Z');
}

/* The lexical root includes the UNC server/share, a drive-root separator,
   or a leading current-drive separator. Drive-relative C: has a two-byte root. */
static size_t rootLength(const wchar_t* text)
{
    if (driveLetter(text[0]) && text[1] == L':') {
        return separator(text[2]) ? 3 : 2;
    }
    if (!separator(text[0])) {
        return 0;
    }
    if (!separator(text[1])) {
        return 1;
    }
    size_t end = 2;
    while (text[end] && !separator(text[end])) {
        ++end;
    }
    if (text[end]) {
        ++end;
    }
    while (text[end] && !separator(text[end])) {
        ++end;
    }
    return text[end] ? end + 1 : end;
}

static int deviceComponent(const wchar_t* text, size_t length)
{
    if (length >= 2 && driveLetter(text[0]) && text[1] == L':') {
        text += 2;
        length -= 2;
    }
    size_t base = 0;
    while (base < length && text[base] != L'.') {
        ++base;
    }
    static const wchar_t* const kDevices[] = {L"CON", L"PRN", L"AUX", L"NUL", L"CLOCK$", L"CONIN$", L"CONOUT$"};
    for (size_t i = 0; i < sizeof kDevices / sizeof kDevices[0]; ++i) {
        if (base == wcslen(kDevices[i]) && CompareStringOrdinal(text, (int)base, kDevices[i], (int)base, TRUE) == CSTR_EQUAL) {
            return 1;
        }
    }
    if (base == 4 && (CompareStringOrdinal(text, 3, L"COM", 3, TRUE) == CSTR_EQUAL
        || CompareStringOrdinal(text, 3, L"LPT", 3, TRUE) == CSTR_EQUAL)) {
        wchar_t digit = text[3];
        return (digit >= L'1' && digit <= L'9') || digit == 0x00b9 || digit == 0x00b2 || digit == 0x00b3;
    }
    return 0;
}

static wchar_t* pathString(const char* name, int length, int allowEmpty)
{
    wchar_t* text = wideString(name, length, allowEmpty, ADA_NAME_ERROR);
    if (text == NULL) {
        return NULL;
    }
    /* File namespaces/alternate data streams are not Ada directory names.
       Internally generated extended-length paths are handled separately. */
    if (separator(text[0]) && separator(text[1]) && (text[2] == L'?' || text[2] == L'.') && separator(text[3])) {
        free(text);
        __ada_raise(ADA_NAME_ERROR);
        return NULL;
    }
    for (size_t i = 0; text[i]; ++i) {
        if (text[i] < 32 || wcschr(L"<>\"|?*", text[i]) != NULL
            || (text[i] == L':' && !(i == 1 && driveLetter(text[0])))) {
            free(text);
            __ada_raise(ADA_NAME_ERROR);
            return NULL;
        }
        if (text[i] == L'/') {
            text[i] = L'\\';
        }
    }
    int unc = separator(text[0]) && separator(text[1]);
    int component = 0;
    for (size_t start = 0; text[start];) {
        size_t end = start;
        while (text[end] && !separator(text[end])) {
            ++end;
        }
        size_t size = end - start;
        size_t base = start;
        if (start == 0 && size >= 2 && driveLetter(text[0]) && text[1] == L':') {
            base += 2;
        }
        size_t partSize = end - base;
        int dots = (partSize == 1 && text[base] == L'.')
            || (partSize == 2 && text[base] == L'.' && text[base + 1] == L'.');
        if (size) {
            ++component;
        }
        int device = size && (!unc || component > 2) && deviceComponent(text + start, size);
        if (device || (size && !dots && (text[end - 1] == L'.' || text[end - 1] == L' '))) {
            free(text);
            __ada_raise(ADA_NAME_ERROR);
            return NULL;
        }
        start = text[end] ? end + 1 : end;
    }
    if (text[0] == L'\\' && text[1] == L'\\') {
        /* A UNC name needs both a server and a share. */
        const wchar_t* share = wcschr(text + 2, L'\\');
        if (share == NULL || share == text + 2 || share[1] == 0 || share[1] == L'\\') {
            free(text);
            __ada_raise(ADA_NAME_ERROR);
            return NULL;
        }
    }
    return text;
}

static wchar_t* plainPath(const wchar_t* text)
{
    if (wcsncmp(text, L"\\\\?\\UNC\\", 8) == 0) {
        size_t length = wcslen(text + 8);
        wchar_t* result = allocateWide(length + 2);
        if (result != NULL) {
            wcscpy(result, L"\\\\");
            wcscpy(result + 2, text + 8);
        }
        return result;
    }
    if (wcsncmp(text, L"\\\\?\\", 4) == 0) {
        text += 4;
    }
    return wideCopy(text, wcslen(text));
}

static wchar_t* fullPath(const wchar_t* text)
{
    DWORD capacity = GetFullPathNameW(text, 0, NULL, NULL);
    if (capacity == 0) {
        hostError(GetLastError());
        return NULL;
    }
    for (;;) {
        wchar_t* result = allocateWide(capacity - 1);
        if (result == NULL) {
            return NULL;
        }
        DWORD count = GetFullPathNameW(text, capacity, result, NULL);
        if (count == 0) {
            DWORD error = GetLastError();
            free(result);
            hostError(error);
            return NULL;
        }
        if (count < capacity) {
            wchar_t* plain = plainPath(result);
            free(result);
            return plain;
        }
        free(result);
        capacity = count;
    }
}

static wchar_t* nativePath(const wchar_t* full)
{
    size_t length = wcslen(full);
    if (length > 32759) {
        __ada_raise(ADA_NAME_ERROR);
        return NULL;
    }
    int unc = full[0] == L'\\' && full[1] == L'\\';
    wchar_t* result = allocateWide(length + (unc ? 6 : 4));
    if (result != NULL) {
        wcscpy(result, unc ? L"\\\\?\\UNC\\" : L"\\\\?\\");
        wcscat(result, full + (unc ? 2 : 0));
    }
    return result;
}

static wchar_t* fileName(const char* name, int length)
{
    wchar_t* text = pathString(name, length, 0);
    if (text == NULL) {
        return NULL;
    }
    wchar_t* full = fullPath(text);
    free(text);
    if (full == NULL) {
        return NULL;
    }
    size_t root = rootLength(full);
    size_t size = wcslen(full);
    while (size > root && separator(full[size - 1])) {
        full[--size] = 0;
    }
    wchar_t* result = nativePath(full);
    free(full);
    return result;
}

static void pathResult(void* result, const wchar_t* text)
{
    wchar_t* plain = plainPath(text);
    if (plain != NULL) {
        for (wchar_t* p = plain; *p; ++p) {
            if (*p == L'\\') {
                *p = L'/';
            }
        }
        stringResult(result, plain);
        free(plain);
    }
}

void __ada_host_current_directory(void* result)
{
    DWORD capacity = GetCurrentDirectoryW(0, NULL);
    if (capacity == 0) {
        __ada_raise(ADA_USE_ERROR);
        return;
    }
    for (;;) {
        wchar_t* path = allocateWide(capacity - 1);
        if (path == NULL) {
            return;
        }
        DWORD count = GetCurrentDirectoryW(capacity, path);
        if (count == 0) {
            free(path);
            __ada_raise(ADA_USE_ERROR);
            return;
        }
        if (count < capacity) {
            pathResult(result, path);
            free(path);
            return;
        }
        free(path);
        capacity = count;
    }
}

void __ada_host_full_name(void* result, const char* name, int length)
{
    wchar_t* text = pathString(name, length, 0);
    if (text != NULL) {
        wchar_t* full = fullPath(text);
        if (full != NULL) {
            pathResult(result, full);
            free(full);
        }
        free(text);
    }
}

static void namePart(void* result, const char* name, int length, int operation)
{
    wchar_t* text = pathString(name, length, 0);
    if (text == NULL) {
        return;
    }
    size_t root = rootLength(text);
    size_t end = wcslen(text);
    while (end > root && separator(text[end - 1])) {
        text[--end] = 0;
    }
    size_t start = end;
    while (start > root && !separator(text[start - 1])) {
        --start;
    }
    wchar_t* simple = text + start;
    wchar_t* dot = wcsrchr(simple, L'.');
    if (dot == simple || wcscmp(simple, L"..") == 0) {
        dot = NULL;
    }
    const wchar_t* value = simple;
    if (operation == 1) {
        if (end == root) {
            __ada_raise(ADA_USE_ERROR);
            free(text);
            return;
        }
        if (start == 0) {
            value = L".";
        } else {
            size_t containing = start;
            while (containing > root && separator(text[containing - 1])) {
                --containing;
            }
            text[containing] = 0;
            value = text;
        }
    } else if (operation == 2) {
        value = dot == NULL ? L"" : dot + 1;
    } else if (operation == 3 && dot != NULL) {
        *dot = 0;
    }
    if ((operation == 0 || operation == 3) && end == root) {
        value = text;
    }
    pathResult(result, value);
    free(text);
}

void __ada_host_simple_name(void* result, const char* name, int length)
{
    namePart(result, name, length, 0);
}

void __ada_host_containing_directory(void* result, const char* name, int length)
{
    namePart(result, name, length, 1);
}

void __ada_host_extension(void* result, const char* name, int length)
{
    namePart(result, name, length, 2);
}

void __ada_host_base_name(void* result, const char* name, int length)
{
    namePart(result, name, length, 3);
}

void __ada_host_compose(void* result, const char* directory, int directoryLength,
    const char* name, int nameLength, const char* extension, int extensionLength)
{
    wchar_t* dir = pathString(directory, directoryLength, 1);
    if (dir == NULL) {
        return;
    }
    wchar_t* text = pathString(name, nameLength, 0);
    wchar_t* ext = text == NULL ? NULL : pathString(extension, extensionLength, 1);
    if (text != NULL && ext != NULL) {
        if (wcspbrk(text, L"\\/:") || wcspbrk(ext, L"\\/:.")) {
            __ada_raise(ADA_NAME_ERROR);
        } else {
            size_t dirSize = wcslen(dir);
            size_t size = dirSize + wcslen(text) + wcslen(ext) + 2;
            wchar_t* value = allocateWide(size);
            if (value != NULL) {
                wcscpy(value, dir);
                /* C:name is drive-relative, whereas C:\\name is absolute. */
                if (dirSize && !separator(dir[dirSize - 1]) && dir[dirSize - 1] != L':') {
                    wcscat(value, L"\\");
                }
                wcscat(value, text);
                if (*ext) {
                    wcscat(value, L".");
                    wcscat(value, ext);
                }
                pathResult(result, value);
                free(value);
            }
        }
    }
    free(ext);
    free(text);
    free(dir);
}

typedef struct HostInfo
{
    BY_HANDLE_FILE_INFORMATION file;
    DWORD type;
} HostInfo;

static int handleInfo(HANDLE handle, HostInfo* info)
{
    memset(info, 0, sizeof *info);
    SetLastError(ERROR_SUCCESS);
    info->type = GetFileType(handle);
    if (info->type != FILE_TYPE_DISK) {
        if (info->type == FILE_TYPE_UNKNOWN && GetLastError() != ERROR_SUCCESS) {
            hostError(GetLastError());
            return 0;
        }
        return 1;
    }
    if (!GetFileInformationByHandle(handle, &info->file)) {
        hostError(GetLastError());
        return 0;
    }
    return 1;
}

static int fileInfo(const wchar_t* path, HostInfo* info, int allowMissing)
{
    HANDLE handle = CreateFileW(path, FILE_READ_ATTRIBUTES, HOST_SHARE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (!allowMissing || !missing(error)) {
            hostError(error);
        }
        return 0;
    }
    int valid = handleInfo(handle, info);
    CloseHandle(handle);
    return valid;
}

static int fileKind(const HostInfo* info)
{
    return info->type != FILE_TYPE_DISK ? 2 : info->file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? 0 : 1;
}

static int64_t fileSize(const HostInfo* info)
{
    uint64_t size = ((uint64_t)info->file.nFileSizeHigh << 32) | info->file.nFileSizeLow;
    if (size > INT64_MAX) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    return (int64_t)size;
}

static int modificationTime(const HostInfo* info, int* year, int* month, int* day, int64_t* seconds)
{
    if (info->type != FILE_TYPE_DISK) {
        return 0;
    }
    FILETIME time = info->file.ftLastWriteTime;
    uint64_t ticks = ((uint64_t)time.dwHighDateTime << 32) | time.dwLowDateTime;
    __int128 delta = (__int128)ticks - WINDOWS_EPOCH_TICKS;
    /* Floor negative timestamps too: FILETIME's epoch predates Unix. */
    int64_t micros = (int64_t)(delta >= 0 ? delta / 10 : -((-delta + 9) / 10));
    int valid;
    __ada_calendar_split(micros, year, month, day, seconds, &valid);
    return valid;
}

int __ada_host_exists(const char* name, int length)
{
    wchar_t* path = fileName(name, length);
    if (path == NULL) {
        return 0;
    }
    HostInfo info;
    int found = fileInfo(path, &info, 1);
    free(path);
    return found;
}

int __ada_host_kind(const char* name, int length)
{
    wchar_t* path = fileName(name, length);
    if (path == NULL) {
        return 0;
    }
    HostInfo info;
    int kind = fileInfo(path, &info, 0) ? fileKind(&info) : 0;
    free(path);
    return kind;
}

int64_t __ada_host_size(const char* name, int length)
{
    wchar_t* path = fileName(name, length);
    if (path == NULL) {
        return 0;
    }
    HostInfo info;
    int64_t size = fileInfo(path, &info, 0) ? fileSize(&info) : 0;
    free(path);
    return size;
}

void __ada_host_modification_time(const char* name, int length, int* year, int* month, int* day, int64_t* seconds)
{
    wchar_t* path = fileName(name, length);
    if (path != NULL) {
        HostInfo info;
        if (fileInfo(path, &info, 0) && !modificationTime(&info, year, month, day, seconds)) {
            __ada_raise(ADA_USE_ERROR);
        }
        free(path);
    }
}

void __ada_host_set_directory(const char* name, int length)
{
    wchar_t* text = pathString(name, length, 0);
    if (text == NULL) {
        return;
    }
    wchar_t* full = fullPath(text);
    free(text);
    if (full == NULL) {
        return;
    }
    /* Keep ordinary working directories in the ordinary Win32 namespace.
       Publishing an extended prefix changes subsequent foreign/CRT full-path
       results too; reserve it for directories that actually need it. */
    wchar_t* extended = wcslen(full) >= MAX_PATH - 1 ? nativePath(full) : NULL;
    if (__ada_task_context()->exception == NULL && !SetCurrentDirectoryW(extended != NULL ? extended : full)) {
        hostError(GetLastError());
    }
    free(extended);
    free(full);
}

static void createDirectory(const char* name, int length, int parents, int formLength)
{
    if (formLength != 0) {
        __ada_raise(ADA_USE_ERROR);
        return;
    }
    wchar_t* text = pathString(name, length, 0);
    if (text == NULL) {
        return;
    }
    wchar_t* full = fullPath(text);
    free(text);
    if (full == NULL) {
        return;
    }
    size_t root = rootLength(full);
    size_t size = wcslen(full);
    while (size > root && separator(full[size - 1])) {
        full[--size] = 0;
    }
    if (parents && size == root) {
        wchar_t* path = nativePath(full);
        HostInfo info;
        if (path != NULL) {
            if (fileInfo(path, &info, 0) && fileKind(&info) != 0) {
                __ada_raise(ADA_NAME_ERROR);
            }
            free(path);
        }
        free(full);
        return;
    }
    for (size_t i = root; i <= size; ++i) {
        if (full[i] != 0 && (!parents || !separator(full[i]))) {
            continue;
        }
        if (i == root && i != size) {
            continue;
        }
        wchar_t saved = full[i];
        full[i] = 0;
        wchar_t* path = nativePath(full);
        if (path == NULL) {
            break;
        }
        if (!CreateDirectoryW(path, NULL)) {
            DWORD error = GetLastError();
            HostInfo info;
            int existing = parents && error == ERROR_ALREADY_EXISTS && fileInfo(path, &info, 0) && fileKind(&info) == 0;
            if (!existing && __ada_task_context()->exception == NULL) {
                hostError(error);
            }
        }
        free(path);
        full[i] = saved;
        if (__ada_task_context()->exception != NULL) {
            break;
        }
    }
    free(full);
}

void __ada_host_create_directory(const char* name, int length, const char* form, int formLength)
{
    (void)form;
    createDirectory(name, length, 0, formLength);
}

void __ada_host_create_path(const char* name, int length, const char* form, int formLength)
{
    (void)form;
    createDirectory(name, length, 1, formLength);
}

void __ada_host_delete_directory(const char* name, int length)
{
    wchar_t* path = fileName(name, length);
    if (path == NULL) {
        return;
    }
    DWORD attributes = GetFileAttributesW(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        hostError(GetLastError());
    } else if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        __ada_raise(ADA_NAME_ERROR);
    } else if (!RemoveDirectoryW(path)) {
        hostError(GetLastError());
    }
    free(path);
}

void __ada_host_delete_file(const char* name, int length)
{
    wchar_t* path = fileName(name, length);
    if (path == NULL) {
        return;
    }
    DWORD attributes = GetFileAttributesW(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        hostError(GetLastError());
    } else if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
        __ada_raise(ADA_NAME_ERROR);
    } else if (!DeleteFileW(path)) {
        hostError(GetLastError());
    }
    free(path);
}

void __ada_host_rename(const char* oldName, int oldLength, const char* newName, int newLength)
{
    wchar_t* source = fileName(oldName, oldLength);
    if (source == NULL) {
        return;
    }
    wchar_t* target = fileName(newName, newLength);
    if (target != NULL && !MoveFileExW(source, target, 0)) {
        DWORD error = GetLastError();
        if (GetFileAttributesW(source) == INVALID_FILE_ATTRIBUTES) {
            hostError(GetLastError());
        } else {
            __ada_raise(error == ERROR_IO_DEVICE ? ADA_DEVICE_ERROR : ADA_USE_ERROR);
        }
    }
    free(target);
    free(source);
}

void __ada_host_copy_file(const char* sourceName, int sourceLength, const char* targetName, int targetLength,
    const char* form, int formLength)
{
    (void)form;
    if (formLength != 0) {
        __ada_raise(ADA_USE_ERROR);
        return;
    }
    wchar_t* source = fileName(sourceName, sourceLength);
    if (source == NULL) {
        return;
    }
    wchar_t* target = fileName(targetName, targetLength);
    HANDLE input = INVALID_HANDLE_VALUE;
    HANDLE output = INVALID_HANDLE_VALUE;
    if (target != NULL) {
        input = CreateFileW(source, GENERIC_READ, HOST_SHARE, NULL, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (input == INVALID_HANDLE_VALUE) {
            hostError(GetLastError());
        } else {
            HostInfo from;
            if (handleInfo(input, &from)) {
                if (fileKind(&from) != 1) {
                    __ada_raise(fileKind(&from) == 0 ? ADA_NAME_ERROR : ADA_USE_ERROR);
                } else {
                    output = CreateFileW(target, GENERIC_WRITE | FILE_READ_ATTRIBUTES, HOST_SHARE,
                        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (output == INVALID_HANDLE_VALUE) {
                        hostError(GetLastError());
                    } else {
                        HostInfo to;
                        if (handleInfo(output, &to)) {
                            if (fileKind(&to) != 1 || (from.file.dwVolumeSerialNumber == to.file.dwVolumeSerialNumber
                                && from.file.nFileIndexHigh == to.file.nFileIndexHigh && from.file.nFileIndexLow == to.file.nFileIndexLow)) {
                                __ada_raise(ADA_USE_ERROR);
                            } else if (!SetEndOfFile(output)) {
                                hostError(GetLastError());
                            } else {
                                char buffer[16384];
                                for (;;) {
                                    DWORD count;
                                    if (!ReadFile(input, buffer, sizeof buffer, &count, NULL)) {
                                        hostError(GetLastError());
                                        break;
                                    }
                                    if (count == 0) {
                                        break;
                                    }
                                    for (DWORD offset = 0; offset < count;) {
                                        DWORD written;
                                        if (!WriteFile(output, buffer + offset, count - offset, &written, NULL)) {
                                            hostError(GetLastError());
                                            break;
                                        }
                                        if (written == 0) {
                                            __ada_raise(ADA_DEVICE_ERROR);
                                            break;
                                        }
                                        offset += written;
                                    }
                                    if (__ada_task_context()->exception != NULL) {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (output != INVALID_HANDLE_VALUE && !CloseHandle(output)) {
        DWORD error = GetLastError();
        if (__ada_task_context()->exception == NULL) {
            hostError(error);
        }
    }
    if (input != INVALID_HANDLE_VALUE) {
        CloseHandle(input);
    }
    free(target);
    free(source);
}

static wchar_t* joinPath(const wchar_t* directory, const wchar_t* name)
{
    size_t length = wcslen(directory);
    wchar_t* result = allocateWide(length + wcslen(name) + 1);
    if (result != NULL) {
        wcscpy(result, directory);
        if (length && !separator(directory[length - 1])) {
            wcscat(result, L"\\");
        }
        wcscat(result, name);
    }
    return result;
}

static int dotEntry(const wchar_t* name)
{
    return wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0;
}

static wchar_t* handlePath(HANDLE handle)
{
    DWORD capacity = GetFinalPathNameByHandleW(handle, NULL, 0, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (capacity == 0) {
        hostError(GetLastError());
        return NULL;
    }
    for (;;) {
        wchar_t* result = allocateWide(capacity);
        if (result == NULL) {
            return NULL;
        }
        DWORD count = GetFinalPathNameByHandleW(handle, result, capacity + 1, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        if (count == 0) {
            DWORD error = GetLastError();
            free(result);
            hostError(error);
            return NULL;
        }
        if (count <= capacity) {
            return result;
        }
        free(result);
        capacity = count;
    }
}

static void deleteTree(const wchar_t* path, int allowLink)
{
    /* Deny deletion/renaming while walking. Opening the reparse point itself
       and inspecting its handle prevents a swapped junction being followed.
       Deletion is committed through that same handle, never a reopened path. */
    HANDLE handle = CreateFileW(path, DELETE | FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        hostError(GetLastError());
        return;
    }
    HostInfo info;
    if (!handleInfo(handle, &info)) {
        CloseHandle(handle);
        return;
    }
    if (fileKind(&info) != 0) {
        CloseHandle(handle);
        __ada_raise(ADA_NAME_ERROR);
        return;
    }
    int reparse = (info.file.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    if (reparse && !allowLink) {
        CloseHandle(handle);
        __ada_raise(ADA_USE_ERROR);
        return;
    }
    if (!reparse) {
        /* Resolve ancestor junctions through the opened handle as well. */
        wchar_t* canonical = handlePath(handle);
        wchar_t* pattern = canonical == NULL ? NULL : joinPath(canonical, L"*");
        if (pattern != NULL) {
            WIN32_FIND_DATAW data;
            HANDLE scan = FindFirstFileW(pattern, &data);
            DWORD scanError = GetLastError();
            free(pattern);
            if (scan == INVALID_HANDLE_VALUE) {
                DWORD error = scanError;
                if (error != ERROR_FILE_NOT_FOUND) {
                    hostError(error);
                }
            } else {
                for (;;) {
                    if (!dotEntry(data.cFileName)) {
                        wchar_t* child = joinPath(canonical, data.cFileName);
                        if (child == NULL) {
                            break;
                        }
                        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            deleteTree(child, 1);
                        } else if (!DeleteFileW(child)) {
                            hostError(GetLastError());
                        }
                        free(child);
                        if (__ada_task_context()->exception != NULL) {
                            break;
                        }
                    }
                    if (!FindNextFileW(scan, &data)) {
                        DWORD error = GetLastError();
                        if (error != ERROR_NO_MORE_FILES) {
                            hostError(error);
                        }
                        break;
                    }
                }
                FindClose(scan);
            }
        }
        free(canonical);
    }
    if (__ada_task_context()->exception == NULL) {
        FILE_DISPOSITION_INFO disposition = {TRUE};
        if (!SetFileInformationByHandle(handle, FileDispositionInfo, &disposition, sizeof disposition)) {
            hostError(GetLastError());
        }
    }
    CloseHandle(handle);
}

void __ada_host_delete_tree(const char* name, int length)
{
    wchar_t* path = fileName(name, length);
    if (path != NULL) {
        deleteTree(path, 0);
        free(path);
    }
}

typedef struct HostEntry
{
    struct HostEntry* next;
    wchar_t* name;
    HostInfo info;
} HostEntry;

struct HostSearch
{
    HostEntry* first;
    HostEntry* current;
};

void __ada_host_search_release(HostSearch* search)
{
    if (search != NULL) {
        HostEntry* entry = search->first;
        while (entry != NULL) {
            HostEntry* next = entry->next;
            free(entry->name);
            free(entry);
            entry = next;
        }
        free(search);
    }
}

/* Windows ordinal, case-insensitive glob; brackets are literal, * and ?
   have the same meanings as the POSIX adapter (on UTF-16 code units here). */
static int matches(const wchar_t* pattern, const wchar_t* name)
{
    const wchar_t* star = NULL;
    const wchar_t* retry = NULL;
    while (*name) {
        if (*pattern == L'?' || (*pattern && *pattern != L'*'
            && (*pattern == *name || CompareStringOrdinal(pattern, 1, name, 1, TRUE) == CSTR_EQUAL))) {
            ++pattern;
            ++name;
        } else if (*pattern == L'*') {
            star = pattern++;
            retry = name;
        } else if (star != NULL) {
            pattern = star + 1;
            name = ++retry;
        } else {
            return 0;
        }
    }
    while (*pattern == L'*') {
        ++pattern;
    }
    return *pattern == 0;
}

HostSearch* __ada_host_search_start(const char* name, int length, const char* pattern, int patternLength, int mask)
{
    wchar_t* path = fileName(name, length);
    if (path == NULL) {
        return NULL;
    }
    wchar_t* glob = wideString(pattern, patternLength, 1, ADA_NAME_ERROR);
    if (glob == NULL) {
        free(path);
        return NULL;
    }
    if (wcspbrk(glob, L"\\/:<>\"|") != NULL) {
        __ada_raise(ADA_NAME_ERROR);
        free(glob);
        free(path);
        return NULL;
    }
    HostInfo directory;
    if (!fileInfo(path, &directory, 0) || fileKind(&directory) != 0) {
        if (__ada_task_context()->exception == NULL) {
            __ada_raise(ADA_NAME_ERROR);
        }
        free(glob);
        free(path);
        return NULL;
    }
    HostSearch* search = calloc(1, sizeof *search);
    if (search == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        free(glob);
        free(path);
        return NULL;
    }
    wchar_t* query = joinPath(path, L"*");
    WIN32_FIND_DATAW data;
    HANDLE scan = INVALID_HANDLE_VALUE;
    if (query != NULL) {
        scan = FindFirstFileW(query, &data);
        DWORD error = GetLastError();
        free(query);
        if (scan == INVALID_HANDLE_VALUE && error != ERROR_FILE_NOT_FOUND) {
            hostError(error);
        }
    }
    HostEntry** tail = &search->first;
    while (scan != INVALID_HANDLE_VALUE && __ada_task_context()->exception == NULL) {
        if (!dotEntry(data.cFileName) && (!*glob || matches(glob, data.cFileName))) {
            wchar_t* child = joinPath(path, data.cFileName);
            HostInfo info;
            if (child != NULL) {
                if (fileInfo(child, &info, 1) && (mask & (1 << fileKind(&info)))) {
                    HostEntry* entry = calloc(1, sizeof *entry);
                    if (entry == NULL) {
                        __ada_raise(ADA_STORAGE_ERROR);
                    } else {
                        entry->name = child;
                        entry->info = info;
                        child = NULL;
                        *tail = entry;
                        tail = &entry->next;
                    }
                }
                free(child);
            }
        }
        if (__ada_task_context()->exception != NULL) {
            break;
        }
        if (!FindNextFileW(scan, &data)) {
            DWORD error = GetLastError();
            if (error != ERROR_NO_MORE_FILES) {
                hostError(error);
            }
            break;
        }
    }
    if (scan != INVALID_HANDLE_VALUE) {
        FindClose(scan);
    }
    free(glob);
    free(path);
    if (__ada_task_context()->exception != NULL) {
        __ada_host_search_release(search);
        return NULL;
    }
    search->current = search->first;
    return search;
}

int __ada_host_search_more(HostSearch* search)
{
    return search != NULL && search->current != NULL;
}

static HostEntry* currentEntry(HostSearch* search)
{
    if (!__ada_host_search_more(search)) {
        __ada_raise(ADA_STATUS_ERROR);
        return NULL;
    }
    return search->current;
}

void __ada_host_search_name(void* result, HostSearch* search)
{
    HostEntry* entry = currentEntry(search);
    if (entry != NULL) {
        pathResult(result, entry->name);
    }
}

int __ada_host_search_kind(HostSearch* search)
{
    HostEntry* entry = currentEntry(search);
    return entry == NULL ? 0 : fileKind(&entry->info);
}

int64_t __ada_host_search_size(HostSearch* search)
{
    HostEntry* entry = currentEntry(search);
    return entry == NULL ? 0 : fileSize(&entry->info);
}

void __ada_host_search_time(HostSearch* search, int* year, int* month, int* day, int64_t* seconds, int* valid)
{
    HostEntry* entry = currentEntry(search);
    *valid = entry != NULL && modificationTime(&entry->info, year, month, day, seconds);
}

void __ada_host_search_step(HostSearch* search)
{
    if (currentEntry(search) != NULL) {
        search->current = search->current->next;
    }
}
