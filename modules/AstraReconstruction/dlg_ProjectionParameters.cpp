// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_ProjectionParameters.h"

dlg_ProjectionParameters::dlg_ProjectionParameters()
{
	setupUi(this);
	connect(cbAlgorithmType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_ProjectionParameters::algorithmChanged);
	connect(cbCorrectCenterOfRotation, &QCheckBox::stateChanged, this, &dlg_ProjectionParameters::centerOfRotationEnabled);
}


void dlg_ProjectionParameters::fillProjectionGeometryValues(QString const & projGeomType, double detSpacingX, double detSpacingY, int detRowCnt, int detColCnt,
	double projAngleStart, double projAngleEnd, int projAnglesCount, double distOrigDet, double distOrigSource)
{
	gbVolumeGeometry->hide();
	gbProjectionInput->hide();
	gbAlgorithm->hide();
	gbCorrections->hide();
	cbProjGeomType->setCurrentText(projGeomType);
	sbProjGeomDetectorPixelsX->setValue(detColCnt);
	sbProjGeomDetectorPixelsY->setValue(detRowCnt);
	dsbProjGeomDetectorSpacingX->setValue(detSpacingX);
	dsbProjGeomDetectorSpacingY->setValue(detSpacingY);
	dsbProjGeomProjAngleStart->setValue(projAngleStart);
	dsbProjGeomProjAngleEnd->setValue(projAngleEnd);
	sbProjGeomProjCount->setValue(projAnglesCount);
	dsbProjGeomDistOriginDetector->setValue(distOrigDet);
	dsbProjGeomDistOriginSource->setValue(distOrigSource);
}


void dlg_ProjectionParameters::fillProjectionGeometryValues(QString const & projGeomType, double detSpacingX, double detSpacingY,
	double projAngleStart, double projAngleEnd, double distOrigDet, double distOrigSource)
{
	lbProjGeomDetectorPixels->hide();
	sbProjGeomDetectorPixelsX->hide();
	lbProjGeomDetectorPixelsX->hide();
	sbProjGeomDetectorPixelsY->hide();
	lbProjGeomDetectorPixelsY->hide();
	sbProjGeomProjCount->hide();
	lbProjGeomProjCount->hide();
	cbProjGeomType->setCurrentText(projGeomType);
	dsbProjGeomDetectorSpacingX->setValue(detSpacingX);
	dsbProjGeomDetectorSpacingY->setValue(detSpacingY);
	dsbProjGeomProjAngleStart->setValue(projAngleStart);
	dsbProjGeomProjAngleEnd->setValue(projAngleEnd);
	dsbProjGeomDistOriginDetector->setValue(distOrigDet);
	dsbProjGeomDistOriginSource->setValue(distOrigSource);
}


void dlg_ProjectionParameters::fillVolumeGeometryValues(int dim[3], double spacing[3])
{
	sbVolGeomDimensionX->setValue(dim[0]);
	sbVolGeomDimensionY->setValue(dim[1]);
	sbVolGeomDimensionZ->setValue(dim[2]);
	dsbVolGeomSpacingX->setValue(spacing[0]);
	dsbVolGeomSpacingY->setValue(spacing[1]);
	dsbVolGeomSpacingZ->setValue(spacing[2]);
}


QStringList dlg_ProjectionParameters::GetDimStringList(int const imgDims[3])
{
	return QStringList()
		<< QString("x (%2)").arg(imgDims[0]) << QString("y (%2)").arg(imgDims[1]) << QString("z (%2)").arg(imgDims[2])
		<< QString("-x (%2)").arg(imgDims[0]) << QString("-y (%2)").arg(imgDims[1]) << QString("-z (%2)").arg(imgDims[2]);
}


void dlg_ProjectionParameters::fillProjInputMapping(int detRowDim, int detColDim, int projAngleDim, const int dim[3])
{
	cbProjInputDetectorRowDim->addItems(GetDimStringList(dim));
	cbProjInputDetectorRowDim->setCurrentIndex(detRowDim);
	cbProjInputDetectorColDim->addItems(GetDimStringList(dim));
	cbProjInputDetectorColDim->setCurrentIndex(detColDim);
	cbProjInputProjAngleDim->addItems(GetDimStringList(dim));
	cbProjInputProjAngleDim->setCurrentIndex(projAngleDim);
}


void dlg_ProjectionParameters::fillAlgorithmValues(int algorithmType, int numberOfIterations, bool initWithFDK)
{
	cbAlgorithmType->setCurrentIndex(algorithmType);
	sbAlgorithmIterations->setValue(numberOfIterations);
	cbInitWithFDK->setChecked(initWithFDK);
	algorithmChanged(algorithmType);
}


void dlg_ProjectionParameters::fillCorrectionValues(bool correctCenterOfRotation, double correctCenterOfRotationOffset)
{	
	cbCorrectCenterOfRotation->setChecked(correctCenterOfRotation);
	dsbCorrectCenterOfRotationOffset->setValue(correctCenterOfRotationOffset);
	centerOfRotationEnabled(correctCenterOfRotation ? Qt::Checked : Qt::Unchecked);
}


int dlg_ProjectionParameters::exec()
{
	resize(width(), minimumSizeHint().height());
	return QDialog::exec();
}


void dlg_ProjectionParameters::checkCenterOfRotationCorrection(int /*algoIdx*/, bool /*centerOfRotCorr*/)
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
	sbAlgorithmIterations->setVisible(idx > 1); // depends on the order of algorithms!
	lbAlgorithmIterations->setVisible(idx > 1);
	cbInitWithFDK->setVisible(idx > 1);
	checkCenterOfRotationCorrection(idx, cbCorrectCenterOfRotation->isChecked());
}


void dlg_ProjectionParameters::centerOfRotationEnabled(int state)
{
	dsbCorrectCenterOfRotationOffset->setEnabled(state == Qt::Checked);
	checkCenterOfRotationCorrection(cbAlgorithmType->currentIndex(), state == Qt::Checked);
}
