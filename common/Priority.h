#ifndef AIDA_PRIORITY_H
#define AIDA_PRIORITY_H

/* Target model mirrored by System's static Ada declarations. */
enum
{
    kAdaPriorityFirst = 0,
    kAdaPriorityLast = 30,
    kAdaAnyPriorityLast = 31,
    kAdaDefaultPriority = (kAdaPriorityFirst + kAdaPriorityLast) / 2
};

#endif
