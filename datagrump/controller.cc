#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

#define DEBUG(X) if (debug_) { cerr << X << endl; }

const static unsigned int kDelayThresh = 100;
const static double kGamma = 0.15;
const static double kWindowDecay = 0.6;
const static double kWindowIncrease = 0.333;

const static unsigned int kGraceMillis = 250;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), the_window_size( 10.0 ), rtt_ewma ( 0.0 ), grace_end( 0 )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void ) {

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

  return (unsigned int)the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
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
  unsigned int delta = timestamp_ack_received - send_timestamp_acked;

  rtt_ewma = kGamma*delta + (1.0 - kGamma)*rtt_ewma;

  if (timestamp_ack_received < grace_end) {
    // do nothing
  } else if (rtt_ewma >= double(kDelayThresh)) {
    the_window_size *= kWindowDecay;
    grace_end = timestamp_ack_received + kGraceMillis;
  } else {
    the_window_size += kWindowIncrease;
  }


  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

void Controller::timeout_occurred( void )
{
//  if ( debug_ ) {
//    cerr << "timeout occurring" << endl;
//  }
//
//  the_window_size >>= 1;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 300; /* timeout of one second */
}
