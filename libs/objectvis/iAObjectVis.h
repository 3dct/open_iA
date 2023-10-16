// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaobjectvis_export.h"

#include "iAObjectVisActor.h"
#include "iAObjectType.h"

#include <iADataSet.h>

#include <vtkType.h>

#include <QList>
#include <QMap>

#include <memory>
#include <vector>

class vtkColorTransferFunction;
class vtkFloatArray;
class vtkImageData;
class vtkRenderer;
class vtkTable;

class QColor;
class QStandardItem;

//! dataset type containing data about a list of objects of same type
class iAobjectvis_API iAObjectsData : public iADataSet
{
public:
	iAObjectsData(vtkTable* m_table, std::shared_ptr<QMap<uint, uint>> m_colMapping);
	vtkTable* m_table;
	std::shared_ptr<QMap<uint, uint>> m_colMapping;
	iAObjectVisType m_visType;
	// maybe also store csv config?
};

//! Base class for 3D visualizations of objects (e.g. fibers or pores) defined in a table.
//!
//! Use the factory method create3DObjectVis in iAObjectVisFactory.h to create a specific instance!
class iAobjectvis_API iAObjectVis: public QObject
{
	Q_OBJECT
public:
	//! the type used for indices into the data table.
	//! (Implementation Note: if vtkTable is replaced by something else, e.g. SPMData or a general table class, this might need to be adapted)
	typedef vtkIdType IndexType;
	static const QColor SelectedColor;
	iAObjectVis(std::shared_ptr<iAObjectsData> data);
	virtual ~iAObjectVis();
	virtual void renderSelection( std::vector<size_t> const & sortedSelInds, int classID, QColor const & classColor, QStandardItem* activeClassItem ) =0;
	virtual void renderSingle(IndexType selectedObjID, int classID, QColor const & classColor, QStandardItem* activeClassItem ) =0;
	virtual void multiClassRendering( QList<QColor> const & classColors, QStandardItem* rootItem, double alpha ) =0;
	virtual void renderOrientationDistribution( vtkImageData* oi ) =0;
	virtual void renderLengthDistribution( vtkColorTransferFunction* cTFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range ) =0;
	virtual double const* bounds() = 0;
	virtual std::shared_ptr<iAObjectVisActor> createActor(vtkRenderer* ren) = 0;

signals:
	void renderRequired();
	void dataChanged();

protected:
	QColor getOrientationColor( vtkImageData* oi, IndexType objID ) const;
	QColor getLengthColor( vtkColorTransferFunction* ctFun, IndexType objID ) const;

	std::shared_ptr<iAObjectsData> m_data;
};