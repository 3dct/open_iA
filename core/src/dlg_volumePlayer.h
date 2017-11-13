/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "ui_VolumePlayer.h"
#include <QtGui/QPalette>
#include <QTimer>
#include <vector>
#include <vtkSmartPointer.h>

class MdiChild;
class iAVolumeStack;

class vtkPiecewiseFunction;

const int CHANNELS_COUNT = 2;

class dlg_volumePlayer : public QDockWidget, public Ui_VolumePlayer
{
	Q_OBJECT

public:
	dlg_volumePlayer(QWidget *parent, iAVolumeStack *volumeStack);
	~dlg_volumePlayer();

private:
	float				getCurrentSpeed();
	int					volumeIndexToSlicerIndex(int volumeIndex);
	int					sliderIndexToVolumeIndex(int slicerIndex);
	QList<int>			getCheckedList();
	int					getNumberOfCheckedVolumes();
	void				showVolume(int volumeIndex);
	void				hideVolume(int volumeIndex);
	bool				volumeIsShown(int volumeIndex);
	void				enableMultiChannelVisualization();
	void				disableMultiChannelVisualization();
	void				setMultiChannelVisualization(int volumeIndex1, int volumeIndex2, double blendingCoeff);
	void				updateMultiChannelVisualization();

	int					m_numberOfVolumes;
	int					m_volumePlayerSpeed;
	QWidget*			m_parent;
	bool				m_isBlendingOn;
	QTimer				m_timer;
	QStringList			m_widgetList;
	QStringList			m_newWidgetList;
	QList <int>			m_outCheckList;
	QStringList			m_list;
	QWidget*			m_newWidget;
	QString				m_tempStr;
	int					m_old_r;
	int					m_numberOfColumns;
	int					m_numberOfRows;
	bool				m_areAllChecked;
	int					m_dimColumn, m_spacColumn, m_fileColumn, m_checkColumn, m_sortColumn;
	bool				m_dimShow, m_spacShow, m_fileShow;
	QAction*			m_contextDimensions;
	QAction*			m_contextSpacing;
	QAction*			m_contextFileName;
	int					m_selectedMaxSpeed;
	short				m_mask;
	std::vector<QCheckBox*>	m_checkBoxes;
	iAVolumeStack*		m_volumeStack;
	int					m_enabledChanels;
	MdiChild*			m_mdiChild;
	vtkSmartPointer<vtkPiecewiseFunction>	m_otf[CHANNELS_COUNT];
	bool				m_multiChannelIsInitialized;

Q_SIGNALS:
	void update(int index, bool isApplyForAll=false);
	void setAllSelected(int c=0);
	void editSpeed();

protected slots:
	void nextVolume();
	void previousVolume();
	void sliderChanged();
	void playVolume();
	void pauseVolume();
	void stopVolume();
	void setSpeed();
	void editMaxSpeed();
	void setChecked(int r, int c);
	void updateView(int r, int c);
	void selectAll(int c);
	void fileNameActive();
	void spacingActive();
	void dimensionsActive();
	void applyForAll();
	void blendingStateChanged(int state);
	void enableVolume(int state);
};
