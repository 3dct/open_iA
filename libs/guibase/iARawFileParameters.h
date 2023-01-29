// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include <vtkImageReader.h>  // for VTK_FILE_BYTE_ORDER_... constants
#include <vtkType.h>         // For types, e.g. VTK_UNSIGNED_SHORT

#include "iAToolsVTK.h"      // for mapVTKTypeToReadableDataType
#include "iAValueTypeVectorHelpers.h"


//! Contains all metadata required to load a raw data file.
//! @deprecated new developments should use iARawFileIO and its parameters() instead
struct iAguibase_API iARawFileParameters
{
	iARawFileParameters();
	//! Size of the dataset in voxels in all 3 dimensions.
	unsigned int m_size[3];
	//! Voxel spacing of the dataset (dimension of a single voxel in  given unit).
	double m_spacing[3];
	//! Default origin of the dataset, typically at the global origin (0, 0, 0).
	double m_origin[3];
	//! Size of an optional header (which must be skipped), typically 0.
	quint64 m_headersize;
	//! The scalar type used for storing single voxel values, as VTK type specifier.
	//! See VTK_UNSIGNED_SHORT and other defines at the same place in vtkType.h
	int  m_scalarType;
	//! The byte order (little endian or big endian) in VTK type constants.
	//! See VTK_FILE_BYTE_ORDER_ defines in include file <vtkImageReader.h>
	//! this type is mapped to list index in raw file dialog in mapVTKByteOrderToIdx in iARawFileParamDlg
	int m_byteOrder;
};

iAguibase_API QVariantMap rawParamsToMap(iARawFileParameters const& p);
iAguibase_API iARawFileParameters rawParamsFromMap(QVariantMap const& map);
