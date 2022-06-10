#ifndef AUTO_WIRED_AUTO_WIRED_H
#define AUTO_WIRED_AUTO_WIRED_H

#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "autowired/injectable.h"

class AutoWired {
public:
    AutoWired() = default;
    ~AutoWired() {
        for (auto& [name, instance] : class_) {
            if (instance) {
                delete instance;
            }
        }
    }

    template <typename T>
    void Register(std::string_view custom_name = "") {
        staticAssert<T>();
        auto name = getClassTypeName<T>(custom_name);
        class_[name] = dynamic_cast<Injectable*>(new T());
    }

    template <typename T>
    void Register(T* t_ptr, std::string_view custom_name = "") {
        staticAssert<T>();
        auto name = getClassTypeName<T>(custom_name);
        class_[name] = dynamic_cast<Injectable*>(t_ptr);
    }

    template <typename T>
    void Wired(T** t_ptr, std::string_view custom_name = "") {
        auto name = getClassTypeName<T>(custom_name);

        // if target class not present, core dump will be.
        *t_ptr = dynamic_cast<T*>(class_.at(name));
    }

    template <typename T>
    T* GetInstance(std::string_view custom_name = "") {
        staticAssert<T>();
        auto name = getClassTypeName<T>(custom_name);

        return dynamic_cast<T*>(class_.at(name));
    }

    template <typename T>
    T* GetInstanceOrNullPtr(std::string_view custom_name = "") {
        staticAssert<T>();
        auto name = getClassTypeName<T>(custom_name);

        if (class_.count(name) == 0) {
            return nullptr;
        }

        return dynamic_cast<T*>(class_.at(name));
    }

    void Init() {
        for (auto& [name, instance] : class_) {
            instance->AutoWired();
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
    void staticAssert() {
        static_assert(std::is_base_of_v<Injectable, T>, "T must be derived from Injectable");
    }

private:
    std::map<std::string, Injectable*> class_;
};

AutoWired& DefaultAutoWired() {
    static AutoWired auto_wired;
    return auto_wired;
}

void InitDefaultAutoWired() {
    DefaultAutoWired().Init();
}

#endif  // AUTO_WIRED_AUTO_WIRED_H
