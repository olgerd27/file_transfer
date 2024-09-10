# Compiler and flags
CC = gcc
CFLAGS = -Wall -g
DEPFLAGS = -MMD -MP

# Source and object files
SRCS = main.c utils.c
OBJS = $(SRCS:.c=.o)

# Include automatically generated dependencies
-include $(OBJS:.o=.d)

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Final executable
myprogram: $(OBJS)
	$(CC) $(CFLAGS) -o myprogram $(OBJS)

all: myprogram

# Clean rule
.PHONY: clean
clean:
	@rm -fv $(OBJS) $(OBJS:.o=.d) myprogram

