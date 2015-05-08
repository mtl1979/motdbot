# compilation flags used under any OS or compiler (may be appended to, below)
CXXFLAGS   += -Imuscle -DMUSCLE_SINGLE_THREAD_ONLY

# compilation flags that are specific to the gcc compiler (hard-coded)
GCCFLAGS    = -fno-exceptions -DMUSCLE_NO_EXCEPTIONS -Wall -W -Wno-multichar

# flags to include when compiling the optimized version (with 'make optimized')
CCOPTFLAGS  = -g

# flags to include when linking (set per operating system, below)
LFLAGS      =

# libraries to include when linking (set per operating system, below)
LIBS        =

# names of the executables to compile
EXECUTABLES = motdbot

# object files to include in all executables
OBJFILES = Message.o AbstractMessageIOGateway.o MessageIOGateway.o String.o AbstractReflectSession.o DataNode.o FilePathInfo.o Directory.o DumbReflectSession.o MiscUtilityFunctions.o StorageReflectSession.o ReflectServer.o SignalHandlerSession.o SignalMultiplexer.o SocketMultiplexer.o StringMatcher.o motdbot.o NetworkUtilityFunctions.o SysLog.o PulseNode.o PathMatcher.o FilterSessionFactory.o RateLimitSessionIOPolicy.o MemoryAllocator.o SetupSystem.o ServerComponent.o ZLibCodec.o ByteBuffer.o QueryFilter.o

# Where to find .cpp files
VPATH = muscle/message muscle/iogateway muscle/reflector muscle/regex muscle/util muscle/syslog muscle/system muscle/zlib muscle/zlib/zlib

# if the OS type variable is unset, try to set it using the uname shell command
ifeq ($(OSTYPE),)
  OSTYPE = $(shell uname)
endif

# IRIX may report itself as IRIX or as IRIX64.  They are both the same to us.
ifeq ($(OSTYPE),IRIX64)
  OSTYPE = IRIX
endif

ifeq ($(OSTYPE),beos)
  ifeq ($(BE_HOST_CPU),ppc)
    CXX = mwcc
    OBJFILES += regcomp.o regerror.o regexec.o regfree.o
    VPATH += ../regex/regex
    CFLAGS += -I../regex/regex
  else # not ppc
    CXXFLAGS += $(GCCFLAGS) $(CCOPTFLAGS)
    LIBS = -lbe -lnet -lroot
    ifeq ($(shell ls 2>/dev/null -1 /boot/develop/headers/be/bone/bone_api.h), /boot/develop/headers/be/bone/bone_api.h)
      CXXFLAGS += -I/boot/develop/headers/be/bone -DBONE
      LIBS = -nodefaultlibs -lbind -lsocket -lbe -lroot -L/boot/beos/system/lib
    endif
  endif # END ifeq ($(BE_HOST_CPU),ppc)
else # not beos
  CXXFLAGS += $(GCCFLAGS) $(CCOPTFLAGS)
  ifeq ($(OSTYPE),freebsd4.0)
    CXXFLAGS += -I/usr/include/machine
  else # not freebsd4.0
    ifeq ($(OSTYPE),darwin)
      CXX = c++
      CXXFLAGS += -I/usr/include/machine
    else # not darwin
      ifeq ($(OSTYPE),IRIX)
        CXXFLAGS += -DSGI -DMIPS
        ifneq (,$(findstring g++,$(CXX))) # if we are using SGI's CC compiler, we gotta change a few things
          CXX = CC
          CCFLAGS = -g2 -n32 -LANG:std -woff 1110,1174,1552,1460,3303
          LFLAGS  = -g2 -n32
          CXXFLAGS += -DNEW_H_NOT_AVAILABLE
        endif # END ifneq (,$(findstring g++,$(CXX)))
      endif # END ifeq ($(OSTYPE),IRIX)
    endif # END ifeq ($(OSTYPE),darwin)
  endif # END ifeq ($(OSTYPE),freebsd4.0)
endif #END ifeq ($(OSTYPE),beos)

all : $(EXECUTABLES)

optimized : CCOPTFLAGS = -O3
optimized : all

motdbot : $(OBJFILES)
	$(CXX) $(LIBS) $(LFLAGS) -o $@ $^

clean :
	rm -f *.o *.xSYM $(EXECUTABLES)
