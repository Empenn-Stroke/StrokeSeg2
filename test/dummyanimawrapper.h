#pragma once

#include <utils/animawrapper.h>

// Dummy used only for tests
class DummyAnimaWrapper : public AnimaWrapper {
  public:
    explicit DummyAnimaWrapper(QObject *parent = nullptr) : AnimaWrapper(parent) {}

    int run(const QStringList &args) override {
        Q_UNUSED(args);
        return 0; // simulate success
    }
};