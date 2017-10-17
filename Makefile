CXXFLAGS=-fopenmp -O2 -march=native
OBJECTS=main matrix_gen

all: $(OBJECTS)

clean:
	rm -f $(OBJECTS)