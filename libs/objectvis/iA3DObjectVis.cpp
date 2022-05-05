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
#include "iA3DObjectVis.h"

#include "iACsvConfig.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
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

QColor iA3DObjectVis::getOrientationColor( vtkImageData* oi, IndexType objID ) const
{
	int ip = qFloor( m_objectTable->GetValue( objID, m_columnMapping->value(iACsvConfig::Phi) ).ToDouble() );
	int it = qFloor( m_objectTable->GetValue( objID, m_columnMapping->value(iACsvConfig::Theta) ).ToDouble() );
	double *p = static_cast<double *>( oi->GetScalarPointer( it, ip, 0 ) );
	return QColor(p[0]*255, p[1]*255, p[2]*255, 255);
}

QColor iA3DObjectVis::getLengthColor( vtkColorTransferFunction* cTFun, IndexType objID ) const
{
	double length = m_objectTable->GetValue( objID, m_columnMapping->value(iACsvConfig::Length) ).ToDouble();
	double dcolor[3];
	cTFun->GetColor( length, dcolor );
	return QColor(dcolor[0]*255, dcolor[1]*255, dcolor[2]*255);
}

void iA3DObjectVis::updateRenderer()
{
	m_ren->Render();
	emit updated();
}

void iA3DObjectVis::show()
{}

const QColor iA3DObjectVis::SelectedColor(255, 0, 0, 255);
