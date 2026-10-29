#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include "../runtime/adart.h"
#include "../runtime/adahost.h"
#include <windows.h>
#include <winioctl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Windows host check failed at line %d (Win32 error %lu)\n", __LINE__, (unsigned long)GetLastError()); abort(); \
} } while (0)
#define LENGTH(text) ((int)strlen(text))

static void checkString(AdaArrayResult* result, const char* expected)
{
    CHECK(__ada_task_context()->exception == NULL);
    CHECK(result->first == 1 && result->length == (int64_t)strlen(expected));
    CHECK(memcmp(result->data, expected, (size_t)result->length) == 0);
    free(result->data);
    memset(result, 0, sizeof *result);
}

/* Directory junction creation does not require the symbolic-link privilege.
   A junction points outside the tree to prove deletion does not follow it. */
static void createJunction(const char* name, const char* target)
{
    wchar_t wideName[1024], wideTarget[1024], full[1024];
    CHECK(MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, name, -1, wideName, 1024) != 0);
    CHECK(MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, target, -1, wideTarget, 1024) != 0);
    DWORD length = GetFullPathNameW(wideTarget, 1024, full, NULL);
    CHECK(length > 0 && length < 1024 && full[1] == L':');
    CHECK(CreateDirectoryW(wideName, NULL));
    HANDLE handle = CreateFileW(wideName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    CHECK(handle != INVALID_HANDLE_VALUE);
    typedef struct JunctionData
    {
        DWORD tag;
        WORD dataLength;
        WORD reserved;
        WORD substituteOffset;
        WORD substituteLength;
        WORD printOffset;
        WORD printLength;
        wchar_t path[2056];
    } JunctionData;
    JunctionData data = {0};
    data.tag = IO_REPARSE_TAG_MOUNT_POINT;
    wcscpy(data.path, L"\\??\\");
    wcscat(data.path, full);
    data.substituteLength = (WORD)(wcslen(data.path) * sizeof(wchar_t));
    data.printOffset = data.substituteLength + sizeof(wchar_t);
    data.printLength = (WORD)(wcslen(full) * sizeof(wchar_t));
    wcscpy((wchar_t*)((char*)data.path + data.printOffset), full);
    data.dataLength = 8 + data.printOffset + data.printLength + sizeof(wchar_t);
    DWORD returned;
    CHECK(DeviceIoControl(handle, FSCTL_SET_REPARSE_POINT, &data, data.dataLength + 8,
        NULL, 0, &returned, NULL));
    CHECK(CloseHandle(handle));
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaArrayResult result = {0};
    static const char kVariable[] = "AIDA_HOST_WINDOWS_TEST";
    __ada_host_env_set(kVariable, LENGTH(kVariable), "", 0);
    CHECK(__ada_host_env_exists("aida_host_windows_test", 22));
    __ada_host_env_value(&result, kVariable, LENGTH(kVariable));
    checkString(&result, "");
    __ada_host_env_set(kVariable, LENGTH(kVariable), "second", 6);
    __ada_host_env_value(&result, "aida_host_windows_test", 22);
    checkString(&result, "second");
    CHECK(getenv(kVariable) != NULL && strcmp(getenv(kVariable), "second") == 0);
    CHECK(SetEnvironmentVariableW(L"AIDA_HOST_NATIVE_CHANGE", L"native"));
    __ada_host_env_value(&result, "AIDA_HOST_NATIVE_CHANGE", 23);
    checkString(&result, "native");
    CHECK(_putenv_s("AIDA_HOST_CRT_CHANGE", "crt") == 0);
    __ada_host_env_value(&result, "AIDA_HOST_CRT_CHANGE", 20);
    checkString(&result, "crt");
    __ada_host_env_set("invalid=name", 12, "value", 5);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR);
    context.exception = NULL;
    int found = 0;
    int count = __ada_host_env_count();
    for (int i = 0; i < count; ++i) {
        __ada_host_env_name(&result, i);
        CHECK(context.exception == NULL && result.length > 0 && ((char*)result.data)[0] != '=');
        if (result.length == LENGTH(kVariable) && memcmp(result.data, kVariable, LENGTH(kVariable)) == 0) {
            found = 1;
        }
        free(result.data);
        __ada_host_env_entry_value(&result, i);
        CHECK(context.exception == NULL);
        free(result.data);
    }
    memset(&result, 0, sizeof result);
    CHECK(found);
    __ada_host_current_directory(&result);
    CHECK(context.exception == NULL);
    char* oldDirectory = malloc((size_t)result.length + 1);
    CHECK(oldDirectory != NULL);
    memcpy(oldDirectory, result.data, (size_t)result.length);
    oldDirectory[result.length] = 0;
    free(result.data);
    memset(&result, 0, sizeof result);
    __ada_host_simple_name(&result, "C:\\alpha\\file.tar.gz", 20);
    checkString(&result, "file.tar.gz");
    __ada_host_containing_directory(&result, "C:\\alpha\\file.txt", 17);
    checkString(&result, "C:/alpha");
    __ada_host_compose(&result, "C:", 2, "file", 4, "txt", 3);
    checkString(&result, "C:file.txt");
    __ada_host_compose(&result, "C:\\", 3, "file", 4, "txt", 3);
    checkString(&result, "C:/file.txt");
    __ada_host_simple_name(&result, "C:.", 3);
    checkString(&result, ".");
    __ada_host_exists("C:/NUL.txt", 10);
    CHECK(context.exception == ADA_NAME_ERROR);
    context.exception = NULL;
    const char* unc = "\\\\server\\share\\entry.txt";
    __ada_host_simple_name(&result, unc, LENGTH(unc));
    checkString(&result, "entry.txt");
    const char* uncRoot = "\\\\server\\share";
    __ada_host_containing_directory(&result, uncRoot, LENGTH(uncRoot));
    CHECK(context.exception == ADA_USE_ERROR);
    context.exception = NULL;
    __ada_host_exists("bad\0name", 8);
    CHECK(context.exception == ADA_NAME_ERROR);
    context.exception = NULL;

    char root[128], source[256], target[256], nested[256], outside[256], junction[256];
    snprintf(root, sizeof root, "aida-win-host-%lu-%llu", (unsigned long)GetCurrentProcessId(), (unsigned long long)GetTickCount64());
    snprintf(source, sizeof source, "%s/original.txt", root);
    snprintf(target, sizeof target, "%s/copy.txt", root);
    snprintf(nested, sizeof nested, "%s/one/two", root);
    snprintf(outside, sizeof outside, "%s/outside", root);
    snprintf(junction, sizeof junction, "%s/one/two/link", root);
    __ada_host_create_path(nested, LENGTH(nested), "", 0);
    __ada_host_create_directory(outside, LENGTH(outside), "", 0);
    CHECK(context.exception == NULL);
    FILE* file = fopen(source, "wb");
    CHECK(file != NULL && fwrite("hello\n", 1, 6, file) == 6 && fclose(file) == 0);
    __ada_host_copy_file(source, LENGTH(source), target, LENGTH(target), "", 0);
    CHECK(context.exception == NULL && __ada_host_size(target, LENGTH(target)) == 6);
    __ada_host_copy_file(source, LENGTH(source), source, LENGTH(source), "", 0);
    CHECK(context.exception == ADA_USE_ERROR && __ada_host_size(source, LENGTH(source)) == 6);
    context.exception = NULL;
    char alias[256];
    snprintf(alias, sizeof alias, "%s/alias.txt", root);
    CHECK(CreateHardLinkA(alias, source, NULL));
    __ada_host_copy_file(source, LENGTH(source), alias, LENGTH(alias), "", 0);
    CHECK(context.exception == ADA_USE_ERROR && __ada_host_size(source, LENGTH(source)) == 6);
    context.exception = NULL;
    __ada_host_rename(source, LENGTH(source), target, LENGTH(target));
    CHECK(context.exception == ADA_USE_ERROR);
    context.exception = NULL;
    HostSearch* search = __ada_host_search_start(root, LENGTH(root), "ORIGINAL.?XT", 12, 2);
    CHECK(context.exception == NULL && __ada_host_search_more(search));
    CHECK(__ada_host_search_kind(search) == 1 && __ada_host_search_size(search) == 6);
    int year, month, day, valid;
    int64_t seconds;
    __ada_host_search_time(search, &year, &month, &day, &seconds, &valid);
    CHECK(valid && year >= 2020);
    __ada_host_search_name(&result, search);
    CHECK(context.exception == NULL && result.length > LENGTH(source));
    free(result.data);
    memset(&result, 0, sizeof result);
    __ada_host_search_step(search);
    CHECK(!__ada_host_search_more(search));
    __ada_host_search_release(search);
    __ada_host_set_directory(root, LENGTH(root));
    CHECK(context.exception == NULL && __ada_host_exists("original.txt", 12));
    __ada_host_set_directory(oldDirectory, LENGTH(oldDirectory));
    CHECK(context.exception == NULL);
    free(oldDirectory);
    createJunction(junction, outside);
    __ada_host_delete_tree(junction, LENGTH(junction));
    CHECK(context.exception == ADA_USE_ERROR);
    context.exception = NULL;
    __ada_host_delete_tree(nested, LENGTH(nested));
    CHECK(context.exception == NULL && __ada_host_exists(outside, LENGTH(outside)));
    CHECK(!__ada_host_exists(nested, LENGTH(nested)));
    __ada_host_delete_tree(root, LENGTH(root));
    CHECK(context.exception == NULL && !__ada_host_exists(root, LENGTH(root)));
    __ada_host_env_clear_all();
    CHECK(context.exception == NULL && __ada_host_env_count() == 0);
    __ada_host_env_set(kVariable, LENGTH(kVariable), "restored", 8);
    __ada_host_env_value(&result, kVariable, LENGTH(kVariable));
    checkString(&result, "restored");
    __ada_host_env_clear(kVariable, LENGTH(kVariable));
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
