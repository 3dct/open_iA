// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARawFileParameters.h"

#include "iAToolsVTK.h"      // for mapVTKTypeToReadableDataType
#include "iARawFileIO.h"
#include "iAValueTypeVectorHelpers.h"

#include <vtkType.h>         // For types, e.g. VTK_UNSIGNED_SHORT

iARawFileParameters::iARawFileParameters():
	headerSize(0),
	scalarType(VTK_UNSIGNED_SHORT),
	byteOrder(itk::CommonEnums::IOByteOrder::LittleEndian)
{
	size.fill(1);
	spacing.fill(1.0);
	origin.fill(0.0);
}

quint64 iARawFileParameters::voxelBytes() const
{
	return mapVTKTypeToSize(scalarType);
}

quint64 iARawFileParameters::fileSize() const
{
	return headerSize + voxelBytes() * size[0] * size[1] * size[2];  // voxelBytes is left-most to take advantage of left-to-right evaluation order for automatic conversion to size_t
}

namespace
{
	QString mapITKByteOrderToString(itk::CommonEnums::IOByteOrder byteOrder)
	{
		return byteOrder == itk::CommonEnums::IOByteOrder::BigEndian ? iAByteOrder::BigEndianStr : iAByteOrder::LittleEndianStr;
	}
}

QVariantMap iARawFileParameters::toMap() const
{
	QVariantMap result;
	result[iARawFileIO::SizeStr] = variantVector<int>({ static_cast<int>(size[0]), static_cast<int>(size[1]), static_cast<int>(size[2]) });
	result[iARawFileIO::SpacingStr] = variantVector(spacing.data(), 3);
	result[iARawFileIO::OriginStr] = variantVector(origin.data(), 3);
	result[iARawFileIO::HeadersizeStr] = headerSize;
	result[iARawFileIO::DataTypeStr] = mapVTKTypeToReadableDataType(scalarType);
	result[iARawFileIO::ByteOrderStr] = mapITKByteOrderToString(byteOrder);
	return result;
}

std::optional<iARawFileParameters> iARawFileParameters::fromMap(QVariantMap const& map)
{   // theoretically we could exit early if at any time ok is false, but that would complicate the code and would probably not even speed up things
	iARawFileParameters result;
	bool ok = setFromVectorVariant<int>(result.size, map[iARawFileIO::SizeStr]);
	ok &= setFromVectorVariant<double>(result.spacing, map[iARawFileIO::SpacingStr]);
	ok &= setFromVectorVariant<double>(result.origin, map[iARawFileIO::OriginStr]);
	result.headerSize = map[iARawFileIO::HeadersizeStr].toUInt();
	result.scalarType = mapReadableDataTypeToVTKType(map[iARawFileIO::DataTypeStr].toString());
	ok &= result.scalarType != -1;
	result.byteOrder = mapStringToITKByteOrder(map[iARawFileIO::ByteOrderStr].toString());
	ok &= result.size[0] > 0 && result.size[1] > 0 && result.size[2] > 0;
	ok &= result.headerSize >= 0 && result.voxelBytes() > 0;
	return ok ? std::optional<iARawFileParameters>(result) : std::nullopt;
}
