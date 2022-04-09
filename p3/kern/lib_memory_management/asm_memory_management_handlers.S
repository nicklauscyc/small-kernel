#include <asm_interrupt_handler_template.h>

CALL_HANDLER(pagefault_handler)

CALL_W_DOUBLE_ARG(new_pages)

CALL_W_SINGLE_ARG(remove_pages)

