// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaobjectvis_export.h"

#include "iAObjectVisActor.h"

#include <vtkType.h>

#include <QList>
#include <QMap>

#include <memory>
#include <vector>

class iAObjectsData;

class vtkColorTransferFunction;
class vtkFloatArray;
class vtkImageData;
class vtkRenderer;
class vtkTable;

class QColor;
class QStandardItem;

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
	iAObjectVis(iAObjectsData const* data);
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

	iAObjectsData const* m_data;
};
