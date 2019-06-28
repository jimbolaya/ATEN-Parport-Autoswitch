#  Makefile for a basic kernel module

CC=gcc
MODCFLAGS := -Wall -DMODULE -D__KERNEL__ -DLINUX -DDEBUG -O2

parswitch.o:  parswitch.c /usr/include/linux/version.h
	$(CC) $(MODCFLAGS) -c parswitch.c
	echo insmod parswich.o to turn it on
	echo insmod parswitch to turn it off
	echo
	echo X and kernel programming do not mix
	echo do the insmod and rmmod from outside X


clean :
	rm -f *~ core *.o
