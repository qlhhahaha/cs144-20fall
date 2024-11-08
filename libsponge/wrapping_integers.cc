#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
	return WrappingInt32{ isn.raw_value() + static_cast<uint32_t>(n) };
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
	// 获取 n 与 isn 之间的偏移量（mod）
	// 实际的 absolute seqno % INT32_RANGE == offset
	uint32_t offset = n - isn;
	// 取checkpoint的高十六位作为初步值
	uint64_t tmp = (checkpoint & 0Xffffffff00000000) + offset;
	uint64_t ans = tmp;
	uint64_t high_32bit_change = 1ul << 32;  // 用于给高32位加一或减一

	if (abs(int64_t(tmp + high_32bit_change - checkpoint)) < abs(int64_t(tmp - checkpoint)))
		ans = tmp + high_32bit_change;
	if (tmp > high_32bit_change && abs(int64_t(tmp - high_32bit_change - checkpoint)) < abs(int64_t(tmp - checkpoint)))
		ans = tmp - high_32bit_change;

	return ans;
}
