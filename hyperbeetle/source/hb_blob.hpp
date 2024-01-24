#pragma once

#include <cstddef> // size_t
#include <utility> // std::move, std::swap

namespace hyperbeetle {

	/*! @brief Simple container for binary data.
	 *
	 *  A blob is a container for binary data.
	 *  Meant to be used to pass to apis which expect binary data inputs.
	 */
	class Blob final {
	public:
		using Deleter = void(*)(Blob&) noexcept;

		constexpr Blob() noexcept = default;
		Blob(void* data, size_t size, Deleter deleter) noexcept : mData(data), mSize(size), mDeleter(deleter) {}
		Blob(Blob const&) = delete;
		Blob& operator=(Blob const&) = delete;
		Blob(Blob&& other) noexcept { *this = std::move(other); }
		Blob& operator=(Blob&& other) noexcept { std::swap(mData, other.mData); std::swap(mSize, other.mSize); std::swap(mDeleter, other.mDeleter); return *this; }
		~Blob() noexcept { if (mDeleter) mDeleter(*this); }

		template<class T = void> T* Data() const { return reinterpret_cast<T*>(mData); }
		size_t Size() const { return mSize; }
		bool Valid() const { return mData; }
		explicit operator bool() const { return mData; }
	private:
		void* mData = nullptr;
		size_t mSize = 0;
		Deleter mDeleter = nullptr;
	};
}