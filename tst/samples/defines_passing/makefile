SRC := main.c
OBJ := $(SRC:.c=.o)
EXE := prog

CFLAGS := -m64
main.o: CFLAGS += -DSOME_VALUE=5

CMD_COMP = gcc $(CFLAGS) -c $< -o $@

all: $(EXE)

$(EXE): $(OBJ)
	gcc $^ -o $@

%.o: %.c
	$(CMD_COMP)

clean:
	@rm -fv $(OBJ) $(EXE)
