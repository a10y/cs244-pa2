#include <iostream>
#include <vector>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

#define DEBUG(X) if (debug_) { cerr << X << endl; }
#define UNUSED(x) ((void)x)

/*
Perform simple proportional control w.r.t. queueing delay to determine the necessary output for things.
*/

const static double kGamma = 0.15;
const static double kWindowDecay = 0.6;
const static double kWindowGrow = 1.4;

const static auto kMinWindow = 5.0;
const static auto kMaxWindow = 100.0; // This is probably a safe assumption

const auto kTargetQDelay = 0.0; // Minimize queueing delay
const static double kProp = 0.0006;

// Estimated RTTprop
static auto est_rtt_prop = 0.0;

// Min over kRTTWin last RTT's
const static auto kRTTWin = 15;
static vector<int> window;

static int outstandingPkts = 0;


const static auto kDelayWin = 40;
static vector<double> q_delay_window;

template<typename T>
static T min_vec(vector<T> myVec) {
  T min = myVec[0];
  for (auto &v : myVec) {
    if (v < min) min = v;
  }
  return min;
}

template<typename T>
static double avg_vec(vector<T> myVec) {
  double sum = 0.0;
  for (auto v : myVec) {
    sum += v;
  }
  return sum / myVec.size();
}

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), the_window_size( 20.0 )
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
  // On sending of a datagram, increment the amount of outstanding data in the network.
  //if ( debug_ ) {
  //  cerr << "At time " << send_timestamp
  //       << " sent datagram " << sequence_number
  //       << " the_window_size=" << the_window_size << endl;
  //}
  UNUSED(sequence_number);
  UNUSED(send_timestamp);
  outstandingPkts++;
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

  // Decrement number of outstanding packets
  // TODO: figure out how to use # of outstanding packets for something useful.
  outstandingPkts--;

  // RTT estimation
  auto rtt_new = timestamp_ack_received - send_timestamp_acked;

  window.push_back(rtt_new);
  if (window.size() > kRTTWin) {
    window.erase(window.begin());
  }
  auto rtt_prop_est = min_vec(window);

  est_rtt_prop = (1 - kGamma)*est_rtt_prop + kGamma*rtt_prop_est;

  // Queueing delay estimation is based on our currently estimated rtt_prop plus total rtt
  // smoothed average queueing delay
  auto q_delay = rtt_new - est_rtt_prop;
  q_delay_window.push_back(q_delay);
  if (q_delay_window.size() > kDelayWin) {
    q_delay_window.erase(q_delay_window.begin());
  }
  auto q_delay_avg = avg_vec(q_delay_window);
  double delta_q_delay = q_delay_avg - q_delay;

  // If q_delay goes up, then we want to decrease our window, otherwise if it goes down increase the window.
  the_window_size += kProp * delta_q_delay;
  the_window_size = min(max(kMinWindow, the_window_size), kMaxWindow);

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
         << "\tdelta_q_delay=" << delta_q_delay
         << "\test_rtt_prop=" << est_rtt_prop
         << "\twindow=" << the_window_size
	 << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  // Assume packets should arrive within 2 RTT's
  return int(2 * est_rtt_prop);
}
