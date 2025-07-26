
#include "Stopwatch.h"
#include <Arduino.h>
#include <stddef.h>

Stopwatch* Stopwatch::_instance = nullptr;

Stopwatch& Stopwatch::getInstance() {
    if (!_instance) {
        _instance = new Stopwatch();
    }
    return *_instance;
}

Stopwatch::Stopwatch() : _startTime(0), _endTime(0), _running(false), _stopped(false) {}

void Stopwatch::start() {
    _startTime = millis();
    _running = true;
    _stopped = false;
}

void Stopwatch::stop() {
    if (_running) {
        _endTime = millis();
        _running = false;
        _stopped = true;
    }
}

void Stopwatch::reset() {
    _startTime = 0;
    _endTime = 0;
    _running = false;
    _stopped = false;
}

bool Stopwatch::isRunning() const {
    return _running;
}

bool Stopwatch::isStopped() const {
    return _stopped;
}

unsigned long Stopwatch::elapsed() const {
    if (_running) {
        return millis() - _startTime;
    } else if (_stopped) {
        return _endTime - _startTime;
    }
    return 0;
}

void Stopwatch::printElapsed(Stream& stream) const {
    unsigned long e = elapsed();
    unsigned int minutes = e / 60000;
    unsigned int seconds = (e % 60000) / 1000;
    unsigned int milliseconds = e % 1000;
    char buffer[20];
    sprintf(buffer, "%02u:%02u:%03u", minutes, seconds, milliseconds);
    stream.print("Elapsed: ");
    stream.println(buffer);
}

void Stopwatch::printResult(Stream& stream) const {
    unsigned long e = elapsed();
    unsigned int minutes = e / 60000;
    unsigned int seconds = (e % 60000) / 1000;
    unsigned int milliseconds = e % 1000;
    char buffer[20];
    sprintf(buffer, "%02u:%02u:%03u", minutes, seconds, milliseconds);
    stream.print("Stopwatch stopped! Time: ");
    stream.println(buffer);
}
