// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "iAObjectType.h"

#include <iADataSet.h>
#include <iAVec3.h>

#include <vtkSmartPointer.h>

class vtkTable;

//! dataset type containing data about a list of objects of same type
class iAobjectvis_API iAObjectsData : public iADataSet
{
public:
	iAObjectsData(QString const& name, iAObjectVisType visType, vtkSmartPointer<vtkTable> m_table, std::shared_ptr<QMap<uint, uint>> m_colMapping);
	QString info() const override;

	//! one row per object to visualize
	vtkSmartPointer<vtkTable> m_table;
	//! mapping of columns (see the respective visualization classes which mappings are required)
	std::shared_ptr<QMap<uint, uint>> m_colMapping;
	//! type of visualization to create
	iAObjectVisType m_visType;
	//! optional (if non-empty vector) information on curved fiber objects
	std::map<size_t, std::vector<iAVec3f>> m_curvedFiberData; // maybe use separate derived object to store this? 
	// maybe also store csv config?
};
