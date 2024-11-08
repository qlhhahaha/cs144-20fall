#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;


/**
 *  \brief 当前 TCPReceiver 大体上有三种状态， 分别是
 *      1. LISTEN，此时 SYN 包尚未抵达。可以通过 listen_flag 标志位来判断是否在当前状态
 *      2. SYN_RECV, 此时 SYN 抵达。只能判断当前不在 1、3状态时才能确定在当前状态
 *      3. FIN_RECV, 此时 FIN 抵达。可以通过 ByteStream end_input 来判断是否在当前状态
 */
void TCPReceiver::segment_received(const TCPSegment& seg) {
	// 如果为listen状态（即还没收到SYN）
	if (this->listen_flag) {
		if (seg.header().syn) {
			// 设置isn初始值
			this->_isn = seg.header().seqno;
			this->listen_flag = false;
		}
		else {
			return;
		}
	}

	uint64_t abs_ackno = this->_reassembler.stream_out().bytes_written() + 1;
	uint64_t cur_abs_ackno = unwrap(seg.header().seqno, _isn, abs_ackno);

	// 注意：SYN标志位也是占了一个序列号的
	uint64_t stream_index = cur_abs_ackno - 1 + seg.header().syn;
	this->_reassembler.push_substring(seg.payload().copy(), stream_index, seg.header().fin);

}

optional<WrappingInt32> TCPReceiver::ackno() const {
	if (this->listen_flag)
		return nullopt;

	// 如果不在 LISTEN 状态，则 ackno 还需要加上一个 SYN 标志的长度
	uint64_t abs_ackno = _reassembler.stream_out().bytes_written() + 1;

	// 判断是否在FIN_RECV状态
	if (_reassembler.stream_out().input_ended())
		abs_ackno++;

	return this->_isn + abs_ackno;
}

size_t TCPReceiver::window_size() const {
	return this->_capacity - this->_reassembler.stream_out().buffer_size();
}
