/** Context switch facilities */

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

struct pcb {
    void *ptd_start; // Start of page table directory
    tcb_t *first_thread; // First thread in linked list

};

struct tcb {
    pcb_t *owning_task;
    tcb_t *next_thread; // Embeded linked list of threads from same process

};

/** Switch to a new process. This function takes a while to return.
 *  If thread belongs to currently running process, only registers are
 *  updated. */
void
switch_process( int tid )
{

}

