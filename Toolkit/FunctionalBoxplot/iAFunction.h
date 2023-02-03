// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cassert>
#include <map>

//! Class representing a generic (single-parameter) function,
//! which can be passed into the functional boxplot calculation.
template <typename ArgType, typename ValType>
class iAFunction : public std::map<ArgType, ValType> {};

/*
template <typename ArgType, typename ValType>
class iAFunction
{
public:
	iAFunction();
	typedef ArgType ArgumentType;
	typedef ValType ValueType;

	ValType get(ArgType a) const;
	void set(ArgType a, ValType);
	ValType operator[](ArgType a) const;
	//ValType & operator[](ArgType a);
	ArgType size() const;

private:
	std::map<ArgType, ValType> m_data;
};


template <typename ArgType, typename ValType>
iAFunction<ArgType, ValType>::iAFunction()
{}

template <typename ArgType, typename ValType>
ValType iAFunction<ArgType, ValType>::get(ArgType idx) const
{
	assert(m_data.find(idx) != m_data.end());
	return m_data.at(idx);
}

template <typename ArgType, typename ValType>
void iAFunction<ArgType, ValType>::set(ArgType idx, ValType v)
{
	m_data[idx] = v;
}

template <typename ArgType, typename ValType>
ValType iAFunction<ArgType, ValType>::operator[](ArgType idx) const
{
	assert(m_data.find(idx) != m_data.end());;
	return m_data[idx];
}


//template <typename ArgType, typename ValType>
//ValType & iAFunction<ArgType, ValType>::operator[](ArgType idx)
//{
//	assert (m_data);
//	assert (idx >= 0 && idx < m_size);
//	return m_data[idx];
//}


template <typename ArgType, typename ValType>
ArgType iAFunction<ArgType, ValType>::size() const
{
	return m_data.size();
}*/
