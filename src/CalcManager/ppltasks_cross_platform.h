// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#if defined(_WIN32) && defined(_MSC_VER)

#include <ppltasks.h>

#else

// Minimal stand-in for the PPL's concurrency::task, used by UnitConverter's
// currency interfaces on platforms without the PPL. Tasks are already-completed
// values; continuations run immediately. Clients that need real asynchrony
// (such as network loads) run them outside this interface.

#include <type_traits>
#include <utility>

namespace concurrency
{
    template <typename T>
    class task
    {
    public:
        task() = default;

        explicit task(T value)
            : m_value(std::move(value))
        {
        }

        T get() const
        {
            return m_value;
        }

        template <typename Func>
        auto then(Func&& func) const -> task<std::invoke_result_t<Func, T>>
        {
            return task<std::invoke_result_t<Func, T>>(std::forward<Func>(func)(m_value));
        }

    private:
        T m_value{};
    };

    template <typename T>
    task<T> task_from_result(T value)
    {
        return task<T>(std::move(value));
    }
}

#endif
