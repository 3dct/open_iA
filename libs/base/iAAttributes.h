// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

#include "iAAttributeDescriptor.h"

#include <QVector>

#include <memory>

class QDomNamedNodeMap;
class QDomElement;
class QTextStream;

//! List of descriptors for e.g. parameter values
using iAAttributes = QVector<std::shared_ptr<iAAttributeDescriptor>>;

//! Create a descriptor from a given text stream
iAbase_API std::shared_ptr<iAAttributes> createAttributes(QTextStream& in);

//! Store the given descriptors in the given text stream
iAbase_API void storeAttributes(QTextStream& out, iAAttributes const& attributes);

//! Find a descriptor with the given name and return its index
iAbase_API int findAttribute(iAAttributes const& attributes, QString const& name);

//! Removes the attribute with the given name
iAbase_API void removeAttribute(iAAttributes& attributes, QString const& name);

//! Count the attribute descriptors of the given type in the given collection
iAbase_API int countAttributes(iAAttributes const& attributes, iAAttributeDescriptor::iAAttributeType type = iAAttributeDescriptor::None);

//! Clone the given list of attributes and return the copy.
iAbase_API iAAttributes cloneAttributes(iAAttributes const& attributes);

//! Merge the given values into the descriptors (returns new descriptors with the given values as default values)
iAbase_API iAAttributes combineAttributesWithValues(iAAttributes const& attributes, QVariantMap const & values);

//! Merge the given values into the descriptors (modifying the given ones in place, in contrast to combineAttributesWithValues, which creates a new list)
iAbase_API void setDefaultValues(iAAttributes& attributes, QVariantMap const& values);

//! In a given key/value map outMap, set values from another map inMap, for all keys that exist in the given attributes and the inMap
//! @param[out] outMap the key/value map that is modified (note that this map isn't emptied before; so all key/value pairs that were
//!        already contained in there, and are not in both attributes and inMap, will remain unchanged.
//! @param[in] attributes list of possible attributes - only keys of outMap which have a corresponding entry in here will be set
//! @param[in] inMap the map of key/values used as input; any values existing in this map AND in attributes, will be set in outMap
iAbase_API void setApplyingValues(QVariantMap& outMap, iAAttributes const & attributes, QVariantMap const & inMap);

//! Merge the values from newValues and baseValues into a new map.
//! Creates and returns a map that contains key/value pairs from both given maps; values that exist in both get assigned the value from newValues
//! @param[in] baseValues used to initialize the resulting map
//! @param[in] newValues all key/value pairs in this map are added to the result, overwriting ones that might already be present from baseValues
iAbase_API QVariantMap joinValues(QVariantMap const & baseValues, QVariantMap const & newValues);

//! Extract just the (default) values from the given descriptors into a map
iAbase_API QVariantMap extractValues(iAAttributes const& attributes);

//! Add a new attribute specified by the parameters to the given attributes list
iAbase_API void addAttr(iAAttributes& attributes,
	QString const& name, iAValueType valueType,
	QVariant defaultValue = 0.0,
	double min = std::numeric_limits<double>::lowest(),
	double max = std::numeric_limits<double>::max());

iAbase_API void storeAttributeValues(QDomElement& xml, iAAttributes const& attributes);

iAbase_API void loadAttributeValues(QDomNamedNodeMap const & xml, iAAttributes & attributes);
