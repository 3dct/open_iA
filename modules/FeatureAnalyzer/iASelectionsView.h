/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iASelection.h"
#include "ui_Selections.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <QList>
#include <QModelIndex>
#include <vtkSmartPointer.h>

class vtkIdTypeArray;
class QTreeWidgetItem;
class vtkSelection;

typedef iAQTtoUIConnector<QWidget, Ui_Selections> SelectionsConnector;

class iASelectionsView : public SelectionsConnector
{
	Q_OBJECT

public:
	iASelectionsView( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~iASelectionsView();

signals:
	void loadedSelection( iASelection * sel );
	void visualizeSelection( iASelection * sel );
	void compareSelections( QList<iASelection*> sels );

public slots:
	void selectionModifiedTreeView( QList<QTreeWidgetItem*> * selItems );
	void selectionModifiedSPMView( vtkVector2i curPlot, vtkIdTypeArray * selIds );
	void selectionModifiedPDMView( QModelIndexList selInds );
	void compareSelectionsSlot();

protected slots:
	void saveCurrentSelection();
	void loadSelection();
	void deleteSelection();
	void visualizeSelectionSlot();

protected:
	iASelection * m_curSelection;
	QList<iASelection*> m_selections;
	int m_selCounter;
};
