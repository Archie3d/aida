#ifndef AIDA_CONCURRENT_H
#define AIDA_CONCURRENT_H

/* Concurrent objects retain one canonical address for dispatch and runtime
   ownership. Untagged objects do not use the tag slot. */
typedef struct AdaConcurrentObject
{
    void* tag;
    void* resource;
} AdaConcurrentObject;

enum { kAdaConcurrentHeaderBytes = 16 };

#endif
