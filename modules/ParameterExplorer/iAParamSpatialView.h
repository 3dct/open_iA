/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include <QMap>
#include <QWidget>

#include <vtkSmartPointer.h>

#include "io/iAITKIO.h"

class iAParamTableView;
class iAImageWidget;

class vtkImageData;

class QSpinBox;
class QToolButton;

class iAParamSpatialView: public QWidget
{
	Q_OBJECT
public:
	iAParamSpatialView(iAParamTableView* table, QString const & basePath);
	void SetImage(int id);
private slots:
	void SlicerModeButtonClicked(bool checked);
	void SliceChanged(int slice);
private:
	iAParamTableView* m_table;
	QString m_basePath;
	QMap<int, vtkSmartPointer<vtkImageData>> m_imageCache;
	QVector<iAITKIO::ImagePointer> m_loadedImgs; // to stop itk from unloading
	int m_curMode;
	int m_sliceNr[3];
	QVector<QToolButton*> slicerModeButton;
	QSpinBox* m_sliceControl;
	iAImageWidget* m_imageWidget;
	QWidget* m_settings;
	QWidget* m_imageContainer;
	bool m_sliceNrInitialized;
};
