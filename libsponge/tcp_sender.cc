#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _window_size(1)
    , _retransmission_count(0)
    , _bytes_in_flight(0)
    , _sent_not_acked(std::queue<std::pair<uint64_t, TCPSegment>>())
    , _retransmission_timer(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const {
    return _bytes_in_flight;
}

void TCPSender::fill_window() {
    // If the receiver has announced a window size of zero, **the fill
    // window method should act like the window size is one**. 
    //! If we directly update the _window_size, then we'll always assume
    //! that the window size is 1. So we'll always retransmit the segment.
    auto window_size = std::max(_window_size, static_cast<uint16_t>(1));

    //? Why we need use while loop?
    while (_bytes_in_flight < window_size) {
        TCPSegment header = TCPSegment();

        // Step1. Check if ISN has been sent.
        //* CLOSED -> SYN_SENT *//
        if (!_syn_sent) {
            header.header().syn = true;
            _syn_sent = true;
        }

        // Step2. Calculate the max payload size.
        // MAX_PALYLOAD_SIZE only control the size of the payload.
        // But window size need to consider the SYN and FIN.
        auto payload_size = std::min(
            TCPConfig::MAX_PAYLOAD_SIZE,
            std::min(
                stream_in().buffer_size(),
                static_cast<size_t>(
                    window_size - _bytes_in_flight - header.header().syn)
            )
        );
        auto payload = _stream.read(payload_size);
        header.payload() = Buffer(std::move(payload));

        // Step3. Check FIN.
        //* FIN_SENT *//
        if (!_fin_sent
            && stream_in().eof() 
            && _bytes_in_flight + header.length_in_sequence_space() < window_size) {

            header.header().fin = true;
            _fin_sent = true;
        }

        // Step4. Check if the header is empty.
        auto header_length = header.length_in_sequence_space();
        if (header_length == 0) break;

        // Step5. Send the TCPSegment and update corresponding variables.
        //* SYN_SENT *//
        header.header().seqno = next_seqno();
        _segments_out.emplace(header);

        // Update _bytes_in_flight.
        _bytes_in_flight += header_length;

        // Trigger the retransmission timer.
        if (!_retransmission_timer.is_running()) {
            _retransmission_timer.start();
            _retransmission_timer.set_timeout(_initial_retransmission_timeout);
        }

        // Step6. backup the sent segment and update the _next_seqno.
        _sent_not_acked.emplace(next_seqno_absolute(), std::move(header));
        _next_seqno += header_length;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    auto abs_ackno = unwrap(ackno, _isn, next_seqno_absolute());
    // Step1. Convert the ackno to absolute value and check if it is valid.
    //? Why we need to discard the ackno?
    //? Why we'll receive the ackno that is greater than the next_seqno_absolute()?
    if (abs_ackno > next_seqno_absolute()) return void();

    bool is_successful = false;

    // Step2. Manage the _sent_not_acked according to the new received ackno.
    //* SYN_ACKED *//
    while (!_sent_not_acked.empty()) {
        auto &[abs_seqno, header] = _sent_not_acked.front();
        if (abs_seqno + header.length_in_sequence_space() <= abs_ackno) {
            // Successfully sent the TCP segment.
            is_successful = true;
            // Update the _bytes_in_flight.
            _bytes_in_flight -= header.length_in_sequence_space();
            _sent_not_acked.pop();
        } else {
            //! Note to break, otherwise infinite loop.
            break;
        }
    }

    // Step3. Manage the retransmission timer.
    if (is_successful) {
        _retransmission_count = 0;
        _retransmission_timer.set_timeout(_initial_retransmission_timeout);
        _retransmission_timer.start();
    }

    /* Page5. When all outstanding data has been acknowledged,
    stop the retransmission timer. */
    if (_bytes_in_flight == 0) {
        _retransmission_timer.stop();
    }
    
    // Step4. Update the _window_size and try to fill the window.
    _window_size = window_size;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // We don't need to check if _retransmission_timer is running.
    // Since, we'll check it in the tick() method.
    _retransmission_timer.tick(ms_since_last_tick);
    if (_retransmission_timer.is_expired()) {
        if (_sent_not_acked.empty()) {
            throw std::runtime_error(
                "Retransmission timer is expired but the _sent_not_acked is empty.");
        }

        // Double the value of RTO.
        _segments_out.push(_sent_not_acked.front().second);

        if (_window_size != 0) {
            /* Page5. If the window size is nonzero */
            _retransmission_timer.double_timeout();
            _retransmission_count++;
        }
        // Reset the retransmission timer and start it.
        _retransmission_timer.start();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _retransmission_count;
}

void TCPSender::send_empty_segment() {
    //! Note, send a segment with window size 1.
    /**
     * If the receiver has announced a window size of zero, **the fill
     * window method should act like the window size is one**. The sender
     * might end up sending a single byte that gets rejected (and not
     * acknowledged) by the receiver, but this can also provoke the
     * receiver into sending a new acknowledgment segment **where it
     * reveals that more space has opened up in its window.** Without this,
     * the sender would never learn that it was allowed to start sending
     * again.
     */
    TCPSegment header = TCPSegment();
    header.header().seqno = next_seqno();
    // Don't update the _bytes_in_flight and trigger the retransmission timer.
    _segments_out.emplace(std::move(header));
}
