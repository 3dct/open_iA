// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <vtkSmartPointer.h>

#include <QString>
#include <QObject>

class dlg_RefSpectra;
class dlg_SimilarityMap;
class dlg_InSpectr;

class iADockWidgetWrapper;
class iAMainWindow;
class iASlicer;

class vtkPiecewiseFunction;

class QThread;

class iAInSpectrTool : public QObject, public iATool
{
	Q_OBJECT

public:
	static const QString Name;
	iAInSpectrTool( iAMainWindow * mainWnd, iAMdiChild * child );

private slots:
	void visualizeXRF( int isOn );
	void updateXRFOpacity( int value );
	void updateXRF();
	void updateXRFVoxelEnergy(double x, double y, double z, int mode );
	void reInitXRF();
	void initXRF();
	void deinitXRF();
	void initXRF( bool enableChannel );
	bool filter_SimilarityMap();
	void magicLensToggled( bool isOn );

private:
	void updateSlicerXRFOpacity();
	QThread* recalculateXRF();
	void initSlicerXRF( bool enableChannel );

	iADockWidgetWrapper* dlgPeriodicTable;
	dlg_RefSpectra* dlgRefSpectra;
	dlg_SimilarityMap * dlgSimilarityMap;
	dlg_InSpectr * dlgXRF;
	uint m_xrfChannelID;
};
