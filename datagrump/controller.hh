#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>

// Filter for different window statistics.
class Filter
{
private
:
  double min_ = -1.0;
  double max_ = -1.0;
  double ewma_ = 0.0;
  double gamma_;
public:
  Filter(double gamma) : gamma_(gamma) {}
  void observe(double measurement) {
    ewma_ = gamma_*measurement + (1.0-gamma_)*ewma_;
    if (min_ < 0.0) {
      min_ = measurement;
    } else {
      min_ = std::min(min_, measurement);
    }
    max_ = std::max(max_, measurement);
  }
  double min() {
    return min_;
  }
  double max() {
    return max_;
  }
  double ewma() {
    return ewma_;
  }
  void clear() {
    ewma_ = 0.0;
    min_ = -1.0;
    max_ = -1.0;
  }
};

class Controller
{
private:
  bool debug_; // Enables debugging output


public:
  /* Public interface for the congestion controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in sender.cc) */

  /* Default constructor */
  Controller( const bool debug );

  /* Get current window size, in datagrams */
  unsigned int window_size( void );

  /* A datagram was sent */
  void datagram_was_sent( const uint64_t sequence_number,
			  const uint64_t send_timestamp );

  /* An ack was received */
  void ack_received( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );

  /* How long to wait (in milliseconds) if there are no acks
     before sending one more datagram */
  unsigned int timeout_ms( void );
};

#endif
