/** @file is_valid_pd.c
 *  @brief Implements page directory and page consistency checks via functions
 *         is_valid_pd() and is_valid_pt()
 */

#include <memory_manager.h>

/** @brief Checks if paget table at index i of a page directory is valid or not.
 *
 *	If the address is < USER_MEM_START, does not validate the address.
 *	TODO perhaps find a way to validate such addresses as well?
 *
 *	@param pt Page table address to check
 *	@param pd_index The index this page table address was stored in the
 *		   page directory
 */
static int
is_valid_pt( uint32_t *pt, int pd_index )
{
	/* Basic page table address checks */
	if (!pt) {
		log_warn("is_valid_pt(): "
                 "pt: %p is NULL!", pt);
		return 0;
	}
	if (!PAGE_ALIGNED(pt)) {
		log_warn("is_valid_pd(): "
                 "pt: %p is not page aligned!", pt);
		return 0;
	}
	if ((uint32_t) pt >= USER_MEM_START) {
		log_warn("is_valid_pt(): pt: %p is above USER_MEM_START!", pt);
		return 0;
	}

	/* Iterate over page table and check each entry */
	for (int i = 0; i < PAGE_SIZE / sizeof(uint32_t); ++i) {
		uint32_t pt_entry = pt[i];
		//if (pd_index == 0x3ff) {
		//	if (pt_entry) log("pt_entry:0x%08lx", pt_entry);
		//}
        assert((((pt_entry) != 0) && (TABLE_ADDRESS(pt_entry) != 0))
	           || (pt_entry == 0));


        //TABLE_ENTRY_INVARIANT(pt_entry));

		/* Check only if entry is non-NULL, ignoring bottom 12 bits */
		if (TABLE_ADDRESS(pt_entry)) {

			uint32_t phys_address = TABLE_ADDRESS(pt_entry);

			/* Present bit must be set */
			if (!(pt_entry & PRESENT_FLAG)) {
				log_warn("is_valid_pt(): "
                         "present bit not set for "
                         "pt:%p "
						 "pd_index:0x%08lx "
                         "pt_entry:0x%08lx "
						 "phys_address:0x%08lx "
                         "pt_index:0x%08lx",
                         pt, pd_index, pt_entry, phys_address, i);
                log_warn("virtual address of pt_entry:%p", &pt_entry);
                MAGIC_BREAK;
				return 0;
			}
			/* pt holds physical frames for user memory */
			if (pd_index >= (USER_MEM_START >> PAGE_DIRECTORY_SHIFT)) {

                /* Frame cannot be equal to pt */
                if (TABLE_ADDRESS(pt_entry) == (uint32_t) pt) {
                    log_warn("is_valid_pt(): "
                             "pt at address:%p same address as frame physical "
                             "address: %p with pt_entry: 0x%08lx at "
                             "index: 0x%08lx",
                             pt, (uint32_t *) phys_address, pt_entry, i);
                }


				/* Frame must be a valid physical address by physalloc */
				if (!is_physframe(phys_address)) {
					log_warn("is_valid_pt(): "
                             "pt at address: %p has invalid frame physical "
							 "address: %p with pt_entry: 0x%08lx at "
							 "index: 0x%08lx",
							 pt, (uint32_t *) phys_address, pt_entry, i);
					return 0;
				}
			/* pt holds physical frame in kernel VM */
			} else {

				/* Frame must be < USER_MEM_START */
				if (phys_address >= USER_MEM_START) {
					log_warn("is_valid_pt(): "
                             "pt at address: %p has invalid frame physical "
							 "address: %p >= USER_MEM_START with pt_entry: "
							 "0x%08lx at index: 0x%08lx",
							 pt, (uint32_t *) phys_address, pt_entry, i);
					return 0;
				}
			}
		}
	}
	return 1;
}

/** @brief Checks if supplied page directory is valid or not
 *
 *	TODO keep a record of all page directory addresses ever given out?
 *
 *	@param pd Page directory to check
 *	@return 1 if pd points to a valid page directory, 0 otherwise
 */
int
is_valid_pd( void *pd )
{
	/* Basic page directory address checks */
	if (!pd) {
		log_warn("is_valid_pd(): pd: %p is NULL!", pd);
		return 0;
	}
	if (!PAGE_ALIGNED(pd)) {
		log_warn("is_valid_pd(): pd: %p is not page aligned!", pd);
		return 0;
	}
	if ((uint32_t) pd >= USER_MEM_START) {
		log_warn("is_valid_pd(): pd: %p is above USER_MEM_START!", pd);
		return 0;
	}
	/* Iterate over page directory and check each entry */
	uint32_t **pd_cast = (uint32_t **) pd;
	for (int i = 0; i < PAGE_SIZE / sizeof(uint32_t); ++i) {
		uint32_t *pd_entry = pd_cast[i];
        assert(TABLE_ENTRY_INVARIANT(pd_entry));


		/* Check only if entry is non-NULL, ignoring bottom 12 bits */
		if (TABLE_ADDRESS(pd_entry)) {
			uint32_t *pt = (uint32_t *) TABLE_ADDRESS(pd_entry);

			/* Present bit must be set */
			if (!((uint32_t) pd_entry & PRESENT_FLAG)) {
				log_warn("is_valid_pd(): "
                "pd at address: %p has non-present pt at address: %p "
						 "with pd_entry: 0x%08lx at index: 0x%08lx",
						 pd, pt, pd_entry, i);
				return 0;
			}
			/* Page table at address must be valid */
			if (!is_valid_pt(pt, i)) {
				log_warn("is_valid_pd(): "
                "pd at address: %p has invalid pt at address: %p "
						 "with pd_entry: 0x%08lx at index: 0x%08lx",
						 pd, pt, pd_entry, i);
				return 0;
			}
		}
	}
	return 1;
}
