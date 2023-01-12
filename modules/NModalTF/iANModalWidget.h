/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iALabellingObjects.h"
#include "iANModalObjects.h"

#include <QList>
#include <QMap>
#include <QWidget>

class iANModalController;
class iANModalPreprocessor;
class iANModalLabelsWidget;
class iASlicer;
class iAMdiChild;

class QLabel;
class QGridLayout;

class iANModalWidget : public QWidget
{
	Q_OBJECT

public:
	iANModalWidget(iAMdiChild* mdiChild);

private:
	iANModalController* m_c;
	QSharedPointer<iANModalPreprocessor> m_preprocessor;
	iAMdiChild* m_mdiChild;

	QGridLayout* m_layoutSlicersGrid;
	iANModalLabelsWidget* m_labelsWidget;

	QLabel* m_labelModalityCount;

	QMap<int, iANModalLabel> m_labels;

private slots:
	void onAllSlicersInitialized();
	void onAllSlicersReinitialized();
	void onHistogramInitialized(int index);

	//void onModalitiesChanged();

	void onSeedsAdded(const QList<iASeed>&);
	void onSeedsRemoved(const QList<iASeed>&);
	void onAllSeedsRemoved();
	void onLabelAdded(const iALabel&);
	void onLabelRemoved(const iALabel&);
	void onLabelsColorChanged(const QList<iALabel>&);

	void onLabelOpacityChanged(int labelId);
	void onLabelRemoverStateChanged(int labelId);
};