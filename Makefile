TARGET =DbTest
INCDIR =./
SRCDIR =./
OBJDIR =obj
CC=gcc
CPPFLAGS=-I$(INCDIR)

SRCS := $(shell find $(SRCDIR) -name "*.cpp")
#OBJS := $(addsuffix .o, $(basename $(SRCS)))
#OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename,$(notdir,$(SRCS)))))
OBJS = $(patsubst %.c,%.o, $(notdir $(SRCS) ))
#OBJS := $(patsubst %,$(OBJDIR)/%,$(SRCS))
DEPS := $(OBJS:.o=.d)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LOADLIBES) $(LDLIBS) -ldl -lstdc++

.PHONY: clean

clean:
# THIS DELETES SOURCE FILES!!!
#	$(RM) $(TARGET) $(OBJS) $(DEPS)

