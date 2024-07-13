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

// resources: https://shengyu7697.github.io/cpp-windows-tcp-socket/


// thread-safe reading and writing
bool Elevator::isPressed(const int btn) {
  std::lock_guard<std::mutex> lock(mtx);
  return pressedBtns[btn];
}

void Elevator::cleanBtn(int btn) {
  std::lock_guard<std::mutex> lock(mtx);
  pressedBtns[btn] = false;
}

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

void Elevator::printState() {
  while (true) {
    std::cout << translateState(currState) << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
// thread-safe writing; user-exposed API
// ** listening
void Elevator::pressBtn(const int btn) {
  std::lock_guard<std::mutex> lock(mtx);
  pressedBtns[btn] = true;
  // std::cout << "Button " << btn << " is pressed." << std::endl;
}

// ** control_thread
void Elevator::stateTransit() {
  while (true) {
    // 1F-IDLE
    if (currState == State::IDLE_1F) {
      if (isPressed(1) || isPressed(3)) {
        currState = State::OPEN_1F;
        actTime = std::time(0);
        cleanBtn(1);
        cleanBtn(3);
      } else if (isPressed(2) || isPressed(4)) {
        currState = State::LIFT;
        actTime = std::time(0);
        cleanBtn(2);
        cleanBtn(4);
      }
    }
    // 1F-OPEN
    else if (currState == State::OPEN_1F) {
      // exception handling: if in the middle of opening, 1/3 is pressed, we
      // need to reset the timer and reopen the door
      while (std::difftime(std::time(0), actTime) < OPEN_TIME) {
        if (isPressed(1) || isPressed(3)) {
          actTime = std::time(0);
          cleanBtn(1);
          cleanBtn(3);
        }
      }
      currState = State::IDLE_1F;
    }
    // LIFT
    else if (currState == State::LIFT) {
      while (std::difftime(std::time(0), actTime) < MOVE_TIME) {
        ;
      }
      cleanBtn(1); // clean internal butttons
      cleanBtn(2); // clean internal butttons
      cleanBtn(4); // it is moving up anyway, not taking an effect
      currState = State::OPEN_2F;
      actTime = std::time(0); // transit to 2F-OPEN
    }                         // 2F-OPEN
    // 2F-OPEN
    else if (currState == State::OPEN_2F) {
      // exception handling
      while (std::difftime(std::time(0), actTime) < OPEN_TIME) {
        if (isPressed(2) || isPressed(4)) {
          actTime = std::time(0);
          cleanBtn(2);
          cleanBtn(4);
        }
      }
      currState = State::IDLE_2F;
      actTime = std::time(0);
    }
    // 2F-IDLE
    else if (currState == State::IDLE_2F) {
      if (isPressed(1) || isPressed(3)) {
        currState = State::GO_DOWN;
        actTime = std::time(0);
        cleanBtn(1);
        cleanBtn(3);
      } else if (isPressed(2) || isPressed(4)) {
        currState = State::OPEN_2F;
        actTime = std::time(0);
        cleanBtn(2);
        cleanBtn(4);
      }
    }
    // GO-DOWN
    else if (currState == State::GO_DOWN) {
      while (std::difftime(std::time(0), actTime) < MOVE_TIME) {
        ;
      }
      cleanBtn(1); // clean internal butttons
      cleanBtn(2); // clean internal butttons
      cleanBtn(3); // it is moving down anyway, not taking an effect
      currState = State::OPEN_1F;
      actTime = std::time(0); // transit to 1F-OPEN
    }
  } // end of while
}
