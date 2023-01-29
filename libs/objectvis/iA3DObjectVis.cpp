// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA3DObjectVis.h"

#include "iACsvConfig.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkTable.h>

#include <QColor>
#include <QtMath>

iA3DObjectVis::iA3DObjectVis(vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping):
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

const QColor iA3DObjectVis::SelectedColor(255, 0, 0, 255);


// iA3DObjectActor

iA3DObjectActor::iA3DObjectActor(vtkRenderer* ren):
	m_ren(ren)
{}

void iA3DObjectActor::updateRenderer()
{
	if (!m_ren)
	{
		return;
	}
	m_ren->Render();
	emit updated();
}

void iA3DObjectActor::show()
{}

void iA3DObjectActor::clearRenderer()
{
	m_ren = nullptr;
}
