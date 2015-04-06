TARGET = table_dbg

CC = gcc
CPPFLAGS =
LDFLAGS =
CFLAGS = -Wall -g
COMPILE = $(CC) $(CPPFLAGS) $(CFLAGS) -c
LINKCC = $(CC) $(LDFLAGS)

LIBA = -lc

SRCS := $(wildcard *.c)
DEPS := $(patsubst %.c,%.d,$(SRCS))
OBJS := $(patsubst %.c,%.o,$(SRCS))

all:$(TARGET)

$(TARGET):$(OBJS) $(DEPS)
	$(LINKCC) $(OBJS) $(LIBA) -o $(TARGET)

%.d:%.c
	$(CC) -MM $(CPPFLAGS) $< > $@

%.o:%.c
	$(COMPILE) $< -o $@

.PHONY:clean
clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

explain:
    @echo "The information represents in the program:"
    @echo "Final executable name: $(TARGET)"
    @echo "Source files: $(SRCS)"
    @echo "Object files: $(OBJS)"

depend:$(DEPS)
	@echo "Dependencies are now up-to-date"

-include $(DEPS)