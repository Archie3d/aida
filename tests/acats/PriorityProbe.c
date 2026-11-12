#include "../../runtime/adathread.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

enum { kUnavailable = 77 };

int main(int argc, char** argv)
{
    int maximum = kAdaPriorityLast;
    if (argc == 2 && strcmp(argv[1], "interrupt") == 0) { maximum = kAdaAnyPriorityLast; }
    else if (argc != 1) { return 1; }
    AdaPriorityThread self = __ada_priority_thread_self();
    int status = __ada_thread_set_priority(self, kAdaDefaultPriority);
    if (status == EACCES || status == EPERM || status == ENOTSUP) {
        printf("maximum native task priority unavailable: %s\n", strerror(status));
        return kUnavailable;
    }
    if (status != 0) {
        fprintf(stderr, "default native priority failed: %s\n", strerror(status));
        return 1;
    }
    status = __ada_thread_set_priority(self, maximum);
    if (status == EACCES || status == EPERM || status == ENOTSUP) {
        printf("maximum native task priority unavailable: %s\n", strerror(status));
        return kUnavailable;
    }
    if (status != 0) {
        fprintf(stderr, "native priority probe failed: %s\n", strerror(status));
        return 1;
    }
    status = __ada_thread_set_priority(self, kAdaDefaultPriority);
    if (status != 0) {
        fprintf(stderr, "native priority restoration failed: %s\n", strerror(status));
        return 1;
    }
    return 0;
}
