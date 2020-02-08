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
#pragma once

#include "iANModalBackgroundRemover.h"

#include <itkImageBase.h>

#include <QWidget>

class MdiChild;
class iAConnector;
class iANModalDisplay;

class iANModalThresholdingWidget : public QWidget {
	Q_OBJECT
public:
	iANModalThresholdingWidget(QWidget *parent, iANModalDisplay *display);
	int threshold();
	QSharedPointer<iAModality> modality();
private:
	int m_threshold;
	QSharedPointer<iAModality> m_mod;
private slots:
	void setThreshold(int);
};

class iANModalDilationBackgroundRemover : public iANModalBackgroundRemover {

public:
	iANModalDilationBackgroundRemover(MdiChild *mdiChild);
	vtkSmartPointer<vtkImageData> removeBackground(QList<QSharedPointer<iAModality>>) override;

private:

	MdiChild *m_mdiChild;

	// return - true if a modality and a threshold were successfully chosen
	//        - false otherwise
	bool selectModalityAndThreshold(QWidget *parent, QList<QSharedPointer<iAModality>> modalities, int &out_threshold, QSharedPointer<iAModality> &out_modality);


	typedef itk::ImageBase<3>::Pointer ImagePointer;

	//iAConnector *m_temp_connector = nullptr;
	//vtkSmartPointer<vtkImageData> m_vtkTempImg;
	ImagePointer m_itkTempImg;

	template<class T>
	void itkBinaryThreshold(iAConnector *conn, int loThresh, int upThresh);

	template<class T>
	void binary_threshold(ImagePointer itkImgPtr);

	template<class T>
	void binary_threshold2(ImagePointer itkImgPtr);
};