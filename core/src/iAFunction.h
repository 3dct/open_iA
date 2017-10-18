/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include <cassert>
#include <map>

template <typename ArgType, typename ValType>
class iAFunction : public std::map<ArgType, ValType> {};


/**
 * Class representing a generic (single-parameter) function as can be passed into
 * the functional boxplot calculation
 */
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

/*
template <typename ArgType, typename ValType>
ValType & iAFunction<ArgType, ValType>::operator[](ArgType idx)
{
	assert (m_data);
	assert (idx >= 0 && idx < m_size);
	return m_data[idx];
}
* /

template <typename ArgType, typename ValType>
ArgType iAFunction<ArgType, ValType>::size() const
{
	return m_data.size();
}*/