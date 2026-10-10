#ifndef ADALOCK_H
#define ADALOCK_H

#include <pthread.h>

/* Private runtime locks. No lock may be held while invoking generated Ada
   callbacks. Initialization/destruction return POSIX error numbers; lock and
   unlock failures indicate broken runtime invariants and abort. */
typedef pthread_mutex_t AdaMutex;
#define ADA_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
int __ada_mutex_init(AdaMutex* mutex);
int __ada_mutex_destroy(AdaMutex* mutex);
int __ada_mutex_trylock(AdaMutex* mutex);
void __ada_mutex_lock(AdaMutex* mutex);
void __ada_mutex_unlock(AdaMutex* mutex);

#endif
