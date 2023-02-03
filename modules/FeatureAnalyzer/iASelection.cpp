// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASelection.h"

#include <vtkIdTypeArray.h>

#include <QTreeWidget>

iASelection::iASelection()
	: ids( vtkSmartPointer<vtkIdTypeArray>::New() )
{
	ids->SetNumberOfComponents( 1 );
}

void iASelection::setTreeItems( QList<QTreeWidgetItem*> * newItems )
{
	treeItems = QList<QTreeWidgetItem*>( *newItems );
}

void iASelection::setIds( vtkIdTypeArray * newIds )
{
	ids = vtkSmartPointer<vtkIdTypeArray>::New();
	if( !newIds )
		return;
	ids->DeepCopy( newIds );
}

void iASelection::setPDMIndices( QModelIndexList & inds )
{
	pdmSelection = inds;
}
