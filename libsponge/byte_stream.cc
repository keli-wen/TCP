#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _capacity(capacity)
    , _buffer_read_count(0)
    , _buffer_written_count(0)
    , _buffer(deque<char>())
    , _end_input(false)
    , _error(false) {}

size_t ByteStream::write(const string &data) {
    if (input_ended())  // input is ended
        return 0;
    size_t data_len = data.length();
    size_t write_len = 0;
    // while the _buffer not full
    while (_buffer.size() < _capacity && write_len < data_len) {
        _buffer.push_back(data[write_len]);
        ++write_len;
    }
    _buffer_written_count += write_len;  // update
    return write_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t _buffer_len = _buffer.size();
    std::string output = "";
    for (int i = 0, _lim = std::min(_buffer_len, len); i < _lim; ++i) {
        output.push_back(_buffer[i]);
    }
    return output;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t _buffer_len = _buffer.size();
    for (int i = 0, _lim = std::min(_buffer_len, len); i < _lim; ++i) {
        _buffer.pop_front();  // removed from the output side
    }
    _buffer_read_count += std::min(_buffer_len, len);  //! pop count not read count
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    auto output = peek_output(len);
    pop_output(len);
    return output;
}

void ByteStream::end_input() { _end_input = true; }

bool ByteStream::input_ended() const { return _end_input; }

size_t ByteStream::buffer_size() const { return _buffer.size(); }

bool ByteStream::buffer_empty() const { return _buffer.empty(); }
// TODO：看的网上的
bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _buffer_written_count; }

size_t ByteStream::bytes_read() const { return _buffer_read_count; }

size_t ByteStream::remaining_capacity() const { return _capacity - _buffer.size(); }
