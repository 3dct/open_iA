/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
#pragma once

#include <vtkSmartPointer.h>

#include <QThread>

class iAColorTheme;

class vtkImageData;
class vtkLookupTable;
class vtkPiecewiseFunction;

class QStandardItemModel;

class iALabelOverlayThread : public QThread
{
public:
	iALabelOverlayThread(vtkSmartPointer<vtkImageData>& labelOverlayImg,
		vtkSmartPointer<vtkLookupTable>& labelOverlayLUT,
		vtkSmartPointer<vtkPiecewiseFunction>& labelOverlayOTF,
		QStandardItemModel* itemModel,
		int labelCount,
		iAColorTheme const * colorTheme,
		int *    imageExtent,
		double * imageSpacing);
	void RebuildLabelOverlayLUT();
	vtkSmartPointer<vtkImageData> drawImage();
	void run();
private:
	vtkSmartPointer<vtkImageData>& m_labelOverlayImg;
	vtkSmartPointer<vtkLookupTable>& m_labelOverlayLUT;
	vtkSmartPointer<vtkPiecewiseFunction>& m_labelOverlayOTF;
	QStandardItemModel* m_itemModel;
	int m_labelCount;
	iAColorTheme const * m_colorTheme;
	int *    m_imageExtent;
	double * m_imageSpacing;
};
