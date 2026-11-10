#include "../../runtime/adathread.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

enum { kUnavailable = 77 };

int main(void)
{
    AdaPriorityThread self = __ada_priority_thread_self();
    int status = __ada_thread_set_priority(self, kAdaDefaultPriority);
    if (status != 0) {
        fprintf(stderr, "default native priority failed: %s\n", strerror(status));
        return 1;
    }
    status = __ada_thread_set_priority(self, kAdaPriorityLast);
    if (status == EACCES || status == EPERM || status == ENOTSUP) {
        printf("maximum native task priority unavailable: %s\n", strerror(status));
        return kUnavailable;
    }
    if (status != 0) {
        fprintf(stderr, "native priority probe failed: %s\n", strerror(status));
        return 1;
    }
    return 0;
}
