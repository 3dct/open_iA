/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
