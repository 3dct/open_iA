/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "dlg_ProjectionParameters.h"


dlg_ProjectionParameters::dlg_ProjectionParameters()
{
	setupUi(this);
	connect(AlgorithmType, SIGNAL(currentIndexChanged(int)), this, SLOT(algorithmChanged(int)));
	connect(CorrectionCenterOfRotation, SIGNAL(stateChanged(int)), this, SLOT(centerOfRotationEnabled(int)));
}


void dlg_ProjectionParameters::fillProjectionGeometryValues(QString const & projGeomType, double detSpacingX, double detSpacingY, int detRowCnt, int detColCnt,
	double projAngleStart, double projAngleEnd, int projAnglesCount, double distOrigDet, double distOrigSource)
{
	gbVolumeGeometry->hide();
	gbProjectionInput->hide();
	gbAlgorithm->hide();
	gbCorrections->hide();
	ProjGeomType->setCurrentText(projGeomType);
	ProjGeomDetectorPixelsX->setValue(detColCnt);
	ProjGeomDetectorPixelsY->setValue(detRowCnt);
	ProjGeomDetectorSpacingX->setValue(detSpacingX);
	ProjGeomDetectorSpacingY->setValue(detSpacingY);
	ProjGeomProjAngleStart->setValue(projAngleStart);
	ProjGeomProjAngleEnd->setValue(projAngleEnd);
	ProjGeomProjCount->setValue(projAnglesCount);
	ProjGeomDistOriginDetector->setValue(distOrigDet);
	ProjGeomDistOriginSource->setValue(distOrigSource);
}


void dlg_ProjectionParameters::fillProjectionGeometryValues(QString const & projGeomType, double detSpacingX, double detSpacingY,
	double projAngleStart, double projAngleEnd, double distOrigDet, double distOrigSource)
{
	ProjGeomDetectorPixelsLabel->hide();
	ProjGeomDetectorPixelsX->hide();
	ProjGeomDetectorPixelsXLabel->hide();
	ProjGeomDetectorPixelsY->hide();
	ProjGeomDetectorPixelsYLabel->hide();
	ProjGeomProjCount->hide();
	ProjGeomProjCountLabel->hide();
	ProjGeomType->setCurrentText(projGeomType);
	ProjGeomDetectorSpacingX->setValue(detSpacingX);
	ProjGeomDetectorSpacingY->setValue(detSpacingY);
	ProjGeomProjAngleStart->setValue(projAngleStart);
	ProjGeomProjAngleEnd->setValue(projAngleEnd);
	ProjGeomDistOriginDetector->setValue(distOrigDet);
	ProjGeomDistOriginSource->setValue(distOrigSource);
}


void dlg_ProjectionParameters::fillVolumeGeometryValues(int dim[3], double spacing[3])
{
	VolGeomDimensionX->setValue(dim[0]);
	VolGeomDimensionY->setValue(dim[1]);
	VolGeomDimensionZ->setValue(dim[2]);
	VolGeomSpacingX->setValue(spacing[0]);
	VolGeomSpacingY->setValue(spacing[1]);
	VolGeomSpacingZ->setValue(spacing[2]);
}


QStringList dlg_ProjectionParameters::GetDimStringList(int const imgDims[3])
{
	return QStringList()
		<< QString("x (%2)").arg(imgDims[0]) << QString("y (%2)").arg(imgDims[1]) << QString("z (%2)").arg(imgDims[2])
		<< QString("-x (%2)").arg(imgDims[0]) << QString("-y (%2)").arg(imgDims[1]) << QString("-z (%2)").arg(imgDims[2]);
}


void dlg_ProjectionParameters::fillProjInputMapping(int detRowDim, int detColDim, int projAngleDim, const int dim[3])
{
	ProjInputDetectorRowDim->addItems(GetDimStringList(dim));
	ProjInputDetectorRowDim->setCurrentIndex(detRowDim);
	ProjInputDetectorColDim->addItems(GetDimStringList(dim));
	ProjInputDetectorColDim->setCurrentIndex(detColDim);
	ProjInputProjAngleDim->addItems(GetDimStringList(dim));
	ProjInputProjAngleDim->setCurrentIndex(projAngleDim);
}


void dlg_ProjectionParameters::fillAlgorithmValues(int algorithmType, int numberOfIterations)
{
	AlgorithmType->setCurrentIndex(algorithmType);
	AlgorithmIterations->setValue(numberOfIterations);
	algorithmChanged(algorithmType);
}


void dlg_ProjectionParameters::fillCorrectionValues(bool correctCenterOfRotation, double correctCenterOfRotationOffset)
{
	CorrectionCenterOfRotation->setChecked(correctCenterOfRotation);
	CorrectionCenterOfRotationOffset->setValue(correctCenterOfRotationOffset);
	centerOfRotationEnabled(correctCenterOfRotation ? Qt::Checked : Qt::Unchecked);
}


int dlg_ProjectionParameters::exec()
{
	resize(width(), minimumSizeHint().height());
	return QDialog::exec();
}


void dlg_ProjectionParameters::checkCenterOfRotationCorrection(int algoIdx, bool centerOfRotCorr)
{
	/*
	bool invalidState = algoIdx < 2 && centerOfRotCorr;
	if (invalidState)
	{
		CorrectionHint->setText("Center of Rotation correction only works with SIRT and CGLS reconstruction algorithms!");
	}
	CorrectionHint->setVisible(invalidState);
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!invalidState);
	*/
}


void dlg_ProjectionParameters::algorithmChanged(int idx)
{
	AlgorithmIterations->setVisible(idx > 1); // depends on the order of algorithms!
	AlgorithmIterationsLabel->setVisible(idx > 1);
	checkCenterOfRotationCorrection(idx, CorrectionCenterOfRotation->isChecked());
}


void dlg_ProjectionParameters::centerOfRotationEnabled(int state)
{
	CorrectionCenterOfRotationOffset->setEnabled(state == Qt::Checked);
	checkCenterOfRotationCorrection(AlgorithmType->currentIndex(), state == Qt::Checked);
}
