// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAttributes.h"

#include "iAValueTypeVectorHelpers.h"

#include <QColor>
#include <QFileInfo>
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

QVariant getValue(iAAttributes const& attributes, QString const& name)
{
	auto idx = findAttribute(attributes, name);
	if (idx == -1)
	{
		return QVariant();
	}
	return attributes[idx]->defaultValue();
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

void setDependencies(iAAttributes& attributes, QString const& name, QStringList const & dependencies)
{
	auto idx = findAttribute(attributes, name);
	if (idx == -1)
	{
		LOG(lvlError, QString("setDependencies: Parameter %1 not found!").arg(name));
	}
	else
	{
		attributes[idx]->setDependencies(dependencies);
	}
}

void setDependency(iAAttributes& attributes, QString const& name, QString const& dependency)
{
	setDependencies(attributes, name, QStringList() << dependency);
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

bool checkAttributes(iAAttributes const& attributes, QVariantMap const& values, QStringList* invalidValues)
{
	if (invalidValues)
	{
		invalidValues->clear();
	}
	for (auto param : attributes)
	{
		if (!isAttributeEnabled(*param.get(), values))
		{
			continue;
		}
		if (!attributeCheck(*param.get(), values[param->name()]))
		{
			if (invalidValues)
			{
				invalidValues->append(param->name());
			}
			else
			{
				return false;
			}
		}
	}
	return invalidValues ? invalidValues->size() > 0 : true;
}

namespace
{
	template<typename T, int Size> bool checkVecType(iAAttributeDescriptor const& param, QVariant const& value)
	{
		bool ok;
		auto valVec = variantToVector<T>(value, &ok);
		if (valVec.size() != Size)
		{
			LOG(lvlError, QString("Parameter %1: Expected %2 values, got %3.").arg(param.name()).arg(Size).arg(valVec.size()));
			return false;
		}
		if (!ok)
		{
			return false;
		}
		bool valOK = true;
		for (qsizetype i=0; i<valVec.size(); ++i)
		{
			auto v = valVec[i];
			if (v < param.min() || v > param.max())
			{
				valOK = false;
				LOG(lvlError,
					QString("Parameter %1: Value %2 for index %3 is outside of valid range %4..%5")
						.arg(param.name())
						.arg(v)
						.arg(i)
						.arg(param.min())
						.arg(param.max()));
			}
		}
		return valOK;
	}
}

bool attributeCheck(iAAttributeDescriptor const & param, QVariant const& paramValue)
{
	bool ok;
	switch (param.valueType())
	{
	case iAValueType::Discrete:
	{
		long long value = paramValue.toLongLong(&ok);
		if (!ok)
		{
			LOG(lvlError, QString("Parameter %1: Expected integer value, %2 given.")
					.arg(param.name())
					.arg(paramValue.toString()));
			return false;
		}
		if (value < param.min() || value > param.max())
		{
			LOG(lvlError, QString("Parameter %1: Given value %2 outside of valid range [%3..%4].")
					.arg(param.name())
					.arg(paramValue.toString())
					.arg(param.min())
					.arg(param.max()));
			return false;
		}
		break;
	}
	case iAValueType::Continuous:
	{
		double value = paramValue.toDouble(&ok);
		if (!ok)
		{
			LOG(lvlError,
				QString("Parameter %1: Expected double value, %2 given.")
					.arg(param.name())
					.arg(paramValue.toString()));
			return false;
		}
		if (value < param.min() || value > param.max())
		{
			LOG(lvlError,
				QString("Parameter %1: Given value %2 outside of valid range [%3..%4].")
					.arg(param.name())
					.arg(paramValue.toString())
					.arg(param.min())
					.arg(param.max()));
			return false;
		}
		break;
	}
	case iAValueType::Vector2:
		return checkVecType<double, 2>(param, paramValue);
	case iAValueType::Vector3:
		return checkVecType<double, 3>(param, paramValue);
	case iAValueType::Vector2i:
		return checkVecType<int, 2>(param, paramValue);
	case iAValueType::Vector3i:
		return checkVecType<int, 3>(param, paramValue);
	case iAValueType::Categorical:
	{
		QStringList values = param.defaultValue().toStringList();
		bool found = false;
		for (QString s : values)
		{
			if (s.startsWith("!"))
			{
				s = s.right(s.length() - 1);
			}
			if (s == paramValue)
			{
				found = true;
			}
		}
		if (!found)
		{
			LOG(lvlError,
				QString("Parameter %1: Given value '%2' not in the list of valid values (%3).")
					.arg(param.name())
					.arg(paramValue.toString())
					.arg(values.join(",")));
			return false;
		}
		break;
	}
	case iAValueType::FileNameOpen:
	{
		QFileInfo file(paramValue.toString());
		if (!file.isFile() || !file.isReadable())
		{
			LOG(lvlError,
				QString("Parameter %1: Given filename '%2' either doesn't reference a file, "
						   "the file does not exist, or it is not readable!")
					.arg(param.name())
					.arg(paramValue.toString()));
			return false;
		}
		break;
	}
	case iAValueType::FileNamesOpen:
	{
		QStringList files = splitPossiblyQuotedString(paramValue.toString());
		for (auto fileName : files)
		{
			QFileInfo file(fileName);
			if (!file.isFile() || !file.isReadable())
			{
				LOG(lvlError,
					QString("Parameter %1: Filename '%2' out of the given list '%3' either doesn't reference a file, "
							"the file does not exist, or it is not readable!")
						.arg(param.name())
						.arg(fileName)
						.arg(paramValue.toString()));
				return false;
			}
		}
		break;
	}
	case iAValueType::Folder:
	{
		// TODO: allow to specify whether the folder can be empty or not!
		QFileInfo file(paramValue.toString());
		if (!paramValue.toString().isEmpty() && !file.isDir())
		{
			LOG(lvlError,
				QString("Parameter '%1': Given value '%2' doesn't reference a folder!")
					.arg(param.name())
					.arg(paramValue.toString()));
			return false;
		}
		break;
	}
	case iAValueType::Color:
	{
		QColor color(paramValue.toString());
		if (!color.isValid())
		{
			LOG(lvlError,
				QString("Parameter '%1': '%2' is not a valid color value; "
						   "please either give a color name (e.g. blue, green, ...) "
						   "or a hexadecimal RGB specifier, like #RGB, #RRGGBB!")
					.arg(param.name())
					.arg(paramValue.toString()));
			return false;
		}
		break;
	}
	case iAValueType::Invalid:
		LOG(lvlError,
			QString("Parameter '%1': Invalid parameter type (please contact developers!)!").arg(param.name()));
		return false;
	default:  // no checks
		break;
	}
	return true;
}

bool isAttributeEnabled(iAAttributeDescriptor const & p, QVariantMap const& values)
{
	auto enabled = true;
	for (auto d : p.dependencies())
	{
		auto dep = d;
		auto inverted = false;
		if (dep.startsWith("!"))
		{
			inverted = true;
			dep.remove(0, 1);
		}
		QString expectedVal;
		if (dep.contains("="))
		{
			auto parts = dep.split("=");
			if (parts.size() > 2)
			{
				LOG(lvlError,
					QString("Parameter %1: Expected only 2 parts to dependency specification, got %2")
						.arg(p.name())
						.arg(parts.size()));
			}
			dep = parts[0];
			expectedVal = parts[1];
		}
		if (!values.contains(dep))
		{
			LOG(lvlError,
				QString("For parameter %1, dependency %2 is not a known parameter here!").arg(p.name()).arg(dep));
		}
		bool test = false;
		if (expectedVal.isEmpty())
		{
			test = values[dep].toBool();
			if (inverted)
			{
				test = !test;
			}
		}
		else
		{
			test = values[dep].toString() == expectedVal;
		}
		if (!test)
		{
			enabled = false;
			break;
		}
	}
	return enabled;
}