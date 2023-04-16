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

iAAttributes cloneAttributes(iAAttributes const& attributes)
{
	iAAttributes combined;
	combined.reserve(attributes.size());
	for (auto const& attr : attributes)
	{
		combined.push_back(attr->clone());
	}
	return combined;
}

iAAttributes combineAttributesWithValues(iAAttributes const& attributes, QVariantMap const & values)
{
	auto clone = cloneAttributes(attributes);
	setDefaultValues(clone, values);
	return clone;
}

void setDefaultValues(iAAttributes& attributes, QVariantMap const& values)
{
	for (auto & attr : attributes)
	{
		if (!values.contains(attr->name()))
		{
			continue;
		}
		if (attr->valueType() == iAValueType::Categorical)
		{
			QStringList comboValues = attr->defaultValue().toStringList();
			QString storedValue = values[attr->name()].toString();
			selectOption(comboValues, storedValue);
			attr->setDefaultValue(comboValues);
		}
		else
		{
			attr->setDefaultValue(values[attr->name()]);
		}
	}
}

void setApplyingValues(QVariantMap& out, iAAttributes const& attributes, QVariantMap const& in)
{
	for (auto attr : attributes)
	{
		if (in.contains(attr->name()))
		{
			out[attr->name()] = in[attr->name()];
		}
	}
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
		result.insert(param->name(), param->valueType() == iAValueType::Categorical
			? selectedOption(param->defaultValue().toStringList())
			: param->defaultValue());
	}
	return result;
}

void addAttr(iAAttributes& attributes, QString const& name, iAValueType valueType, QVariant defaultValue, double min, double max)
{
	attributes.push_back(iAAttributeDescriptor::createParam(name, valueType, defaultValue, min, max));
}
