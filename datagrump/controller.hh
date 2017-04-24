#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>

class Controller
{
private:
  bool debug_; // Enables debugging output

  // parameters for estimating the RTTprop
  unsigned int est_rtt_prop_; // Estimate of RTT for propagation (no queueing delay)


  // parameters for estimating delivery rate
  unsigned int interval_start_;
  unsigned int interval_pkts_recv_;
  double est_deliv_rate_;
  double max_deliv_rate_;


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
