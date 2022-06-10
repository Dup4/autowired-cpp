#include <gtest/gtest.h>

#include "autowired/autowired.h"
#include "autowired/injectable.h"

class AutoWiredTest : public testing::Test {
protected:
    virtual void SetUp() override {}
};

class A : public Injectable {
public:
    void AutoWired() override {}

    int GetValue() {
        return value_;
    }

private:
    int value_{1};
};

class B : public Injectable {
public:
    void AutoWired() override {
        DefaultAutoWired().Wired(&a_);
    }

    int GetValue() {
        return a_->GetValue();
    }

private:
    A *a_{nullptr};
};

class C : public Injectable {
public:
    void AutoWired() override {
        DefaultAutoWired().Wired(&b_);
    }

    int GetValue() {
        return b_->GetValue();
    }

private:
    B *b_{nullptr};
};

TEST_F(AutoWiredTest, auto_wired_test) {
    DefaultAutoWired().Register<A>();
    DefaultAutoWired().Register<B>();
    InitDefaultAutoWired();

    C c;
    c.AutoWired();

    EXPECT_EQ(1, c.GetValue());
}
