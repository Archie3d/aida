/* POSIX host services. All temporary ownership is local to a call or to a
   Search_Guard; callbacks run in Ada with no C locks held. */
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#endif
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#endif
#include "adart.h"
#include "adacalendar.h"
#include "adahost.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <crt_externs.h>
#define HOST_ENVIRON (*_NSGetEnviron())
#else
extern char** environ;
#define HOST_ENVIRON environ
#endif
#if defined(__linux__)
#include <sys/syscall.h>
extern long syscall(long number, ...);
#endif
#if defined(_WIN32)
#include <windows.h>
#endif

static char* hostString(const char* data, int length, int allowEmpty, const AdaException* error)
{
    if (length < 0 || (!allowEmpty && length == 0) || (length > 0 && memchr(data, 0, (size_t)length))) {
        __ada_raise(error);
        return NULL;
    }
    char* result = malloc((size_t)length + 1);
    if (result == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    if (length > 0) {
        memcpy(result, data, (size_t)length);
    }
    result[length] = 0;
    return result;
}

static void stringResult(void* result, const char* value)
{
    size_t length = strlen(value);
    if (length > INT_MAX) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return;
    }
    __ada_array_result(result, value, 1, (int64_t)length, 1);
}

static void hostError(int error)
{
    __ada_raise(error == ENOENT || error == ENOTDIR || error == ENAMETOOLONG || error == ELOOP
        ? ADA_NAME_ERROR : error == EIO ? ADA_DEVICE_ERROR : ADA_USE_ERROR);
}

static char* environmentName(const char* name, int length)
{
    if (length <= 0 || memchr(name, '=', (size_t)length)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return NULL;
    }
    return hostString(name, length, 0, ADA_CONSTRAINT_ERROR);
}

void __ada_host_env_value(void* result, const char* name, int length)
{
    char* key = environmentName(name, length);
    if (key == NULL) {
        return;
    }
    const char* value = getenv(key);
    if (value == NULL) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    } else {
        stringResult(result, value);
    }
    free(key);
}

int __ada_host_env_exists(const char* name, int length)
{
    /* Invalid names cannot designate a variable. */
    if (length <= 0 || memchr(name, '=', (size_t)length) || memchr(name, 0, (size_t)length)) {
        return 0;
    }
    char* key = environmentName(name, length);
    if (key == NULL) {
        return 0;
    }
    int found = getenv(key) != NULL;
    free(key);
    return found;
}

void __ada_host_env_set(const char* name, int length, const char* value, int valueLength)
{
    char* key = environmentName(name, length);
    if (key == NULL) {
        return;
    }
    char* text = hostString(value, valueLength, 1, ADA_CONSTRAINT_ERROR);
    if (text != NULL && setenv(key, text, 1) != 0) {
        __ada_raise(errno == ENOMEM ? ADA_STORAGE_ERROR : ADA_CONSTRAINT_ERROR);
    }
    free(text);
    free(key);
}

void __ada_host_env_clear(const char* name, int length)
{
    char* key = environmentName(name, length);
    if (key != NULL) {
        if (unsetenv(key) != 0) {
            __ada_raise(ADA_CONSTRAINT_ERROR);
        }
        free(key);
    }
}

void __ada_host_env_clear_all(void)
{
    while (HOST_ENVIRON != NULL && HOST_ENVIRON[0] != NULL) {
        const char* entry = HOST_ENVIRON[0];
        const char* equal = strchr(entry, '=');
        if (equal == NULL || equal == entry || equal - entry > INT_MAX) {
            __ada_raise(ADA_PROGRAM_ERROR);
            return;
        }
        __ada_host_env_clear(entry, (int)(equal - entry));
        if (__ada_task_context()->exception != NULL) {
            return;
        }
    }
}

int __ada_host_env_count(void)
{
    int count = 0;
    if (HOST_ENVIRON != NULL) {
        while (HOST_ENVIRON[count] != NULL) {
            if (count == INT_MAX - 1) {
                __ada_raise(ADA_CONSTRAINT_ERROR);
                return 0;
            }
            ++count;
        }
    }
    return count;
}

static const char* environmentEntry(int index)
{
    if (index < 0 || index >= __ada_host_env_count()) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return NULL;
    }
    return HOST_ENVIRON[index];
}

void __ada_host_env_name(void* result, int index)
{
    const char* entry = environmentEntry(index);
    if (entry == NULL) {
        return;
    }
    const char* equal = strchr(entry, '=');
    __ada_array_result(result, entry, 1, equal == NULL ? (int64_t)strlen(entry) : equal - entry, 1);
}

