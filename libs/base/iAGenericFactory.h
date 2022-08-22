/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <memory>

// (unique pointers -> trouble in static context?).
// error C2280: "std::unique_ptr<iAIFileIOFactory,std::default_delete<iAIFileIOFactory>> &std::unique_ptr<iAIFileIOFactory,std::default_delete<iAIFileIOFactory>>::operator =(const std::unique_ptr<iAIFileIOFactory,std::default_delete<iAIFileIOFactory>> &)" : Es wurde versucht, auf eine gelöschte Funktion zu verweisen

//! Generic factory class with shared pointers
template <typename BaseType>
class iAGenericFactory
{
public:
	virtual std::shared_ptr<BaseType> create() = 0;
	virtual ~iAGenericFactory() {}
};

//! Factory for a specific typed derived from BaseType.
template <typename DerivedType, typename BaseType>
class iASpecificFactory : public iAGenericFactory<BaseType>
{
public:
	std::shared_ptr<BaseType> create() override
	{
		return std::make_shared<DerivedType>();
	}
};
