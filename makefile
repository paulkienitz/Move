# This is version 2 of Move/Name, for dos 2.04+ only, unlike older versions.
# This makefile is for the PD make on fish disk 69, not the Manx make.

# Originally we had a bunch of other source files which supported parsing
# command line args according to a template.  These are necessary if you
# want to compile a version compatible with AmigaDOS 1.x, but at this point
# we are only supporting an AmigaDOS 2.04+ version of Move/Name.


CFLAGS = -ps -ss ## -hi II16
C = $(CFLAGS)
L = -m +q

S = -bs -s0f0n

# make a sdb'able and db'able version:

ram\:smove : smove.o
	ln $L -g -w -o ram:smove smove.o -lc16
	@dr -l ram:smove\#?

smove.o : move.c
	cc $C $S -o smove.o move


# make a finished non-debuggable version:

m : ram:Move

ram\:Move : move.o
	ln $L -o ram:Move move.o purify.o -lc16
	@protect ram:Move +p
	@dr ram:Move

move.o : move.c
	cc $C move

# purify.o : purify.a		# commented out 'cause I keep purify.o
#	as purify.a		# in my object libraries directory


# make an alternate finished version with slightly different properties:

n : ram:Name

ram\:Name : name.o
	ln $L -o ram:Name name.o purify.o -lc16
	@protect ram:Name +p
	@dr ram:Name

name.o : move.c
	cc $C -d NAME -o name.o move.c
