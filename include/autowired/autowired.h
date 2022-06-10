#ifndef AUTO_WIRED_AUTO_WIRED_H
#define AUTO_WIRED_AUTO_WIRED_H

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <iostream>
using namespace std;

#include "autowired/injectable.h"

class AutoWired {
    enum class AutoWiredType {
        None = 0,
        Injectable = 1 << 1,
    };

    struct node {
        uint32_t type_flag{0};
        void* instance{nullptr};
        std::function<void()> free{[] {}};
    };

public:
    AutoWired() = default;
    ~AutoWired() {
        for (auto& [name, i] : class_) {
            i.free();
        }
    }

    template <typename T>
    void Register(std::string_view custom_name = "") {
        Register<T>(nullptr, custom_name);
    }

    template <typename T>
    void Register(T* t_ptr, std::string_view custom_name = "") {
        auto name = getClassTypeName<T>(custom_name);
        if (class_.count(name)) {
            return;
        }

        if (!t_ptr) {
            t_ptr = new T();
        }

        auto type_flag = getTypeFlag<T>();
        auto free = [t_ptr] {
            delete t_ptr;
        };

        class_[name] = node{
                .type_flag = type_flag,
                .instance = static_cast<void*>(t_ptr),
                .free = free,
        };

        if constexpr (std::is_base_of_v<Injectable, T>) {
            injectable_class_[name] = dynamic_cast<Injectable*>(t_ptr);
        }
    }

    template <typename T>
    void Wired(T** t_ptr, std::string_view custom_name = "") {
        auto name = getClassTypeName<T>(custom_name);

        // if target class not present, core dump will be.
        *t_ptr = static_cast<T*>(class_.at(name).instance);
    }

    template <typename T>
    T* GetInstance(std::string_view custom_name = "") {
        auto name = getClassTypeName<T>(custom_name);

        // if target class not present, core dump will be.
        return static_cast<T*>(class_.at(name).instance);
    }

    template <typename T>
    T* GetInstanceOrNullPtr(std::string_view custom_name = "") {
        auto name = getClassTypeName<T>(custom_name);

        if (class_.count(name) == 0) {
            return nullptr;
        }

        return static_cast<T*>(class_.at(name).instance);
    }

    void Init() {
        for (auto& [name, i] : injectable_class_) {
            i->AutoWired();
        }
    }

private:
    template <typename T>
    std::string getClassTypeName(std::string_view custom_name) {
        std::string res = "";
        res += typeid(T*).name();
        res += custom_name;
        return res;
    }

    template <typename T>
    uint32_t getTypeFlag() {
        uint32_t type_flag = 0;

        if (std::is_base_of_v<Injectable, T>) {
            type_flag |= static_cast<int>(AutoWiredType::Injectable);
        }

        return type_flag;
    }

    bool hasType(uint32_t type_flag, AutoWiredType type) {
        return (type_flag & static_cast<int>(type)) != 0;
    }

private:
    std::map<std::string, node> class_;
    std::map<std::string, Injectable*> injectable_class_;
};

inline AutoWired& DefaultAutoWired() {
    static AutoWired auto_wired;
    return auto_wired;
}

inline void InitDefaultAutoWired() {
    DefaultAutoWired().Init();
}

#endif  // AUTO_WIRED_AUTO_WIRED_H
