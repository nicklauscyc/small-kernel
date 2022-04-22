###########################################################################
# This is the include file for the make file.
# You should have to edit only this file to get things to build.
###########################################################################

###########################################################################
# Tab stops
###########################################################################
# If you use tabstops set to something other than the international
# standard of eight characters, this is your opportunity to inform
# our print scripts.
TABSTOP = 4

###########################################################################
# Compiler
###########################################################################
# Selections (see handout for details):
#
# gcc - default (what we will grade with)
# clang - Clang/LLVM
# clangalyzer - Clang/LLVM plus static analyzer
#
# "gcc" may have a better Simics debugging experience
#
# "clang" may provide helpful warnings, assembly
# code may be more readable
#
# "clangalyzer" will likely complain more than "clang"
#
# Use "make veryclean" if you adjust CC.
CC = gcc

###########################################################################
# DEBUG
###########################################################################
# You can set CONFIG_DEBUG to any mixture of the words
# "kernel" and "user" to #define the DEBUG flag when
# compiling the respective type(s) of code.  The ordering
# of the words doesn't matter, and repeating a word has
# no additional effect.
#
# Use "make veryclean" if you adjust CONFIG_DEBUG.
#
CONFIG_DEBUG = user kernel

###########################################################################
# NDEBUG
###########################################################################
# You can set CONFIG_NDEBUG to any mixture of the words
# "kernel" and "user" to #define the NDEBUG flag when
# compiling the respective type(s) of code.  The ordering
# of the words doesn't matter, and repeating a word has
# no additional effect.  Defining NDEBUG will cause the
# checks using assert() to be *removed*.
#
# Use "make veryclean" if you adjust CONFIG_NDEBUG.
#
CONFIG_NDEBUG =

###########################################################################
# The method for acquiring project updates.
###########################################################################
# This should be "afs" for any Andrew machine, "web" for non-andrew machines
# and "offline" for machines with no network access.
#
# "offline" is strongly not recommended as you may miss important project
# updates.
#
UPDATE_METHOD = afs

###########################################################################
# WARNING: When we test your code, the two TESTS variables below will be
# blanked.  Your kernel MUST BOOT AND RUN if 410TESTS and STUDENTTESTS
# are blank.  It would be wise for you to test that this works.
###########################################################################

###########################################################################
# Test programs provided by course staff you wish to run
###########################################################################
# A list of the test programs you want compiled in from the 410user/progs
# directory.
#
410TESTS = getpid_test1 loader_test1 loader_test2 exec_basic exec_nonexist\
           fork_test1 new_pages remove_pages_test1 print_basic readline_basic\
		   actual_wait wait_getpid fork_wait fork_wait_bomb fork_exit_bomb\
		   sleep_test1 stack_test1 swexn_basic_test swexn_cookie_monster\
		   swexn_dispatch swexn_regs yield_desc_mkrun make_crash\
		   mem_permissions cho cho2 cho_variant score make_crash_helper\
		   knife exec_basic_helper\
		   remove_pages_test2 \

###########################################################################
# Test programs you have written which you wish to run
###########################################################################
# A list of the test programs you want compiled in from the user/progs
# directory.
#
STUDENTTESTS = test_suite exec_args_test exec_args_test_helper new_pages_test

###########################################################################
# Data files provided by course staff to build into the RAM disk
###########################################################################
# A list of the data files you want built in from the 410user/files
# directory.
#
410FILES =

###########################################################################
# Data files you have created which you wish to build into the RAM disk
###########################################################################
# A list of the data files you want built in from the user/files
# directory.
#
STUDENTFILES = readfile_test_data

###########################################################################
# Object files for your thread library
###########################################################################
THREAD_OBJS = malloc.o panic.o

# Thread Group Library Support.
#
# Since libthrgrp.a depends on your thread library, the "buildable blank
# P3" we give you can't build libthrgrp.a.  Once you install your thread
# library and fix THREAD_OBJS above, uncomment this line to enable building
# libthrgrp.a:
#410USER_LIBS_EARLY += libthrgrp.a

