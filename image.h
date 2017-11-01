#pragma once

int mapTo(double x, double fromMin, double fromMax, int toMin, int toMax) {
	return (x - fromMin) * (toMax - toMin) / ((double) fromMax - fromMin) + toMin;
}

template<typename T>
std::pair<T, T> findBounds(Matrix<T> a) {
	int size = a.getSize() * a.getSize();
	T max = a[0][0];
	T min = a[0][0];

	#pragma omp parallel for reduction(max: max) reduction(min: min)
	for(int i = 0; i < size; i++) {
		T val = a.raw()[i];
		if(val < min) {
			min = val;
		} else if(val > max) {
			max = val;
		}
	}

	return std::make_pair(min, max);
}

template<typename T>
void saveImage(const char *file, Matrix<T> &a) {
	std::ofstream out(file);
	out << "P3\n"
	    << a.getSize() << " " << a.getSize() << "\n"
	    << "255\n";

	T min, max;
	std::tie(min, max) = findBounds(a);

	std::string store;
	store.reserve(2 * a.getSize() * a.getSize() * 3 * 4);
	std::ostringstream o(store);
	for(int row = 0; row < a.getSize(); row++) {
		for(int col = 0; col < a.getSize(); col++) {
			int val = mapTo(a[row][col], min, max, 0, 255 * 255 * 255);
			o << ((val & 0xFF0000) >> 16) << " "
			  << ((val & 0x00FF00) >> 8) << " "
			  << (val & 0x0000FF) << " "
			  << " ";
		}
		o << '\n';
	}
	out << o.str();
}