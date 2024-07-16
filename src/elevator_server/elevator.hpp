#ifndef ELEVATOR_HPP // header guards
#define ELEVATOR_HPP

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <iostream>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unordered_map>

enum class State {
  IDLE_1F,
  OPEN_1F,
  LIFT,
  IDLE_2F,
  OPEN_2F,
  GO_DOWN,
};

class Elevator {
private:
  static const int BTN_NUM = 4;
  const int OPEN_TIME = 2;
  const int MOVE_TIME = 5;
  State currState;
  std::time_t actTime = 0; // action time
  std::mutex
      mtx; // `pressedBtns` has sync issues; use mutex lock to protect it!
  std::mutex
      stateMtx; // `currState` has sync issues; use mutex lock to protect it!
  std::condition_variable
      stateCv; // Condition variable for state change notifications
  bool pressedBtns[BTN_NUM + 1];
  // thread-safe reading and writing methods for `pressedBtns`
  bool isPressed(const int btn);
  void cleanBtn(const int btn);
  // thread-safe reading and writing methods for `currState`
  State getState();
  void setState(const State &state);

public:
  // constructor
  Elevator() : currState(State::IDLE_1F), actTime(0) {
    std::fill(pressedBtns, pressedBtns + (BTN_NUM + 1), false);
  };
  // destructor
  ~Elevator(){};
  void printState();
  void pressBtn(const int btn);
  void stateTransit();
};

#endif // ELEVATOR_HPP