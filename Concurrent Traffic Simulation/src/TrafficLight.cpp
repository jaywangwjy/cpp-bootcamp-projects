#include <iostream>
#include <random>
#include "TrafficLight.h"

template <typename T> T MessageQueue<T>::receive() {

  // perform queue modification under the lock
  std::unique_lock<std::mutex> uLock(_mutex);
  _cond.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable

  // remove last vector element from queue
  T msg = std::move(_queue.back());
  _queue.pop_back();

  return msg;
}

template <typename T> void MessageQueue<T>::send(T &&msg) {

  // perform vector modification under the lock
  std::lock_guard<std::mutex> uLock(_mutex);

  // add vector to queue & notify client after pushing new Vehicle into vector
  _queue.push_back(std::move(msg));
  _cond.notify_one();
}

TrafficLight::TrafficLight() {

  _currentPhase = TrafficLightPhase::kRed;
}

void TrafficLight::waitForGreen() {

  while (true) {

    TrafficLightPhase message = _queue.receive();
    if (message == TrafficLightPhase::kGreen)
      return;
  }
}

TrafficLightPhase TrafficLight::getCurrentPhase() {

  return _currentPhase;
}

void TrafficLight::simulate() {

  threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

void TrafficLight::cycleThroughPhases() {
    // Generate a random cycle time between 4 to 6 seconds
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(4.0, 6.0);
    double cycleTime = dist(mt) * 1000; // Convert to milliseconds

    // Initialize stopwatch
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        // Measure time elapsed since the last cycle
        auto currentTime = std::chrono::steady_clock::now();
        long t_delta = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();

        if (t_delta >= cycleTime) {
            // Toggle between red and green states
            if (_currentPhase == TrafficLightPhase::kGreen)
                _currentPhase = TrafficLightPhase::kRed;
            else if (_currentPhase == TrafficLightPhase::kRed)
                _currentPhase = TrafficLightPhase::kGreen;

            // Send the current phase to the message queue using move semantics
            _queue.send(std::move(_currentPhase));

            // Reset stopwatch and renew randomized cycle time
            startTime = std::chrono::steady_clock::now();
            cycleTime = dist(mt) * 1000;
        }

        // Wait for 1 millisecond before the next iteration
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
