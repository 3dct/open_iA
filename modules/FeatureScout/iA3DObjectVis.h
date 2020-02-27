/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "FeatureScout_export.h"

#include <iAvec3.h>

#include <vtkType.h>

#include <QList>
#include <QMap>
#include <QSharedPointer>

#include <vector>

class vtkColorTransferFunction;
class vtkFloatArray;
class vtkImageData;
class vtkRenderer;
class vtkTable;

class QColor;
class QStandardItem;

//! Base class for 3D visualizations of objects (e.g. fibers or pores) defined in a table
//! use the factory method create3DObjectVis to create a specific instance!
class FeatureScout_API iA3DObjectVis: public QObject
{
	Q_OBJECT
public:
	//! the type used for indices into the data table.
	//! (Implementation Note: if vtkTable is replaced by something else, e.g. SPMData or a general table class, this might need to be adapted)
	typedef vtkIdType IndexType;
	static const QColor SelectedColor;
	iA3DObjectVis(vtkRenderer* ren, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping );
	virtual ~iA3DObjectVis();
	virtual void show();
	virtual void renderSelection( std::vector<size_t> const & sortedSelInds, int classID, QColor const & classColor, QStandardItem* activeClassItem ) =0;
	virtual void renderSingle(IndexType selectedObjID, int classID, QColor const & classColor, QStandardItem* activeClassItem ) =0;
	virtual void multiClassRendering( QList<QColor> const & classColors, QStandardItem* rootItem, double alpha ) =0;
	virtual void renderOrientationDistribution( vtkImageData* oi ) =0;
	virtual void renderLengthDistribution( vtkColorTransferFunction* cTFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range ) =0;
	virtual double const * bounds() =0;
signals:
	void updated();
protected:
	QColor getOrientationColor( vtkImageData* oi, IndexType objID ) const;
	QColor getLengthColor( vtkColorTransferFunction* ctFun, IndexType objID ) const;
	virtual void updateRenderer();
	vtkRenderer* m_ren;
	vtkTable* m_objectTable;
	QSharedPointer<QMap<uint, uint> > m_columnMapping;
};
