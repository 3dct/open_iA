// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAstraFilterRunner.h"

#include "dlg_ProjectionParameters.h"

#include "iAAstraAlgorithm.h"

#include <iALog.h>
#include <iACudaHelper.h>

#include <iAMainWindow.h>
#include <iAMdiChild.h>

#include <astra/Logging.h>

#include <vtkImageData.h>

#include <QMessageBox>

IAFILTER_RUNNER_CREATE(iAAstraFilterRunner);

namespace
{
	void astraLogCallback(const char* msg, size_t len)
	{
		char* allMsg = new char[len + 1];
		std::memcpy(allMsg, msg, len);
		allMsg[len] = 0;
		QString qtStr(allMsg);
		LOG(lvlInfo, qtStr.trimmed());
	}
}

void iAAstraFilterRunner::run(std::shared_ptr<iAFilter> filter, iAMainWindow* mainWnd)
{
	if (!isCUDAAvailable())
	{
		QMessageBox::warning(mainWnd, "ASTRA",
			"ASTRA toolbox operations require a CUDA-capable device, but no CUDA device was found."
			"In case this machine has an NVidia card, it might help to install the latest driver!");
		return;
	}
	astra::CLogger::setOutputScreen(1, astra::LOG_INFO);
	bool success = astra::CLogger::setCallbackScreen(astraLogCallback);
	if (!success)
	{
		LOG(lvlWarn, "Setting Astra log callback failed!");
	}
	iAFilterRunnerGUI::run(filter, mainWnd);
}

