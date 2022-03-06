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
# Do -N-O-T- just dump things randomly in here until your kernel builds.
# If you do that, you run an excellent chance of turning in something
# which can't be graded.  If you think the build infrastructure is
# somehow restricting you from doing something you need to do, contact
# the course staff--don't just hit it with a hammer and move on.
#
# [Once you've read this message, please edit it out of your config.mk]
# [Once you've read this message, please edit it out of your config.mk]
# [Once you've read this message, please edit it out of your config.mk]
###########################################################################

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
UPDATE_METHOD = web

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
410TESTS =

###########################################################################
# Test programs you have written which you wish to run
###########################################################################
# A list of the test programs you want compiled in from the user/progs
# directory.
#
STUDENTTESTS =

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
STUDENTFILES =

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
SYSCALL_OBJS = syscall.o

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
			  keybd_driver.o timer_driver.o install_handler.o \
			  asm_interrupt_handler.o


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
410REQPROGS = idle init shell

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
STUDENTREQPROGS =
