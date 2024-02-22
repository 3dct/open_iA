// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAObjectVis.h"

#include "iACsvConfig.h"
#include "iAObjectsData.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkTable.h>

#include <QColor>
#include <QtMath>

iAObjectVis::iAObjectVis(iAObjectsData const* data):
	m_data(data)
{}

iAObjectVis::~iAObjectVis()
{}

QColor iAObjectVis::getOrientationColor( vtkImageData* oi, IndexType objID ) const
{
	int ip = qFloor( m_data->m_table->GetValue( objID, m_data->m_colMapping->value(iACsvConfig::Phi) ).ToDouble() );
	int it = qFloor( m_data->m_table->GetValue( objID, m_data->m_colMapping->value(iACsvConfig::Theta) ).ToDouble() );
	double *p = static_cast<double *>( oi->GetScalarPointer( it, ip, 0 ) );
	return QColor(p[0]*255, p[1]*255, p[2]*255, 255);
}

QColor iAObjectVis::getLengthColor( vtkColorTransferFunction* cTFun, IndexType objID ) const
{
	double length = m_data->m_table->GetValue( objID, m_data->m_colMapping->value(iACsvConfig::Length) ).ToDouble();
	double dcolor[3];
	cTFun->GetColor( length, dcolor );
	return QColor(dcolor[0]*255, dcolor[1]*255, dcolor[2]*255);
}

const QColor iAObjectVis::SelectedColor(255, 0, 0, 255);


// iAObjectVisActor

iAObjectVisActor::iAObjectVisActor(vtkRenderer* ren):
	m_ren(ren)
{}

void iAObjectVisActor::updateRenderer()
{
	if (!m_ren)
	{
		return;
	}
	m_ren->Render();
	emit updated();
}

void iAObjectVisActor::show()
{}

void iAObjectVisActor::hide()
{}

void iAObjectVisActor::clearRenderer()
{
	m_ren = nullptr;
}
