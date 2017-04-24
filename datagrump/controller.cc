#include <iostream>
#include <vector>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

#define DEBUG(x) ({if (debug_) { cerr << x << endl; }})
#define UNUSED(...) ((void)__VA_ARGS__)


// Initial cwnd and RTTprop
const static auto kInitCwnd = 10.0;
const static auto kInitRTT = 100;

// How many RTT's must pass before something is considered "timed out"
const static auto kTimeoutFactor = 4.0;

// Length of an interval in MS
const static auto kIntervalLength = 1000;

const static auto kCwndGain = 1.0;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    est_rtt_prop_(kInitRTT),
    interval_start_(0),
    interval_pkts_recv_(0),
    est_deliv_rate_(0.01),
    max_deliv_rate_(0.0)
{
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void ) {
  return kCwndGain * est_rtt_prop_ * est_deliv_rate_;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  UNUSED(sequence_number);
  UNUSED(send_timestamp);

  if (debug_) {
    cerr
      << "Sent datagram"
      << "\t seqno=" << sequence_number
      << "\t est_rtt=" << est_rtt_prop_
      << "\t est_deliv=" << est_deliv_rate_
      << endl;
  }

  // See if we should begin a new interval here
  auto now = timestamp_ms();
  if (now > interval_start_ + kIntervalLength) {
    interval_start_ = now;
    interval_pkts_recv_ = 0;
    max_deliv_rate_ = 0.0;
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  UNUSED(recv_timestamp_acked);
  UNUSED(sequence_number_acked);
  UNUSED(send_timestamp_acked);
  UNUSED(timestamp_ack_received);

  // Update information about current interval for delivery rate estimate
  interval_pkts_recv_++;
  auto now = timestamp_ms();
  auto time_since_interval_start = now - interval_start_;
  auto deliv_rate = double(interval_pkts_recv_) / time_since_interval_start;
  est_deliv_rate_ = max(deliv_rate, max_deliv_rate_);

  unsigned int rtt_new = timestamp_ack_received - send_timestamp_acked;
  cerr << "rtt_new=" << rtt_new << "\t" << "est_rtt=" << est_rtt_prop_ << endl;
  est_rtt_prop_ = min(est_rtt_prop_, rtt_new);

  if ( debug_ ) {
    cerr
      << "Received seq=" << sequence_number_acked
      << "\t est_rtt_prop_=" << est_rtt_prop_
      << "\t est_deliv_rate_=" << est_deliv_rate_
      << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return kTimeoutFactor*est_rtt_prop_;
}
