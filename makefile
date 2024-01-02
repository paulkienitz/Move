# This is version 2 of Move/Name, for dos 2.04+ only, unlike older versions.
# This makefile is for the PD make on fish disk 69, not the Manx make.

# Originally we had a bunch of other source files which supported parsing
# command line args according to a template.  These are necessary if you
# want to compile a version compatible with AmigaDOS 1.x, but at this point
# we are only supporting an AmigaDOS 2.04+ version of Move/Name.


# release targets:

both : m n

m : ram:Move

n : ram:Name

# debug targets:

dbg : sm sn

sm : ram:smove

sn : ram:sname




CFLAGS = -ps -ss
C = $(CFLAGS)
L = -m +q
S = -bs -s0f0n


# sdb'able and db'able versions:

ram\:smove : t:smove.o
	ln $L -g -w -o ram:smove t:smove.o -lc16
	## @delete quiet t:smove.o
	@dr -l ram:smove\#?

t\:smove.o : move.c
	cc $C $S -o t:smove.o move.c

ram\:sname : t:sname.o
	ln $L -g -w -o ram:sname t:sname.o -lc16
	## @delete quiet t:sname.o
	@dr -l ram:sname\#?

t\:sname.o : move.c
	cc $C $S -d NAME -o t:sname.o move.c



# finished non-debuggable versions:

ram\:Move : t:move.o lib:purify.o
	ln $L -o ram:Move t:move.o lib:purify.o -lc16
	@protect ram:Move +p
	## @delete quiet t:move.o
	@dr ram:Move

t\:move.o : move.c
	cc $C -o t:move.o move

ram\:Name : t:name.o lib:purify.o
	ln $L -o ram:Name t:name.o lib:purify.o -lc16
	@protect ram:Name +p
	##@delete quiet t:name.o
	@dr ram:Name

t\:name.o : move.c
	cc $C -d NAME -o t:name.o move.c


# this goes in lib so it's shared with other projects like CLImax:

lib\:purify.o : purify.a
	as purify.a -o lib:purify.o
