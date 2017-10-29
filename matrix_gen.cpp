#include <iostream>
#include <random>
#include <iomanip>
#include <functional>
#include <unistd.h>
#include <string.h>

void writeBin(int n, std::function<int(void)> gen) {
	// write dimension of matrix
	write(1, &n, sizeof(n));

	#pragma omp parallel for
	for(int i = 0; i < n; i++) {
		double values[n];
		for(int j = 0; j < n; j++) {
			values[j] = gen();
		}

	#pragma omp critical
		write(1, values, sizeof(*values) * n);
	}
}

void writeText(int n, std::function<int(void)> gen) {
	std::cout << n << '\n';
	for(int i = 0; i < n; i++) {
		for(int j = 0; j < n; j++) {
			std::cout << std::setw(5) << gen() << " ";
		}
		std::cout << "\n";
	}
}

int main(int argc, char **argv) {
	if(argc != 3) {
		std::cout << argv[0] << " matrixSize bin|txt\n";
		return 1;
	}

	int n = atoi(argv[1]);
	char *fmt = argv[2];

	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> uni(-10000, 10000);
	auto gen = [&]() {return uni(rng);};

	if(strcmp(fmt, "bin") == 0) {
		writeBin(n, gen);
	} else if(strcmp(fmt, "txt") == 0) {
		writeText(n, gen);
	} else {
		std::cerr << "Invalid format\n";
		return 1;
	}

	return 0;
}