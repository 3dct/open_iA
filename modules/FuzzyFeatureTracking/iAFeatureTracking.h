/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
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
#pragma once

#include <string>
#include <vector>

#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkVariantArray.h>

#include "iAFeatureTrackingCorrespondence.h"

class iAFeatureTracking
{
private:
	std::string file1;
	std::string file2;
	int lineOffset;
	vtkTable *u;
	vtkTable *v;
	float dissipationThreshold;
	float overlapThreshold;
	float volumeThreshold;
	float overallMatchingPercentage;
	int maxSearchValue;
	std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > > *uToV;
	std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > > *vToU;
	std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > > *allUtoV;
	std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > > *allVtoU;
	std::string outputFilename;
	std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
	std::vector<std::string> split(const std::string &s, char delim);
	vtkTable &readTableFromFile(const std::string &filename, int dataLineOffset);
	void sortCorrespondencesByOverlap(std::vector<iAFeatureTrackingCorrespondence> &correspondences);
	std::vector<iAFeatureTrackingCorrespondence>& getCorrespondences(const vtkVariantArray &row, vtkTable &table, int maxSearchValue, bool useZ);
	void ComputeOverallMatchingPercentage();

public:
	iAFeatureTracking(std::string fileName1, std::string fileName2, int lineOffset, std::string outputFilename, float dissipationThreshold,
		float overlapThreshold, float volumeThreshold, int maxSearchValue);
	void TrackFeatures();
	std::vector<iAFeatureTrackingCorrespondence> FromUtoV(unsigned int uId);
	std::vector<iAFeatureTrackingCorrespondence> FromVtoU(unsigned int vId);
	float GetOverallMatchingPercentage();
		
	size_t getNumberOfEventsInU();
	size_t getNumberOfEventsInV();
	vtkTable* getU();
	vtkTable* getV();
};
