// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : sequencer.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#include "board.h"
#include "sequencer.h"
#include "hardware/timer.h"

RocketState_t seqPreLaunch(){

    return PRE_FLIGHT;
}


RocketState_t seqPyroRdy(){

    return PYRO_RDY;
}

RocketState_t seqAscend(){

    return ASCEND;
}

RocketState_t seqDeploy(){

    return DEPLOY_TIMER;
}

RocketState_t seqDescend(){

    return DESCEND;
}

RocketState_t seqTouchdown(){

    return TOUCHDOWN;
}