###########################################################################
# Object files for your syscall wrappers
###########################################################################
SYSCALL_OBJS = syscall.o gettid.o fork.o test.o yield.o deschedule.o \
			   exec.o make_runnable.o get_ticks.o sleep.o print.o \
			   set_cursor_pos.o get_cursor_pos.o set_term_color.o \
			   new_pages.o remove_pages.o readfile.o halt.o vanish.o \
			   readline.o task_vanish.o set_status.o swexn.o

###########################################################################
# Object files for your automatic stack handling
###########################################################################
AUTOSTACK_OBJS = autostack.o

###########################################################################
# Parts of your kernel
###########################################################################
#
# Kernel object files you want included from 410kern/
#
410KERNEL_OBJS = load_helper.o
#
# Kernel object files you provide in from kern/
#

KERNEL_OBJS = console.o kernel.o loader.o malloc_wrappers.o \
			  memory_manager.o task_manager.o iret_travel.o \
			  keybd_driver.o timer_driver.o install_handler.o \
			  asm_interrupt_handler.o context_switch.o \
			  scheduler.o logger.o tests.o atomic_utils.o panic.o\
			  \
			  lib_thread_management/asm_thread_management_handlers.o \
			  lib_thread_management/gettid.o \
			  lib_thread_management/get_ticks.o \
			  lib_thread_management/yield.o \
			  lib_thread_management/make_runnable.o \
			  lib_thread_management/deschedule.o \
			  lib_thread_management/sleep.o \
			  lib_thread_management/swexn.o \
			  lib_thread_management/hashmap.o \
			  lib_thread_management/mutex.o \
			  \
			  lib_life_cycle/asm_life_cycle_handlers.o \
			  lib_life_cycle/save_child_regs.o \
			  lib_life_cycle/fork.o \
			  lib_life_cycle/exec.o \
			  lib_life_cycle/vanish.o \
			  lib_life_cycle/task_vanish.o \
			  lib_life_cycle/set_status.o \
			  \
			  lib_memory_management/asm_memory_management_handlers.o \
			  lib_memory_management/new_pages.o \
			  lib_memory_management/remove_pages.o \
			  lib_memory_management/physalloc.o \
			  lib_memory_management/pagefault_handler.o \
			  lib_memory_management/is_valid_pd.o \
			  \
			  lib_console/asm_console_handlers.o \
			  lib_console/print.o \
			  lib_console/get_cursor_pos.o \
			  lib_console/set_cursor_pos.o \
			  lib_console/set_term_color.o \
			  lib_console/readline.o \
			  \
			  lib_misc/asm_misc_handlers.o \
			  lib_misc/readfile.o \
			  lib_misc/halt.o \
			  lib_misc/call_halt.o \
			  \
			  fault_handlers/asm_fault_handlers.o \
			  fault_handlers/divide_handler.o \
			  fault_handlers/debug_handler.o \
			  fault_handlers/breakpoint_handler.o \
			  fault_handlers/overflow_handler.o \
			  fault_handlers/bound_handler.o \
			  fault_handlers/invalid_opcode_handler.o \
			  fault_handlers/float_handler.o \
			  fault_handlers/segment_not_present_handler.o \
			  fault_handlers/stack_fault_handler.o \
			  fault_handlers/general_protection_handler.o \
			  fault_handlers/alignment_check_handler.o \
			  fault_handlers/non_maskable_handler.o \
			  fault_handlers/machine_check_handler.o \
			  fault_handlers/simd_handler.o


###########################################################################
# WARNING: Do not put **test** programs into the REQPROGS variables.  Your
#          kernel will probably not build in the test harness and you will
#          lose points.
###########################################################################

###########################################################################
# Mandatory programs whose source is provided by course staff
###########################################################################
# A list of the programs in 410user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN.
#
# The shell is a really good thing to keep here.  Don't delete idle
# or init unless you are writing your own, and don't do that unless
# you have a really good reason to do so.
#
410REQPROGS = idle shell

###########################################################################
# Mandatory programs whose source is provided by you
###########################################################################
# A list of the programs in user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN.
#
# Leave this blank unless you are writing custom init/idle/shell programs
# (not generally recommended).  If you use STUDENTREQPROGS so you can
# temporarily run a special debugging version of init/idle/shell, you
# need to be very sure you blank STUDENTREQPROGS before turning your
# kernel in, or else your tweaked version will run and the test harness
# won't.
#
STUDENTREQPROGS = init
