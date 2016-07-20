/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 

// FeatureTracking.h

#ifndef FT_H
#define FT_H

	#include <string>
	#include <iostream>
	#include <vector>
	#include <iterator>
	#include <sstream>
	#include <fstream>
	#include <vtkTable.h>
	#include <vtkSmartPointer.h>
	#include <vtkTypeUInt32Array.h>
	#include <vtkVariantArray.h>

	#include "iAFeatureTrackingCorrespondence.h"


	using namespace std;

	class iAFeatureTracking
	{
	private:
		string file1;
		string file2;
		int lineOffset;
		vtkTable *u;
		vtkTable *v;
		float dissipationThreshold;
		float overlapThreshold;
		float volumeThreshold;
		float overallMatchingPercentage;
		int maxSearchValue;
		vector<pair<vtkIdType, vector<iAFeatureTrackingCorrespondence> > > *uToV;
		vector<pair<vtkIdType, vector<iAFeatureTrackingCorrespondence> > > *vToU;
		vector<pair<vtkIdType, vector<iAFeatureTrackingCorrespondence> > > *allUtoV;
		vector<pair<vtkIdType, vector<iAFeatureTrackingCorrespondence> > > *allVtoU;
		string outputFilename;
		vector<string> &split(const string &s, char delim, vector<string> &elems);
		vector<string> split(const string &s, char delim);
		vtkTable &readTableFromFile(const string &filename, int dataLineOffset);
		void sortCorrespondencesByOverlap(vector<iAFeatureTrackingCorrespondence> &correspondences);
		vector<iAFeatureTrackingCorrespondence>& getCorrespondences(const vtkVariantArray &row, vtkTable &table, int maxSearchValue, bool useZ);
		void ComputeOverallMatchingPercentage();

	public:
		iAFeatureTracking(string fileName1, string fileName2, int lineOffset, string outputFilename, float dissipationThreshold,
			float overlapThreshold, float volumeThreshold, int maxSearchValue);
		void TrackFeatures();
		vector<iAFeatureTrackingCorrespondence> FromUtoV(unsigned int uId);
		vector<iAFeatureTrackingCorrespondence> FromVtoU(unsigned int vId);
		float GetOverallMatchingPercentage();
		
		size_t getNumberOfEventsInU();
		size_t getNumberOfEventsInV();
		vtkTable* getU();
		vtkTable* getV();
	};

#endif