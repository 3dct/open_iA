/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenb�ck, Artem & Alexander Amirkhanov, B. Fr�hler   *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
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

vtkSmartPointer<vtkPiecewiseFunction> BuildLabelOverlayOTF(int labelCount)
{
	auto result = vtkSmartPointer<vtkPiecewiseFunction>::New();
	for (int i = 0; i < labelCount; ++i)
	{
		result->AddPoint(i, 1);
	}
	result->AddPoint(labelCount, 0);
	return result;
}

vtkSmartPointer<vtkLookupTable> BuildLabelOverlayLUT(int labelCount, iAColorTheme const * colorTheme)
{
	auto result = vtkSmartPointer<vtkLookupTable>::New();
	result->SetNumberOfTableValues(labelCount + 1);
	result->SetRange(0.0, labelCount);
	for (int i = 0; i<labelCount; ++i)
	{
		QColor c(colorTheme->GetColor(i));
		result->SetTableValue(i,
			c.red() / 255.0,
			c.green() / 255.0,
			c.blue() / 255.0,
			1);	// all other labels are opaque
	}
	result->SetTableValue(labelCount, 0.0, 0.0, 0.0, 0.0); // value m_labelCount is transparent
	result->Build();
	return result;
}

void iALabelOverlayThread::RebuildLabelOverlayLUT()
{
	m_labelOverlayLUT = BuildLabelOverlayLUT(m_labelCount, m_colorTheme);
	m_labelOverlayOTF = BuildLabelOverlayOTF(m_labelCount);
}

vtkSmartPointer<vtkImageData> iALabelOverlayThread::drawImage()
{
	RebuildLabelOverlayLUT();
	vtkSmartPointer<vtkImageData> result = vtkSmartPointer<vtkImageData>::New();
	result->SetExtent(m_imageExtent);
	result->SetSpacing(m_imageSpacing);
	result->AllocateScalars(VTK_INT, 1);
	clearImage(result, m_labelCount);

	for (int l = 0; l<m_itemModel->rowCount(); ++l)
	{
		QStandardItem * labelItem = m_itemModel->item(l);
		for (int i = 0; i<labelItem->rowCount(); ++i)
		{
			QStandardItem* coordItem = labelItem->child(i);
			int x = coordItem->data(Qt::UserRole + 1).toInt();
			int y = coordItem->data(Qt::UserRole + 2).toInt();
			int z = coordItem->data(Qt::UserRole + 3).toInt();
			drawPixel(result, x, y, z, l);
		}
	}
	return result;
}

void iALabelOverlayThread::run()
{
	m_labelOverlayImg = drawImage();
}
