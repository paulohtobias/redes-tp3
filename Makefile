#Main Makefile
CC := gcc
CFLAGS := -g -Wall -MMD

#Binary
BIN := main.out

#Directories
IDIR := ./include
SDIR := ./src

#Files
SOURCE := .c

#Paths
INCLUDE_PATHS := -I$(IDIR)

#Libraries
LIBS :=
#CFLAGS += `pkg-config --cflags $(LIBS)`
#LOADLIBES := `pkg-config --libs $(LIBS)`

#Compilation line
COMPILE := $(CC) $(CFLAGS) $(INCLUDE_PATHS)

#FILEs
#---------------Source----------------#
SRCS := $(wildcard $(SDIR)/*$(SOURCE)) $(wildcard $(SDIR)/*/*$(SOURCE))

#---------------Object----------------#
OBJS := $(SRCS:$(SDIR)/%$(SOURCE)=$(ODIR)/%.o)
#-------------Dependency--------------#
DEPS := $(SRCS:$(SDIR)/%$(SOURCE)=$(ODIR)/%.d)

all: $(OBJS)
	$(COMPILE) $(OBJS) main$(SOURCE) -o $(BIN) $(LOADLIBES)

# Build main application for debug
debug: CFLAGS += -g -DDEBUG
debug: all

# Include all .d files
-include $(DEPS)

$(ODIR)/%.o: $(SDIR)/%$(SOURCE)
	$(COMPILE) -c $< -o $@ $(LOADLIBES)

.PHONY : clean
clean :
	-rm -f obj/* *.d $(BIN)

init:
	mkdir -p include
	mkdir -p src
	mkdir -p obj

run:
	./$(BIN)
