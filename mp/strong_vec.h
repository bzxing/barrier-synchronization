#ifndef INC_STRONG_VEC_H
#define INC_STRONG_VEC_H

#include <utility>

template <class RawVector, class StrongInt>
class StrongVec
{
public:
	using T = typename RawVector::value_type;
	using value_type = T;
	using size_type = StrongInt;
	using iterator = typename RawVector::iterator;
	using const_iterator = typename RawVector::const_iterator;
	using reference = typename RawVector::reference;
	using const_reference = typename RawVector::const_reference;

	StrongVec() = default;

	StrongVec(size_type n)
	:
		m_raw_vec(n.valid_base())
	{

	}

	StrongVec(size_type n, const T & val)
	:
		m_raw_vec(n.valid_base(), val)
	{

	}

	template <class T>
	void push_back(T && elem)
	{
		m_raw_vec.push_back(std::forward<T>(elem));
	}

	template <class... Args>
	void emplace_back(Args &&... args)
	{
		m_raw_vec.emplace_back(std::forward<Args>(args)...);
	}

	void resize(size_type n)
	{
		m_raw_vec.resize(n.valid_base());
	}

	reference back()
	{
		return m_raw_vec.back();
	}

	const_reference back() const
	{
		return m_raw_vec.back();
	}

	void clear() { m_raw_vec.clear(); }

	void reserve(size_type n) { m_raw_vec.reserve(n.valid_base()); }
	void shrink_to_fit() { m_raw_vec.shrink_to_fit(); }

	bool empty() const { return m_raw_vec.empty(); }
	size_type size() const { return StrongInt(m_raw_vec.size()); }
	size_type capacity() const { return StrongInt(m_raw_vec.capacity()); }


	T & operator[](size_type n) { return m_raw_vec[n.valid_base()]; }
	const T & operator[](size_type n) const { return m_raw_vec[n.valid_base()]; }
	T & at(size_type n) { return m_raw_vec.at(n.valid_base()); }
	const T & at(size_type n) const { return m_raw_vec.at(n.valid_base()); }

	iterator begin() { return m_raw_vec.begin(); }
	iterator end() { return m_raw_vec.end(); }

	const_iterator begin() const { return m_raw_vec.begin(); }
	const_iterator end() const { return m_raw_vec.end(); }

	auto make_range()
	{
		return boost::make_iterator_range(begin(), end());
	}
	auto make_range() const
	{
		return boost::make_iterator_range(begin(), end());
	}

	friend bool operator==(const StrongVec & lhs, const StrongVec & rhs)
	{
		return lhs.m_raw_vec == rhs.m_raw_vec;
	}

	friend bool operator!=(const StrongVec & lhs, const StrongVec & rhs)
	{
		return lhs.m_raw_vec != rhs.m_raw_vec;
	}

private:
	RawVector m_raw_vec;
};

#endif
