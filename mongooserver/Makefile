# Makefile for Linux
#
#

CC      = gcc
CXX     = g++

# compiling flags here
CFLAGS 	+= -std=c99 -ansi
CFLAGS 	+= -O2 -Wall -g -ansi
CFLAGS 	+= -fomit-frame-pointer
CFLAGS 	+= -I.

LINKER  = gcc -o

# linking flags here
LFLAGS  = -Wall -I. -lm

# project name (generate executable with this name)
BIN   	= mongooserver

# change these to proper directories where each file should be
SRCDIR 	= .
OBJDIR  = .
BINDIR  = .

SRCS  := $(wildcard $(SRCDIR)/*.c)
OBJS  := $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)


$(BINDIR)/$(BIN): $(OBJS)
	@echo Linking ... $<
	@$(LINKER) $@ $(LFLAGS) $(OBJS)
	@echo "Linking complete!"

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	@echo Compiling ... $<
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@$(RM) $(OBJS)
	@$(RM) $(BINDIR)/$(BIN)
	@echo "Cleanup complete!"

###___END___

