CXXFLAGS=-fopenmp -O2 -march=native -g
OBJECTS=main matrix_gen

all: $(OBJECTS)

clean:
	rm -f $(OBJECTS)