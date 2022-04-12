
#include <asm_interrupt_handler_template.h>

/* should be call w void val? */
CALL_W_RETVAL_HANDLER(vanish)
CALL_W_RETVAL_HANDLER(task_vanish)
CALL_W_RETVAL_HANDLER(set_status)

CALL_W_RETVAL_HANDLER(fork)
CALL_W_DOUBLE_ARG(exec)

