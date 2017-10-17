#include <iostream>
#include <random>
#include <iomanip>


int main(int argc, char **argv) {
	if(argc != 2) {
		std::cout << argv[0] << " matrixSize\n";
		return 1;
	}

	int n = atoi(argv[1]);

	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> uni(-10000, 10000);

	std::cout << n << '\n';
	for(int i = 0; i < n; i++) {
		for(int j = 0; j < n; j++) {
			std::cout << std::setw(5) << uni(rng) << " ";
		}
		std::cout << "\n";
	}


	return 0;
}