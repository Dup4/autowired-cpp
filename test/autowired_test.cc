#include "gtest/gtest.h"

#include "autowired/autowired.h"

class A {
public:
    int GetValue() {
        return value_;
    }

private:
    int value_{1};
};

class B {
public:
    void AutoWired() {
        auto_wired::DefaultAutoWired().Wired(&a_);
    }

    int GetValue() {
        return a_->GetValue();
    }

private:
    A *a_{nullptr};
};

class C {
public:
    void AutoWired() {
        auto_wired::DefaultAutoWired().Wired(&b_);
    }

    int GetValue() {
        return b_->GetValue();
    }

private:
    B *b_{nullptr};
};

class D {
public:
    void Init() {
        value_ = 1;
    }

    int GetValue() {
        return value_;
    }

private:
    int value_;
};

class E {
public:
    void Init() {
        value_ = d_->GetValue() + 1;
    }

    int GetValue() {
        return value_;
    }

private:
    int value_;

public:
    void AutoWired() {
        auto_wired::DefaultAutoWired().Wired(&d_);
    }

private:
    D *d_{nullptr};
};

class F {
public:
    void Init() {
        value_ = e_->GetValue() + 1;
    }

    int GetValue() {
        return value_;
    }

private:
    int value_;

public:
    void AutoWired() {
        auto_wired::DefaultAutoWired().Wired(&e_);
    }

private:
    E *e_{nullptr};
};

class AutoWiredTest : public testing::Test {
protected:
    virtual void SetUp() override {
        auto_wired::DefaultAutoWired().Register<A>();
        auto_wired::DefaultAutoWired().Register<B>(auto_wired::AutoWired::RegisterOptions::WithNeedAutoWired());
        auto_wired::DefaultAutoWired().Register<D>(auto_wired::AutoWired::RegisterOptions::WithNeedInit());
        auto_wired::DefaultAutoWired().Register<E>(auto_wired::AutoWired::RegisterOptions::WithNeedAutoWired(),
                auto_wired::AutoWired::RegisterOptions::WithNeedInit());
        auto_wired::DefaultAutoWired().Register<F>(auto_wired::AutoWired::RegisterOptions::WithNeedAutoWired(),
                auto_wired::AutoWired::RegisterOptions::WithNeedInit());
        auto_wired::DefaultAutoWired().AutoWiredAll();
        auto_wired::DefaultAutoWired().InitAll();
    }
};

TEST_F(AutoWiredTest, auto_wired_test) {
    C c;
    c.AutoWired();

    EXPECT_EQ(1, c.GetValue());

    EXPECT_EQ(1, auto_wired::DefaultAutoWired().GetInstance<A>()->GetValue());
    EXPECT_EQ(1, auto_wired::DefaultAutoWired().GetInstance<B>()->GetValue());
    EXPECT_EQ(nullptr, auto_wired::DefaultAutoWired().GetInstanceOrNullPtr<C>());
}

TEST_F(AutoWiredTest, need_init_test) {
    EXPECT_EQ(1, auto_wired::DefaultAutoWired().GetInstance<D>()->GetValue());
    EXPECT_EQ(2, auto_wired::DefaultAutoWired().GetInstance<E>()->GetValue());
    EXPECT_EQ(3, auto_wired::DefaultAutoWired().GetInstance<F>()->GetValue());
}
