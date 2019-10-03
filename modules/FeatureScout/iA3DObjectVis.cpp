/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iA3DObjectVis.h"

#include "iACsvConfig.h"
#include "iA3DLabelledVolumeVis.h"
#include "iA3DLineObjectVis.h"
#include "iA3DCylinderObjectVis.h"
#include "iA3DNoVis.h"
#include "iA3DEllipseObjectVis.h"

#include <iARenderer.h>
#include <mdichild.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkOpenGLRenderer.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkTable.h>

#include <QColor>
#include <QtMath>

iA3DObjectVis::iA3DObjectVis(vtkRenderer* ren, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping ):
	m_ren(ren),
	m_objectTable(objectTable),
	m_columnMapping(columnMapping)
{}

iA3DObjectVis::~iA3DObjectVis()
{}

QColor iA3DObjectVis::getOrientationColor( vtkImageData* oi, size_t objID ) const
{
	int ip = qFloor( m_objectTable->GetValue( objID, m_columnMapping->value(iACsvConfig::Phi) ).ToDouble() );
	int it = qFloor( m_objectTable->GetValue( objID, m_columnMapping->value(iACsvConfig::Theta) ).ToDouble() );
	double *p = static_cast<double *>( oi->GetScalarPointer( it, ip, 0 ) );
	return QColor(p[0]*255, p[1]*255, p[2]*255, 255);
}

QColor iA3DObjectVis::getLengthColor( vtkColorTransferFunction* cTFun, size_t objID ) const
{
	double length = m_objectTable->GetValue( objID, m_columnMapping->value(iACsvConfig::Length) ).ToDouble();
	double dcolor[3];
	cTFun->GetColor( length, dcolor );
	return QColor(dcolor[0]*255, dcolor[1]*255, dcolor[2]*255);
}

void iA3DObjectVis::updateRenderer()
{
	m_ren->Render();
}

void iA3DObjectVis::show()
{}

const QColor iA3DObjectVis::SelectedColor(255, 0, 0, 255);


QSharedPointer<iA3DObjectVis> create3DObjectVis(int visualization, MdiChild* mdi, vtkTable* table,
	QSharedPointer<QMap<uint, uint> > columnMapping, QColor const & color,
	std::map<size_t, std::vector<iAVec3f> > & curvedFiberInfo, int numberOfCylinderSides, size_t segmentSkip)
{
	switch (visualization)
	{
		default:
		case iACsvConfig::UseVolume:
			return QSharedPointer<iA3DObjectVis>(new iA3DLabelledVolumeVis(mdi->renderer()->renderer(), mdi->colorTF(),
				mdi->opacityTF(), table, columnMapping, mdi->imagePointer()->GetBounds()));
		case iACsvConfig::Lines:
			return QSharedPointer<iA3DObjectVis>(new iA3DLineObjectVis(mdi->renderer()->renderer(), table, columnMapping, color, curvedFiberInfo, segmentSkip));
		case iACsvConfig::Cylinders:
			return QSharedPointer<iA3DObjectVis>(new iA3DCylinderObjectVis(mdi->renderer()->renderer(), table, columnMapping, color, curvedFiberInfo, numberOfCylinderSides, segmentSkip));
		case iACsvConfig::Ellipses:
			return QSharedPointer<iA3DObjectVis>(new iA3DEllipseObjectVis(mdi->renderer()->renderer(), table, columnMapping, color));
		case iACsvConfig::NoVis:
			return QSharedPointer<iA3DObjectVis>(new iA3DNoVis());
	}
}
