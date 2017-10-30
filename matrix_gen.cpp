#include <iostream>
#include <random>
#include <iomanip>
#include <functional>
#include <string.h>

void writeBin(int n, double from, double to) {
	// write dimension of matrix
	fwrite(&n, sizeof(n), 1, stdout);

	#pragma omp parallel
	{
		std::random_device rd;
		std::mt19937 rng(rd());
		std::uniform_real_distribution<double> uni(from, to);

		#pragma omp for
		for(int i = 0; i < n; i++) {
			double values[n];
			for (int j = 0; j < n; j++) {
				values[j] = uni(rng);
			}

			#pragma omp critical
			fwrite(values, sizeof(*values), n, stdout);
		}
	}
}

void writeText(int n, double from, double to) {
	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_real_distribution<double> uni(from, to);

	std::cout << n << '\n';
	for(int i = 0; i < n; i++) {
		for(int j = 0; j < n; j++) {
			std::cout << std::setw(5) << uni(rng) << " ";
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

	double from = -10000;
	double to = -from;

	if(strcmp(fmt, "bin") == 0) {
		writeBin(n, from, to);
	} else if(strcmp(fmt, "txt") == 0) {
		writeText(n, from, to);
	} else {
		std::cerr << "Invalid format\n";
		return 1;
	}

	return 0;
}