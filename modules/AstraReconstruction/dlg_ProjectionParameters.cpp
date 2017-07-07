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
#include "dlg_ProjectionParameters.h"


dlg_ProjectionParameters::dlg_ProjectionParameters()
{
	setupUi(this);
	connect(AlgorithmType, SIGNAL(currentIndexChanged(int)), this, SLOT(algorithmChanged(int)));
}


void dlg_ProjectionParameters::algorithmChanged(int idx)
{
	AlgorithmIterations->setVisible(idx > 1); // depends on the order of algorithms!
	AlgorithmIterationsLabel->setVisible(idx > 1);
}


void dlg_ProjectionParameters::fillProjectionGeometryValues(QString const & projGeomType, double detSpacingX, double detSpacingY, int detRowCnt, int detColCnt,
	double projAngleStart, double projAngleEnd, int projAnglesCount, double distOrigDet, double distOrigSource)
{
	gbVolumeGeometry->hide();
	gbProjectionInput->hide();
	gbAlgorithm->hide();
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
	VolGeomDimensionX->setText(QString("%1").arg(dim[0]));
	VolGeomDimensionY->setText(QString("%1").arg(dim[1]));
	VolGeomDimensionZ->setText(QString("%1").arg(dim[2]));
	VolGeomSpacingX->setText(QString("%1").arg(spacing[0]));
	VolGeomSpacingY->setText(QString("%1").arg(spacing[1]));
	VolGeomSpacingZ->setText(QString("%1").arg(spacing[2]));
}


QStringList dlg_ProjectionParameters::GetDimStringList(int const imgDims[3])
{
	return QStringList()
		<< QString("x (%2)").arg(imgDims[0]) << QString("y (%2)").arg(imgDims[1]) << QString("z (%2)").arg(imgDims[2])
		<< QString("-x (%2)").arg(imgDims[0]) << QString("-y (%2)").arg(imgDims[1]) << QString("-z (%2)").arg(imgDims[2]);
}


void dlg_ProjectionParameters::fillProjInputMapping(int detRowDim, int detColDim, int projAngleDim, int dim[3])
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
}


int dlg_ProjectionParameters::exec()
{
	resize(width(), minimumSizeHint().height());
	return QDialog::exec();
}