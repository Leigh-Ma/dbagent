TARGET = driver_dbg
LIB_OBJ = #lib_agt_driver.a

LIB_DIR = ../libs/
CP = cp -rf
AR = ar rc
CC = gcc
CPPFLAGS =
LDFLAGS = -L/usr/local/mysql/lib/ -L$(LIB_DIR)
CFLAGS = -Wall -g
COMPILE = $(CC) $(CPPFLAGS) $(CFLAGS) -c
LINKCC = $(CC) $(LDFLAGS)

LIBA = -lmysqlclient -l_agt_utils
SRCS := $(wildcard *.c */*.c)
OBJS := $(patsubst %.c,%.o,$(SRCS))

all:$(TARGET) $(LIB_OBJ)

$(TARGET):$(OBJS)
	$(LINKCC) $(OBJS) $(LIBA) -o $(TARGET)

$(LIB_OBJ):$(OBJS)
	$(AR) $(LIB_OBJ) $(OBJS) $(LIBA)
	$(CP) $(LIB_OBJ) $(LIB_DIR)
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
