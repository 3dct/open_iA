// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <vtkSmartPointer.h>

#include <qtypes.h>

#include <array>
#include <map>

class iAvtkPlaneWidget;
class vtkImageResliceMapper;
class vtkImageSlice;

class iADockWidgetWrapper;
class iAMainWindow;
class iAMdiChild;
class iAQVTKWidget;

class iASnapshotInfo
{
public:
	std::array<float, 3> position;
	std::array<float, 4> rotation;
};

enum class iAMoveAxis: quint8
{
	X,
	Y,
	Z
};

class iAPlaneSliceTool : public iATool
{
public:
	static const QString Name;
	iAPlaneSliceTool(iAMainWindow* mainWnd, iAMdiChild* child);
	quint64 addSnapshot(iASnapshotInfo info);
	void removeSnapshot(quint64 id);
	void clearSnapshots();
	void moveSlice(quint64 id, iAMoveAxis axis, float value);
	~iAPlaneSliceTool();
private:
	iAQVTKWidget* m_sliceWidget;
	iADockWidgetWrapper* m_dw;
	vtkSmartPointer<iAvtkPlaneWidget> m_planeWidget;
	vtkSmartPointer<vtkImageResliceMapper> m_reslicer;
	vtkSmartPointer<vtkImageSlice> m_imageSlice;
	std::map<quint64, iASnapshotInfo> m_snapshots;
	quint64 m_nextSnapshotID;
};
