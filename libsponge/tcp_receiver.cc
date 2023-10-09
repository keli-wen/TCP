#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const auto &header = seg.header();

    if (!_isn.has_value()) {
        // Drop before *LISTEN*.
        if (!header.syn) return void();
        // Set the Initial Seq Number.
        _isn = header.seqno;
    }

    // *SYN RECV*
    //? Why we need this?
    // WrappingInt32 seqno = header.seqno;

    // Denote the first byte that the receiver needs from the sender.
    uint64_t abs_ackno = stream_out().bytes_written() + 1;
    uint64_t abs_seqno = unwrap(header.seqno, _isn.value(), abs_ackno);

    //! Note: Need more explanation?
    uint64_t stream_index = abs_seqno + header.syn - 1;

    // If header.fin is true, then state tranfer to *FIN_RECV*.
    _reassembler.push_substring(
        seg.payload().copy(), stream_index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    // LISTEN
    if (!_isn.has_value()) return nullopt;
    // SYN_RECV
    WrappingInt32 ackno = wrap(stream_out().bytes_written() + 1, _isn.value());
    // FIN_RECV
    if (stream_out().input_ended()) ackno = ackno + 1;
    return ackno;
}

size_t TCPReceiver::window_size() const {
    // Window size = capacity - buffer already received.
    return _capacity - stream_out().buffer_size();
}
