// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iATool.h"

#include <vtkSmartPointer.h>

#include <QDockWidget>
#include <QTimer>

#include <vector>

class iAVolumeViewer;
class iAMdiChild;

class QCheckBox;

//class vtkPiecewiseFunction;
//const int CHANNELS_COUNT = 2;


class Ui_VolumePlayer;

class iAVolumePlayerWidget : public QDockWidget
{
	Q_OBJECT

public:
	iAVolumePlayerWidget(iAMdiChild* child, std::vector<iAVolumeViewer*> const& volumes);

private:
	float currentSpeed() const;
	size_t listToVolumeIndex(int listIndex);
	int getNumberOfCheckedVolumes();
	void adjustSliderMax();
	bool m_isBlendingOn;
	QTimer m_timer;
	int m_old_r;
	int m_dimColumn, m_spacColumn, m_fileColumn, m_checkColumn, m_sortColumn;
	bool m_dimShow, m_spacShow, m_fileShow;
	QAction* m_contextDimensions;
	QAction* m_contextSpacing;
	QAction* m_contextFileName;
	int m_selectedMaxSpeed;
	std::vector<QCheckBox*> m_checkBoxes;
	std::vector<iAVolumeViewer*> m_volumeViewers;
	std::unique_ptr<Ui_VolumePlayer> m_ui;
	int m_prevStep;
	std::pair<size_t, size_t> m_prevVolIdx;
	iAMdiChild* m_child;

signals:
	void setAllSelected(int c = 0);

private slots:
	void nextVolume();
	void previousVolume();
	void sliderChanged();
	void playVolume();
	void pauseVolume();
	void stopVolume();
	void setSpeed();
	void editMaxSpeed();
	void setChecked(int r, int c);
	//void updateView(int r, int c);
	void selectAll(int c);
	void fileNameActive();
	void spacingActive();
	void dimensionsActive();
	void blendingStateChanged(int state);
	void enableVolume(int state);
	void applyForAll();
};


class iAVolumePlayerTool : public iATool
{
public:
	iAVolumePlayerTool(iAMainWindow* wnd, iAMdiChild* child);
private:
	iAVolumePlayerWidget* m_volumePlayer;
};
