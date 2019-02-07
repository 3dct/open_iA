/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "ui_ProjectionParameters.h"

#include <QDialog>

class dlg_ProjectionParameters : public QDialog, public Ui_ProjectionParameters
{
	Q_OBJECT
public:
	dlg_ProjectionParameters();
	void fillProjectionGeometryValues(QString const & projGeomType, double detSpacingX, double detSpacingY, int detRowCnt, int detColCnt,
		double projAngleStart, double projAngleEnd, int projAnglesCount, double distOrigDet, double distOrigSource);
	void fillProjectionGeometryValues(QString const & projGeomType, double detSpacingX, double detSpacingY,
		double projAngleStart, double projAngleEnd, double distOrigDet, double distOrigSource);
	void fillVolumeGeometryValues(int dim[3], double spacing[3]);
	void fillProjInputMapping(int detRowDim, int detColDim, int projAngleDim, const int dim[3]);
	void fillAlgorithmValues(int algorithmType, int numberOfIterations);
	void fillCorrectionValues(bool correctCenterOfRotation, double correctCenterOfRotationOffset);
	int exec() override;
private slots:
	void algorithmChanged(int idx);
	void centerOfRotationEnabled(int);
private:
	QStringList GetDimStringList(int const imgDims[3]);
	void checkCenterOfRotationCorrection(int algoIdx, bool centerOfRotCorr);
};
