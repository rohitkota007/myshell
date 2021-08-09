## Compiler Options (change accordingly for C/C++)
CC = g++
SUFFIX = c

## Add compiler flags as needed
CCFLAGS = -g -Wall
LDFLAGS = -g -Wall

## Paths
SHELL_ROOT=.
SRCDIR = $(SHELL_ROOT)
OBJDIR = $(SHELL_ROOT)
BINDIR = $(SHELL_ROOT)

## Name of target file
TARGET = shell

SRCS := $(wildcard $(SRCDIR)/*.$(SUFFIX))
OBJS := $(SRCS:$(SRCDIR)/%.$(SUFFIX)=$(OBJDIR)/%.o)

.PHONY: all setup clean
all: setup $(BINDIR)/$(TARGET)

## Setting Up
setup:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

## Compiling
$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.$(SUFFIX)
	$(CC) -c $< -o $@ $(CCFLAGS) -MD

## Including dependencies
-include $(OBJS:.o=.d)

## Building executable
$(BINDIR)/$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

## CleanUp
clean:
	rm -rf $(OBJS) $(OBJS:.o=.d) $(BINDIR)/$(TARGET) 
