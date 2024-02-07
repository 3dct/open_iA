// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <QList>
#include <QModelIndex>

class vtkIdTypeArray;
class QTreeWidgetItem;

struct iASelection
{
	iASelection();
	void setPDMIndices( QModelIndexList & inds);
	void setTreeItems( QList<QTreeWidgetItem*> * newItems );
	void setIds( vtkIdTypeArray  * newIds );

	vtkVector2i spmCurPlot;
	QModelIndexList pdmSelection;
	QList<QTreeWidgetItem*> treeItems;
	vtkSmartPointer<vtkIdTypeArray>  ids;
	QString selText;
};
