#ifndef ELEVATOR_HPP
#define ELEVATOR_HPP

#include <chrono>
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
  State currState;
  int currFloor;
  std::time_t actTime = 0;
  bool pressedBtns[5]; // 1-indexed
  std::mutex
      mtx; // `pressedBtns` has sync issues; use mutex lock to protect it!
  const int OPEN_TIME = 2;
  const int MOVE_TIME = 5;
  // thread-safe reading and writing
  bool isPressed(const int btn);
  void cleanBtn(int btn);

public:
  // constructor
  Elevator() : currState(State::IDLE_1F), currFloor(1), actTime(0) {
    std::fill(pressedBtns, pressedBtns + 5, false);
  };
  // destructor
  ~Elevator(){};
  void printState();
  void pressBtn(const int btn);
  void stateTransit();
};

#endif // ELEVATOR_HPP