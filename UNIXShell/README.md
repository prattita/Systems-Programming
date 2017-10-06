Implementation of a functional UNIX Shell. 
Allows for the creation and management of UNIX processes with fork, exec, and wait
Uses signals handlers that handle exceptional control flow

Files:

Makefile	# Compiles the shell program and runs the tests
README.md	# This file
tsh.c		  # The shell program 
tshref		# The reference shell binary.

# Files used to test the shell
sdriver.pl	# The trace-driven shell driver
trace*.txt	# The 15 trace files that control the shell driver
tshref.out 	# Example output of the reference shell on all 15 traces

# Little C programs that are called by the trace files
myspin.c	      # Takes argument <n> and spins for <n> seconds
mysplit.c	      # Forks a child that spins for <n> seconds
mystop.c        # Spins for <n> seconds and sends SIGTSTP to itself
myint.c         # Spins for <n> seconds and sends SIGINT to itself
