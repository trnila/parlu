#include <stdio.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <exception>
#include <iomanip>
#include <assert.h>
#include <string.h>

template<typename T>
class Matrix {
public:
	Matrix(int n) {
		data = new T[n * n]();
		size = n;

		for(int i = 0; i < n; i++) {
			data[i * n + i] = 1;
		}
	}

	~Matrix() {
		if(data) {
			delete data;
		}
	}

	T* operator[](int row) {
		return &data[row * size];
	}

	int getSize() const {
		return size;
	}

	friend std::ostream& operator<<(std::ostream& out, Matrix<float>& m);
	friend Matrix<float> load(std::istream&);
	Matrix<T>& operator=(const Matrix<T> &m) {
		assert(getSize() == m.getSize());

		for(int i = 0; i < size; i++) {
			m.data[i] = data[i];
		}
	}

	bool operator==(const Matrix<T> &m) {
		assert(getSize() == m.getSize());
		return memcmp(data, m.data, getSize() * sizeof(T)) == 0;
	}

	bool operator!=(const Matrix<T> &m) {
		return !(*this == m);
	}

private:
	int size;
	T *data;
};

std::ostream& operator<<(std::ostream& out, Matrix<float>& m) {
	for(int r = 0; r < m.size; r++) {
		for(int c = 0; c < m.size; c++) {
			out << m[r][c] << ' ';
		}
		out << '\n';
	}
	std::cout << "====\n";
	return out;
}

template<typename T>
void decompose(Matrix<T>& matrix, Matrix<T>& out) {
	for(int k = 0; k < matrix.getSize(); k++) {
		for(int i = k + 1; i < matrix.getSize(); i++) {
			out[i][k] = matrix[i][k] / matrix[k][k];
		}

		for(int j = k + 1; j < matrix.getSize(); j++) {
			for(int i = k + 1; i < matrix.getSize(); i++) {
				matrix[i][j] = matrix[i][j] - out[i][k] * matrix[k][j];
			}
		}
	}

	// null it faggot
	for(int r = 0; r < matrix.getSize(); r++) {
		for (int i = 0; i < r; ++i) {
			matrix[r][i] = 0;
		}
	}
}


Matrix<float> load(std::istream &f) {
	int size;
	f >> size;
	assert(size > 0);

	Matrix<float> m(size);

	for(int i = 0; i < size * size; i++) {
		f >> m.data[i];
	}

	return m;
}


template<typename T>
Matrix<T> mult(Matrix<T>& a, Matrix<T>& b) {
	if(a.getSize() != b.getSize()) {
		throw std::runtime_error("Could not multiply matrixes with different size");
	}


	Matrix<T> res(a.getSize());
	for(int r = 0; r < a.getSize(); r++) {
		for(int c = 0; c < a.getSize(); c++) {
			res[r][c] = 0;
			for(int j = 0; j < a.getSize(); j++) {
				res[r][c] += a[r][j] * b[j][c];
			}
		}
	}

	return res;
}

int main(int argc, char**argv) {
	std::istream *input;
	std::ifstream file;
	if(argc == 2) {
		file.open(argv[1]);
		input = &file;
	} else {
		input = &std::cin;
	}


	Matrix<float> matrix = load(*input);
	Matrix<float> l(matrix.getSize());

	std::cout.precision(3);
	std::cout << std::setw(2);
	std::cout << std::fixed;

	decompose(matrix, l);
	std::cout << l;
	std::cout << matrix;

	Matrix<float> check = mult(l, matrix);
	std::cout << check;

	if(matrix != check) {
		std::cout << "===ERROR===\n";
	}

}
