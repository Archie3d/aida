#ifndef ADATERMINATION_H
#define ADATERMINATION_H

#include "../common/Concurrent.h"

enum AdaTerminationCause
{
    kTerminationNormal,
    kTerminationAbnormal,
    kTerminationUnhandledException
};

/* Immutable protected access descriptor: code, lexical link, receiver. */
typedef struct AdaTerminationHandler
{
    void (*code)(void);
    void* link;
    AdaConcurrentObject* object;
} AdaTerminationHandler;

#endif
