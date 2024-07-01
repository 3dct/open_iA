// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iASlicerMode.h>    // for iAAxisIndex
#include <iATool.h>

#include <vtkSmartPointer.h>

#include <QObject>
#include <QString>

#include <array>
#include <map>

class iAvtkPlaneWidget;
class vtkImageResliceMapper;
class vtkImageSlice;

class iADockWidgetWrapper;
class iAMainWindow;
class iAMdiChild;
class iAQCropLabel;
class iAQVTKWidget;

class QTableWidget;
class QWidget;

class iASnapshotInfo
{
public:
	std::array<float, 3> position;
	std::array<float, 3> normal;
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
	static std::shared_ptr<iATool> create(iAMainWindow* mainWnd, iAMdiChild* child);
	iAPlaneSliceTool(iAMainWindow* mainWnd, iAMdiChild* child);
	~iAPlaneSliceTool();
	void loadState(QSettings& projectFile, QString const& fileName) override;
	void saveState(QSettings& projectFile, QString const& fileName) override;

	std::pair<quint64, int> addSnapshot(iASnapshotInfo info);
	std::map<quint64, iASnapshotInfo> const & snapshots();
	void removeSnapshot(quint64 id);
	void clearSnapshots();

signals:
	void snapshotRemoved(quint64 id);
	void snapshotAdded(quint64 id, iASnapshotInfo const & info);
	void snapshotsCleared();

private:
	void updateSlice();
	void updateSliceFromUser();
	void resetPlaneParameters(iAMdiChild* child, iAAxisIndex axis, bool posSign);

	iAQVTKWidget* m_sliceWidget;
	QTableWidget* m_snapshotTable;
	iAQCropLabel* m_curPosLabel;
	iADockWidgetWrapper * m_sliceDW, * m_listDW;
	vtkSmartPointer<iAvtkPlaneWidget> m_planeWidget;
	vtkSmartPointer<vtkImageResliceMapper> m_reslicer;
	vtkSmartPointer<vtkImageSlice> m_imageSlice;
	std::map<quint64, iASnapshotInfo> m_snapshots;
	quint64 m_nextSnapshotID;
};

// TODO: better generalization / move to some common qt table widget helper file:
bool removeTableEntry(QTableWidget* tw, quint64 id);
