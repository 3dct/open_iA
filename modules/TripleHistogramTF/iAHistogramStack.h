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
#include <QStackedLayout>

#include "mdichild.h"
#include "BCoord.h"
#include "iAWeightedTransfer.h"

// Slicer
#include "iASimpleSlicerWidget.h"

class iAHistogramStack : public QWidget
{
	Q_OBJECT

public:
	iAHistogramStack(QWidget* parent, MdiChild *mdiChild, Qt::WindowFlags f = 0);
	~iAHistogramStack();

	void setWeight(BCoord bCoord);
	void setSlicerMode(iASlicerMode slicerMode, int dimensionLength);
	void setSliceNumber(int sliceNumber);
	void setModalityLabel(QString label, int index);

	iAWeightedTransfer* getTransferFunction();
	QSharedPointer<iAModality> getModality(int index);

	void removeModality(QSharedPointer<iAModality> modality, MdiChild* mdiChild);
	void addModality(QSharedPointer<iAModality> modality, MdiChild* mdiChild);
	void updateModalities(MdiChild* mdiChild); // Temporary // TODO: remove

	bool isReady();

private slots:
	void updateTransferFunction(int index);

signals:
	void transferFunctionChanged();
	void modalityAdded(QSharedPointer<iAModality> modality, int index);

protected:
	void resizeEvent(QResizeEvent* event);

private:
	void removeModality(QSharedPointer<iAModality> modality);
	void addModality(QSharedPointer<iAModality> modality);
	void updateModalities2(MdiChild* mdiChild);

	void adjustStretch(int totalWidth);

	void disable();
	void enable();

	QList<QSharedPointer<iAModality>> m_modalitiesAvailable;
	QSharedPointer<iAModality> m_modalitiesActive[3];
	iAWeightedTransfer *m_transferFunction;

	// Widgets and stuff
	QStackedLayout *m_stackedLayout;
	QLabel *m_disabledLabel;
	QGridLayout *m_gridLayout;
	QLabel *m_weightLabels[3];
	QLabel *m_modalityLabels[3];
	iASimpleSlicerWidget *m_slicerWidgets[3];
	iADiagramFctWidget* m_histograms[3];
	
};
