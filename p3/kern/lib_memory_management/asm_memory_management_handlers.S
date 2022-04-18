#include <asm_interrupt_handler_template.h>


CALL_W_DOUBLE_ARG(new_pages)

CALL_W_SINGLE_ARG(remove_pages)

CALL_VAR_ARGS_FAULT_HANDLER_W_ERROR(pagefault_handler)

