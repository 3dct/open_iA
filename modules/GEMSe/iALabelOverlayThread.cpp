/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "pch.h"
#include "iALabelOverlayThread.h"

#include "iAColorTheme.h"
#include "iAVtkDraw.h"

#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

#include <QStandardItemModel>

iALabelOverlayThread::iALabelOverlayThread(vtkSmartPointer<vtkImageData>& labelOverlayImg,
	vtkSmartPointer<vtkLookupTable>& labelOverlayLUT,
	vtkSmartPointer<vtkPiecewiseFunction>& labelOverlayOTF,
	QStandardItemModel* itemModel,
	int labelCount,
	iAColorTheme const * colorTheme,
	int *    imageExtent,
	double * imageSpacing
) :
	m_labelOverlayImg(labelOverlayImg),
	m_labelOverlayLUT(labelOverlayLUT),
	m_labelOverlayOTF(labelOverlayOTF),
	m_itemModel(itemModel),
	m_labelCount(labelCount),
	m_colorTheme(colorTheme),
	m_imageExtent(imageExtent),
	m_imageSpacing(imageSpacing)
{}

void iALabelOverlayThread::RebuildLabelOverlayLUT()
{
	m_labelOverlayLUT = vtkSmartPointer<vtkLookupTable>::New();
	m_labelOverlayLUT->SetNumberOfTableValues(m_labelCount + 1);
	m_labelOverlayLUT->SetRange(0.0, m_labelCount);
	m_labelOverlayLUT->SetTableValue(0, 0.0, 0.0, 0.0, 0.0); //label 0 is transparent

	m_labelOverlayOTF = vtkSmartPointer<vtkPiecewiseFunction>::New();
	m_labelOverlayOTF->AddPoint(0, 0);
	for (int i = 0; i<m_labelCount; ++i)
	{
		QColor c(m_colorTheme->GetColor(i));
		m_labelOverlayLUT->SetTableValue(i + 1,
			c.red() / 255.0,
			c.green() / 255.0,
			c.blue() / 255.0,
			1);	// all other labels are opaque
		m_labelOverlayOTF->AddPoint(i + 1, 1);
	}
	m_labelOverlayLUT->Build();
}

vtkSmartPointer<vtkImageData> iALabelOverlayThread::drawImage()
{
	RebuildLabelOverlayLUT();
	vtkSmartPointer<vtkImageData> result = vtkSmartPointer<vtkImageData>::New();
	result->SetExtent(m_imageExtent);
	result->SetSpacing(m_imageSpacing);
	result->AllocateScalars(VTK_INT, 1);
	clearImage(result, 0);

	for (int l = 0; l<m_itemModel->rowCount(); ++l)
	{
		QStandardItem * labelItem = m_itemModel->item(l);
		for (int i = 0; i<labelItem->rowCount(); ++i)
		{
			QStandardItem* coordItem = labelItem->child(i);
			int x = coordItem->data(Qt::UserRole + 1).toInt();
			int y = coordItem->data(Qt::UserRole + 2).toInt();
			int z = coordItem->data(Qt::UserRole + 3).toInt();
			drawPixel(result, x, y, z, l + 1);
		}
	}
	return result;
}

void iALabelOverlayThread::run()
{
	m_labelOverlayImg = drawImage();
}
