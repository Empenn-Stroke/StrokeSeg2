#pragma once

#include <utils/animawrapper.h>

class MockAnimaWrapper : public AnimaWrapper {
  public:
    QStringList lastCommand;
    int callCount = 0;
    bool shouldFail = false;

    // Change 'void' to 'int' (or whatever type is in animawrapper.h)
    int run(const QStringList &command) override {
        if (shouldFail) {
            throw std::runtime_error("Simulated ANIMA error");
        }

        lastCommand = command;
        callCount++;

        return 0; // Return 0 to simulate a successful process execution
    }
};