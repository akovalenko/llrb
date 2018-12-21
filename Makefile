
all:	example_int example_string

example_int: example_int.o llrb.o

example_string: example_string.o llrb.o

clean:
	-rm example_int.o example_string.o llrb.o example_int example_string
