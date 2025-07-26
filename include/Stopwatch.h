#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <Arduino.h>


class Stopwatch {
public:
    static Stopwatch& getInstance();
    void start();
    void stop();
    void reset();
    bool isRunning() const;
    bool isStopped() const;
    unsigned long elapsed() const;
    void printElapsed(Stream& stream) const;
    void printResult(Stream& stream) const;
private:
    Stopwatch();
    Stopwatch(const Stopwatch&) = delete;
    Stopwatch& operator=(const Stopwatch&) = delete;
    static Stopwatch* _instance;
    unsigned long _startTime;
    unsigned long _endTime;
    bool _running;
    bool _stopped;
};

#endif // STOPWATCH_H
