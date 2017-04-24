#include <iostream>
#include <vector>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

#define DEBUG(X) if (debug_) { cerr << X << endl; }

/*
Perform simple proportional control w.r.t. queueing delay to determine the necessary output for things.
*/

const static double kGamma = 0.15;
const static double kWindowDecay = 0.6;
const static double kWindowGrow = 1.4;

const static auto kMinWindow = 5.0;
const static auto kMaxWindow = 100.0; // This is probably a safe assumption

const auto kTargetQDelay = 0.0; // Minimize queueing delay
const static double kProp = 0.001;

// Estimated RTT
static auto est_rtt = 0.0;
//static auto last_q_delay = 0.0;

// Min over kRTTWin last RTT's
const static auto kRTTWin = 25;
static vector<int> window;

template<typename T>
static T min_vec(vector<T> myVec) {
  T min = myVec[0];
  for (auto &v : myVec) {
    if (v < min) min = v;
  }
  return min;
}

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), the_window_size( kMinWindow )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void ) {

  //if ( debug_ ) {
  //  cerr << "At time " << timestamp_ms()
  //       << " window size is " << the_window_size << endl;
  //}

  return int(the_window_size);
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number
         << " the_window_size=" << the_window_size << endl;
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

  // RTT estimation
  auto rtt_new = timestamp_ack_received - send_timestamp_acked;

  window.push_back(rtt_new);
  if (window.size() > kRTTWin) {
    window.erase(window.begin());
  }
  auto rtt_prop_est = min_vec(window);

  est_rtt = (1 - kGamma)*est_rtt + kGamma*rtt_prop_est;

  // Queueing delay estimation
  auto q_delay = rtt_new - rtt_prop_est;
  double err_q_delay = (est_rtt - double(q_delay));

  // We want to decrease cwnd if err_q_delay is large, otherwise we assume that there is something going on here

  // Feed into PID controller
  the_window_size += kProp * err_q_delay;
  the_window_size = min(max(kMinWindow, the_window_size), kMaxWindow);


  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
         << " err_q_delay=" << err_q_delay
         << " est_rtt=" << est_rtt
	 << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  // Assume packets should arrive within 2 RTT's
  return int(2 * est_rtt);
}
