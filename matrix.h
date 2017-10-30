#pragma once

template<typename T>
class Matrix {
public:
	Matrix(): Matrix(1) {}

	Matrix(int n) {
		data = new T[n * n]();
		size = n;

		for(int i = 0; i < n; i++) {
			data[i * n + i] = 1;
		}
	}

	Matrix(const Matrix& m) {
		size = m.getSize();
		data = new T[size * size]();

		for(int i = 0; i < size * size; i++) {
			data[i] = m.data[i];
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

	template<typename I>
	friend std::ostream& operator<<(std::ostream& out, Matrix<I>& m);

	template<typename I>
	friend Matrix<I> loadTxt(std::istream&);

	template<typename I>
	friend Matrix<I> loadBin(std::istream&);

	Matrix<T>& operator=(Matrix<T> m) {
		if(getSize() != m.getSize()) {
			size = m.size;
			delete data;
			data = new T[m.size * m.size];
		}

		for(int i = 0; i < size * size; i++) {
			data[i] = m.data[i];
		}

		return *this;
	}

	bool operator==(const Matrix<T> &m) {
		assert(getSize() == m.getSize());

		bool equals = true;
		int len = len = size * size;
		#pragma omp parallel for
		for(int i = 0; i < len; i++) {
			if(fabs(data[i] - m.data[i]) > 0.001) {
				equals = false;
				printf("%f %f\n", data[i], m.data[i]);
			}
		}

		return equals;
	}

	bool operator!=(const Matrix<T> &m) {
		return !(*this == m);
	}

private:
	int size;
	T *data;
};

template<typename T>
std::ostream& operator<<(std::ostream& out, Matrix<T>& m) {
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