# Malloc Implementation 

Implemented a light version of the C memory allocator and deallocator in C.
Tested utilization and throughput given by different memory management approaches and heuristics.


# Main Files:

- mm.{c,h}	
	mm.c is the main file

- mdriver.c	
	The malloc driver that tests the mm.c file

- short{1,2}-bal.rep
	Two small tracefiles

- Makefile	
	Builds the driver

# Support files for the driver

- config.h	    Configures the malloc lab driver
- fsecs.{c,h} 	Wrapper function for the different timer packages
- clock.{c,h} 	Routines for accessing the Pentium and Alpha cycle counters
- fcyc.{c,h}	  Timer functions based on cycle counters
- ftimer.{c,h}	Timer functions based on interval timers and gettimeofday()
- memlib.{c,h}	Models the heap and sbrk function
