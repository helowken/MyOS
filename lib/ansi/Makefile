INC = ../../include
INCLUDE = -I$(INC)
C_FLAGS = -c -m32 -D_M16 -ffreestanding -Wall -Werror

LIBRARY = ../libc.a

all: $(LIBRARY)

clean: 
	rm -f $(LIBRARY) 
	rm *.o

OBJECTS = \
	  $(LIBRARY)(memcpy.o)

%.o: %.c
	gcc $(C_FLAGS) $(INCLUDE) $< -o $@

$(LIBRARY): $(OBJECTS)
	ar rv U $@ *.o


