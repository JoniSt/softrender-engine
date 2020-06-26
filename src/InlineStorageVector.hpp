/*
 * InlineStorageVector.hpp
 *
 *  Created on: 17.06.2020
 *      Author: jonathan
 */

#ifndef INLINESTORAGEVECTOR_HPP_
#define INLINESTORAGEVECTOR_HPP_

#include <cstdint>
#include <vector>
#include <algorithm>
#include <variant>

using namespace std;

namespace ttlhacker {


/**
 * A vector-like class that stores up to numInlineElements elements
 * within its own storage. Once more than numInlineElements elements
 * are added, the elements are moved to an actual vector.
 *
 * @tparam T A default-constructible type to store in this InlineStorageVector.
 * @tparam numInlineElements The number of elements to store directly within the InlineStorageVector.
 */
template<
	typename T,
	size_t numInlineElements = max(
			(ptrdiff_t)1,
			((ptrdiff_t)sizeof(vector<T>) - (ptrdiff_t)sizeof(size_t)) / max((ptrdiff_t)1, (ptrdiff_t)sizeof(T)))>
class InlineStorageVector {
private:

	struct InlineStorage {
		size_t nElems = 0;
		T elems[numInlineElements];
	};

	variant<InlineStorage, vector<T>> storage;

public:
	/**
	 * Puts a copy of the given element into this list.
	 *
	 * @param elem
	 */
	void put(T& elem) {
		if (InlineStorage * const inlineStorage = get_if<InlineStorage>(&storage)) {
			//Storage is currently inline. Add to the array if possible
			//or move all elements to a vector.
			size_t &nElems = inlineStorage->nElems;

			//If there is enough space left, just put the element into the inline storage.
			if (nElems < numInlineElements) {
				inlineStorage->elems[nElems] = elem;
				nElems++;
				return;
			}

			//There's not enough space left, move all elements into a vector
			//and store that instead.

			vector<T> vec;
			vec.reserve(nElems + 1);
			for (size_t i = 0; i < nElems; i++) {
				vec.push_back(move(inlineStorage->elems[i]));
			}
			//storage.emplace(move(vec));
			storage = move(vec);
		}

		//Storage is not inline but in a vector instead.
		//Just add to that.
		get<vector<T>>(storage).push_back(elem);
	}

	/**
	 * @return The number of elements.
	 */
	size_t size() const {
		if (const InlineStorage * const inlineStorage = get_if<InlineStorage>(&storage)) {
			return inlineStorage->nElems;
		} else {
			return get<vector<T>>(storage).size();
		}
	}

	/**
	 * @param i The index of the element to get. Must be within bounds.
	 * @return A reference to the i-th element of this InlineStorageVector.
	 */
	T& operator[](size_t i) {
		if (InlineStorage * const inlineStorage = get_if<InlineStorage>(storage)) {
			return inlineStorage->elems[i];
		} else {
			return get<vector<T>>(storage)[i];
		}
	}

	/**
	 * @param i The index of the element to get. Must be within bounds.
	 * @return A reference to the i-th element of this InlineStorageVector.
	 */
	const T& operator[](size_t index) const {
		if (const InlineStorage * const inlineStorage = get_if<InlineStorage>(&storage)) {
			return inlineStorage->elems[index];
		} else {
			return get<vector<T>>(storage)[index];
		}
	}

	/**
	 * Clears this InlineStorageVector.
	 * After invoking this method, the size of this InlineStorageVector will be 0
	 * and its storage will be inline.
	 */
	void clear() {
		storage.template emplace<InlineStorage>();
	}

	T * begin() {
		if (InlineStorage * const inlineStorage = get_if<InlineStorage>(&storage)) {
			return inlineStorage->elems;
		} else {
			return get<vector<T>>(storage).data();
		}
	}

	T * end() {
		return begin() + size();
	}
};

}


#endif /* INLINESTORAGEVECTOR_HPP_ */
