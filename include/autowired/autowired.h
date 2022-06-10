#ifndef AUTO_WIRED_AUTO_WIRED_H
#define AUTO_WIRED_AUTO_WIRED_H

#include <atomic>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "autowired/need_autowired.h"
#include "autowired/need_init.h"

class AutoWired {
    enum class AutoWiredType {
        NEED_AUTO_WIRED = 1 << 0,
        NEED_INIT = 1 << 1,
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

        if constexpr (std::is_base_of_v<NeedAutoWired, T>) {
            need_auto_wired_class_[name] = dynamic_cast<NeedAutoWired*>(t_ptr);
        }

        if constexpr (std::is_base_of_v<NeedInit, T>) {
            need_init_class_[name] = dynamic_cast<NeedInit*>(t_ptr);
        }
    }

    template <typename T>
    void Wired(T** t_ptr, std::string_view custom_name = "") {
        auto name = getClassTypeName<T>(custom_name);

        // if target class not present, core dump will be.
        *t_ptr = static_cast<T*>(class_.at(name).instance);

        if (current_run_auto_wired_name_ != EXTERNAL_CLASS_NAME) {
            graph_[name].push_back(std::string(current_run_auto_wired_name_));
            ++degree_[current_run_auto_wired_name_];
        }
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

    void AutoWiredAll() {
        {
            bool expected = false;
            if (!has_run_auto_wired_all_.compare_exchange_strong(expected, true)) {
                return;
            }
        }

        for (auto& [name, i] : need_auto_wired_class_) {
            current_run_auto_wired_name_ = name;
            i->AutoWired();
        }

        current_run_auto_wired_name_ = EXTERNAL_CLASS_NAME;
    }

    void InitAll() {
        {
            bool expected = false;
            if (!has_run_init_all_.compare_exchange_strong(expected, true)) {
                return;
            }
        }

        if (has_run_auto_wired_all_ == false) {
            throw std::runtime_error("AutoWiredAll() must be called before InitAll()");
        }

        auto [has_loop, topological_order] = getTopologicalOrder();
        if (has_loop) {
            throw std::runtime_error("Dependencies has loop, please initialize manually");
        }

        for (const auto& name : topological_order) {
            if (need_init_class_.count(name)) {
                need_init_class_.at(name)->Init();
            }
        }
    }

private:
    std::pair<bool, std::vector<std::string>> getTopologicalOrder() {
        std::vector<std::string> order;
        std::vector<std::string> unordered;

        for (auto& [name, i] : class_) {
            if (degree_.count(name) == 0 || degree_.at(name) == 0) {
                unordered.push_back(name);
            }
        }

        while (!unordered.empty()) {
            auto name = unordered.back();
            unordered.pop_back();
            order.push_back(name);

            if (!graph_.count(name)) {
                continue;
            }

            for (auto& i : graph_.at(name)) {
                --degree_.at(i);
                if (degree_.at(i) == 0) {
                    unordered.push_back(i);
                }
            }
        }

        if (order.size() != class_.size()) {
            return std::make_pair(true, std::vector<std::string>{});
        }

        return std::make_pair(false, order);
    }

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

        if (std::is_base_of_v<NeedAutoWired, T>) {
            type_flag |= static_cast<int>(AutoWiredType::NEED_AUTO_WIRED);
        }

        if (std::is_base_of_v<NeedInit, T>) {
            type_flag |= static_cast<int>(AutoWiredType::NEED_INIT);
        }

        return type_flag;
    }

    bool hasType(uint32_t type_flag, AutoWiredType type) {
        return (type_flag & static_cast<int>(type)) != 0;
    }

private:
    std::map<std::string, node> class_;

    std::map<std::string, NeedAutoWired*> need_auto_wired_class_;
    std::map<std::string, NeedInit*> need_init_class_;

    std::atomic<bool> has_run_auto_wired_all_{false};
    std::atomic<bool> has_run_init_all_{false};

    std::map<std::string, std::vector<std::string>> graph_;
    std::map<std::string, uint32_t> degree_;

    std::string current_run_auto_wired_name_{EXTERNAL_CLASS_NAME};

    inline static constexpr std::string_view EXTERNAL_CLASS_NAME = "EXTERNAL_CLASS_NAME";
};

inline AutoWired& DefaultAutoWired() {
    static AutoWired auto_wired;
    return auto_wired;
}

#endif  // AUTO_WIRED_AUTO_WIRED_H
