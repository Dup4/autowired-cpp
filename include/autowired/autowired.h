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

#include "./types_check/has_auto_wired.h"
#include "./types_check/has_de_init.h"
#include "./types_check/has_init.h"

namespace auto_wired {

class AutoWired {
public:
    struct RegisterOptions {
        std::string custom_name{""};
        bool need_init{false};
        bool need_de_init{false};
        bool need_auto_wired{false};

    public:
        static auto WithCustomName(const std::string& custom_name) {
            return [custom_name](RegisterOptions& options) {
                options.custom_name = custom_name;
            };
        }

        static auto WithNeedInit(bool need_init = true) {
            return [need_init](RegisterOptions& options) {
                options.need_init = need_init;
            };
        }

        static auto WithNeedDeInit(bool need_de_init = true) {
            return [need_de_init](RegisterOptions& options) {
                options.need_de_init = need_de_init;
            };
        }

        static auto WithNeedAutoWired(bool need_auto_wired = true) {
            return [need_auto_wired](RegisterOptions& options) {
                options.need_auto_wired = need_auto_wired;
            };
        }

        template <typename... Opt>
        static auto CreateRegisterOptions(Opt&&... opts) {
            RegisterOptions options;
            (std::forward<Opt>(opts)(options), ...);

            return options;
        }
    };

private:
    struct classNode {
        void* instance{nullptr};
        RegisterOptions options;
        std::function<void()> func_Free{[] {}};
        std::function<void()> func_Init{[] {}};
        std::function<void()> func_DeInit{[] {}};
        std::function<void()> func_AutoWired{[] {}};
    };

    inline const static RegisterOptions default_register_options{"", false, false, false};
    using RegisterOptionsSetFunction = std::function<void(RegisterOptions&)>;

public:
    AutoWired() = default;
    ~AutoWired() {
        for (auto& [name, i] : class_) {
            i.func_Free();
        }
    }

    template <typename T, typename... Opt>
    void Register(Opt&&... opts) {
        registerImpl<T>(nullptr, RegisterOptions::CreateRegisterOptions(std::forward<Opt>(opts)...));
    }

    template <typename T, typename... Opt>
    void Register(T* t_ptr, Opt&&... opts) {
        registerImpl(t_ptr, RegisterOptions::CreateRegisterOptions(std::forward<Opt>(opts)...));
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

        for (auto& [name, i] : class_) {
            if (i.options.need_auto_wired) {
                current_run_auto_wired_name_ = name;
                i.func_AutoWired();
            }
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

        for (auto& name : topological_order) {
            if (class_.at(name).options.need_init) {
                class_.at(name).func_Init();
            }
        }
    }

    void DeInitAll() {
        {
            bool expected = false;
            if (!has_run_de_init_all_.compare_exchange_strong(expected, true)) {
                return;
            }
        }

        if (has_run_auto_wired_all_ == false) {
            throw std::runtime_error("AutoWiredAll() must be called before DeInitAll()");
        }

        auto [has_loop, topological_order] = getTopologicalOrder();
        if (has_loop) {
            throw std::runtime_error("Dependencies has loop, please deinit manually");
        }

        for (auto& name : topological_order) {
            if (class_.at(name).options.need_de_init) {
                class_.at(name).func_DeInit();
            }
        }
    }

private:
    template <typename T>
    void registerImpl(T* t_ptr, RegisterOptions options) {
        auto name = getClassTypeName<T>(options.custom_name);
        if (class_.count(name)) {
            return;
        }

        if (!t_ptr) {
            t_ptr = new T();
        }

        classNode node;
        node.instance = static_cast<void*>(t_ptr);
        node.options = std::move(options);
        node.func_Free = [t_ptr] {
            delete t_ptr;
        };

        if constexpr (internal::has_init_v<T>) {
            node.func_Init = [t_ptr] {
                t_ptr->Init();
            };
        }

        if constexpr (internal::has_de_init_v<T>) {
            node.func_DeInit = [t_ptr] {
                t_ptr->DeInit();
            };
        }

        if constexpr (internal::has_auto_wired_v<T>) {
            node.func_AutoWired = [t_ptr] {
                t_ptr->AutoWired();
            };
        }

        class_.emplace(name, std::move(node));
    }

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

private:
    std::map<std::string, classNode> class_;

    std::atomic<bool> has_run_auto_wired_all_{false};
    std::atomic<bool> has_run_init_all_{false};
    std::atomic<bool> has_run_de_init_all_{false};

    std::map<std::string, std::vector<std::string>> graph_;
    std::map<std::string, uint32_t> degree_;

    std::string current_run_auto_wired_name_{EXTERNAL_CLASS_NAME};

    inline static constexpr std::string_view EXTERNAL_CLASS_NAME = "EXTERNAL_CLASS_NAME";
};

inline AutoWired& DefaultAutoWired() {
    static AutoWired auto_wired;
    return auto_wired;
}

}  // namespace auto_wired

#endif  // AUTO_WIRED_AUTO_WIRED_H
