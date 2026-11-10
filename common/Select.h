#ifndef ADA_SELECT_LAYOUT_H
#define ADA_SELECT_LAYOUT_H

/* Internal compiler/runtime select table ABI. Each alternative is 32 bytes. */
enum AdaSelectKind
{
    ADA_SELECT_ACCEPT,
    ADA_SELECT_DELAY,
    ADA_SELECT_TERMINATE
};

enum AdaSelectClock
{
    ADA_SELECT_MONOTONIC,
    ADA_SELECT_CALENDAR,
    ADA_SELECT_RELATIVE
};

enum AdaEntryCallMode
{
    ADA_ENTRY_WAIT,
    ADA_ENTRY_TIMED,
    ADA_ENTRY_CONDITIONAL
};

enum { kAdaSelectChoiceBytes = 32 };

/* Optional trailing argument of dispatch calls. Ordinary implementations ignore
   it; entry adapters report selection without changing the source profile. */
enum
{
    kAdaDispatchSelectBytes = 32,
    kAdaDispatchDeadline = 0,
    kAdaDispatchMode = 8,
    kAdaDispatchClock = 12,
    kAdaDispatchSelected = 16,
    kAdaDispatchAsynchronous = 20,
    kAdaDispatchHandle = 24
};

#endif
