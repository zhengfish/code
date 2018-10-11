###############################################################################
## Makefile for BasicFTPD
##
## Usage: make platform=<linux|solaris|freebsd>
##
## Example: make platform=linux
##
###############################################################################

# The compiler to be used
CC = gcc

# Target file name (without extension).
TARGET = basicftpd

# Set the platform parameters
# Initialize
BUILDPLATFORM = NONE
# Linux parameters
ifeq ($(platform),linux)
  BUILDPLATFORM = LINUX
  LDFLAGS = -lstdc++ -lpthread
endif
# Solaris parameters
ifeq ($(platform),solaris)
  BUILDPLATFORM = SOLARIS
  LDFLAGS = -lnsl -lsocket -lstdc++ -lpthread
endif
# FreeBSD parameters
ifeq ($(platform),freebsd)
  BUILDPLATFORM = BSD
  LDFLAGS = -lc_r -pthread -lcompat
endif

# Set the compiler options
CFLAGS = -O2 -pedantic -Wall -Wstrict-prototypes -fno-builtin -D$(BUILDPLATFORM)


# List C++ source files here. (C++ dependencies are automatically generated.)
# Core source files
FILESCORE = BlowfishCrypt.cpp CmdLine.cpp Crypto.cpp Dll.cpp FSUtils.cpp \
            Ftps.cpp FtpsXfer.cpp Log.cpp Service.cpp SiteInfo.cpp Sock.cpp \
            SSLSock.cpp StrUtils.cpp Termcli.cpp Termsrv.cpp Thr.cpp Timer.cpp
# BasicFTPD source files
FILESBASIC = basicmain.cpp

# Set the directory path for the core files
SRCCORE = $(FILESCORE:%=../core/%)
# Set the directory path for the BasicFTPD files
SRCBASIC = $(FILESBASIC:%=./%)

# Define the core object files.
OBJCORE = $(SRCCORE:.cpp=.o)
# Define the BasicFTPD object files.
OBJBASIC = $(SRCBASIC:.cpp=.o)


# Default target (no arguments to make)
all: begin checkplatform build

begin:
ifeq ($(BUILDPLATFORM),NONE)
	@echo "Usage: make platform=<linux|solaris|freebsd>"
else
	@echo "Building for" $(platform)
endif

checkplatform:
ifeq ($(BUILDPLATFORM),NONE)
	$(error No platform was specified) 
endif

build: $(OBJCORE) $(OBJBASIC)
	$(CC) -o $(TARGET) $(OBJCORE) $(OBJBASIC) $(LDFLAGS)

# The clean target is used to remove all machine generated files 
# and start over from a clean slate.
clean:
	rm -f $(OBJCORE)
	rm -f $(OBJBASIC)
	rm -f $(SRCCORE:.cpp=.d)
	rm -f $(SRCBASIC:.cpp=.d)
	rm -f $(TARGET)

# Compile: create object files from C source files.
%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

# Automatically generate C++ source code dependencies. 
#
%.d: %.cpp
	set -e; $(CC) -MM $(CFLAGS) $< \
	| sed 's,\(.*\)\.o[ :]*,\1.o \1.d : ,g' > $@; \
	[ -s $@ ] || rm -f $@

-include $(SRCCORE:.cpp=.d)
-include $(SRCBASIC:.cpp=.d)
