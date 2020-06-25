/*
 * Rectangle.hpp
 *
 *  Created on: 18.06.2020
 *      Author: jonathan
 */

#ifndef INTRECTANGLE_HPP_
#define INTRECTANGLE_HPP_

#include <type_traits>
#include <algorithm>
#include <cassert>

using namespace std;

namespace ttlhacker {

template<typename Coord, enable_if_t<is_integral<Coord>::value, int> = 0>
struct IntRectangle {
	using UCoord = typename make_unsigned<Coord>::type;
	Coord x, y;
	UCoord width, height;

	IntRectangle():
		x(0), y(0), width(0), height(0)
	{
		//Nothing else to do
	}

	IntRectangle(Coord x, Coord y, UCoord width, UCoord height):
		x(x), y(y), width(width), height(height)
	{
		//Nothing else to do
	}

	/**
	 * @return True if this IntRectangle doesn't contain any pixels.
	 */
	bool isEmpty() const {
		return (width == 0) || (height == 0);
	}

	/**
	 * @return The largest X coordinate contained within this IntRectangle.
	 */
	int32_t getLastX() const {
		return x + width - 1;
	}

	/**
	 * @return The largest Y coordinate contained within this IntRectangle.
	 */
	int32_t getLastY() const {
		return y + height - 1;
	}

	/**
	 * @param other
	 * @return True if this IntRectangle intersects the other, false if not.
	 */
	bool intersects(const IntRectangle<Coord>& other) const {
		if (isEmpty() || other.isEmpty()) return false;
		if (other.x > getLastX()) return false;
		if (x > other.getLastX()) return false;
		if (other.y > getLastY()) return false;
		if (y > other.getLastY()) return false;
		return true;
	}

	/**
	 * @param other
	 * @return The intersection between this IntRectangle and the other or a default-constructed IntRectangle if the two don't overlap.
	 */
	IntRectangle<Coord> getIntersection(const IntRectangle<Coord>& other) const {
		if (!intersects(other)) {
			return IntRectangle<Coord>();
		}

		IntRectangle<Coord> result;
		result.x = max(x, other.x);
		result.y = max(y, other.y);
		Coord lastX = min(getLastX(), other.getLastX());
		Coord lastY = min(getLastY(), other.getLastY());

		assert(lastX >= result.x);
		assert(lastY >= result.y);

		result.width = lastX - result.x + 1;
		result.height = lastY - result.y + 1;

		return result;
	}
};

}


#endif /* INTRECTANGLE_HPP_ */
