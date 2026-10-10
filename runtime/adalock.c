#include "adalock.h"
#include <stdlib.h>

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
