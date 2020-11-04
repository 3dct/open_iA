/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include <iASlicerMode.h>

#include <vtkTransform.h>

#include <QSharedPointer>
#include <QWidget>

class iAModality;
class iASingleSlicerSettings;
class iASlicer;

class vtkCamera;

class iASimpleSlicerWidget : public QWidget
{
	Q_OBJECT

public:
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	iASimpleSlicerWidget(QWidget* parent = nullptr, bool enableInteraction = false, Qt::WindowFlags f = 0);
#else
	iASimpleSlicerWidget(QWidget* parent = nullptr, bool enableInteraction = false, Qt::WindowFlags f = QFlags<Qt::WindowType>());
#endif
	~iASimpleSlicerWidget();

	void setSlicerMode(iASlicerMode slicerMode);
	iASlicerMode getSlicerMode();

	void setSliceNumber(int sliceNumber);
	int getSliceNumber();

	bool hasHeightForWidth() const override;
	int heightForWidth(int width) const override;

	void applySettings(iASingleSlicerSettings const & settings);
	void changeModality(QSharedPointer<iAModality> modality);

	void setCamera(vtkCamera* camera);

	iASlicer* getSlicer() { return m_slicer; }

public slots:
	void update();

private:
	bool m_enableInteraction;
	vtkTransform *m_slicerTransform;
	iASlicer *m_slicer;
};
