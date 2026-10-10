#include "adalock.h"
#include <stdlib.h>

int __ada_mutex_init(AdaMutex* mutex)
{
    return pthread_mutex_init(mutex, NULL);
}

int __ada_mutex_destroy(AdaMutex* mutex)
{
    return pthread_mutex_destroy(mutex);
}

int __ada_mutex_trylock(AdaMutex* mutex)
{
    return pthread_mutex_trylock(mutex);
}

void __ada_mutex_lock(AdaMutex* mutex)
{
    if (pthread_mutex_lock(mutex) != 0) {
        abort();
    }
}

void __ada_mutex_unlock(AdaMutex* mutex)
{
    if (pthread_mutex_unlock(mutex) != 0) {
        abort();
    }
}
