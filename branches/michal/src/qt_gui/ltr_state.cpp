
#include "ltr_state.h"
#include <ltlib.h>

TrackerState *TrackerState::ts = NULL;

TrackerState& TrackerState::trackerStateInst()
{
  if(ts == NULL){
    ts = new TrackerState();
  }
  return *ts;
}

TrackerState::TrackerState()
{
  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(pollState()));
  timer->start(200);
}

TrackerState::~TrackerState()
{
  delete timer;
}

void TrackerState::pollState()
{
  static lt_state_type last_state = STOPPED;
  lt_state_type current_state = lt_get_tracking_state();
  if(last_state != current_state){
    switch(current_state){
      case STOPPED:
        emit trackerStopped();
        break;
      case RUNNING:
        emit trackerRunning();
        break;
      case PAUSED:
        emit trackerPaused();
        break;
    }
    last_state = current_state;
  }
}

