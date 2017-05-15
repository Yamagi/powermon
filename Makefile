# Base CFLAGS
CFLAGS := -O2 -fomit-frame-pointer -std=c99 \
		  -pedantic -Wall -Wextra -MMD -pipe

# Base LDFLAGS
LDFLAGS := -lcursesw -lm

# -----------

# When make is invoked by "make VERBOSE=1" print
# the compiler and linker commands.

ifdef VERBOSE
Q :=
else
Q := @
endif

# -----------

# Phony targets
.PHONY : all clean

# -----------

# Builds everything
all :
	@echo "Building powermon"
	$(MAKE) powermon

# -----------

# Clean
clean :
	@echo "===> CLEAN"
	$(Q)rm -rf build/ release

# -----------

# Main target
powermon:
	@echo "===> Building powermon"
	${Q}mkdir -p release
	$(MAKE) release/powermon

# -----------

# Converter rules
build/%.o: %.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	$(Q)$(CC) -c $(CFLAGS) -o $@ $<

# -----------

OBJS_ = \
	src/cpuid.o \
	src/main.o \
	src/monitor.o \
	src/msr.o

# -----------

# Rewrite pathes to our object directory
OBJS = $(patsubst %,build/%,$(OBJS_))

# -----------

# Header dependencies
DEPS= $(OBJS:.o=.d)
-include $(DEPS)

# -----------

release/powermon: $(OBJS)
	@echo "===> LD $@"
	$(Q)$(CC) $(OBJS) $(LDFLAGS) -o $@

