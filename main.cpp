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
#include <sstream>
#include "thread_pool.h"
#include "measure.h"
#include "matrix.h"
#include "image.h"

typedef double CellType;
const int MAX_PRINTABLE_MATRIX_SIZE = 20;

void print(const char* name, Matrix<CellType> &m) {
	if(m.getSize() <= MAX_PRINTABLE_MATRIX_SIZE) {
		std::cout << name << "\n" << m;
	}
}

void initPermVector(std::vector<int> &P) {
	for(int i = 0; i < P.size(); i++) {
		P[i] = i;
	}
}

/**
 * when function completes, matrix contains U, out contains L matrix
 */
template<typename T>
void decomposeOpenMP(Matrix<T> &matrix, Matrix<T> &out, std::vector<int> &P) {
	const int size = matrix.getSize();

	initPermVector(P);

	for(int k = 0; k < size; k++) {
		int maxIndex = k;
		T maxVal = fabs(matrix[k][k]);
		for(int i = k + 1; i < size; i++) {
			if(fabs(matrix[i][k]) > maxVal) {
				maxVal = fabs(matrix[i][k]);
				maxIndex = i;
			}
		}

		if(maxIndex != k) {
			for(int i = 0; i < size; i++) {
				std::swap(matrix[k][i], matrix[maxIndex][i]);
			};

			std::swap(P[k], P[maxIndex]);

			for(int i = 0; i < k; i++) {
				std::swap(out[k][i], out[maxIndex][i]);
			}
		}

		#pragma omp parallel for
		for(int i = k + 1; i < size; i++) {
			out[i][k] = matrix[i][k] / matrix[k][k];
			if(isnan(out[i][k])) {
				out[i][k] = 0;
			}
			matrix[i][k] = 0;
		}

		#pragma omp parallel for
		for(int j = k + 1; j < size; j++) {
			for(int i = k + 1; i < size; i++) {
				matrix[i][j] -= out[i][k] * matrix[k][j];
				if(isnan(matrix[i][j])) {
					matrix[i][j] = 0;
				}
			}
		}
	}
}

template<typename T>
void decomposeC11Threads(Matrix<T> &matrix, Matrix<T> &out, std::vector<int> &P) {
	ThreadPool pool(4);

	const int size = matrix.getSize();
	initPermVector(P);

	for(int k = 0; k < size; k++) {
		int maxIndex = k;
		T maxVal = fabs(matrix[k][k]);
		for(int i = k + 1; i < size; i++) {
			if(fabs(matrix[i][k]) > maxVal) {
				maxVal = fabs(matrix[i][k]);
				maxIndex = i;
			}
		}
		
		if(maxIndex != k) {
			for(int i = 0; i < size; i++) {
				std::swap(matrix[k][i], matrix[maxIndex][i]);
			};

			std::swap(P[k], P[maxIndex]);

			for(int i = 0; i < k; i++) {
				std::swap(out[k][i], out[maxIndex][i]);
			}
		}

		for(int i = k + 1; i < size; i++) {
			out[i][k] = matrix[i][k] / matrix[k][k];
			if(isnan(out[i][k])) {
				out[i][k] = 0;
			}
			matrix[i][k] = 0;
		}

		int workSize = 200;
		Barrier barrier((size - 1 - k - 1 + workSize) / workSize);
		for(int j = k + 1; j < size; j += workSize) {
			int from = j;
			int to = std::min(size, j + workSize);
			pool.add([&matrix, &out, k, from, size, to]() {
				for(int j = from; j < to; j++) {
					for (int i = k + 1; i < size; i++) {
						matrix[i][j] -= out[i][k] * matrix[k][j];
						if (isnan(matrix[i][j])) {
							matrix[i][j] = 0;
						}
					}
				}
			}, barrier);
		}
		barrier.wait();
	}
}

// a == P * b ?
template<typename T>
bool equals(const Matrix<T> &a, const Matrix<T> &b, const std::vector<int> &P) {
	for(int i = 0; i < P.size(); i++) {
		for(int j = 0; j < P.size(); j++) {
			if(fabs(a[P[i]][j] - b[i][j]) > CMP_PRECISION) {
				printf(">>%d %d %f != %f\n", i, P[i], a[P[i]][j], b[i][j]);
				return false;
			}
		}
	}

	return true;
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

	ThreadPool workers(4);

	Matrix<CellType> orig;
	char *fmt = argv[1];
	if(strcmp(fmt, "bin") == 0) {
		orig = loadBin<CellType>(*input);
	} else if(strcmp(fmt, "txt") == 0) {
		orig = loadTxt<CellType>(*input);
	}

	std::unordered_map<std::string, void (*)(Matrix<CellType> &, Matrix<CellType> &, std::vector<int> &)> tests = {
			{"decomposeOpenMP", decomposeOpenMP},
			{"decomposeC11Threads", decomposeC11Threads},
	};

	for(auto fn: tests) {
		Matrix<CellType> matrix = orig;
		Matrix<CellType> l(matrix.getSize());
		std::vector<int> P;
		P.resize(matrix.getSize());

		std::cout << fn.first << "\n";

		{
			PROFILE_BLOCK("\tdecomposition");
			fn.second(matrix, l, P);
		}

		{
			Matrix<CellType> check;
			{
				PROFILE_BLOCK("\tmult");
				check = l * matrix; // P * L * U
			}

			{
				PROFILE_BLOCK("\tcheck");
				if (!equals(orig, check, P)) {
					std::cout << "===ERROR===\n";
					returnCode = 1;
				}
			}

			print("A=", orig);
			print("L=", l);
			print("U=", matrix);
			print("A=P*L*U=", check);

			{
				PROFILE_BLOCK("\tsave images");
				std::unordered_map<std::string, Matrix<CellType>*> map = {
						{"orig.ppm", &orig},
						{"l.ppm", &l},
						{"u.ppm", &matrix},
						{"check.ppm", &check}

				};

				Barrier barrier(map.size());
				for(auto &job: map) {
					workers.add([&]() {
						saveImage(job.first.c_str(), *job.second);
					}, barrier);
				}
				barrier.wait();
			}
		}
	}

	return returnCode;
}
