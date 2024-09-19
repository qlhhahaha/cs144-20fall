#include "stream_reassembler.hh"

#include <cassert>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
	: _output(capacity),
	_capacity(capacity),
	_next_assembled_idx(0),
	_unassembled_bytes_num(0),
	_eof_idx(-1),
	_unassembled_strs()
{}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string& data, const size_t index, const bool eof) {

	// 获取 map 中第一个大于 index 的迭代器指针
	auto pos_iter = _unassembled_strs.upper_bound(index);
	// 尝试获取一个小于等于 index 的迭代器指针
	if (pos_iter != _unassembled_strs.begin())
		pos_iter--;
	/**
	 *  此时迭代器有三种情况，
	 *  1. 一种是为 end_iter，表示不存在任何数据
	 *  2. 一种是为非 end_iter，
	 *      a. 一种表示确实存在一个小于等于 idx 的迭代器指针
	 *      b. 一种表示当前指向的是大于 idx 的迭代器指针
	 */

	 // 获取当前segment的新起始位置
	size_t new_idx = index;
	// 如果在暂存区中，index前面有存好的子串
	if (pos_iter != _unassembled_strs.end() && pos_iter->first <= index) {
		const size_t up_idx = pos_iter->first;

		// 如果当前segment前面出现了重叠
		if (index < up_idx + pos_iter->second.size())
			new_idx = up_idx + pos_iter->second.size();
	}
	// 如果前面没有子串，则和当前读取到的pos进行比较
	else if (index < _next_assembled_idx)
		new_idx = _next_assembled_idx;

	// segment新起始位置对应到的 data 索引
	const size_t data_start_pos = new_idx - index;
	// 当前segment将保存的 data 的长度
	ssize_t data_size = data.size() - data_start_pos;

	// 获取当前segment的下个segment时，需要考虑到 new_idx 可能会和 down_idx 重合
	pos_iter = _unassembled_strs.lower_bound(new_idx);

	// 如果和下面的segment冲突，则截断长度
	while (pos_iter != _unassembled_strs.end() && new_idx <= pos_iter->first) {
		const size_t data_end_pos = new_idx + data_size;
		// 如果存在重叠区域
		if (pos_iter->first < data_end_pos) {
			// 如果是部分重叠
			if (data_end_pos < pos_iter->first + pos_iter->second.size()) {
				data_size = pos_iter->first - new_idx;
				break;
			}
			// 如果是全部重叠
			else {
				_unassembled_bytes_num -= pos_iter->second.size();
				pos_iter = _unassembled_strs.erase(pos_iter);
				continue;
			}
		}
		// 如果不重叠则直接 break
		else
			break;
	}

	// 检测是否存在数据超出了容量。注意这里的容量并不是指可保存的字节数量，而是指可保存的窗口大小
	//! NOTE: 注意这里我们仍然接收了 index 小于 first_unacceptable_idx  但
	//        index + data.size >= first_unacceptable_idx 的那部分数据
	//        这是因为处于安全考虑，最好减少算术运算操作以避免上下溢出
	//        同时多余的那部分数据最多也只会多占用 1kb 左右，属于可承受范围之内
	size_t first_unacceptable_idx = _next_assembled_idx + _capacity - _output.buffer_size();
	if (first_unacceptable_idx <= new_idx)
		return;

	// 判断是否还有数据是独立的， 顺便检测当前segment是否被上一个segment完全包含
	if (data_size > 0) {
		const string new_data = data.substr(data_start_pos, data_size);
		// 如果新segment可以直接写入
		if (new_idx == _next_assembled_idx) {
			const size_t write_byte = _output.write(new_data);
			_next_assembled_idx += write_byte;
			// 如果没写全，则将其保存起来
			if (write_byte < new_data.size()) {
				// _output 写不下了，插入进 _unassembled_strs 中
				const string data_to_store = new_data.substr(write_byte, new_data.size() - write_byte);
				_unassembled_bytes_num += data_to_store.size();
				_unassembled_strs.insert(make_pair(_next_assembled_idx, std::move(data_to_store)));
			}
		}
		else {
			const string data_to_store = new_data.substr(0, new_data.size());
			_unassembled_bytes_num += data_to_store.size();
			_unassembled_strs.insert(make_pair(new_idx, std::move(data_to_store)));
		}
	}

	// 一定要处理重叠segment的情况
	for (auto iter = _unassembled_strs.begin(); iter != _unassembled_strs.end(); /* nop */) {
		assert(_next_assembled_idx <= iter->first);
		// 如果当前刚好是一个可被接收的信息
		if (iter->first == _next_assembled_idx) {
			const size_t write_num = _output.write(iter->second);
			_next_assembled_idx += write_num;
			// 如果没写全，则说明写满了，保留剩余没写全的部分并退出
			if (write_num < iter->second.size()) {
				_unassembled_bytes_num += iter->second.size() - write_num;
				_unassembled_strs.insert(make_pair(_next_assembled_idx, std::move(iter->second.substr(write_num))));

				_unassembled_bytes_num -= iter->second.size();
				_unassembled_strs.erase(iter);
				break;
			}
			// 如果写全了，则删除原有迭代器，并进行更新
			_unassembled_bytes_num -= iter->second.size();
			iter = _unassembled_strs.erase(iter);
		}
		// 否则直接离开
		else
			break;
	}
	if (eof)
		_eof_idx = index + data.size();
	if (_eof_idx <= _next_assembled_idx)
		_output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes_num; }

bool StreamReassembler::empty() const { return _unassembled_bytes_num == 0; }