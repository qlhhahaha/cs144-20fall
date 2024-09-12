
#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) {
	_capacity = capacity;
}

size_t ByteStream::write(const string& data) {
	if (_input_ended_flag) {
		return 0;
	}

	size_t real_write_size = min(data.size(), _capacity - _buffer.size());
	for (size_t i = 0; i < real_write_size; i++) {
		_buffer.push_back(data[i]);
		_write_size++;
	}

	return real_write_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
	size_t peek_num = min(len, _buffer.size());
	return string(_buffer.begin(), _buffer.begin() + peek_num);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
	_read_size += len;
	for (size_t i = 0; i < len; i++) {
		_buffer.pop_front();
	}
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
	string read_result = this->peek_output(len);
	this->pop_output(len);
	return read_result;
}

void ByteStream::end_input() {
	_input_ended_flag = true;
}

bool ByteStream::input_ended() const {
	return _input_ended_flag;
}


size_t ByteStream::buffer_size() const {
	return _buffer.size();
}

bool ByteStream::buffer_empty() const {
	return _buffer.empty();
}

bool ByteStream::eof() const {
	return input_ended() && buffer_empty();
}

size_t ByteStream::bytes_written() const {
	return _write_size;
}

size_t ByteStream::bytes_read() const {
	return _read_size;
}

size_t ByteStream::remaining_capacity() const {
	return _capacity - _buffer.size();
}