void __ada_host_env_entry_value(void* result, int index)
{
    const char* entry = environmentEntry(index);
    if (entry != NULL) {
        const char* equal = strchr(entry, '=');
        stringResult(result, equal == NULL ? "" : equal + 1);
    }
}

static char* currentDirectory(void)
{
    size_t capacity = 256;
    for (;;) {
        char* value = malloc(capacity);
        if (value == NULL) {
            __ada_raise(ADA_STORAGE_ERROR);
            return NULL;
        }
        if (getcwd(value, capacity) != NULL) {
            return value;
        }
        int error = errno;
        free(value);
        if (error != ERANGE || capacity > INT_MAX / 2) {
            __ada_raise(ADA_USE_ERROR);
            return NULL;
        }
        capacity *= 2;
    }
}

/* Resolve existing prefixes, then normalize missing suffixes lexically.
   Resolving each prefix before .. preserves paths through directory symlinks. */
static char* fullName(const char* name)
{
    char* base = name[0] == '/' ? strdup("") : currentDirectory();
    if (base == NULL) {
        if (__ada_task_context()->exception == NULL) {
            __ada_raise(ADA_STORAGE_ERROR);
        }
        return NULL;
    }
    size_t size = strlen(base) + strlen(name) + 2;
    char* joined = malloc(size);
    char* result = strdup("/");
    if (joined == NULL || result == NULL) {
        free(base);
        free(joined);
        free(result);
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    snprintf(joined, size, "%s/%s", base, name);
    free(base);
    char* save = NULL;
    for (char* part = strtok_r(joined, "/", &save); part != NULL; part = strtok_r(NULL, "/", &save)) {
        size_t used = strlen(result);
        if (strcmp(part, ".") == 0) {
            continue;
        }
        if (strcmp(part, "..") == 0) {
            while (used > 1 && result[used - 1] != '/') {
                --used;
            }
            if (used > 1) {
                --used;
            }
            result[used] = 0;
        } else {
            size_t length = strlen(part);
            char* expanded = realloc(result, used + length + 2);
            if (expanded == NULL) {
                free(result);
                free(joined);
                __ada_raise(ADA_STORAGE_ERROR);
                return NULL;
            }
            result = expanded;
            if (used > 1) {
                result[used++] = '/';
            }
            memcpy(result + used, part, length + 1);
            char* resolved = realpath(result, NULL);
            if (resolved != NULL) {
                free(result);
                result = resolved;
            } else if (errno != ENOENT && errno != ENOTDIR) {
                int error = errno;
                free(result);
                free(joined);
                hostError(error);
                return NULL;
            }
        }
    }
    free(joined);
    return result;
}

void __ada_host_current_directory(void* result)
{
    char* value = currentDirectory();
    if (value != NULL) {
        stringResult(result, value);
        free(value);
    }
}

void __ada_host_full_name(void* result, const char* name, int length)
{
    char* text = hostString(name, length, 0, ADA_NAME_ERROR);
    if (text == NULL) {
        return;
    }
    char* value = fullName(text);
    if (value != NULL) {
        stringResult(result, value);
        free(value);
    }
    free(text);
}

static char* trimmedName(const char* name, int length)
{
    char* text = hostString(name, length, 0, ADA_NAME_ERROR);
    if (text != NULL) {
        size_t size = strlen(text);
        while (size > 1 && text[size - 1] == '/') {
            text[--size] = 0;
        }
    }
    return text;
}

static void namePart(void* result, const char* name, int length, int operation)
{
    char* text = trimmedName(name, length);
    if (text == NULL) {
        return;
    }
    char* slash = strrchr(text, '/');
    char* simple = slash == NULL ? text : slash + 1;
    char* dot = strrchr(simple, '.');
    /* Leading dots and the special names . and .. are not extensions. */
    if (dot == simple || strcmp(simple, "..") == 0) {
        dot = NULL;
    }
    const char* value = simple;
    if (operation == 1) {
        if (strcmp(text, "/") == 0) {
            __ada_raise(ADA_USE_ERROR);
            free(text);
            return;
        }
        if (slash == NULL) {
            value = ".";
        } else {
            if (slash == text) {
                slash[1] = 0;
            } else {
                *slash = 0;
            }
            value = text;
        }
    } else if (operation == 2) {
        value = dot == NULL ? "" : dot + 1;
    } else if (operation == 3 && dot != NULL) {
        *dot = 0;
    }
    if ((operation == 0 || operation == 3) && strcmp(text, "/") == 0) {
        value = "/";
    }
    stringResult(result, value);
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
    char* dir = hostString(directory, directoryLength, 1, ADA_NAME_ERROR);
    if (dir == NULL) {
        return;
    }
    char* text = hostString(name, nameLength, 0, ADA_NAME_ERROR);
    char* ext = text == NULL ? NULL : hostString(extension, extensionLength, 1, ADA_NAME_ERROR);
    if (text != NULL && ext != NULL) {
        if (strchr(text, '/') || strchr(ext, '/') || strchr(ext, '.')) {
            __ada_raise(ADA_NAME_ERROR);
        } else {
            size_t size = strlen(dir) + strlen(text) + strlen(ext) + 3;
            char* value = malloc(size);
            if (value == NULL) {
                __ada_raise(ADA_STORAGE_ERROR);
            } else {
                snprintf(value, size, "%s%s%s%s%s", dir,
                    *dir && dir[strlen(dir) - 1] != '/' ? "/" : "", text, *ext ? "." : "", ext);
                stringResult(result, value);
                free(value);
            }
        }
    }
    free(ext);
    free(text);
    free(dir);
}

static int fileStat(const char* name, int length, struct stat* info, int missingAllowed)
{
    char* text = hostString(name, length, 0, ADA_NAME_ERROR);
    if (text == NULL) {
        return 0;
    }
    int status = stat(text, info);
    int error = errno;
    free(text);
    if (status != 0) {
        if (!missingAllowed || (error != ENOENT && error != ENOTDIR)) {
            hostError(error);
        }
        return 0;
    }
    return 1;
}

static int fileKind(const struct stat* info)
{
    return S_ISDIR(info->st_mode) ? 0 : S_ISREG(info->st_mode) ? 1 : 2;
}

int __ada_host_exists(const char* name, int length)
{
    struct stat info;
    return fileStat(name, length, &info, 1);
}

int __ada_host_kind(const char* name, int length)
{
    struct stat info;
    return fileStat(name, length, &info, 0) ? fileKind(&info) : 0;
}

int64_t __ada_host_size(const char* name, int length)
{
    struct stat info;
    if (!fileStat(name, length, &info, 0)) {
        return 0;
    }
    if (info.st_size < 0 || (__int128)info.st_size > INT64_MAX) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    return (int64_t)info.st_size;
}

static int modificationTime(const struct stat* info, int* year, int* month, int* day, int64_t* seconds)
{
    __int128 micros = (__int128)info->st_mtime * 1000000;
#if defined(__APPLE__)
    micros += info->st_mtimespec.tv_nsec / 1000;
#elif !defined(_WIN32)
    micros += info->st_mtim.tv_nsec / 1000;
#endif
    int valid = 0;
    if (micros >= INT64_MIN && micros <= INT64_MAX) {
        __ada_calendar_split((int64_t)micros, year, month, day, seconds, &valid);
    }
    return valid;
}

void __ada_host_modification_time(const char* name, int length, int* year, int* month, int* day, int64_t* seconds)
{
    struct stat info;
    if (fileStat(name, length, &info, 0)) {
        if (!modificationTime(&info, year, month, day, seconds)) {
            __ada_raise(ADA_USE_ERROR);
        }
    }
}

void __ada_host_set_directory(const char* name, int length)
{
    char* text = hostString(name, length, 0, ADA_NAME_ERROR);
    if (text != NULL) {
        if (chdir(text) != 0) {
            hostError(errno);
        }
        free(text);
    }
}

static void createDirectory(const char* name, int length, int parents, int formLength)
{
    if (formLength != 0) {
        __ada_raise(ADA_USE_ERROR);
        return;
    }
    char* text = trimmedName(name, length);
    if (text == NULL) {
        return;
    }
    for (char* end = text + 1; ; ++end) {
        if (*end != 0 && (!parents || *end != '/')) {
            continue;
        }
        char saved = *end;
        *end = 0;
        if (mkdir(text, 0777) != 0) {
            int error = errno;
            struct stat info;
            if (!parents || error != EEXIST || stat(text, &info) != 0 || !S_ISDIR(info.st_mode)) {
                hostError(error);
                free(text);
                return;
            }
        }
        *end = saved;
        if (saved == 0) {
            break;
        }
    }
    free(text);
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
    char* text = hostString(name, length, 0, ADA_NAME_ERROR);
    if (text != NULL) {
        if (rmdir(text) != 0) {
            hostError(errno);
        }
        free(text);
    }
}

void __ada_host_delete_file(const char* name, int length)
{
    struct stat info;
    if (!fileStat(name, length, &info, 0)) {
        return;
    }
    if (S_ISDIR(info.st_mode)) {
        __ada_raise(ADA_NAME_ERROR);
        return;
    }
    char* text = hostString(name, length, 0, ADA_NAME_ERROR);
    if (text != NULL) {
        if (unlink(text) != 0) {
            hostError(errno);
        }
        free(text);
    }
}

void __ada_host_rename(const char* oldName, int oldLength, const char* newName, int newLength)
{
    char* source = hostString(oldName, oldLength, 0, ADA_NAME_ERROR);
    if (source == NULL) {
        return;
    }
    char* target = hostString(newName, newLength, 0, ADA_NAME_ERROR);
    if (target != NULL) {
        int status;
#if defined(__APPLE__)
        status = renamex_np(source, target, RENAME_EXCL);
#elif defined(__linux__) && defined(SYS_renameat2)
        status = (int)syscall(SYS_renameat2, AT_FDCWD, source, AT_FDCWD, target, 1 /* RENAME_NOREPLACE */);
#elif defined(_WIN32)
        status = MoveFileA(source, target) ? 0 : -1;
        if (status != 0) {
            errno = EACCES;
        }
#else
        /* Never fall back to overwriting a target after a racy existence test. */
        status = -1;
        errno = ENOTSUP;
#endif
        if (status != 0) {
            int error = errno;
            struct stat info;
            if (lstat(source, &info) != 0) {
                hostError(errno);
            } else {
                __ada_raise(error == EIO ? ADA_DEVICE_ERROR : ADA_USE_ERROR);
            }
        }
    }
    free(source);
    free(target);
}

void __ada_host_copy_file(const char* sourceName, int sourceLength, const char* targetName, int targetLength,
    const char* form, int formLength)
{
    (void)form;
    if (formLength != 0) {
        __ada_raise(ADA_USE_ERROR);
        return;
    }
    char* source = hostString(sourceName, sourceLength, 0, ADA_NAME_ERROR);
    if (source == NULL) {
        return;
    }
    char* target = hostString(targetName, targetLength, 0, ADA_NAME_ERROR);
    int input = -1;
    int output = -1;
    int error = 0;
    if (target != NULL) {
        input = open(source, O_RDONLY | O_NONBLOCK);
        struct stat from, to;
        if (input < 0 || fstat(input, &from) != 0) {
            error = errno;
        } else if (!S_ISREG(from.st_mode)) {
            error = S_ISDIR(from.st_mode) ? ENOTDIR : ENOTSUP;
        } else {
            output = open(target, O_WRONLY | O_CREAT | O_NONBLOCK, 0666);
            if (output < 0 || fstat(output, &to) != 0) {
                error = errno;
            } else if (!S_ISREG(to.st_mode) || (from.st_dev == to.st_dev && from.st_ino == to.st_ino)) {
                error = EINVAL;
            } else if (ftruncate(output, 0) != 0) {
                error = errno;
            } else {
                char buffer[16384];
                ssize_t count;
                for (;;) {
                    count = read(input, buffer, sizeof buffer);
                    if (count < 0 && errno == EINTR) {
                        continue;
                    }
                    if (count <= 0) {
                        if (count < 0) {
                            error = errno;
                        }
                        break;
                    }
                    for (ssize_t offset = 0; offset < count;) {
                        ssize_t written = write(output, buffer + offset, (size_t)(count - offset));
                        if (written < 0 && errno == EINTR) {
                            continue;
                        }
                        if (written <= 0) {
                            error = written < 0 ? errno : EIO;
                            break;
                        }
                        offset += written;
                    }
                    if (error) {
                        break;
                    }
                }
            }
        }
    }
    if (output >= 0 && close(output) != 0 && !error) {
        error = errno;
    }
    if (input >= 0) {
        close(input);
    }
    free(source);
    free(target);
    if (error) {
        hostError(error);
    }
}

#if !defined(_WIN32)
/* Descriptor-relative traversal refuses symlinks at every recursive descent.
   No directory stream or descriptor survives an error return. */
static int deleteTreeAt(int parent, const char* name)
{
    int fd = openat(parent, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (fd < 0) {
        return errno;
    }
    DIR* directory = fdopendir(fd);
    if (directory == NULL) {
        int error = errno;
        close(fd);
        return error;
    }
    int error = 0;
    for (;;) {
        errno = 0;
        struct dirent* entry = readdir(directory);
        if (entry == NULL) {
            error = errno;
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        struct stat info;
        if (fstatat(fd, entry->d_name, &info, AT_SYMLINK_NOFOLLOW) != 0) {
            error = errno;
        } else if (S_ISDIR(info.st_mode)) {
            error = deleteTreeAt(fd, entry->d_name);
        } else if (unlinkat(fd, entry->d_name, 0) != 0) {
            error = errno;
        }
        if (error) {
            break;
        }
    }
    if (closedir(directory) != 0 && !error) {
        error = errno;
    }
    if (!error && unlinkat(parent, name, AT_REMOVEDIR) != 0) {
        error = errno;
    }
    return error;
}
#endif

void __ada_host_delete_tree(const char* name, int length)
{
    char* text = trimmedName(name, length);
    if (text == NULL) {
        return;
    }
#if !defined(_WIN32)
    int error = deleteTreeAt(AT_FDCWD, text);
    if (error) {
        if (error == ELOOP) {
            __ada_raise(ADA_USE_ERROR);
        } else {
            hostError(error);
        }
    }
#else
    __ada_raise(ADA_USE_ERROR);
#endif
    free(text);
}

typedef struct HostEntry
{
    struct HostEntry* next;
    char* name;
    struct stat info;
} HostEntry;

typedef struct HostSearch
{
    HostEntry* first;
    HostEntry* current;
} HostSearch;

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

/* Byte glob: * and ?; all other bytes, including brackets, are literal.
   Backtracking only advances the last star, avoiding recursive stack growth. */
static int matches(const char* pattern, const char* name)
{
    const char* star = NULL;
    const char* retry = NULL;
    while (*name) {
        if (*pattern == '?' || (*pattern != '*' && *pattern == *name)) {
            ++pattern;
            ++name;
        } else if (*pattern == '*') {
            star = pattern++;
            retry = name;
        } else if (star != NULL) {
            pattern = star + 1;
            name = ++retry;
        } else {
            return 0;
        }
    }
    while (*pattern == '*') {
        ++pattern;
    }
    return *pattern == 0;
}

HostSearch* __ada_host_search_start(const char* name, int length, const char* pattern, int patternLength, int mask)
{
    char* text = hostString(name, length, 0, ADA_NAME_ERROR);
    if (text == NULL) {
        return NULL;
    }
    char* glob = hostString(pattern, patternLength, 1, ADA_NAME_ERROR);
    char* full = glob == NULL ? NULL : fullName(text);
    free(text);
    if (glob == NULL || full == NULL) {
        free(glob);
        free(full);
        return NULL;
    }
    if (strchr(glob, '/') != NULL) {
        free(glob);
        free(full);
        __ada_raise(ADA_NAME_ERROR);
        return NULL;
    }
    DIR* directory = opendir(full);
    if (directory == NULL) {
        int error = errno;
        free(glob);
        free(full);
        hostError(error);
        return NULL;
    }
    HostSearch* search = calloc(1, sizeof *search);
    int error = search == NULL ? ENOMEM : 0;
    HostEntry** tail = search == NULL ? NULL : &search->first;
    while (!error) {
        errno = 0;
        struct dirent* item = readdir(directory);
        if (item == NULL) {
            error = errno;
            break;
        }
        if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0
            || (*glob && !matches(glob, item->d_name))) {
            continue;
        }
        size_t size = strlen(full) + strlen(item->d_name) + 2;
        char* path = malloc(size);
        if (path == NULL) {
            error = ENOMEM;
            break;
        }
        snprintf(path, size, "%s%s%s", full, strcmp(full, "/") == 0 ? "" : "/", item->d_name);
        struct stat info;
        if (stat(path, &info) != 0) {
            /* Disappearing entries and dangling symlinks are omitted. */
            error = errno;
            free(path);
            if (error == ENOENT) {
                error = 0;
                continue;
            }
            break;
        }
        if (!(mask & (1 << fileKind(&info)))) {
            free(path);
            continue;
        }
        HostEntry* entry = calloc(1, sizeof *entry);
        if (entry == NULL) {
            free(path);
            error = ENOMEM;
            break;
        }
        entry->name = path;
        entry->info = info;
        *tail = entry;
        tail = &entry->next;
    }
    if (closedir(directory) != 0 && !error) {
        error = errno;
    }
    free(glob);
    free(full);
    if (error) {
        __ada_host_search_release(search);
        if (error == ENOMEM) {
            __ada_raise(ADA_STORAGE_ERROR);
        } else {
            hostError(error);
        }
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
        stringResult(result, entry->name);
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
    if (entry == NULL) {
        return 0;
    }
    if (entry->info.st_size < 0 || (__int128)entry->info.st_size > INT64_MAX) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    return (int64_t)entry->info.st_size;
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
