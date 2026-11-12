#ifndef AIDA_PRIORITY_H
#define AIDA_PRIORITY_H

enum AdaPriorityMode
{
    kAdaPriorityInherited = 0,
    kAdaPriorityExplicit = 1,
    kAdaPriorityInterrupt = 2
};

enum AdaLockingPolicy
{
    kAdaLockingDefault = 0,
    kAdaCeilingLocking = 1,
    kAdaNativeLocking = 2
};

/* Target model mirrored by System's static Ada declarations. */
enum
{
    kAdaPriorityFirst = 0,
    kAdaPriorityLast = 30,
    kAdaAnyPriorityLast = 31,
    kAdaDefaultPriority = (kAdaPriorityFirst + kAdaPriorityLast) / 2
};

#endif
