#include <util.h>
#include <common_kern.h> /* USER_MEM_START */


// TODO: Just move to memory manager? Then easily check pd/pt
/** @brief Checks if a user pointer is valid.
 *	Valid means the pointer is non-NULL, belongs to
 *	user memory and is in an allocated memory region.
 *
 *	@param ptr Pointer to check
 *	@return 1 if valid, 0 if not */
int
is_user_pointer_valid(void *ptr)
{
	/* Check not kern memory and non-NULL */
	if ((unsigned int)ptr < USER_MEM_START)
		return 0;

	/* Check if allocated. Can try accessing and handling pf? */
	return 1;
}
