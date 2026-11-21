// Argon: a lookup's result: a reference to what it found, or nothing (std::optional of a reference, which the
// standard library does not have). Reading an empty one throws std::bad_optional_access, as std::optional::value does.
#pragma once
#include <optional>
#include <type_traits>

template <class T> class Found
{
  public:
    Found() = default;
    Found(std::nullopt_t)
    {
    }
    Found(T &value) : value_(&value)
    {
    }
    // a Found<U> as a Found<T> where a U & is a T & (a non-const one as a const one)
    template <class U>
        requires std::is_convertible_v<U *, T *>
    Found(const Found<U> &other) : value_(other ? &*other : nullptr)
    {
    }

    explicit operator bool() const
    {
        return value_ != nullptr;
    }
    T &operator*() const
    {
        return checked();
    }
    T *operator->() const
    {
        return &checked();
    }

  private:
    T &checked() const
    {
        if (!value_)
        {
            throw std::bad_optional_access();
        }
        return *value_;
    }

    T *value_ = nullptr;
};
