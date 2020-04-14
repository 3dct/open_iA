/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iANameMapper.h"
#include "open_iA_Core_export.h"

#include <QStringList>

//! Maps numbers to names via a given list.
class open_iA_Core_API iAListNameMapper : public iANameMapper
{
public:
	iAListNameMapper(QStringList const & names) :
		m_names(names)
	{}
	virtual QString name(int idx) const
	{
		return m_names[idx];
	}

	virtual int GetIdx(QString const & name, bool & ok) const
	{
		for (int i = 0; i < m_names.size(); ++i)
		{
			if (m_names[i] == name)
			{
				ok = true;
				return i;
			}
		}
		ok = false;
		return -1;
	}

	virtual int size() const
	{
		return m_names.size();
	}
private:
	QStringList m_names;
};
