#include "../../runtime/adart.h"

/* Model the hidden-parameter entry in a separate translation unit so the C
   compiler cannot inline it into the benchmark caller. The frame work matches
   __ada_trace_enter; only context acquisition differs. */
AdaTaskContext* hiddenContextEntry(AdaTaskContext* context, AdaTraceFrame* frame,
                                  const char* routine, const char* location)
{
    frame->previous = context->m_currentTrace;
    frame->routine = routine;
    frame->location = location;
    context->m_currentTrace = frame;
    return context;
}
