#ifndef ADATASKSERVICES_H
#define ADATASKSERVICES_H

void* __ada_task_attributes_create(void);
void __ada_task_attributes_close(void* key);
void* __ada_task_attribute_acquire(void* key, void* task);
void __ada_task_attribute_release(void* token);
void* __ada_task_attribute_get(void* token);
void __ada_task_attribute_put(void* token, void* value);
void __ada_task_attributes_terminated(void* task);

void __ada_suspension_destroy(void** slot);
void __ada_suspension_set(void** slot, int value);
int __ada_suspension_state(void** slot);
void __ada_suspension_wait(void** slot);

#endif
