// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	void fillAlgorithmValues(int algorithmType, int numberOfIterations, bool initWithFDK);
	void fillCorrectionValues(bool correctCenterOfRotation, double correctCenterOfRotationOffset);
	int exec() override;
private slots:
	void algorithmChanged(int idx);
	void centerOfRotationEnabled(int);
private:
	QStringList GetDimStringList(int const imgDims[3]);
	void checkCenterOfRotationCorrection(int algoIdx, bool centerOfRotCorr);
};
