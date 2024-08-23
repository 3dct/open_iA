// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <itkCommonEnums.h>    // for itk::CommonEnums::IOByteOrder

#include <QVariantMap>

#include <array>
#include <optional>

//! Contains all metadata required to load a raw data file.
struct iARawFileParameters
{
	//! Create default raw file parameters
	iARawFileParameters();
	//! default comparison operator (only == required actually)
	bool operator==(const iARawFileParameters&) const = default;
	//! Size of the dataset in voxels in all 3 dimensions.
	std::array<unsigned int, 3> size;
	//! Voxel spacing of the dataset (dimension of a single voxel in  given unit).
	std::array<double, 3> spacing;
	//! Default origin of the dataset, typically at the global origin (0, 0, 0).
	std::array<double, 3> origin;
	//! Size of an optional header (which must be skipped), typically 0.
	quint32 headerSize;
	//! The scalar type used for storing single voxel values, as VTK type specifier.
	//! See VTK_UNSIGNED_SHORT and other defines at the same place in vtkType.h
	int  scalarType;
	//! The byte order (little endian or big endian) in ITK type constants.
	itk::CommonEnums::IOByteOrder byteOrder;
	//! Size (in bytes) of the whole file with the current parameters
	quint64 fileSize() const;
	//! Size (in bytes) of the data of a single voxel
	quint64 voxelBytes() const;
	//! convert values to map
	QVariantMap toMap() const;
	//! create an instance of iARawFileParameters from the given values; empty if given value not valid
	static std::optional<iARawFileParameters> fromMap(QVariantMap const& map);
};

