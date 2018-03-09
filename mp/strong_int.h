#ifndef INC_STRONG_INT_H
#define INC_STRONG_INT_H

#include <type_traits>
#include <algorithm>
#include <limits>
#include <string>
#include <iostream>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/assert.hpp>

template <class T, class Tag>
class StrongInt
{

    static_assert(std::is_integral<T>::value, "Must be integral type");
    static_assert(std::is_unsigned<T>::value != std::is_signed<T>::value,
        "Must be either signed or unsigned but not both");

    static constexpr T kInvalidId =
        std::is_unsigned<T>::value ?
            std::numeric_limits<T>::max() :
            std::numeric_limits<T>::min();

    T & get_valid_raw()
    {
        BOOST_ASSERT(is_valid());
        return m_raw_int;
    }

    T & get_raw()
    {
        return m_raw_int;
    }

    const T & get_valid_raw() const
    {
        BOOST_ASSERT(is_valid());
        return m_raw_int;
    }

    const T & get_raw() const
    {
        return m_raw_int;
    }

public:
    using Base = T;

    constexpr StrongInt() = default;

    template <class U>
    explicit constexpr StrongInt(U u) :
        m_raw_int( boost::numeric_cast<T>(u) )
    {
        BOOST_ASSERT(is_valid());
    }

    StrongInt(const StrongInt &) = default;
    StrongInt & operator=(const StrongInt &) = default;


    constexpr bool is_valid() const
    {
        return m_raw_int != kInvalidId;
    }

    T valid_base() const
    {
        BOOST_ASSERT(is_valid());
        return m_raw_int;
    }

    friend bool operator<(StrongInt a, StrongInt b) { return a.get_raw() < b.get_raw(); }
    friend bool operator<=(StrongInt a, StrongInt b) { return a.get_raw() <= b.get_raw(); }
    friend bool operator>(StrongInt a, StrongInt b) { return a.get_raw() > b.get_raw(); }
    friend bool operator>=(StrongInt a, StrongInt b) { return a.get_raw() >= b.get_raw(); }
    friend bool operator==(StrongInt a, StrongInt b) { return a.get_raw() == b.get_raw(); }
    friend bool operator!=(StrongInt a, StrongInt b) { return a.get_raw() != b.get_raw(); }

    friend StrongInt operator+(StrongInt a, StrongInt b)
    {
        if (!a.is_valid() || !b.is_valid())
        {
            return StrongInt();
        }

        return StrongInt(a.get_valid_raw() + b.get_valid_raw());
    }

    friend StrongInt operator-(StrongInt a, StrongInt b)
    {
        if (!a.is_valid() || !b.is_valid())
        {
            return StrongInt();
        }
        return StrongInt(a.get_valid_raw() - b.get_valid_raw());
    }

    friend StrongInt operator*(StrongInt a, StrongInt b)
    {
        if (!a.is_valid() || !b.is_valid())
        {
            return StrongInt();
        }
        return StrongInt(a.get_valid_raw() * b.get_valid_raw());
    }

    friend StrongInt operator/(StrongInt a, StrongInt b)
    {
        if (!a.is_valid() || !b.is_valid())
        {
            return StrongInt();
        }
        return StrongInt(a.get_valid_raw() / b.get_valid_raw());
    }

    friend StrongInt operator%(StrongInt a, StrongInt b)
    {
        if (!a.is_valid() || !b.is_valid())
        {
            return StrongInt();
        }
        return StrongInt(a.get_valid_raw() % b.get_valid_raw());
    }

    friend StrongInt operator+(StrongInt rhs)
    {
        return rhs;
    }

    friend StrongInt operator-(StrongInt rhs)
    {
        static_assert(std::is_signed<T>::value, "Are you sure?");

        if (!rhs.is_valid())
        {
            return StrongInt();
        }

        return StrongInt(-get_valid_raw());
    };


    StrongInt & operator+=(StrongInt rhs)
    {
        if (!this->is_valid() || !rhs.is_valid())
        {
            return *this;
        }
        get_valid_raw() += rhs.get_valid_raw();
        return *this;
    }

    StrongInt & operator-=(StrongInt rhs)
    {
        if (!this->is_valid() || !rhs.is_valid())
        {
            return *this;
        }
        get_valid_raw() -= rhs.get_valid_raw();
        return *this;
    }

    StrongInt & operator*=(StrongInt rhs)
    {
        if (!this->is_valid() || !rhs.is_valid())
        {
            return *this;
        }
        get_valid_raw() *= rhs.get_valid_raw();
        return *this;
    }

    StrongInt & operator/=(StrongInt rhs)
    {
        if (!this->is_valid() || !rhs.is_valid())
        {
            return *this;
        }
        get_valid_raw() /= rhs.get_valid_raw();
        return *this;
    }

    StrongInt & operator++()
    {
        if (!this->is_valid())
        {
            return *this;
        }
        ++get_valid_raw();
        return *this;
    }

    StrongInt & operator--()
    {
        if (!this->is_valid())
        {
            return *this;
        }
        --get_valid_raw();
        return *this;
    }

    StrongInt operator++(int)
    {
        if (!this->is_valid())
        {
            return *this;
        }
        return StrongInt(get_valid_raw()++);
    }

    StrongInt operator--(int)
    {
        if (!this->is_valid())
        {
            return *this;
        }
        return StrongInt(get_valid_raw()--);
    }

    friend std::ostream & operator<<(std::ostream & os, const StrongInt & self)
    {
        os << self.to_string();
        return os;
    }

    static StrongInt max(const StrongInt & a, const StrongInt & b)
    {
        if (!a.is_valid())
        {
            return b;
        }

        if (!b.is_valid())
        {
            return a;
        }

        return StrongInt(std::max(a.get_valid_raw(), b.get_valid_raw()));
    }

    static StrongInt min(const StrongInt & a, const StrongInt & b)
    {
        if (!a.is_valid())
        {
            return b;
        }

        if (!b.is_valid())
        {
            return a;
        }

        return StrongInt(std::min(a.get_valid_raw(), b.get_valid_raw()));
    }

    std::string to_string() const
    {
        if (!this->is_valid())
        {
            return std::string("--");
        }
        return std::to_string(this->m_raw_int);
    }

private:
    T m_raw_int = kInvalidId;
};

namespace std
{
    template <class T, class U>
    std::string to_string(const StrongInt<T, U> & i)
    {
        return i.to_string();
    }

    template <class T, class U>
    StrongInt<T, U> max(const StrongInt<T, U> & a, const StrongInt<T, U> & b)
    {
        return StrongInt<T, U>::max(a, b);
    }

    template <class T, class U>
    StrongInt<T, U> min(const StrongInt<T, U> & a, const StrongInt<T, U> & b)
    {
        return StrongInt<T, U>::min(a, b);
    }
}

#endif
