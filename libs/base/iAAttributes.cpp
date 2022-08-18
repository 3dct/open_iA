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
#include "iAAttributes.h"

#include <QTextStream>

QSharedPointer<iAAttributes> createAttributes(QTextStream & in)
{
	auto result = QSharedPointer<iAAttributes>::create();
	while (!in.atEnd())
	{
		QString line = in.readLine();
		auto descriptor = iAAttributeDescriptor::create(line);
		if (descriptor)
		{
			result->push_back(descriptor);
		}
		else
		{
			return QSharedPointer<iAAttributes>::create();
		}
	}
	return result;
}

void storeAttributes(QTextStream & out, iAAttributes const & attributes)
{
	for (int i = 0; i < attributes.size(); ++i)
	{
		out << attributes[i]->toString();
	}
}

int findAttribute(iAAttributes const& attributes, QString const & name)
{
	for (int i = 0; i < attributes.size(); ++i)
	{
		if (attributes[i]->name() == name)
		{
			return i;
		}
	}
	return -1;
}

int countAttributes(iAAttributes const& attributes, iAAttributeDescriptor::iAAttributeType type)
{
	int count = 0;
	for (int i = 0; i < attributes.size(); ++i)
	{
		if (type == iAAttributeDescriptor::None	|| attributes[i]->attribType() == type)
		{
			count++;
		}
	}
	return count;
}


iAAttributes combineAttributesWithValues(iAAttributes const& attributes, QMap<QString, QVariant> values)
{
	iAAttributes combined;
	combined.reserve(attributes.size());
	for (auto param : attributes)
	{
		QSharedPointer<iAAttributeDescriptor> p(param->clone());
		if (p->valueType() == iAValueType::Categorical)
		{
			QStringList comboValues = p->defaultValue().toStringList();
			QString storedValue = values[p->name()].toString();
			selectOption(comboValues, storedValue);
			p->setDefaultValue(comboValues);
		}
		else
		{
			p->setDefaultValue(values[p->name()]);
		}
		combined.push_back(p);
	}
	return combined;
}
