// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <vtkSmartPointer.h>

#include <QObject>
#include <QString>
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

class QTableWidget;
class QWidget;

class iASnapshotInfo
{
public:
	std::array<float, 3> position;
	std::array<float, 4> rotation;
};

Q_DECLARE_METATYPE(iASnapshotInfo);

enum class iAMoveAxis: quint8
{
	X,
	Y,
	Z
};

class iAPlaneSliceTool : public QObject, public iATool
{
	Q_OBJECT
public:
	static const QString Name;
	iAPlaneSliceTool(iAMainWindow* mainWnd, iAMdiChild* child);
	quint64 addSnapshot(iASnapshotInfo info);
	void removeSnapshot(quint64 id);
	void clearSnapshots();
	void moveSlice(quint64 id, iAMoveAxis axis, float value);
	~iAPlaneSliceTool();
signals:
	void snapshotRemoved(quint64 id);
	void snapshotAdded(quint64 id, iASnapshotInfo const & info);
	void snapshotsCleared();
private:
	iAQVTKWidget* m_sliceWidget;
	QWidget* m_listContainerWidget;
	QTableWidget* m_snapshotTable;
	iADockWidgetWrapper * m_sliceDW, * m_listDW;
	vtkSmartPointer<iAvtkPlaneWidget> m_planeWidget;
	vtkSmartPointer<vtkImageResliceMapper> m_reslicer;
	vtkSmartPointer<vtkImageSlice> m_imageSlice;
	std::map<quint64, iASnapshotInfo> m_snapshots;
	quint64 m_nextSnapshotID;
};

// TODO: generalize / move to e.g. iAStringHelper? 
template <size_t N>
QString array2string(std::array<float, N> ar)
{
	QString result;
	for (int i = 0; i < N; ++i)
	{
		result += QString::number(ar[i]);
		if (i < N - 1)
		{
			result += QString(", ");
		}
	}
	return result;
}