bool iAAstraFilterRunner::askForParameters(std::shared_ptr<iAFilter> filter, QVariantMap& parameters,
	iAMdiChild* sourceMdi, iAMainWindow* mainWnd, bool /*askForAdditionalInput*/)
{
	dlg_ProjectionParameters dlg;
	dlg.setWindowTitle(filter->name());
	auto img = sourceMdi->firstImageData();
	if (!img)
	{
		return false;
	}
	int const* inputDim = img->GetDimensions();
	if (filter->name() == "ASTRA Forward Projection")
	{
		dlg.fillProjectionGeometryValues(
			parameters[AstraParameters::ProjGeometry].toString(),
			parameters[AstraParameters::DetSpcX].toDouble(),
			parameters[AstraParameters::DetSpcY].toDouble(),
			parameters[AstraParameters::DetRowCnt].toUInt(),
			parameters[AstraParameters::DetColCnt].toUInt(),
			parameters[AstraParameters::ProjAngleStart].toDouble(),
			parameters[AstraParameters::ProjAngleEnd].toDouble(),
			parameters[AstraParameters::ProjAngleCnt].toUInt(),
			parameters[AstraParameters::DstOrigDet].toDouble(),
			parameters[AstraParameters::DstOrigSrc].toDouble());
	}
	else
	{
		dlg.fillProjectionGeometryValues(
			parameters[AstraParameters::ProjGeometry].toString(),
			parameters[AstraParameters::DetSpcX].toDouble(),
			parameters[AstraParameters::DetSpcY].toDouble(),
			parameters[AstraParameters::ProjAngleStart].toDouble(),
			parameters[AstraParameters::ProjAngleEnd].toDouble(),
			parameters[AstraParameters::DstOrigDet].toDouble(),
			parameters[AstraParameters::DstOrigSrc].toDouble());
		int volDim[3] = {
			parameters[AstraParameters::VolDimX].toInt(),
			parameters[AstraParameters::VolDimY].toInt(),
			parameters[AstraParameters::VolDimZ].toInt()
		};
		double volSpacing[3] = {
			parameters[AstraParameters::VolSpcX].toDouble(),
			parameters[AstraParameters::VolSpcY].toDouble(),
			parameters[AstraParameters::VolSpcZ].toDouble()
		};
		dlg.fillVolumeGeometryValues(volDim, volSpacing);
		dlg.fillProjInputMapping(parameters[AstraParameters::DetRowDim].toInt(),
			parameters[AstraParameters::DetColDim].toInt(),
			parameters[AstraParameters::ProjAngleDim].toInt(),
			inputDim);
		if (parameters[AstraParameters::AlgoType].toString().isEmpty())
		{
			parameters[AstraParameters::AlgoType] = AstraParameters::algorithmStrings()[1];
		}
		dlg.fillAlgorithmValues(AstraParameters::mapAlgoStringToIndex(parameters[AstraParameters::AlgoType].toString()),
			parameters[AstraParameters::NumberOfIterations].toUInt(),
			parameters[AstraParameters::InitWithFDK].toBool());
		dlg.fillCorrectionValues(parameters[AstraParameters::CenterOfRotCorr].toBool(),
			parameters[AstraParameters::CenterOfRotOfs].toDouble());
	}
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}

	parameters[AstraParameters::ProjGeometry] = dlg.cbProjGeomType->currentText();
	parameters[AstraParameters::DetSpcX] = dlg.dsbProjGeomDetectorSpacingX->value();
	parameters[AstraParameters::DetSpcY] = dlg.dsbProjGeomDetectorSpacingY->value();
	parameters[AstraParameters::ProjAngleStart] = dlg.dsbProjGeomProjAngleStart->value();
	parameters[AstraParameters::ProjAngleEnd] = dlg.dsbProjGeomProjAngleEnd->value();
	parameters[AstraParameters::DstOrigDet] = dlg.dsbProjGeomDistOriginDetector->value();
	parameters[AstraParameters::DstOrigSrc] = dlg.dsbProjGeomDistOriginSource->value();
	if (filter->name() == "ASTRA Forward Projection")
	{
		parameters[AstraParameters::DetRowCnt] = dlg.sbProjGeomDetectorPixelsY->value();
		parameters[AstraParameters::DetColCnt] = dlg.sbProjGeomDetectorPixelsX->value();
		parameters[AstraParameters::ProjAngleCnt] = dlg.sbProjGeomProjCount->value();
	}
	else    // Reconstruction:
	{
		int detRowDim = dlg.cbProjInputDetectorRowDim->currentIndex();
		int detColDim = dlg.cbProjInputDetectorColDim->currentIndex();
		int projAngleDim = dlg.cbProjInputProjAngleDim->currentIndex();
		parameters[AstraParameters::VolDimX] = dlg.sbVolGeomDimensionX->value();
		parameters[AstraParameters::VolDimY] = dlg.sbVolGeomDimensionY->value();
		parameters[AstraParameters::VolDimZ] = dlg.sbVolGeomDimensionZ->value();
		parameters[AstraParameters::VolSpcX] = dlg.dsbVolGeomSpacingX->value();
		parameters[AstraParameters::VolSpcY] = dlg.dsbVolGeomSpacingY->value();
		parameters[AstraParameters::VolSpcZ] = dlg.dsbVolGeomSpacingZ->value();
		parameters[AstraParameters::AlgoType] = AstraParameters::mapAlgoIndexToString(dlg.cbAlgorithmType->currentIndex());
		parameters[AstraParameters::NumberOfIterations] = dlg.sbAlgorithmIterations->value();
		parameters[AstraParameters::InitWithFDK] = dlg.cbInitWithFDK->isChecked();
		parameters[AstraParameters::CenterOfRotCorr] = dlg.cbCorrectCenterOfRotation->isChecked();
		parameters[AstraParameters::CenterOfRotOfs] = dlg.dsbCorrectCenterOfRotationOffset->value();
		if ((detColDim % 3) == (detRowDim % 3) || (detColDim % 3) == (projAngleDim % 3) || (detRowDim % 3) == (projAngleDim % 3))
		{
			QMessageBox::warning(mainWnd, "ASTRA", "One of the axes (x, y, z) has been specified for more than one usage out of "
				"(detector row / detector column / projection angle) dimensions. "
				"Make sure each axis is used exactly for one dimension!");
			return false;
		}
		parameters[AstraParameters::DetRowDim] = detRowDim;
		parameters[AstraParameters::DetColDim] = detColDim;
		parameters[AstraParameters::ProjAngleDim] = projAngleDim;
	}
	return true;
}
