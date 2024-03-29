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
iAbase_API iAAttributes combineAttributesWithValues(iAAttributes const& attributes, QVariantMap const & values);

//! In a given key/value map outMap, set values from another map inMap, for all keys that exist in the given attributes and the inMap
//! @param outMap the key/value map that is modified (note that this map isn't emptied before; so all key/value pairs that were
//!        already contained in there, and are not in both attributes and inMap, will remain unchanged.
//! @param attributes list of possible attributes - only keys of outMap which have a corresponding entry in here will be set
//! @param inMap the map of key/values used as input; any values existing in this map AND in attributes, will be set in outMap
iAbase_API void setApplyingValues(QVariantMap& outMap, iAAttributes const & attributes, QVariantMap const & inMap);

//! Merge the values from newValues and baseValues into a new map.
//! Creates and returns a map that contains key/value pairs from both given maps; values that exist in both get assigned the value from newValues
//! @param baseValues used to initialize the resulting map
//! @param newValues all key/value pairs in this map are added to the result, overwriting ones that might already be present from baseValues
iAbase_API QVariantMap joinValues(QVariantMap const & baseValues, QVariantMap const & newValues);

//! Extract just the (default) values from the given descriptors into a map
iAbase_API QVariantMap extractValues(iAAttributes const& attributes);

//! Add a new attribute specified by the parameters to the given attributes list
iAbase_API void addAttr(iAAttributes& attributes,
	QString const& name, iAValueType valueType,
	QVariant defaultValue = 0.0,
	double min = std::numeric_limits<double>::lowest(),
	double max = std::numeric_limits<double>::max());
