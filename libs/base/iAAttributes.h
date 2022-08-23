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

//! Extract just the values from the given descriptors into a map
iAbase_API QVariantMap extractValues(iAAttributes const& attributes);
