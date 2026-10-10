#include "adamaster.h"

_Static_assert(sizeof(AdaMaster) == 8, "master record QBE ABI");

void __ada_master_enter(AdaTaskContext* context, AdaMaster* master, int kind)
{
    (void)context;
    (void)master;
    (void)kind;
}

void __ada_master_activate(AdaTaskContext* context, AdaMaster* master)
{
    (void)context;
    (void)master;
}

void __ada_master_await(AdaTaskContext* context, AdaMaster* master)
{
    (void)context;
    (void)master;
}

void __ada_master_leave(AdaTaskContext* context, AdaMaster* master)
{
    (void)context;
    (void)master;
}
