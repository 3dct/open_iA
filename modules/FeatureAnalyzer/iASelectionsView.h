// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iASelection.h"
#include "ui_Selections.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <QList>
#include <QModelIndex>
#include <vtkSmartPointer.h>

class vtkIdTypeArray;
class QTreeWidgetItem;

typedef iAQTtoUIConnector<QWidget, Ui_Selections> SelectionsConnector;

class iASelectionsView : public SelectionsConnector
{
	Q_OBJECT

public:
	iASelectionsView(QWidget* parent = nullptr);
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
