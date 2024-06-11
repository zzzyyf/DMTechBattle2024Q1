CXX=g++
CPPFLAGS=-Wall -g3 -std=c++2a

DBGFLAGS=-O3 -DNDEBUG

INC=-Ithird_party
LIBS=-L/usr/local/lib -Lthird_party/libs -pthread -lz # shouldn't use -lpthread

SRCDIR=./src
SRCS=$(wildcard $(SRCDIR)/*.cpp)

BUILDDIR=./obj
OBJS=$(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SRCS))

TARGET=benchmark.exe

.PHONY: build
build : $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

$(BUILDDIR)/%.o : $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(DBGFLAGS) $(INC) -c $< -o $@

.PHONY: clean
clean :
	rm -rf $(BUILDDIR)/*
	rm -r $(TARGET)
