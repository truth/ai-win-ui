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

    size_t Subscribe(Listener listener) {
        if (listener) {
            size_t id = ++m_nextId;
            m_listeners.push_back({id, std::move(listener)});
            return id;
        }
        return 0;
    }

    void Unsubscribe(size_t id) {
        if (id == 0) return;
        for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it) {
            if (it->first == id) {
                m_listeners.erase(it);
                return;
            }
        }
    }

    void Notify() const {
        // Iterate by index or copy vector to allow Unsubscribe during Notify?
        // Simple case: just iterate. If they unsubscribe during Notify it might break the iterator.
        // Copying is safer.
        auto copy = m_listeners;
        for (const auto& pair : copy) {
            if (pair.second) {
                pair.second(m_value);
            }
        }
    }

    operator const T&() const { return m_value; }

private:
    Observable* nothrow_this() { return this; }
    T m_value{};
    size_t m_nextId = 0;
    std::vector<std::pair<size_t, Listener>> m_listeners;
};

} // namespace ai_win_ui
