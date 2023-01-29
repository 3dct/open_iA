// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAbase_export.h"

#include "iAAttributeDescriptor.h"

#include <QVector>
#include <QSharedPointer>

class QTextStream;

//! List of descriptors for e.g. parameter values
using iAAttributes = QVector<QSharedPointer<iAAttributeDescriptor>>;

//! Create a descriptor from a given text stream
iAbase_API QSharedPointer<iAAttributes> createAttributes(QTextStream& in);

//! Store the given descriptors in the given text stream
iAbase_API void storeAttributes(QTextStream& out, iAAttributes const& attributes);

//! Find a descriptor with the given name and return its index
iAbase_API int findAttribute(iAAttributes const& attributes, QString const& name);

//! Count the attribute descriptors of the given type in the given collection
iAbase_API int countAttributes(iAAttributes const& attributes, iAAttributeDescriptor::iAAttributeType type = iAAttributeDescriptor::None);

//! Merge the given values into the descriptors (returns new descriptors with the given values as default values)
iAbase_API iAAttributes combineAttributesWithValues(iAAttributes const& attributes, QVariantMap values);

//! Merge the values from newValues into baseValues
iAbase_API QVariantMap joinValues(QVariantMap const & baseValues, QVariantMap const & newValues);

//! Extract just the values from the given descriptors into a map
iAbase_API QVariantMap extractValues(iAAttributes const& attributes);

iAbase_API void addAttr(iAAttributes& attributes,
	QString const& name, iAValueType valueType,
	QVariant defaultValue = 0.0,
	double min = std::numeric_limits<double>::lowest(),
	double max = std::numeric_limits<double>::max());
