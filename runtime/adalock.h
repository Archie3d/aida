#ifndef ADALOCK_H
#define ADALOCK_H

#include <pthread.h>

/* Private runtime locks. No lock may be held while invoking generated Ada
   callbacks. The remaining threading API is a separate roadmap step. */
typedef pthread_mutex_t AdaMutex;
#define ADA_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
void __ada_mutex_lock(AdaMutex* mutex);
void __ada_mutex_unlock(AdaMutex* mutex);

#endif
