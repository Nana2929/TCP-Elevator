#include <arpa/inet.h>
#include <chrono>
#include <ctime>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "elevator.hpp"

/*********
 * ELEVATOR_SIGNAL_PRESS_1F = 1
 * ELEVATOR_SIGNAL_PRESS_1F = 2
 * ELEVATOR_SIGNAL_FLOOR_1_UP = 3
 * ELEVATOR_SIGNAL_FLOOR_2_DOWN = 4
 **********/

// thread-safe reading and writing
/*
although the operation of these functions are rather simple,
they involve acquiring a lock and thus may not be good to be `inline`
*/
bool Elevator::isPressed(const int btn) {
  // first check if btn is valid, if not return
  std::lock_guard<std::mutex> lock(mtx);
  return pressedBtns[btn];
}

void Elevator::cleanBtn(const int btn) {
  if (btn <= 0 || btn > BTN_NUM) {
    return;
  }
  std::lock_guard<std::mutex> lock(mtx);
  pressedBtns[btn] = false;
}
State Elevator::getState() {
  std::lock_guard<std::mutex> lock(stateMtx);
  return currState;
}
void Elevator::setState(const State &state) {
  std::lock_guard<std::mutex> lock(stateMtx);
  currState = state;
  stateCv.notify_one(); // Notify the printState thread
}

// translate state to string
static std::string translateState(const State &state) {
  static std::unordered_map<State, std::string> state_map = {
      {State::IDLE_1F, "➡️⬅️ 1F-IDLE"},
      {State::OPEN_1F, "⬅️➡️ 1F-OPEN"},
      {State::LIFT, "🔼 LIFT"},
      {State::IDLE_2F, "⬅️➡️ 2F-IDLE"},
      {State::OPEN_2F, "⬅️➡️ 2F-OPEN"},
      {State::GO_DOWN, "🔽 GO-DOWN"}};
  if (state_map.find(state) != state_map.end()) {
    return state_map[state];
  } else {
    return "UNKNOWN";
  }
}
// ** printing thread
void Elevator::printState() {
  while (true) {
    {
      std::unique_lock<std::mutex> lock(stateMtx);
      stateCv.wait_for(
          lock, std::chrono::seconds(1)); // Wait for 1 second or a state change
    }
    // Access state safely
    State cs = getState();
    std::cout << translateState(cs) << std::endl;
  }
}
// ** func for the listening thread
void Elevator::pressBtn(const int btn) {
  std::lock_guard<std::mutex> lock(mtx);
  pressedBtns[btn] = true;
  // std::cout << "Button " << btn << " is pressed." << std::endl;
}

// ** control_thread + logic
void Elevator::stateTransit() {
  while (true) {
    // IDLE_1F: it means it is DOOR CLOSED and it has no action
    if (getState() == State::IDLE_1F) {
      if (isPressed(1) || isPressed(3)) {
        setState(State::OPEN_1F);
        actTime = std::time(0);
        cleanBtn(1);
        cleanBtn(3);
      } else if (isPressed(2) || isPressed(4)) {
        setState(State::LIFT);
        actTime = std::time(0);
        cleanBtn(2);
        cleanBtn(4);
      }
    }
    // OPEN_1F
    else if (getState() == State::OPEN_1F) {
      // exception handling: if in the middle of opening, 1 or 3 action is take,
      // we reset the timer and reopen the door
      while (std::difftime(std::time(0), actTime) < OPEN_TIME) {
        if (isPressed(1) || isPressed(3)) {
          actTime = std::time(0);
          cleanBtn(1);
          cleanBtn(3);
        }
      }
      // once the door has opened for 2 seconds, it will transit to IDLE_1F
      setState(State::IDLE_1F);
    }
    // LIFT
    else if (getState() == State::LIFT) {
      while (std::difftime(std::time(0), actTime) < MOVE_TIME) {
        ;
      }
      cleanBtn(
          1); // clean internal butttons because it is already taking action
      cleanBtn(
          2); // clean internal butttons becayse it is already taking action
      cleanBtn(4); // if it is moving up anyway, it is not taking an effect of
                   // going to 2nd floor to meet the request
      // only the action pressing button `3` will be recorded
      setState(State::OPEN_2F);
      actTime = std::time(0);
    }
    // OPEN_2F
    else if (getState() == State::OPEN_2F) {
      // exception handling
      while (std::difftime(std::time(0), actTime) < OPEN_TIME) {
        if (isPressed(2) || isPressed(4)) {
          actTime = std::time(0);
          cleanBtn(2);
          cleanBtn(4);
        }
      }
      setState(State::IDLE_2F);
      actTime = std::time(0);
    }
    // IDLE_2F
    else if (getState() == State::IDLE_2F) {
      if (isPressed(2) || isPressed(4)) {
        setState(State::OPEN_2F);
        actTime = std::time(0);
        cleanBtn(2);
        cleanBtn(4);
      } else if (isPressed(1) || isPressed(3)) {
        setState(State::GO_DOWN);
        actTime = std::time(0);
        cleanBtn(1);
        cleanBtn(3);
      }
    }
    // GO_DOWN
    else if (getState() == State::GO_DOWN) {
      while (std::difftime(std::time(0), actTime) < MOVE_TIME) {
        ;
      }
      cleanBtn(1); // clean internal butttons
      cleanBtn(2); // clean internal butttons
      cleanBtn(3); // it is moving down anyway, not taking an effect
      setState(State::OPEN_1F);
      actTime = std::time(0); // transit to 1F-OPEN
    }
  } // end of while
}
