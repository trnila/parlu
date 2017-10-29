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
#include "measure.h"
#include "matrix.h"

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
	print("x", matrix);

	Matrix<double> orig(matrix), l(matrix.getSize());

	std::cout.precision(3);
	std::cout << std::setw(2);
	std::cout << std::fixed;

	{
		PROFILE_BLOCK("decomposition");
		decomposeOpenMP(matrix, l);
	}
	print("hotovo", matrix);
	//print("l", l);
	//print("u", matrix);

	{
		PROFILE_BLOCK("MULT");
		Matrix<double> check = mult(l, matrix);
		//print("lu", check);
		//print("oriG", orig);

		{
			PROFILE_BLOCK("CHECK");
			if (orig != check) {
				std::cout << "===ERROR===\n";
			}
		}
	}
}
