// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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


iAAttributes combineAttributesWithValues(iAAttributes const& attributes, QVariantMap values)
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

QVariantMap joinValues(QVariantMap const& baseValues, QVariantMap const& newValues)
{
	QVariantMap result(baseValues);
	result.insert(newValues);
	return result;
}

QVariantMap extractValues(iAAttributes const& attributes)
{
	QVariantMap result;
	for (auto param : attributes)
	{
		result.insert(param->name(), param->defaultValue());
	}
	return result;
}

void addAttr(iAAttributes& attributes, QString const& name, iAValueType valueType, QVariant defaultValue, double min, double max)
{
	attributes.push_back(iAAttributeDescriptor::createParam(name, valueType, defaultValue, min, max));
}
