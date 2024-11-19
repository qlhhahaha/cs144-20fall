#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
// TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
// 	: _isn(fixed_isn.value_or(WrappingInt32{ random_device()() }))
// 	, _initial_retransmission_timeout{ retx_timeout }
// 	, _stream(capacity) {
// }
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
	: _isn(fixed_isn.value_or(WrappingInt32{ random_device()() }))
	, _initial_retransmission_timeout{ retx_timeout }
	, _stream(capacity)
	, _rto{ retx_timeout } {
}


uint64_t TCPSender::bytes_in_flight() const {
	return _bytes_in_flight;
}


void TCPSender::fill_window() {
	if (!_syn_sent) {
		_syn_sent = true;
		TCPSegment seg;
		seg.header().syn = true;
		_send_segment(seg);
		return;
	}

	// 如果携带syn的segment都还没被确认，就什么都不做
	if (!_segments_outstanding.empty() && _segments_outstanding.front().header().syn)
		return;

	// If _stream is empty but input has not ended, do nothing.
	if (!_stream.buffer_size() && !_stream.eof())
		// Lab4 behavior: if incoming_seg.length_in_sequence_space() is not zero, send ack.
		return;

	if (_fin_sent)
		return;

	if (_receiver_window_size) {
		while (_receiver_free_space) {
			TCPSegment seg;
			size_t payload_size = min({ _stream.buffer_size(),
									   static_cast<size_t>(_receiver_free_space),
									   static_cast<size_t>(TCPConfig::MAX_PAYLOAD_SIZE) });

			// 要发送的数据存在_stream中的
			seg.payload() = Buffer{ _stream.read(payload_size) };

			// 如果payload_size大于receiver的接受空间，说明至少还需要两次发送，所以还不能fin
			if (_stream.eof() && static_cast<size_t>(_receiver_free_space) > payload_size) {
				seg.header().fin = true;
				_fin_sent = true;
			}

			_send_segment(seg);

			if (_stream.buffer_empty())
				break;
		}
	}
	else if (_receiver_free_space == 0) {
		// 如果远程窗口大小为 0, 则把其视为 1 进行操作(为了让sender能收到ack反馈)
		TCPSegment seg;

		if (_stream.eof()) {
			seg.header().fin = true;
			_fin_sent = true;
			_send_segment(seg);
		}

		else if (!_stream.buffer_empty()) {
			seg.payload() = Buffer{ _stream.read(1) };
			_send_segment(seg);
		}
	}
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
	// pop seg from segments_outstanding
	// deduct bytes_inflight
	// reset rto, reset _consecutive_retransmissions
	// reset timer
	// stop timer if bytes_inflight == 0
	// fill window
	// update _receiver_window_size
	uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
	if (!_ack_valid(abs_ackno)) {
		cout << "invalid ackno!\n";
		return;
	}

	_receiver_window_size = window_size;
	_receiver_free_space = window_size;

	while (!_segments_outstanding.empty()) {
		TCPSegment seg = _segments_outstanding.front();
		// 如果队头的seg收到了ack，则出队
		if (unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= abs_ackno) {
			_bytes_in_flight -= seg.length_in_sequence_space();
			_segments_outstanding.pop();
			_time_elapsed = 0;
			_rto = _initial_retransmission_timeout;
			_consecutive_retransmissions = 0;
		}
		else
			break;
	}

	if (!_segments_outstanding.empty()) {
		_receiver_free_space = static_cast<uint16_t>(abs_ackno + static_cast<uint64_t>(window_size) -
			unwrap(_segments_outstanding.front().header().seqno, _isn, _next_seqno) - _bytes_in_flight);
	}

	// 还没开始发送segment时就不启动计时器了
	if (!_bytes_in_flight)
		_timer_running = false;
	fill_window();
}


//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
	if (!_timer_running)
		return;

	_time_elapsed += ms_since_last_tick;
	if (_time_elapsed >= _rto) {
		_segments_out.push(_segments_outstanding.front());
		if (_receiver_window_size || _segments_outstanding.front().header().syn) {
			// 每次重传把时间阈值×2（避免网络拥塞时还在不断发送），且记录重传次数
			_consecutive_retransmissions++;
			_rto *= 2;
		}
		_time_elapsed = 0;  // 记得清零记录时间
	}
}


unsigned int TCPSender::consecutive_retransmissions() const {
	return _consecutive_retransmissions;
}


void TCPSender::send_empty_segment() {
	TCPSegment seg;
	seg.header().seqno = wrap(_next_seqno, _isn);
	_segments_out.push(seg);
}


bool TCPSender::_ack_valid(uint64_t abs_ackno) {
	// 如果传入的ack不可靠则直接丢弃(都还没发送，怎么可能接到确认号？)
	if (_segments_outstanding.empty())
		return abs_ackno <= _next_seqno;
	return (abs_ackno <= _next_seqno &&
		abs_ackno >= unwrap(_segments_outstanding.front().header().seqno, _isn, _next_seqno));
}


void TCPSender::_send_segment(TCPSegment& seg) {
	seg.header().seqno = wrap(_next_seqno, _isn);
	_next_seqno += seg.length_in_sequence_space();
	_bytes_in_flight += seg.length_in_sequence_space();

	if (_syn_sent)
		_receiver_free_space -= seg.length_in_sequence_space();

	_segments_out.push(seg);
	_segments_outstanding.push(seg);

	// 一旦开始发送segment就要启动计时器
	if (!_timer_running) {
		_timer_running = true;
		_time_elapsed = 0;
	}
	// cout << "seqno: " << seg.header().seqno;
	// cout << "payload " << seg.payload().str();
	// cout << " receiver_free_space " << _receiver_free_space;
	// cout << " seg_length " << seg.length_in_sequence_space() << "\n";
}