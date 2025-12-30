#ifndef __STREAM_HPP__
#define __STREAM_HPP__

#include <cstdint>
#include <cstring>

struct stream {
public:
	stream(uint8_t* head, size_t size) :head(head), size(size), cursor(head) {}

	template<class T>
	inline const T& read() {
		if (head + size < cursor + sizeof(T)) {
			static T _;
			fault = true;
			return _;
		}
		const T& ret = *reinterpret_cast<const T*>(cursor);
		cursor += sizeof(T);
		return ret;
	}

	template<class... T>
	inline std::tuple<T...> reads() {
		return std::tuple<T...>{read<T>()...};
	}

	inline void readRaw(void* r, size_t size) {
		if (head + this->size < cursor + size) {
			fault = true;
			return;
		}
		if (r) std::memcpy(r, cursor, size);
		cursor += size;
	}

	template<class T>
	inline void write(const T& v) {
		static_assert(!std::is_pointer_v<T> && std::is_copy_assignable_v<T>);
		if (head + size < cursor + sizeof(T)) {
			fault = true;
			return;
		}
		*reinterpret_cast<T*>(cursor) = v;
		cursor += sizeof(T);
	}

	template<class... T>
	inline void writes(T&&... v) {
		(write(std::forward<T>(v)), ...);
	}

	inline void writeRaw(const void* w, size_t size) {
		if (head + this->size < cursor + size) {
			fault = true;
			return;
		}
		std::memcpy(cursor, w, size);
		cursor += size;
	}

	inline size_t tell() {
		return cursor - head;
	}

	inline bool hadFault() const {
		return fault;
	}

private:
	uint8_t* const head;
	uint8_t* cursor;
	const size_t size;
	bool fault = false;
};

#endif // !__STREAM_HPP__
