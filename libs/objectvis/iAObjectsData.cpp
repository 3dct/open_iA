// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAObjectsData.h"

iAObjectsData::iAObjectsData(iAObjectVisType visType, vtkSmartPointer<vtkTable> table, std::shared_ptr<QMap<uint, uint>> colMapping) :
	iADataSet(iADataSetType::Objects),
	m_table(table),
	m_colMapping(colMapping),
	m_visType(visType)
{}