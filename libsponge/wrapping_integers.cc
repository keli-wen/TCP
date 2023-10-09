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
    return WrappingInt32(static_cast<uint32_t>(n + isn.raw_value()));
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
    // Since the low 32 bits don't influence the result, we can directly
    // calculate them.
    uint32_t low32bit = static_cast<uint32_t>(n - isn);
    uint64_t hight32bit_add1 = (checkpoint + (1 << 31)) & 0xFFFFFFFF00000000;
    uint64_t hight32bit_minus1 = (checkpoint - (1 << 31)) & 0xFFFFFFFF00000000;
    uint64_t absolute_add1 = hight32bit_add1 | low32bit;
    uint64_t absolute_minus1 = hight32bit_minus1 | low32bit;

    // Compare the distance between the checkpoint and the two possible
    // results.
    if (max(absolute_add1, checkpoint) - min(absolute_add1, checkpoint) <=
        max(absolute_minus1, checkpoint) - min(absolute_minus1, checkpoint)) {
        return absolute_add1;
    } else {
        return absolute_minus1;
    }
    return -1; // Error.
}
