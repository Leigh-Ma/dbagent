TARGET = ipc_man ipc_client

LIB_DIR = ../libs/
CP = cp -rf
AR = ar rc
CC = gcc
CPPFLAGS =
LDFLAGS = -levent
CFLAGS = -Wall -g
COMPILE = $(CC) $(CPPFLAGS) $(CFLAGS) -c
LINKCC = $(CC) $(LDFLAGS)

LIBA =

SRCS := $(wildcard *.c */*.c)
OBJS := $(patsubst %.c,%.o,$(SRCS))

all:$(TARGET) $(LIB_OBJ)


ipc_man: ipc_man.o
	$(LINKCC) ipc_man.o $(LIBA) -o ipc_man
	
ipc_client: ipc_client.o
	$(LINKCC) ipc_client.o $(LIBA) -o ipc_client

%.o:%.c
	$(COMPILE) $< -o $@

.PHONY:clean
clean:
	rm -f $(OBJS) $(TARGET) $(LIB_OBJ)

explain:
	@echo "The information represents in the program:"
	@echo "Final executable name: $(TARGET)"
	@echo "Source files: $(SRCS)"
	@echo "Object files: $(OBJS)"
