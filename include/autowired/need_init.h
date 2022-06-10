#ifndef AUTO_WIRED_NEED_INIT_H
#define AUTO_WIRED_NEED_INIT_H

class NeedInit {
public:
    virtual ~NeedInit() {}
    virtual void Init() = 0;
};

#endif  // AUTO_WIRED_NEED_INIT_H
