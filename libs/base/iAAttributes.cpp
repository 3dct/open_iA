// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAttributes.h"

#include "iAValueTypeVectorHelpers.h"

#include <QTextStream>

std::shared_ptr<iAAttributes> createAttributes(QTextStream & in)
{
	auto result = std::make_shared<iAAttributes>();
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
			return std::make_shared<iAAttributes>();
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

void removeAttribute(iAAttributes& attributes, QString const& name)
{
	auto idx = findAttribute(attributes, name);
	if (idx != -1)
	{
		attributes.remove(idx);
	}
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
			auto v = values[attr->name()];
			// for vector types, ensure the right number of valid values
			if ((attr->valueType() != iAValueType::Vector2  || variantToVector<double>(v).size() == 2) &&
				(attr->valueType() != iAValueType::Vector2i || variantToVector<int   >(v).size() == 2) &&
				(attr->valueType() != iAValueType::Vector3  || variantToVector<double>(v).size() == 3) &&
				(attr->valueType() != iAValueType::Vector3i || variantToVector<int   >(v).size() == 3))
			{
				attr->setDefaultValue(v);
			}
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

#include "iASettings.h"

#include <QDomElement>
#include <QDomNamedNodeMap>

void storeAttributeValues(QDomElement& xml, iAAttributes const& attributes)
{
	auto values = extractValues(attributes);
	for (auto a : attributes)
	{
		xml.setAttribute( configStorageName(a->name()), values[a->name()].toString());
	}
}

void loadAttributeValues(QDomNamedNodeMap const & xml, iAAttributes & attributes)
{
	for (auto a : attributes)
	{
		QString valStr = xml.namedItem(configStorageName(a->name())).nodeValue();
		QVariant v;
		switch (a->valueType())
		{
		case iAValueType::Continuous:
		{
			bool ok;
			v = valStr.toDouble(&ok);
			if (!ok)
			{
				LOG(lvlWarn, QString("loadAttributeValues: Invalid value %1 for attribute %2!").arg(valStr).arg(a->name()));
			}
			break;
		}
		case iAValueType::Discrete:
		{
			bool ok;
			v = valStr.toInt(&ok);
			if (!ok)
			{
				LOG(lvlWarn, QString("loadAttributeValues: Invalid value %1 for attribute %2!").arg(valStr).arg(a->name()));
			}
			break;
		}
		case iAValueType::Boolean:
		{
			v = valStr == "true";
			break;
		}
		case iAValueType::Categorical:
		{
			QStringList l(a->defaultValue().toStringList());
			selectOption(l, valStr);
			v = l;
			break;
		}
		default:
			[[fallthrough]];
		case iAValueType::String:
		{
			v = valStr;
			break;
		}
		}
		a->setDefaultValue(v);
	}
}