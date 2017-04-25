#include <iostream>
#include <vector>
#include <unordered_map>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

#define DEBUG(x) ({if (debug_) { cerr << x << endl; }})
#define UNUSED(...) ((void)__VA_ARGS__)

enume Mode {
  TRAIN;
  EVAL;
};

class Approximator {
private:
public:
  Approximator() {}
  // evaluate the approximated function at point x
  double evaluate(double x) {
  }

  // Observe a new example point of the function we wish to approximate
  double observe(double x, double y) {
  }
};

static Mode mode = TRAIN;
static double cwnd_ = 10.0; // initial cwnd
static unordered_map<unsigned int, unsigned int> packet_windows;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void ) {
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  UNUSED(sequence_number);
  UNUSED(send_timestamp);

  // store the cwnd for when this packet was sent
  int cwnd = int(cwnd_);
  packet_windows[sequence_number] = cwnd;


  if (debug_) {
    cerr
      << "Sent datagram"
      << "\t seqno=" << sequence_number
      << endl;
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

  switch (mode) {
    case TRAIN: {
      auto send_cwnd = packet_windows[sequence_number_acked];
      double rtt = double(timestamp_ack_received - send_timestamp_acked);
      approximator.add_point(send_cwnd, rtt);
    }
    case EVAL: {
      // use the existing model to predict cwnd based on most recent delay
    }
  }



  if ( debug_ ) {
    cerr
      << "Received seq=" << sequence_number_acked
      << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return kRTTGain*rtt_filter_.min();
}
