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

#include "iAModuleInterface.h"

class MdiChild;
class iAProbabilitySource;
class iAFilter;

class iASegmentationModuleInterface : public iAModuleInterface
{
	Q_OBJECT

public:
	void Initialize();

private slots:
	void binary_threshold();
	void otsu_Threshold_Filter();
	void maximum_Distance_Filter();
	void watershed_seg();
	void morph_watershed_seg();
	void adaptive_Otsu_Threshold_Filter();
	void rats_Threshold_Filter();
	void otsu_Multiple_Threshold_Filter();
	void fcm_seg();
	void kfcm_seg();
	void mskfcm_seg();
	bool CalculateSegmentationMetrics();
	
	void FuzzyCMeansFinished();
private:

	double btlower, btupper, btoutside, btinside;	//!< Binary threshold parameters
	
	//! @{ Otsu threshold parameters
	double otBins, otinside, otoutside;
	bool otremovepeaks;
	//! @}

	double mdfli, mdfbins; int mdfuli;				//!< Maximum distance filter parameters
	double wsLevel, wsThreshold;					//!< Watershed parameters

	//! @{ Morphological watershed parameters
	double mwsLevel;								
	bool mwsMarkWSLines, mwsFullyConnected;
	//! @}

	//! @{ Adaptive Otsu parametesr
	double aotBins, aotOutside, aotInside, aotRadius; 
	unsigned int aotSamples, aotLevels, aotControlpoints;
	//! @}

	double rtPow, rtOutside, rtInside;				//!< Rats threshold parameters
	double omtBins, omtThreshs, omtVe;				//!< Otsu multiple thresholds parameters

	//! @{ Fuzzy C-Means parameters
	unsigned int fcmMaxIter, fcmNumOfThreads, fcmNumOfClasses;
	double fcmMaxError, fcmM, fcmBgPixel;
	QString fcmCentroidString;
	bool fcmIgnoreBg;
	iAProbabilitySource* m_probSource;
	//! @}

	void StartFCMThread(QSharedPointer<iAFilter> filter);
};
