#pragma once

template<typename T>
class Matrix {
public:
	Matrix(): Matrix(1) {}

	Matrix(int n) {
		resize(n);

		for(int i = 0; i < n; i++) {
			data[i * n + i] = 1;
		}
	}

	Matrix(const Matrix& m) {
		resize(m.size);

		for(int i = 0; i < size * size; i++) {
			data[i] = m.data[i];
		}
	}

	~Matrix() {
		if(data) {
			delete[] data;
		}

		if(rows) {
			delete[] rows;
		}
	}

	void swapRows(int a, int b) {
		std::swap(rows[a], rows[b]);
	}

	T* operator[](int row) {
		return &data[row * size];
	}

	const T* operator[](int row) const {
		return &data[row * size];
	}

	const int getSize() const {
		return size;
	}

	template<typename I>
	friend std::ostream& operator<<(std::ostream& out, Matrix<I>& m);

	template<typename I>
	friend Matrix<I> loadTxt(std::istream&);

	template<typename I>
	friend Matrix<I> loadBin(std::istream&);

	Matrix<T>& operator=(Matrix<T> m) {
		resize(m.size);

		for(int i = 0; i < size * size; i++) {
			data[i] = m.data[i];
		}

		return *this;
	}

	bool operator==(const Matrix<T> &m) {
		assert(getSize() == m.getSize());

		bool equals = true;
		int len = size * size;
		#pragma omp parallel for
		for(int i = 0; i < len; i++) {
			if(fabs(data[i] - m.data[i]) > 0.001 || isnan(data[i]) || isnan(m.data[i]) || isinf(m.data[i]) || isinf(m.data[i])) {
				equals = false;
			}
		}

		return equals;
	}

	bool operator!=(const Matrix<T> &m) {
		return !(*this == m);
	}

private:
	int size = 0;
	T *data = nullptr;
	int *rows = nullptr;

	void resize(int size) {
		if(size == this->size) {
			return;
		}

		this->size = size;

		if(data) {
			delete[] data;
		}

		data = new T[size * size]();

		if(rows) {
			delete[] rows;
		}

		rows = new int[size];
		for(int i = 0; i < size; i++) {
			rows[i] = i;
		}
	}
};

template<typename T>
Matrix<T> operator*(const Matrix<T>& a, const Matrix<T>& b) {
	if(a.getSize() != b.getSize()) {
		throw std::runtime_error("Could not multiply matrixes with different size");
	}

	const int size = a.getSize();

	Matrix<T> res(size);
	#pragma omp parallel for
	for(int c = 0; c < size; c++) {
		for(int r = 0; r < size; r++) {
			double val = 0;
			for(int j = 0; j < size; j++) {
				val += a[r][j] * b[j][c];
			}
			res[r][c] = val;
		}
	}

	return res;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, Matrix<T>& m) {
	for(int r = 0; r < m.size; r++) {
		for(int c = 0; c < m.size; c++) {
			out << std::fixed << std::setw(12) << m[r][c] << ' ';
		}
		out << '\n';
	}
	std::cout << '\n';
	return out;
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