# This is version 2 of Move/Name, for dos 2.04+ only, unlike older versions.
# This makefile is for the PD make on fish disk 69, not the Manx make.

# Originally we had a bunch of other source files which supported parsing
# command line args according to a template.  These are necessary if you
# want to compile a version compatible with AmigaDOS 1.x, but at this point
# we are only supporting an AmigaDOS 2.04+ version of Move/Name.


# release targets supporting AmigaDOS 1.x:

both : m n

m : ram:Move

n : ram:Name

# debug targets:

dbg : sm sn

sm : ram:smove

sn : ram:sname

# AmigaDOS 2.x-only targets:

two : m2 n2

m2 : ram:Move2

n2 : ram:Name2


CFLAGS = -ps -ss
C = $(CFLAGS)
L = -m +q
S = -bs -s0f0n


# sdb'able and db'able versions:

ram\:smove : t:smove.o lib:temparse.lib
	ln $L -g -w -o ram:smove t:smove.o -ltemparse -lc16
	## @delete quiet t:smove.o
	@dr -l ram:smove\#?

t\:smove.o : move.c lib:temparse.lib
	cc $C $S -d ONE -o t:smove.o move.c

ram\:sname : t:sname.o lib:temparse.lib
	ln $L -g -w -o ram:sname t:sname.o -ltemparse -lc16
	## @delete quiet t:sname.o
	@dr -l ram:sname\#?

t\:sname.o : move.c
	cc $C $S -d NAME -d ONE -o t:sname.o move.c



# finished non-debuggable versions for 1.x:

ram\:Move : t:move.o lib:purify.o lib:temparse.lib
	ln $L -o ram:Move t:move.o purify.o -ltemparse -lc16
	@protect ram:Move +p
	## @delete quiet t:move.o
	@dr ram:Move

t\:move.o : move.c
	cc $C -d ONE -o t:move.o move

ram\:Name : t:name.o lib:purify.o lib:temparse.lib
	ln $L -o ram:Name t:name.o purify.o temparse.lib -lc16
	@protect ram:Name +p
	##@delete quiet t:name.o
	@dr ram:Name

t\:name.o : move.c
	cc $C -d NAME -d ONE -o t:name.o move.c



# finished non-debuggable versions for 2.x only:

ram\:Move2 : t:move2.o lib:purify.o
	ln $L -o ram:Move2 t:move2.o purify.o -lc16
	@protect ram:Move2 +p
	## @delete quiet t:move2.o
	@dr ram:Move2

t\:move2.o : move.c
	cc $C -o t:move2.o move

ram\:Name2 : t:name2.o lib:purify.o
	ln $L -o ram:Name2 t:name2.o purify.o temparse.lib -lc16
	@protect ram:Name2 +p
	##@delete quiet t:name2.o
	@dr ram:Name2

t\:name2.o : move.c
	cc $C -d NAME -o t:name2.o move.c
        

# this goes in lib so it's shared with other projects like CLImax:

lib\:purify.o : purify.a
	as purify.a -o lib:purify.o
