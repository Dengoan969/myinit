all: myinit

myinit: myinit.o
	gcc -o myinit myinit.o

myinit.o: myinit.c
	  gcc -c myinit.c

.PHONY: all