#ifndef ADAHOST_H
#define ADAHOST_H

#include <stdint.h>
typedef struct HostSearch HostSearch;

void __ada_host_env_value(void* result, const char* name, int length);
int __ada_host_env_exists(const char* name, int length);
void __ada_host_env_set(const char* name, int length, const char* value, int valueLength);
void __ada_host_env_clear(const char* name, int length);
void __ada_host_env_clear_all(void);
int __ada_host_env_count(void);
void __ada_host_env_name(void* result, int index);
void __ada_host_env_entry_value(void* result, int index);
void __ada_host_current_directory(void* result);
void __ada_host_full_name(void* result, const char* name, int length);
void __ada_host_simple_name(void* result, const char* name, int length);
void __ada_host_containing_directory(void* result, const char* name, int length);
void __ada_host_extension(void* result, const char* name, int length);
void __ada_host_base_name(void* result, const char* name, int length);
void __ada_host_compose(void* result, const char* directory, int directoryLength,
    const char* name, int nameLength, const char* extension, int extensionLength);
int __ada_host_exists(const char* name, int length);
int __ada_host_kind(const char* name, int length);
int64_t __ada_host_size(const char* name, int length);
void __ada_host_modification_time(const char* name, int length, int* year, int* month, int* day, int64_t* seconds);
void __ada_host_set_directory(const char* name, int length);
void __ada_host_create_directory(const char* name, int length, const char* form, int formLength);
void __ada_host_create_path(const char* name, int length, const char* form, int formLength);
void __ada_host_delete_directory(const char* name, int length);
void __ada_host_delete_file(const char* name, int length);
void __ada_host_rename(const char* oldName, int oldLength, const char* newName, int newLength);
void __ada_host_copy_file(const char* sourceName, int sourceLength, const char* targetName, int targetLength,
    const char* form, int formLength);
void __ada_host_delete_tree(const char* name, int length);
void __ada_host_search_release(HostSearch* search);
HostSearch* __ada_host_search_start(const char* name, int length, const char* pattern, int patternLength, int mask);
int __ada_host_search_more(HostSearch* search);
void __ada_host_search_name(void* result, HostSearch* search);
int __ada_host_search_kind(HostSearch* search);
int64_t __ada_host_search_size(HostSearch* search);
void __ada_host_search_time(HostSearch* search, int* year, int* month, int* day, int64_t* seconds, int* valid);
void __ada_host_search_step(HostSearch* search);

#endif
