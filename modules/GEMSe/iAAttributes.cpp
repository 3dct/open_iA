/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "iAAttributes.h"

#include "iAAttributeDescriptor.h"

#include <QTextStream>


QSharedPointer<iAAttributes> iAAttributes::Create(QTextStream & in)
{
	QSharedPointer<iAAttributes> result(new iAAttributes);
	while (!in.atEnd())
	{
		QString line = in.readLine();
		QSharedPointer<iAAttributeDescriptor> descriptor =
			iAAttributeDescriptor::Create(line);
		if (descriptor)
		{
			result->m_attributes.push_back(descriptor);
		}
		else
		{
			return QSharedPointer<iAAttributes>(new iAAttributes);
		}
	}
	return result;
}

int iAAttributes::size() const
{
	return m_attributes.size();
}


QSharedPointer<iAAttributeDescriptor> iAAttributes::at(int idx)
{
	return m_attributes[idx];
}

QSharedPointer<iAAttributeDescriptor const> iAAttributes::at(int idx) const
{
	return m_attributes[idx];
}


void iAAttributes::Add(QSharedPointer<iAAttributeDescriptor> range)
{
	m_attributes.push_back(range);
}

void iAAttributes::Store(QTextStream & out)
{
	for (int i = 0; i < m_attributes.size(); ++i)
	{
		out << m_attributes[i]->ToString();
	}
}

int iAAttributes::Find(QString const & name)
{
	for (int i = 0; i < m_attributes.size(); ++i)
	{
		if (m_attributes[i]->GetName() == name)
		{
			return i;
		}
	}
	return -1;
}
