#include <stdio.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <exception>
#include <iomanip>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <functional>
#include <unordered_map>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "thread_pool.h"
#include "measure.h"
#include "matrix.h"

#include <unistd.h>

void print(const char* name, Matrix<double> &m) {
//	std::cout << name << "\n" << m;
}

/**
 * when function completes, matrix contains U, out contains L matrix
 */
template<typename T>
void decomposeOpenMP(Matrix<T> &matrix, Matrix<T> &out) {
	for(int k = 0; k < matrix.getSize(); k++) {
		#pragma omp parallel for
		for(int i = k + 1; i < matrix.getSize(); i++) {
			out[i][k] = matrix[i][k] / matrix[k][k];
		}

		#pragma omp parallel for
		for(int j = k + 1; j < matrix.getSize(); j++) {
			for(int i = k + 1; i < matrix.getSize(); i++) {
				matrix[i][j] -= out[i][k] * matrix[k][j];
			}
		}
	}

	#pragma omp parallel for
	for (int r = 0; r < matrix.getSize(); r++) {
		for (int i = 0; i < r; ++i) {
			matrix[r][i] = 0;
		}
	}
}

template<typename T>
void decomposeC11Threads(Matrix<T> &matrix, Matrix<T> &out) {
	ThreadPool pool(4);
	for(int k = 0; k < matrix.getSize(); k++) {
		for(int i = k + 1; i < matrix.getSize(); i++) {
			out[i][k] = matrix[i][k] / matrix[k][k];
		}

		Barrier b(matrix.getSize() - (k + 1));
		for(int j = k + 1; j < matrix.getSize(); j++) {
			pool.add([&matrix, &out, j, k, &b]() {
				BarrierReleaser releaser(b);
				for (int i = k + 1; i < matrix.getSize(); i++) {
					matrix[i][j] -= out[i][k] * matrix[k][j];
				}
			});
		}

		b.wait();
	}

	Barrier b(matrix.getSize());
	for (int r = 0; r < matrix.getSize(); r++) {
		pool.add([&matrix, r, &b]() {
			BarrierReleaser releaser(b);
			for (int i = 0; i < r; ++i) {
				matrix[r][i] = 0;
			}
		});
	}
	b.wait();
}

template<typename T>
Matrix<T> loadTxt(std::istream &f) {
	PROFILE_BLOCK("load");
	int size;
	f >> size;
	assert(size > 0);

	Matrix<T> m(size);

	for(int i = 0; i < size * size; i++) {
		f >> m.data[i];
	}

	return m;
}

template<typename T>
Matrix<T> loadBin(std::istream &f) {
	PROFILE_BLOCK("loadBin");
	int size;

	f.read((char*) &size, sizeof(size));
	assert(size > 0);

	Matrix<T> m(size);
	f.read((char*) m.data, size * size * sizeof(T));

	return m;
}


template<typename T>
Matrix<T> mult(Matrix<T>& a, Matrix<T>& b) {
	if(a.getSize() != b.getSize()) {
		throw std::runtime_error("Could not multiply matrixes with different size");
	}


	Matrix<T> res(a.getSize());
	#pragma omp parallel for
	for(int c = 0; c < a.getSize(); c++) {
		for(int r = 0; r < a.getSize(); r++) {
			res[r][c] = 0;
			for(int j = 0; j < a.getSize(); j++) {
				res[r][c] += a[r][j] * b[j][c];
			}
		}
	}

	return res;
}

int main(int argc, char**argv) {
	/*const int n = 3;
	Barrier b(n);
	for(int i = 0; i <1000; i++) {
		printf("send %d\n", i);
		pool.add([i, &b]() {
			BarrierReleaser releaser(b);
			printf("ahoj %d\n", i);
			sleep(1 + rand() % 5);
		});

		if(i % n == 0 && i != 0) {
			printf("Waiting\n");
			b.wait();
			b = Barrier(n);
			printf("NEW barrier\n");
		}
	}*/


	if(argc < 2) {
		std::cerr << argv[0] << " bin|txt [input]\n";
		return 1;
	}

	std::istream *input;
	std::ifstream file;
	if(argc == 3) {
		file.open(argv[2]);
		input = &file;
	} else {
		input = &std::cin;
	}

	Matrix<double> matrix;

	char *fmt = argv[1];
	if(strcmp(fmt, "bin") == 0) {
		matrix = loadBin<double>(*input);
	} else if(strcmp(fmt, "txt") == 0) {
		matrix = loadTxt<double>(*input);
	}

	Matrix<double> orig(matrix);

	std::cout.precision(3);
	std::cout << std::setw(2);
	std::cout << std::fixed;

	std::unordered_map<std::string, void (*)(Matrix<double> &, Matrix<double> &)> tests = {
			{"decomposeOpenMP", decomposeOpenMP},
			{"decomposeC11Threads", decomposeC11Threads},
	};

	for(auto fn: tests) {
		Matrix<double> l(matrix.getSize());
		matrix = orig;

		std::cout << fn.first << "\n";

		{
			PROFILE_BLOCK("\tdecomposition");
			fn.second(matrix, l);
		}

		{
			PROFILE_BLOCK("\tMULT");
			Matrix<double> check = mult(l, matrix);
			//print("lu", check);
			//print("oriG", orig);

			{
				PROFILE_BLOCK("\tCHECK");
				if (orig != check) {
					std::cout << "===ERROR===\n";
				}
			}
		}
	}
}
