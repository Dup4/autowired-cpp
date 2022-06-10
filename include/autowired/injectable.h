#ifndef AUTO_WIRED_INJECTABLE_H
#define AUTO_WIRED_INJECTABLE_H

class Injectable {
public:
    virtual ~Injectable() {}
    virtual void AutoWired() = 0;
};

#endif  // AUTO_WIRED_INJECTABLE_H
