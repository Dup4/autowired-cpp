#include <gtest/gtest.h>

#include "autowired/autowired.h"
#include "autowired/need_autowired.h"

class A {
public:
    int GetValue() {
        return value_;
    }

private:
    int value_{1};
};

class B : public NeedAutoWired {
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

class C : public NeedAutoWired {
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

class D : public NeedInit {
public:
    void Init() override {
        value_ = 1;
    }

    int GetValue() {
        return value_;
    }

private:
    int value_;
};

class E : public NeedAutoWired, public NeedInit {
public:
    void Init() override {
        value_ = d_->GetValue() + 1;
    }

    int GetValue() {
        return value_;
    }

private:
    int value_;

public:
    void AutoWired() override {
        DefaultAutoWired().Wired(&d_);
    }

private:
    D *d_{nullptr};
};

class F : public NeedAutoWired, public NeedInit {
public:
    void Init() override {
        value_ = e_->GetValue() + 1;
    }

    int GetValue() {
        return value_;
    }

private:
    int value_;

public:
    void AutoWired() override {
        DefaultAutoWired().Wired(&e_);
    }

private:
    E *e_{nullptr};
};

class AutoWiredTest : public testing::Test {
protected:
    virtual void SetUp() override {
        DefaultAutoWired().Register<A>();
        DefaultAutoWired().Register<B>();
        DefaultAutoWired().Register<D>();
        DefaultAutoWired().Register<E>();
        DefaultAutoWired().Register<F>();
        DefaultAutoWired().AutoWiredAll();
        DefaultAutoWired().InitAll();
    }
};

TEST_F(AutoWiredTest, auto_wired_test) {
    C c;
    c.AutoWired();

    EXPECT_EQ(1, c.GetValue());

    EXPECT_EQ(1, DefaultAutoWired().GetInstance<A>()->GetValue());
    EXPECT_EQ(1, DefaultAutoWired().GetInstance<B>()->GetValue());
    EXPECT_EQ(nullptr, DefaultAutoWired().GetInstanceOrNullPtr<C>());
}

TEST_F(AutoWiredTest, need_init_test) {
    EXPECT_EQ(1, DefaultAutoWired().GetInstance<D>()->GetValue());
    EXPECT_EQ(2, DefaultAutoWired().GetInstance<E>()->GetValue());
    EXPECT_EQ(3, DefaultAutoWired().GetInstance<F>()->GetValue());
}
