#pragma once

#include <functional>
#include <vector>

namespace ai_win_ui {

template <typename T>
class Observable {
public:
    using Listener = std::function<void(const T&)>;

    Observable() = default;
    explicit Observable(T value) : m_value(std::move(value)) {}

    const T& Get() const { return m_value; }

    void Set(T newValue) {
        if (m_value == newValue) {
            return;
        }
        m_value = std::move(newValue);
        Notify();
    }

    Observable& operator=(T newValue) {
        Set(std::move(newValue));
        return *nothrow_this();
    }

    void Subscribe(Listener listener) {
        if (listener) {
            m_listeners.push_back(std::move(listener));
        }
    }

    void Notify() const {
        for (const auto& listener : m_listeners) {
            if (listener) {
                listener(m_value);
            }
        }
    }

    operator const T&() const { return m_value; }

private:
    Observable* nothrow_this() { return this; }
    T m_value{};
    std::vector<Listener> m_listeners;
};

} // namespace ai_win_ui
