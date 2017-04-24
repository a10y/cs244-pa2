#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

#define DEBUG(X) if (debug_) { cerr << X << endl; }

// const static unsigned int kDelayThresh = 100;
const static double kGamma = 0.15;
const static double kWindowDecay = 0.8;
const static double kWindowGrow = 1.2;
const static unsigned int kGraceMillis = 50;
const static unsigned int kMinWindowSize = 4;

const static unsigned int kMaxRTT = 100;
const static unsigned int kMinRTT = 75;

// const static unsigned int kWindowSizeRTTProduct = 1000;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), the_window_size( kMinWindowSize ), rtt_ewma ( 0.0 ), grace_end( 0 )
{
  for (int i = 0; i < NUM_TIMESTAMPS; ++i) {
    rtt[i] = 0;
  }
}

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

double Controller::interpolate( void )
{
  double total[NUM_TIMESTAMPS];
  for(int i = 0; i < NUM_TIMESTAMPS; ++i) {
    total[i] = 1;
  }
  
  for(int i = 0; i < NUM_TIMESTAMPS; ++i) {
    for(int j = 0; j < NUM_TIMESTAMPS; ++j) {
      if(i == j) {
	total[i] *= rtt[INTERVAL_LEN * i];
      } else {
	total[i] *= 1.0 * (NUM_TIMESTAMPS - j) / (i - j);
      }
    }
  }

  double sum = 0;
  for(int i = 0; i < NUM_TIMESTAMPS; ++i) {
    sum += total[i];
  }

  return sum;
  //return rtt_ewma + (rtt_ewma - rtt[0]) / NUM_TIMESTAMPS;
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

  // Update list of rtts. Lower index -> earlier in time.
  for(int i = 0; i < NUM_TIMESTAMPS * INTERVAL_LEN - 1; ++i) {
    rtt[i] = rtt[i+1];
  }
  rtt_ewma = kGamma*delta + (1.0 - kGamma)*rtt_ewma;
  rtt[NUM_TIMESTAMPS * INTERVAL_LEN - 1] = rtt_ewma;
  double predicted_rtt = interpolate();

  for(int i = 0; i < NUM_TIMESTAMPS * INTERVAL_LEN; i++) {
    cout << rtt[i] << ' ';
  }
  cout << predicted_rtt << endl;

  if (timestamp_ack_received < grace_end) {
    // do nothing
  } else if (predicted_rtt >= double(kMaxRTT)) {
    the_window_size *= kWindowDecay;
    grace_end = timestamp_ack_received + kGraceMillis;
  } else if (predicted_rtt <= double(kMinRTT)) {
    the_window_size *= kWindowGrow;
    grace_end = timestamp_ack_received + kGraceMillis;
  }

  if (the_window_size < double(kMinWindowSize)) {
    the_window_size = kMinWindowSize;
  }

  /*if (timestamp_ack_received < grace_end) {
    // do nothing
  } else if (rtt_ewma >= double(kDelayThresh)) {
    the_window_size *= kWindowDecay;
    grace_end = timestamp_ack_received + kGraceMillis;
  } else {
    the_window_size += 1.0 / the_window_size;
    }*/


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
