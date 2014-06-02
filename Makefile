BINARIES = elastictab

VERSION_MAJOR	:= 0
VERSION_MINOR	:= 1
VERSION_AGE	:= 0
VERSION_EXTRA	:=

VERSION_STRING	= $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_AGE)$(if $(VERSION_EXTRA),-$(VERSION_EXTRA),)

# Compiler Settings

srcdir  := .
prefix	?= .
prefix	:= $(realpath $(prefix))
DESTDIR ?= $(prefix)/

RM	 = rm -f

# CC 	:= clang++
CC 	:= gcc

DEBUG 	?=
HARDEN	?=

ifdef DEBUG

DEFS		+= -DDEBUG
# CXXFLAGS	+= -g -O -march=native -mtune=native
CXXFLAGS	+= -g
LDFLAGS		+= -g

else

DEFS		+= -DNDEBUG
CXXFLAGS	+= -O3 -march=native -mtune=native -fomit-frame-pointer
LDFLAGS		+=

endif

ifdef HARDEN

# general more pedandic about errors !
CXXFLAGS += -Wextra -Wsign-conversion -Wformat-security -Wformat
CXXFLAGS += -Werror

# stack protection
CXXFLAGS += -fstack-protector-all -Wstack-protector --param=ssp-buffer-size=4

# ALSR
CXXFLAGS += -fPIE -pie

# traps signed overflows, but is currently broken in gcc
# CXXFLAGS += -ftrapv

# adds some checks to common function like memcpy, strcpy (feature_test_macros(7))
CXXFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2

# no writing to any ELF-section in the binary (more linker-related)
CXXFLAGS += -Wl,-z,relro,-z,now
LDFLAGS  += -Wl,-z,relro,-z,now

CXXFLAGS += -fexceptions

endif

DEFS	+=
LIBS	+=

Q	?= @

# General Compiler Settings (please don't change, make build-dependend changes on the variables above)

ALL_DEFS	 = -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 $(DEFS) -DVERSION="\"$(VERSION_STRING)\""
ALL_CFLAGS	 = -I. -I$(srcdir) $(INCLUDES) $(ALL_DEFS) -Wall -pthread -pipe -std=c99 $(CXXFLAGS)
ALL_LDFLAGS	 = -lm -lrt $(LDFLAGS)
ALL_LIBS	 = -lpthread $(LIBS)

# Tool Settings

SRCS_C 	= $(wildcard $(srcdir)/*.c)
SRCS_H	= $(wildcard $(srcdir)/*.h)
SRCS	= $(SRCS_C)

OBJS		= $(SRCS:.c=.o)

.SUFFIXES:
.SUFFIXES: .c .o .h .d .d.tmp

.PHONY: clean all
.DEFAULT: all
all: $(BINARIES)

clean:
	-@echo "  [RM]    $(BINARIES) $(OBJS) $(DEPS)"
	$(Q)$(RM) $(BINARIES) $(OBJS) $(DEPS)

# Automatic Dependency creation
# 	http://make.paulandlesley.org/autodep.html

DEPS		= $(SRCS:.c=.d)

define MAKEDEPEND =
	$(Q)gcc $(ALL_CFLAGS) -MM -MP -MF $*.d $<
	@mv $*.d $*.d.tmp && \
		sed 's/\($*\)\.o[ :]*/\1.o $*.d: /g' < $*.d.tmp > $*.d && \
		$(RM) $*.d.tmp; [ -s $*.d ] || $(RM) $*.d
endef

-include $(wildcard $(DEPS))

# Building

elastictab: $(OBJS)
	-@echo "  [LN]    $^"
	$(Q)$(CC) $(ALL_LDFLAGS) -o $@ $(OBJS) $(ALL_LIBS)

%.d: %.c
	-@echo "  [MKDEP] $<"
	$(MAKEDEPEND)

%.o: %.c
	-@echo "  [MKDEP] $<"
	$(MAKEDEPEND)
	-@echo "  [CC]    $<"
	$(Q)$(CC) $(ALL_CFLAGS) -c -o $@ $<
