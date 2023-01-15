#
# Makefile for ec editor
#

all: ec

UNAME = uname -s
OS = $(UNAME:sh)$(shell $(UNAME))
CFLAGS_EXTRA = -D$(OS)

SRC = ec.cc ecbuf.cc termx.cc keyx.cc
OBJ = $(SRC:.cc=.o)

ec: ec.o ecbuf.o termx.o keyx.o
	$(CXX) $(OBJ) -lcurses -o $@

#	$(CXX) $(OBJ) -ltermcap -o $@
#	$(CXX) $(OBJ) -ltermcap -lstdc++ -o $@

clean:
	rm -f *.o .del-depend

distclean: clean
	rm -f ec .del-* *.E *.asm *.dis .gdb_history

# ----------------- Standard Rules ------------------

.SUFFIXES:  .cc .asm .dis

CXXFLAGS = -g -Wall -O2 $(CFLAGS_EXTRA)

CXX = g++
LD = ld
RM = rm -f

# generate list of dependencies to be included into Makefile
.del-depend:
	$(CXX) $(CFLAGS) $(CFLAGS_$@) -M $(SRC) > $@

%.o: %.cc
	@ $(RM) $@
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_$@) -c $<

# preprocess only
%.E: %.cc
	@ $(RM) $@
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_$@) -E $< > $@

# disassemble C++ (renames .s to .asm so .s doesn't get assembled)
%.asm: %.cc
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_$@) -S $<
	mv $(*F).s $@

%.nm: %.o
	nm $< > $@

%.U: %.o
	nm $< | grep ' U ' > $@

# disassemble binary and object files
%.dis: %
	objdump --disassemble-all --show-raw-insn --source $< > $@

%.dis: %.o
	objdump --disassemble-all --show-raw-insn --source $< > $@

include .del-depend
