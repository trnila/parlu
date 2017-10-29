#pragma once

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
	friend Matrix<I> load(std::istream&);

	Matrix<T>& operator=(const Matrix<T> &m) {
		assert(getSize() == m.getSize());

		for(int i = 0; i < size * size; i++) {
			m.data[i] = data[i];
		}
	}

	bool operator==(const Matrix<T> &m) {
		assert(getSize() == m.getSize());

		bool equals = true;
		for(int i = 0, len = size * size; i < len; i++) {
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