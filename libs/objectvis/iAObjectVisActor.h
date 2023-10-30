// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaobjectvis_export.h"

#include <QObject>

class vtkRenderer;

//! Base class for visualizing a given iAObjectVis (which delivers the data for the 3D objects to visualize).
//! For each iAObjectVis-derived object you can create multiple iAObjectVisActor (for example through the
//! iAObjectVis::createActor method, or through specialized methods for that purpose in subclasses).
class iAobjectvis_API iAObjectVisActor : public QObject
{
	Q_OBJECT
public:
	iAObjectVisActor(vtkRenderer* ren);
	virtual void show();
	virtual void hide();
	virtual void updateRenderer();
	void clearRenderer();
signals:
	void updated();
protected:
	vtkRenderer* m_ren;
};
// implementations: see end of iAObjectVis.cpp
