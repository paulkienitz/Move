# temparse.lib will hopefully someday become templateparse.library --
# a means of parsing AmigaDOS command templates without either AmigaDOS
# 2.x or arp.library -- also available for Aztec linker as temparse.lib

temparse.lib: arse.c cookt.c temparse.c patmatch.c fault.c packet.c
	cc -ps -wp -bs -s0f0n -o t:arse.o     arse.c
	cc -ps -wp -bs -s0f0n -o t:cookt.o    cookt.c
	cc -ps -wp -bs -s0f0n -o t:temparse.o temparse.c
	cc -ps -wp -bs -s0f0n -o t:patmatch.o patmatch.c
	cc -ps -wp -bs -s0f0n -o t:fault.o    fault.c
	cc -ps -wp -bs -s0f0n -o t:packet.o   packet.c
	lb temparse t:temparse t:cookt t:arse t:patmatch t:fault t:packet
	@-list temparse.lib

temparseL.lib: arse.c cookt.c temparse.c patmatch.c fault.c packet.c
	cc -pl -mc -md -wp -bs -s0f0n -o t:arse.o     arse.c
	cc -pl -mc -md -wp -bs -s0f0n -o t:cookt.o    cookt.c
	cc -pl -mc -md -wp -bs -s0f0n -o t:temparse.o temparse.c
	cc -pl -mc -md -wp -bs -s0f0n -o t:patmatch.o patmatch.c
	cc -pl -mc -md -wp -bs -s0f0n -o t:fault.o    fault.c
	cc -pl -mc -md -wp -bs -s0f0n -o t:packet.o   packet.c
	lb temparseL t:temparse t:cookt t:arse t:patmatch t:fault t:packet
	@-list temparseL.lib

testframe: arse.c cookt.c temparse.c patmatch.c fault.c packet.c
	cc -ps -wp -bs -s0f0n -o t:arse.o     arse.c
	cc -ps -wp -bs -s0f0n -o t:cookt.o    cookt.c
	cc -ps -wp -bs -s0f0n -o t:patmatch.o patmatch.c
	cc -ps -wp -bs -s0f0n -o t:fault.o    fault.c
	cc -ps -wp -bs -s0f0n -o t:packet.o   packet.c
	cc -ps -wp -bs -s0f0n -d TESTFRAME -o t:temparse.o temparse.c
	ln -g -w -o testframe t:temparse t:cookt t:arse t:patmatch t:fault t:packet -lc16
	@-list testframe

clean:
	@-delete t:\#?.o ram:FRX\#?

frx : ram:FRX

ram\:FRX: frx.c temparse.lib lib:purify.o
	cc -ps -wp -bs -s0f0n -o t:frx.o frx.c
	ln +q -m -g -o ram:FRX t:frx.o temparse.lib purify.o -lc16
	protect ram:FRX +p
	@-list ram:FRX\#? nohead
