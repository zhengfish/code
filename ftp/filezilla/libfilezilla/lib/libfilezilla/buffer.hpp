#ifndef LIBFILEZILLA_BUFFER_HEADER
#define LIBFILEZILLA_BUFFER_HEADER

#include "libfilezilla.hpp"

/** \file
* \brief Declares fz::buffer
*/

namespace fz {

/**
 * \brief The buffer class is a simple buffer where data can be appended at the end and consumed at the front.
 * Think of it as a deque with contiguous storage.
 *
 * This class is useful when buffering data for sending over the network, or for buffering data for further 
 * piecemeal processing after having received it.
 *
 * In general, copying/moving data around is expensive and allocations are even more expensive. Using this
 * class helps to limit both to the bare minimum.
 *
 */
class FZ_PUBLIC_SYMBOL buffer final
{
public:
	buffer() = default;

	/// Initially reserves the passed capacity
	explicit buffer(size_t capacity);

	buffer(buffer const& buf);
	buffer(buffer && buf);

	~buffer() { delete[] data_; }

	buffer& operator=(buffer const& buf);
	buffer& operator=(buffer && buf);

	/// Undefined if buffer is empty
	unsigned char const* get() const { return pos_; }
	unsigned char* get() { return pos_; }

	/** \brief Returns a writable buffer guaranteed to be large enough for write_size bytes, call add when done.
	 *
	 * The returned pointer is pointing just after the data already stored in the buffer.

	 * Calling this function does not does not affect size().

	 * \sa append
	 *
	 * \par Example:
	 * \code
	 * fz::buffer buf;
	 * size_t to_read = 44;
	 * int read = recv(fd, buf.get(to_read), to_read, 0); // Read stuff from some socket
	 * if (read > 0)
	 *     buf.add(read); // Adjust size with the amount actually written into the buffer
	 * \endcode
	 *
	 */
	unsigned char* get(size_t write_size);

	/// Increase size by the passed amount. Call this after having obtained a writable buffer with get(size_t write_size)
	void add(size_t added);

	/** \brief Removes consumed bytes from the beginning of the buffer.
	 *
	 * Undefined if consumed > size
	 */
	void consume(size_t consumed);

	size_t size() const { return size_; }

	/**
	 * Does not release the memory.
	 */
	void clear();

	/** \brief Appends the passed data to the buffer.
	 *
	 * The number of reallocations as result to repeated append are amortized O(1)
	 */
	void append(unsigned char const* data, size_t len);
	void append(std::string const& str);

	bool empty() const { return size_ == 0; }
	explicit operator bool() const {
		return size_ != 0;
	}

	void reserve(size_t capacity);

	/// Gets element at offset i. Does not do bounds checking
	unsigned char operator[](size_t i) const { return pos_[i]; }
	unsigned char & operator[](size_t i) { return pos_[i]; }

	bool operator==(buffer const& rhs) const;

	bool operator!=(buffer const& rhs) const {
		return !(*this == rhs);
	}
private:

	// Invariants:
	//   size_ <= capacity_
	//   data_ <= pos_
	//   pos_ <= data_ + capacity_
	//   pos_ + size_ <= data_ + capacity_
	unsigned char* data_{};
	unsigned char* pos_{};
	size_t size_{};
	size_t capacity_{};
};

}

#endif
