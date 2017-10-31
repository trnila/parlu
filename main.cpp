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
#include <unordered_map>
#include "thread_pool.h"
#include "measure.h"
#include "matrix.h"

typedef double CellType;

void print(const char* name, Matrix<CellType> &m) {
	if(m.getSize() <= 20) {
		std::cout << name << "\n" << m;
	}
}

/**
 * when function completes, matrix contains U, out contains L matrix
 */
template<typename T>
void decomposeOpenMP(Matrix<T> &matrix, Matrix<T> &out, Matrix<T> &P) {
	for(int k = 0; k < matrix.getSize(); k++) {
		int maxIndex = k;
		T maxVal = fabs(matrix[k][k]);
		for(int i = k + 1; i < matrix.getSize(); i++) {
			if(fabs(matrix[i][k]) > maxVal) {
				maxVal = fabs(matrix[i][k]);
				maxIndex = i;
			}
		}

		if(maxIndex != k) {
			for(int i = 0; i < matrix.getSize(); i++) {
				std::swap(matrix[k][i], matrix[maxIndex][i]);
				std::swap(P[i][k], P[i][maxIndex]);
				std::swap(out[k][i], out[maxIndex][i]);
			};
		}

		#pragma omp parallel for
		for(int i = k + 1; i < matrix.getSize(); i++) {
			out[i][k] = matrix[i][k] / matrix[k][k];
			if(isnan(out[i][k])) {
				out[i][k] = 0;
			}
		}

		#pragma omp parallel for
		for(int j = k + 1; j < matrix.getSize(); j++) {
			for(int i = k + 1; i < matrix.getSize(); i++) {
				matrix[i][j] -= out[i][k] * matrix[k][j];
				if(isnan(matrix[i][j])) {
					matrix[i][j] = 0;
				}
			}
		}
	}

	#pragma omp parallel for
	for (int r = 0; r < matrix.getSize(); r++) {
		for (int i = 0; i < r; ++i) {
			matrix[r][i] = 0;
		}
	}

	#pragma omp parallel for
	for (int r = 0; r < matrix.getSize(); r++) {
		out[r][r] = 1;
		for (int i = r + 1; i < matrix.getSize(); ++i) {
			out[r][i] = 0;
		}
	}
}

template<typename T>
void decomposeC11Threads(Matrix<T> &matrix, Matrix<T> &out, Matrix<T> &P) {
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

int mapTo(int x, int fromMin, int fromMax, int toMin, int toMax) {
	return (x - fromMin) * (toMax - toMin) / ((double) fromMax - fromMin) + toMin;
}

template<typename T>
void image(const char *file, Matrix<T> &a) {
	std::ofstream out(file);
	out << "P3\n"
	    << a.getSize() << " " << a.getSize() << "\n"
		<< "255\n";

	for(int row = 0; row < a.getSize(); row++) {
		for(int col = 0; col < a.getSize(); col++) {
			int val = mapTo(a[row][col], -10000, 10000, 0, 255 * 255 * 255);
			out << ((val & 0xFF0000) >> 16) << " "
			    << ((val & 0x00FF00) >> 8) << " "
			    << (val & 0x0000FF) << " "
			    << " ";
		}
		out << '\n';
	}
}

int main(int argc, char**argv) {
	int returnCode = 0;
	if(argc < 2) {
		std::cerr << argv[0] << " bin|txt [input]\n";
		return 1;
	}

	std::cout.precision(3);

	std::istream *input;
	std::ifstream file;
	if(argc == 3) {
		file.open(argv[2]);
		input = &file;
	} else {
		input = &std::cin;
	}

	Matrix<CellType> matrix;

	char *fmt = argv[1];
	if(strcmp(fmt, "bin") == 0) {
		matrix = loadBin<CellType>(*input);
	} else if(strcmp(fmt, "txt") == 0) {
		matrix = loadTxt<CellType>(*input);
	}

	Matrix<CellType> orig(matrix);

	std::unordered_map<std::string, void (*)(Matrix<CellType> &, Matrix<CellType> &, Matrix<CellType> &)> tests = {
			{"decomposeOpenMP", decomposeOpenMP},
			//{"decomposeC11Threads", decomposeC11Threads},
	};

	for(auto fn: tests) {
		Matrix<CellType> l(matrix.getSize());
		Matrix<CellType> P(matrix.getSize());
		matrix = orig;

		std::cout << fn.first << "\n";

		{
			PROFILE_BLOCK("\tdecomposition");
			fn.second(matrix, l, P);
		}

		{
			Matrix<CellType> check;
			{
				PROFILE_BLOCK("\tMULT");
				check = P * l * matrix; // P * L * U
			}

			if (orig != check) {
				std::cout << "===ERROR===\n";
				returnCode = 1;
			}

			print("A=", orig);
			print("L=", l);
			print("U=", matrix);
			print("P=", P);
			print("A=L*U=", check);

			image("orig.ppm", orig);
			image("l.ppm", l);
			image("u.ppm", matrix);
			image("check.ppm", check);
		}
	}

	return returnCode;
}
