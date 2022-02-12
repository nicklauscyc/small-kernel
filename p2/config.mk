###########################################################################
#
#    #####          #######         #######         ######            ###
#   #     #            #            #     #         #     #           ###
#   #                  #            #     #         #     #           ###
#    #####             #            #     #         ######             #
#         #            #            #     #         #
#   #     #            #            #     #         #                 ###
#    #####             #            #######         #                 ###
#
#
# Please read the directions in README and in this config.mk carefully.
# Do -N-O-T- just dump things randomly in here until your project builds.
# If you do that, you run an excellent chance of turning in something
# which can't be graded.  If you think the build infrastructure is
# somehow restricting you from doing something you need to do, contact
# the course staff--don't just hit it with a hammer and move on.
#
# [Once you've read this message, please edit it out of your config.mk!!]
###########################################################################



###########################################################################
# This is the include file for the make file.
###########################################################################
# You should have to edit only this file to get things to build.
#

###########################################################################
# Tab stops
###########################################################################
# If you use tabstops set to something other than the international
# standard of eight characters, this is your opportunity to inform
# our print scripts.
TABSTOP = 4

##################################################
# Compiler
##################################################
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
# You can set CONFIG_DEBUG to "user" if you want
# the DEBUG flag to be #define'd for user code
# (all of P2 is user code).  This will cause the
# checks in contracts.h to be made, at the expense
# of some performance.
#
# Use "make veryclean" if you adjust CONFIG_DEBUG.
#
CONFIG_DEBUG = user

###########################################################################
# NDEBUG
###########################################################################
# You can set CONFIG_NDEBUG to "user" if you want
# the NDEBUG flag to be #define'd for user code
# (all of P2 is user code). This will cause the
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
# WARNING: Do not put extraneous test programs into the REQPROGS variables.
#          Doing so will put your grader in a bad mood which is likely to
#          make him or her less forgiving for other issues.
###########################################################################

###########################################################################
# Mandatory programs whose source is provided by course staff
###########################################################################
# A list of the programs in 410user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN
#
# The idle process is a really good thing to keep here.
#
410REQPROGS = idle

###########################################################################
# Mandatory programs provided in binary form by course staff
###########################################################################
# A list of the programs in 410user/progs which are provided in binary
# form and NECESSARY FOR THE KERNEL TO RUN
#
# You may move these to 410REQPROGS to test your syscall stubs once
# they exist, but we will grade you against the binary versions.
# This should be fine.
#
410REQBINPROGS = shell init

###########################################################################
# WARNING: When we test your code, the two TESTS variables below will be
# ignored.  Your kernel MUST RUN WITHOUT THEM.
###########################################################################

###########################################################################
# Test programs provided by course staff you wish to run
###########################################################################
# A list of the test programs you want compiled in from the 410user/progs
# directory
#
410TESTS = cat

###########################################################################
# Test programs you have written which you wish to run
###########################################################################
# A list of the test programs you want compiled in from the user/progs
# directory
#
STUDENTTESTS = test_set_status

###########################################################################
# Object files for your thread library
###########################################################################
THREAD_OBJS = malloc.o panic.o

# Thread Group Library Support.
#
# Since libthrgrp.a depends on your thread library, the "buildable blank
# P2" we give you can't build libthrgrp.a.  Once you set up your thread
# library and fix THREAD_OBJS above, uncomment this line to enable building
# libthrgrp.a:
#410USER_LIBS_EARLY += libthrgrp.a

###########################################################################
# Object files for your syscall wrappers
###########################################################################
SYSCALL_OBJS = syscall.o set_status.o vanish.o task_vanish.o print.o \
			   fork.o exec.o wait.o gettid.o yield.o deschedule.o \
			   make_runnable.o get_ticks.o sleep.o swexn.o new_pages.o \
			   getchar.o readline.o set_term_color.o get_cursor_pos.o \
			   set_cursor_pos.o halt.o readfile.o \
			   get_ticks.o

###########################################################################
# Object files for your automatic stack handling
###########################################################################
AUTOSTACK_OBJS = autostack.o

###########################################################################
# Data files provided by course staff to be included in the RAM disk
###########################################################################
# A list of the data files you want copied in from the 410user/files
# directory (see 410user/progs/cat.c)
#
410FILES =

###########################################################################
# Data files you provide to be included in the RAM disk
###########################################################################
# A list of the data files you want copied in from the user/files
# directory (see 410user/progs/cat.c)
#
STUDENTFILES =

