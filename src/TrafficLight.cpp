#include <chrono>
#include <future>
#include <iostream>
#include <random>

#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _condition.wait(lock);
    auto message{std::move(_queue.front())};
    _queue.pop_front();

    return message;
}

template <typename T>
void MessageQueue<T>::send(T&& msg)
{
    const std::lock_guard<std::mutex> lock(_mutex);
    _queue.emplace_back(msg);
    _condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = red;
}

void TrafficLight::waitForGreen()
{
    bool condition{true};

    while (condition)
    {
        condition = (_queue.receive() == green) ? false : true;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread{&TrafficLight::cycleThroughPhases, this});
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(4000, 6000);

    while (true)
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(distr(eng)));
        _currentPhase = (_currentPhase == red) ? green : red;
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        auto msg = _currentPhase;
        auto is_sent = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, &_queue, std::move(msg));
        is_sent.wait();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}