INC = ../../include
INCLUDE = -I$(INC)
C_FLAGS = -c -m32 -D_M16 -ffreestanding -Wall -Werror

LIBRARY = ../libc.a

all: $(LIBRARY)

clean: 
	rm $(LIBRARY)

OBJECTS = \
	  $(LIBRARY)(memcpy.o)

%.o: %.c
	gcc $(C_FLAGS) $(INCLUDE) $< -o $@

$(LIBRARY): $(OBJECTS)
	ar rv U $@ *.o
	rm *.o


