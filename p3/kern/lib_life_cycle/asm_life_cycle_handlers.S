
#include <asm_interrupt_handler_template.h>

/* should be call w void val? */
CALL_W_RETVAL_HANDLER(vanish)
CALL_W_SINGLE_ARG(task_vanish)
CALL_W_SINGLE_ARG(set_status)
CALL_W_SINGLE_ARG(wait)

CALL_W_RETVAL_HANDLER(fork)
CALL_W_DOUBLE_ARG(exec)

