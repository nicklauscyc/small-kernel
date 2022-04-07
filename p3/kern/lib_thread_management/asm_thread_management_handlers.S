
#include <asm_interrupt_handler_template.h>

CALL_W_RETVAL_HANDLER(gettid)

CALL_W_SINGLE_ARG(yield)

CALL_W_SINGLE_ARG(deschedule)

CALL_W_SINGLE_ARG(make_runnable)
