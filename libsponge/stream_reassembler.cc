#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _cur_index(0)
    , _eof_index(std::numeric_limits<size_t>::max())
    , _unassembled_bytes_count(0)
    , _buffer(std::vector<std::pair<char, bool>>(capacity, std::make_pair('$', false))) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    const size_t length = data.length();

    size_t begin = std::max(index, _cur_index);
    size_t end = std::min(index + length, std::min(_eof_index, _cur_index + _capacity - _output.buffer_size()));
    // If eof is true, update the eof_index.
    if (eof) {
        //! We can't use end, since end may be smaller than
        //! index + length (when the output byteStream is full).
        _eof_index = std::min(_eof_index, index + length);
    }

    // Assgin the data to the buffer.
    for (size_t i = begin, j = begin - index; i < end; ++i, ++j) {
        // i: the index of the buffer.
        // j: the index of the data.
        if (!_buffer[i % _capacity].second) {
            _buffer[i % _capacity] = std::make_pair(data[j], true);
            // Write a byte to the reassembled stream.
            ++_unassembled_bytes_count;
        } else {
            assert(_buffer[i % _capacity].first == data[j] && "The data is not the same.");
        }
    }

    // Write the data to the output stream.
    std::string write_data = "";
    while (_cur_index < _eof_index && _buffer[_cur_index % _capacity].second) {
        write_data.push_back(_buffer[_cur_index % _capacity].first);
        _buffer[_cur_index % _capacity].second = false;
        ++_cur_index;
        --_unassembled_bytes_count;
    }

    if (!write_data.empty()) {
        _output.write(write_data);
    }

    // Check if the input is ended.
    if (_cur_index == _eof_index) {
        //! Note btyeStream.eof() <=> (input_ended() && buffer.empty())
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes_count; }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }
