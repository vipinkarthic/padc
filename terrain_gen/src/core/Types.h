#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

template <typename T>
class Grid2D {
   public:
	Grid2D() : w_(0), h_(0) {}
	Grid2D(int width, int height) : w_(width), h_(height), data_((size_t)width * (size_t)height) {}
	Grid2D(int width, int height, const T& init) : w_(width), h_(height), data_((size_t)width * (size_t)height, init) {}

	Grid2D(int width, int height, std::vector<T>&& flatData) : w_(width), h_(height), data_(std::move(flatData)) {
		assert((size_t)w_ * (size_t)h_ == data_.size());
	}

	void resize(int width, int height) {
		w_ = width;
		h_ = height;
		data_.assign((size_t)w_ * (size_t)h_, T());
	}
	void resize(int width, int height, const T& init) {
		w_ = width;
		h_ = height;
		data_.assign((size_t)w_ * (size_t)h_, init);
	}

	int width() const { return w_; }
	int height() const { return h_; }
	size_t size() const { return data_.size(); }

	T* data() { return data_.empty() ? nullptr : data_.data(); }
	const T* data() const { return data_.empty() ? nullptr : data_.data(); }

	inline size_t index(int x, int y) const {
		assert(x >= 0 && x < w_ && y >= 0 && y < h_);
		return (size_t)y * (size_t)w_ + (size_t)x;
	}

	inline T& operator()(int x, int y) { return data_[index(x, y)]; }
	inline const T& operator()(int x, int y) const { return data_[index(x, y)]; }

	inline T& at(int x, int y) { return operator()(x, y); }
	inline const T& at(int x, int y) const { return operator()(x, y); }

	void fill(const T& v) { std::fill(data_.begin(), data_.end(), v); }

	template <typename Fn>
	void forEach(Fn&& fn) {
#pragma omp parallel for collapse(2) schedule(static)
		for (int y = 0; y < h_; ++y) {
			for (int x = 0; x < w_; ++x) {
				fn(x, y, data_[(size_t)y * (size_t)w_ + (size_t)x]);
			}
		}
	}

	std::vector<T> toVector() const { return data_; }

	static Grid2D<T> fromVector(int width, int height, const std::vector<T>& flat) {
		assert((size_t)width * (size_t)height == flat.size());
		Grid2D<T> g(width, height);
		g.data_ = flat;
		return g;
	}

   private:
	int w_ = 0, h_ = 0;
	std::vector<T> data_;
};

inline void flatIndexToXY(size_t idx, int width, int& outX, int& outY) {
	outX = (int)(idx % (size_t)width);
	outY = (int)(idx / (size_t)width);
}

template <typename T, typename Fn>
static Grid2D<T> makeGridFromFn(int width, int height, Fn&& f) {
	Grid2D<T> g(width, height);
#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			g(x, y) = f(x, y);
		}
	}
	return g;
}

using GridFloat = Grid2D<float>;
using GridU8 = Grid2D<uint8_t>;
using GridInt = Grid2D<int>;
