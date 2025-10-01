// Adapted from Qt's QScopeGuard definition for use in our non-Qt projects.

#pragma once

#include <algorithm>

namespace wsl {

template <typename F> class ScopeGuard;
template <typename F> ScopeGuard<F> wsScopeGuard(F f);

template <typename F>
class ScopeGuard
{
public:
    ScopeGuard(ScopeGuard &&other) noexcept
        : m_func(std::move(other.m_func))
        , m_invoke(other.m_invoke)
    {
        other.dismiss();
    }

    ~ScopeGuard()
    {
        if (m_invoke)
            m_func();
    }

    void dismiss() noexcept
    {
        m_invoke = false;
    }

private:
    explicit ScopeGuard(F f) noexcept
        : m_func(std::move(f))
    {
    }

    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard &operator=(const ScopeGuard &) = delete;

    F m_func;
    bool m_invoke = true;
    friend ScopeGuard wsScopeGuard<F>(F);
};


template <typename F>
ScopeGuard<F> wsScopeGuard(F f)
{
    return ScopeGuard<F>(std::move(f));
}

} // end namespace wsl
