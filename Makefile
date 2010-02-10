LLVM_CXXFLAGS = `llvm-config --cxxflags`
LLVM_LDFLAGS = `llvm-config --ldflags --libs backend engine`

.cpp.o :
	g++ $(LLVM_CXXFLAGS) -m32 -Wall -g -c -MMD -MF$@.dep -o $@ $<

OBJS = \
	src-new/error.o \
	src-new/printer.o \
	src-new/lexer.o \
	src-new/parser.o \
	src-new/env.o \
	src-new/loader.o \
	src-new/llvm.o \
	src-new/types.o \
	src-new/main.o

clay2llvm : $(OBJS)
	g++ -m32 -g -o clay2llvm $(OBJS) $(LLVM_LDFLAGS)

clean :
	rm -f clay2llvm
	rm -f src-new/*.o
	rm -f src-new/*.dep

-include src-new/*.dep
