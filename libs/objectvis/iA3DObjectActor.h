// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAobjectvis_export.h"

#include <QObject>

class vtkRenderer;

//! Base class for visualizing a given iA3DObjectVis (which delivers the data for the 3D objects to visualize).
//! For each iA3DObjectVis-derived object you can create multiple iA3DObjectActor (for example through the
//! iA3DObjectVis::createActor method, or through specialized methods for that purpose in subclasses).
class iAobjectvis_API iA3DObjectActor : public QObject
{
	Q_OBJECT
public:
	iA3DObjectActor(vtkRenderer* ren);
	virtual void show();
	virtual void updateRenderer();
	void clearRenderer();
signals:
	void updated();
protected:
	vtkRenderer* m_ren;
};
// implementations: see end of iA3DObjectVis.cpp
