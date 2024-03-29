// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QDockWidget>

class QStackedLayout;
class QLabel;

class iAMdiChild;
class iATripleModalityWidget;
class iABCoord;

class vtkSmartVolumeMapper;
class vtkVolume;
class vtkRenderer;


class dlg_tf_3mod : public QDockWidget
{
	Q_OBJECT

public:
	dlg_tf_3mod(iAMdiChild* parent, Qt::WindowFlags f = Qt::WindowFlags());

private:
	iATripleModalityWidget *m_tripleModalityWidget;
};
