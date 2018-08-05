/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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
#pragma once

#include <QWidget>
#include "mdichild.h"

#include "iATransferFunction.h"

// Modality
#include <QSharedPointer>;
class iAModality;

// Slicer
#include "iASimpleSlicerWidget.h"

class iAModalityWidget : public QWidget
{
	Q_OBJECT

public:
	iAModalityWidget(QWidget* parent, QSharedPointer<iAModality> modality, MdiChild *mdiChild, Qt::WindowFlags f = 0);
	~iAModalityWidget();

	void setWeight(double weight);
	void setSlicerMode(iASlicerMode slicerMode, int dimensionLength);
	void setSliceNumber(int sliceNumber);

	iATransferFunction* getTransferFunction();

private slots:
	void updateTransferFunction();

signals:
	void modalityTfChanged();

protected:
	void resizeEvent(QResizeEvent* event);

private:
	QVBoxLayout *m_rightWidgetLayout;
	QHBoxLayout *m_mainLayout;
	QLabel *m_weightLabel;
	iASimpleSlicerWidget *m_slicerWidget;
	iADiagramFctWidget* m_histogram;
	
};
