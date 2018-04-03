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
 
#include "pch.h"
#include "iAFeatureTracking.h"

#include <vtkTypeUInt32Array.h>

#include <sstream>

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
	
std::vector<std::string> &iAFeatureTracking::split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> iAFeatureTracking::split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

vtkTable &iAFeatureTracking::readTableFromFile(const std::string &filename, int dataLineOffset) {
	vtkTable *t = vtkTable::New();
	
	VTK_CREATE(vtkTypeUInt32Array, idArray);
	idArray->SetName("id");
	VTK_CREATE(vtkTypeUInt32Array, posXArray);
	posXArray->SetName("posX");
	VTK_CREATE(vtkTypeUInt32Array, posYArray);
	posYArray->SetName("posY");
	VTK_CREATE(vtkTypeUInt32Array, posZArray);
	posZArray->SetName("posZ");
	VTK_CREATE(vtkTypeUInt32Array, volumeVxArray);
	volumeVxArray->SetName("volume");
	VTK_CREATE(vtkTypeUInt32Array, dimXVxArray);
	dimXVxArray->SetName("dimX");
	VTK_CREATE(vtkTypeUInt32Array, dimYVxArray);
	dimYVxArray->SetName("dimY");
	VTK_CREATE(vtkTypeUInt32Array, dimZVxArray);
	dimZVxArray->SetName("dimZ");

	if(filename != "") {
		std::string line;
		ifstream inputStream(filename);
		if(inputStream.is_open()) {
			for(int i = 0; i < dataLineOffset; i++) 
				std::getline(inputStream, line);
			// at header row
			std::getline(inputStream, line);
			std::vector<std::string> splittedRow;
			while(getline(inputStream, line)) {
				splittedRow = split(line, ',');
				try {
					idArray->InsertNextValue(stoi(splittedRow.at(0)));
					posXArray->InsertNextValue(stoi(splittedRow.at(5)));
					posYArray->InsertNextValue(stoi(splittedRow.at(6)));
					posZArray->InsertNextValue(stoi(splittedRow.at(7)));
					volumeVxArray->InsertNextValue(stoi(splittedRow.at(9)));
					dimXVxArray->InsertNextValue(stoi(splittedRow.at(10)));
					dimYVxArray->InsertNextValue(stoi(splittedRow.at(11)));
					dimZVxArray->InsertNextValue(stoi(splittedRow.at(12)));
				} catch(std::exception) {}
			}
		}
	}
	t->AddColumn(idArray);
	t->AddColumn(posXArray);
	t->AddColumn(posYArray);
	t->AddColumn(posZArray);
	t->AddColumn(volumeVxArray);
	t->AddColumn(dimXVxArray);
	t->AddColumn(dimYVxArray);
	t->AddColumn(dimZVxArray);

	return *t;
}

void iAFeatureTracking::sortCorrespondencesByOverlap(std::vector<iAFeatureTrackingCorrespondence> &correspondences) {
	iAFeatureTrackingCorrespondence *temp;
	for(size_t i = correspondences.size(); i > 1; i--) {
		for(int j = 0; j < i - 1; j++) {
			if(correspondences.at(j).overlap < correspondences.at(j + 1).overlap) {
				temp = new iAFeatureTrackingCorrespondence(correspondences.at(j));
				correspondences.at(j) = correspondences.at(j + 1);
				correspondences.at(j + 1) = *temp;
			}
		}
	}
}

void sortIntVector(std::vector<int> &v) {
	int temp;
	for(size_t i = v.size(); i > 1; i--) {
		for(size_t j = 0; j < i - 1; j++) {
			if(v.at(j) > v.at(j + 1)) {
				temp = v.at(j);
				v.at(j) = v.at(j + 1);
				v.at(j + 1) = temp;
			}
		}
	}
}

int nrOfOccurences(std::vector<int> &v, int occurence) {
	int result = 0;
	for(int i = 0; i < v.size(); i++) 
		if(i == occurence)
			result++;
	return result;
}

std::vector<iAFeatureTrackingCorrespondence>& iAFeatureTracking::getCorrespondences(
	const vtkVariantArray &row, 
	vtkTable &table, 
	int maxSearchValue, 
	bool useZ) {

	auto correspondences = new std::vector<iAFeatureTrackingCorrespondence>();

	int inputCenterX = row.GetValue(1).ToInt();
	int inputCenterY = row.GetValue(2).ToInt();
	int inputCenterZ = row.GetValue(3).ToInt();
	int inputVolume = row.GetValue(4).ToInt();
	int inputDimensionX = row.GetValue(5).ToInt();
	int inputDimensionY = row.GetValue(6).ToInt();
	int inputDimensionZ = row.GetValue(7).ToInt();
	int inputMinX = inputCenterX - inputDimensionX / 2;
	int inputMaxX = inputCenterX + inputDimensionX / 2;
	int inputMinY = inputCenterY - inputDimensionY / 2;
	int inputMaxY = inputCenterY + inputDimensionY / 2;
	int inputMinZ = inputCenterZ - inputDimensionZ / 2;
	int inputMaxZ = inputCenterZ + inputDimensionZ / 2;

	int currentVolume;
	int currentCenterX;
	int currentCenterY;
	int currentCenterZ;
	int currentDimensionX;
	int currentDimensionY;
	int currentDimensionZ;
	int currentMinX;
	int currentMaxX;
	int currentMinY;
	int currentMaxY;
	int currentMinZ;
	int currentMaxZ;

	vtkVariantArray *currentRow;

	for(int i = 0; i < table.GetNumberOfRows(); i++) {
		currentRow = table.GetRow(i);

		currentCenterX = currentRow->GetValue(1).ToInt();
		currentCenterY = currentRow->GetValue(2).ToInt();
		currentCenterZ = currentRow->GetValue(3).ToInt();
		currentVolume = currentRow->GetValue(4).ToInt();
		currentDimensionX = currentRow->GetValue(5).ToInt();
		currentDimensionY = currentRow->GetValue(6).ToInt();
		currentDimensionZ = currentRow->GetValue(7).ToInt();
		currentMinX = currentCenterX - currentDimensionX / 2 - (maxSearchValue);
		currentMaxX = currentCenterX + currentDimensionX / 2 + (maxSearchValue);
		currentMinY = currentCenterY - currentDimensionY / 2 - (maxSearchValue);
		currentMaxY = currentCenterY + currentDimensionY / 2 + (maxSearchValue);
		currentMinZ = currentCenterZ - currentDimensionZ / 2 - (maxSearchValue);
		currentMaxZ = currentCenterZ + currentDimensionZ / 2 + (maxSearchValue);
		/*currentMinX = currentCenterX - currentDimensionX / 2;
		currentMaxX = currentCenterX + currentDimensionX / 2;
		currentMinY = currentCenterY - currentDimensionY / 2;
		currentMaxY = currentCenterY + currentDimensionY / 2;
		currentMinZ = currentCenterZ - currentDimensionZ / 2;
		currentMaxZ = currentCenterZ + currentDimensionZ / 2;*/

		if((
			(currentMinX < inputMaxX && currentMinX >= inputMinX) ||	
			(currentMaxX > inputMinX && currentMaxX <= inputMaxX) ||
			(currentMinX <= inputMinX && currentMaxX >= inputMaxX)
			) && (
			(currentMinY < inputMaxY && currentMinY >= inputMinY) ||
			(currentMaxY > inputMinY && currentMaxY <= inputMaxY) ||
			(currentMinY <= inputMinY && currentMaxY >= inputMaxY)
			) && (
			(currentMinZ < inputMaxZ && currentMinZ >= inputMinZ) ||
			(currentMaxZ > inputMinZ && currentMaxZ <= inputMaxZ) ||
			(currentMinZ <= inputMinZ && currentMaxZ >= inputMaxZ)
			)) {
				float xOverlap = 1.f;
				float yOverlap = 1.f;
				float zOverlap = 1.f;

				if(currentMinX > inputMinX) 
					xOverlap -= (currentMinX - inputMinX) / (inputDimensionX * 1.f);
				
				if(currentMaxX < inputMaxX) 
					xOverlap -= (inputMaxX - currentMaxX) / (inputDimensionX * 1.f);

				if(currentMinY > inputMinY)
					yOverlap -= (currentMinY - inputMinY) / (inputDimensionY * 1.f);

				if(currentMaxY < inputMaxY)
					yOverlap -= (inputMaxY - currentMaxY) / (inputDimensionY * 1.f);

				if(currentMinZ > inputMinZ)
					zOverlap -= (currentMinZ - inputMinZ) / (inputDimensionZ * 1.f);

				if(currentMaxZ < inputMaxZ)
					zOverlap -= (inputMaxZ - currentMaxZ) / (inputDimensionZ * 1.f);

				float overlap;
				if(useZ)
					overlap = xOverlap * yOverlap * zOverlap;
				else 
					overlap = xOverlap * yOverlap;
				correspondences->push_back(
					*new iAFeatureTrackingCorrespondence(i + 1,
										overlap, 
										inputVolume / (float)currentVolume, 
										false, 
										0.f,
										Continuation)
				);
		}
	}
	sortCorrespondencesByOverlap(*correspondences);
	return *correspondences;
}


// public methods
iAFeatureTracking::iAFeatureTracking(std::string fileName1, std::string fileName2, int lineOffset, std::string outputFilename,
								 float dissipationThreshold, float overlapThreshold, float volumeThreshold, 
								 int maxSearchValue)
{
	this->file1 = fileName1;
	this->file2 = fileName2;
	this->lineOffset = lineOffset;
	this->outputFilename = outputFilename;
	this->dissipationThreshold = dissipationThreshold;
	this->overlapThreshold = overlapThreshold;
	this->volumeThreshold = volumeThreshold;
	this->maxSearchValue = maxSearchValue;
	uToV = new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
	vToU = new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
	allUtoV = new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
	allVtoU = new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
}

float iAFeatureTracking::GetOverallMatchingPercentage() {
	return this->overallMatchingPercentage;
}

void iAFeatureTracking::ComputeOverallMatchingPercentage() {
	if(file1 == "") {
		overallMatchingPercentage = 1.f;
		return;
	}
	overallMatchingPercentage = 0.f;
	size_t n = allUtoV->size();
	for(size_t i = 0; i < allUtoV->size(); i++) {
		for(size_t j = 0; j < allUtoV->at(i).second.size(); j++) {
			if(allUtoV->at(i).second.at(j).isTakenForCurrentIteration) {
				overallMatchingPercentage += allUtoV->at(i).second.at(j).likelyhood;
			}
		}
	}
	overallMatchingPercentage = overallMatchingPercentage / n;
}

void iAFeatureTracking::TrackFeatures() {
	u = &readTableFromFile(file1, lineOffset);
	v = &readTableFromFile(file2, lineOffset);
	
	auto dissipated =                new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
	auto continuated =               new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
	auto created =                   new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
	auto merged =                    new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
	auto splitted =                  new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
	auto mergeCandidates =           new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
	auto splitCandidates =           new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
	auto continuatedAfterMergeTest = new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();


	vtkVariantArray *row = vtkVariantArray::New();

	// main computation ==============================================================================================
	for(int i = 0; i < u->GetNumberOfRows(); i++) {
		u->GetRow(i, row);
		auto result = &getCorrespondences(*row, *v, maxSearchValue, true);
		uToV->push_back(std::make_pair(i + 1, *result));
	}

	// compute vToU out of uToV ======================================================================================
	vToU->reserve(v->GetNumberOfRows());
	for(int i = 0; i < v->GetNumberOfRows(); i++) {
		auto result = new std::vector<iAFeatureTrackingCorrespondence>();
		vToU->push_back(std::make_pair(i + 1, *result));
	}
	for(unsigned int i = 0; i < uToV->size(); i++) {
		for(unsigned int j = 0; j < uToV->at(i).second.size(); j++) {
			iAFeatureTrackingCorrespondence tmp = uToV->at(i).second.at(j);
			vToU->at(tmp.id - 1).second.push_back(
				*new iAFeatureTrackingCorrespondence(uToV->at(i).first, tmp.overlap, tmp.volumeRatio,
				tmp.isTakenForCurrentIteration, tmp.likelyhood, tmp.featureEvent));
		}
	}

	// compute creation ==============================================================================================
	for(unsigned int i = 0; i < vToU->size(); i++) {
		if(vToU->at(i).second.size() == 0) {
			auto vec = new std::vector<iAFeatureTrackingCorrespondence>();
			vec->push_back(*new iAFeatureTrackingCorrespondence(0, 0.f, 0.f, true, 1.f, Creation));
			created->push_back(std::make_pair(i + 1, *vec));
		}
	}

	// look for merge/continuation/split and dissipation in uToV and vToU ============================================
	// this section removes all obvious pores to leave just those with difficult values left for further computation
	for(unsigned int i = 0; i < uToV->size(); i++) {
		if(uToV->at(i).second.size() > 0) {
			if(uToV->at(i).second.at(0).overlap >= dissipationThreshold) {
				if(vToU->at(uToV->at(i).second.at(0).id - 1).second.size() == 1) {
					if( uToV->at(i).second.size() == 1) {
						// Continuation
						auto vec = &uToV->at(i).second;
						vec->at(0).featureEvent = Continuation;
						vec->at(0).isTakenForCurrentIteration = true;
						continuated->push_back(make_pair(i + 1, *vec));
					}
					else {
						// Bifurcation/Continuation
						auto vec = &uToV->at(i).second;
						for (auto c = vec->begin(); c != vec->end(); c++)
							c->featureEvent = Bifurcation;
						splitCandidates->push_back(make_pair(i + 1, *vec));
					}
				}
				else if(vToU->at(uToV->at(i).second.at(0).id - 1).second.size() > 1) {
					unsigned int j = 0;
					while(j < uToV->at(i).second.size()) {
						if(	uToV->at(i).second.at(j).volumeRatio >= (float)(1.0f - volumeThreshold) &&
							uToV->at(i).second.at(j).volumeRatio <= (float)(1.0f + volumeThreshold) && 
							uToV->at(i).second.at(j).overlap >= overlapThreshold)
							break;
						j++;
					}
					if(j < uToV->at(i).second.size()) {
						// Continuation
						auto vec = &uToV->at(i).second;
						vec->at(j).isTakenForCurrentIteration = true;
						continuated->push_back(std::make_pair(i + 1, *vec));
						for(unsigned int k = 0; k < uToV->at(i).second.size(); k++) 
							if(k != j)
								vToU->at(uToV->at(i).second.at(k).id - 1).second.pop_back(); // just for decreasing size
					} else {
						unsigned int j = 0;
						int cnt = 0;
						while(j < uToV->at(i).second.size()) {
							if(vToU->at(uToV->at(i).second.at(j).id - 1).second.size() == 1)
								cnt++;
							j++;
						}
						if(cnt > 0) {
							// Bifurcation/Continuation
							auto vec = &uToV->at(i).second;
							for (auto c = vec->begin(); c != vec->end(); c++)
								c->featureEvent = Bifurcation;
							splitCandidates->push_back(make_pair(i + 1, *vec));
						} else {
							// Amalgamation/Continuation/Dissipation
							auto vec = &uToV->at(i).second;
							for (auto c = vec->begin(); c != vec->end(); c++)
								c->featureEvent = Amalgamation;
							mergeCandidates->push_back(make_pair(i + 1, *vec));
						}
					}
				}
			} else {
				// Dissipation
				// almost never happening
				auto vec = &uToV->at(i).second;
				for(unsigned int k = 0; k < uToV->at(i).second.size(); k++) 
					vToU->at(uToV->at(i).second.at(k).id - 1).second.pop_back(); // just for decreasing size
				vec->clear();
				vec->push_back(*new iAFeatureTrackingCorrespondence(0, 1.f, 1.f, true, 1.f, Dissipation));
				vec->at(vec->size() - 1).isTakenForCurrentIteration = true;
				dissipated->push_back(std::make_pair(i + 1, *vec));

			}
		} else {
			// Dissipation
			auto vec = &uToV->at(i).second;
			vec->push_back(*new iAFeatureTrackingCorrespondence(0, 1.f, 1.f, true, 1.f, Dissipation));
			dissipated->push_back(std::make_pair(i + 1, *vec));
		}
	}

	// do work with left pores listed in mergeCandidates, splitCandidates & continuatedAfterMergeTest
	// which is the vector containing those pores, that were comptuted within a nontrivial computation
	// and are needed to evaluate the other pores(i.e. removing their results from those left to compute)


	// if for any reason this algorithm shouldn't be able to evaluate all pores, the following section should 
	// be executed till there is no change any more *1


	// first mergeCandidates -----------------------------------------------------------------------------
	// remove all correspondences already connected to another continuated pore
	for (auto p = continuated->begin(); p != continuated->end(); p++) {
		for (auto it = mergeCandidates->begin();
			it != mergeCandidates->end();) 
		{
			for (auto it2 = it->second.begin();
				it2 != it->second.end();) {
					if(p->second.at(0).id == it2->id) 
						it2 = it->second.erase(it2);
					else 
						it2++;
			}
			it++;
		}
	}

	for (auto it = mergeCandidates->begin();
		it != mergeCandidates->end();) {
		float currentMax = 0.f;
		int currentMaxId = -1;
		for(unsigned int i = 0; i < it->second.size(); i++) {
			float volumeRatio;
			if(it->second.at(i).volumeRatio > 1.f)
				volumeRatio = 1.f;
			else
				volumeRatio = it->second.at(i).volumeRatio;
			if((it->second.at(i).overlap * 2.f) + volumeRatio > currentMax) {
				currentMaxId = i;
				currentMax = (it->second.at(i).overlap * 2.f) + it->second.at(i).volumeRatio;
			}
		}
		auto vec = new std::vector<iAFeatureTrackingCorrespondence>();
		if(currentMaxId > -1) {
			vec->push_back(*new iAFeatureTrackingCorrespondence(it->second.at(currentMaxId)));
			continuatedAfterMergeTest->push_back(std::make_pair(it->first, *vec));
		} else {
			vec->push_back(*new iAFeatureTrackingCorrespondence(0, 1.f, 1.f, true, 0.f, Dissipation));
			dissipated->push_back(std::make_pair(it->first, *vec));
		}
		it = mergeCandidates->erase(it);
	}

	// now splitCandidates --------------------------------------------------------------------------------------
	// remove all found pores
	for (auto p = continuated->begin(); p != continuated->end(); p++) {
		for (auto it = splitCandidates->begin();
			it != splitCandidates->end();) 
		{
			for (auto it2 = it->second.begin();
				it2 != it->second.end();) {
					if(p->second.at(0).id == it2->id) 
						it2 = it->second.erase(it2);
					else 
						it2++;
			}
			it++;
		}
	}
	for (auto p = continuatedAfterMergeTest->begin(); p != continuatedAfterMergeTest->end(); p++) {
		for (auto it = splitCandidates->begin();
			it != splitCandidates->end();) 
		{
			for (auto it2 = it->second.begin();
				it2 != it->second.end();) {
					if(p->second.at(0).id == it2->id) 
						it2 = it->second.erase(it2);
					else 
						it2++;
			}
			it++;
		}
	}

	// categorize
	for (auto p = splitCandidates->begin(); p != splitCandidates->end(); p++) {
		if(p->second.size() == 0) {
			//Dissipated
		}
		else if(p->second.size() == 1) {
			// Continuated
			auto vec = new std::vector<iAFeatureTrackingCorrespondence>();
			vec->push_back(*new iAFeatureTrackingCorrespondence(p->second.at(0)));
			vec->at(0).featureEvent = Continuation;
			continuatedAfterMergeTest->push_back(make_pair(p->first, *vec));
		}
		else {
			// splitted
			auto vec = new std::vector<iAFeatureTrackingCorrespondence>();
			for (auto c = p->second.begin(); c != p->second.end(); c++)
				vec->push_back(*new iAFeatureTrackingCorrespondence(*c));
			vec->at(0).featureEvent = Bifurcation;
			splitted->push_back(make_pair(p->first, *vec));
		}
	}

	// removing all continuated pores as they are not needed for further computation and would 
	// be disturbing for decision in next step
	for (auto it = continuatedAfterMergeTest->begin();
		it != continuatedAfterMergeTest->end();) {
		unsigned int i = 0;
		while(i < it->second.size()) {
			if(it->second.at(i).isTakenForCurrentIteration)
				break;
			i++;
		}
		if(i < it->second.size()) {
			continuated->push_back(*it);
			it = continuatedAfterMergeTest->erase(it);
		}
		else 
			it++;
	}

	// finally compute all out of the left pores in continuatedAfterMergeTest
	auto occurences = new std::vector<int>();
	for (auto p = continuatedAfterMergeTest->begin(); p != continuatedAfterMergeTest->end(); p++)
		for (auto c = p->second.begin(); c != p->second.end(); c++)
			occurences->push_back(c->id);

	for (auto p = continuatedAfterMergeTest->begin(); p != continuatedAfterMergeTest->end(); p++) {
		if(nrOfOccurences(*occurences, p->second.at(0).id) == 1) {
			// Continuation
			auto vec = new std::vector<iAFeatureTrackingCorrespondence>();
			vec->push_back(*new iAFeatureTrackingCorrespondence(p->second.at(0)));
			vec->at(0).featureEvent = Continuation;
			vec->at(0).isTakenForCurrentIteration = true;
			continuated->push_back(std::make_pair(p->first, *vec));
		} else
		{
			// Amalgamation
			auto vec = new std::vector<iAFeatureTrackingCorrespondence>();
			vec->push_back(*new iAFeatureTrackingCorrespondence(p->second.at(0)));
			vec->at(0).featureEvent = Amalgamation;
			vec->at(0).isTakenForCurrentIteration = true;
			merged->push_back(std::make_pair(p->first, *vec));
		}
	}
	// end of the section which should be executed till there is no change from step to step any more *1

	// setting all correspondence for splitted pores to Bifurcation
	for (auto it = splitted->begin();
		it != splitted->end(); it++)
	for (auto it2 = it->second.begin();
			it2 != it->second.end(); it2++)
				it2->featureEvent = Bifurcation;

	if(outputFilename != "") {
		ofstream out;
		out.open(outputFilename);
		out << "Dissipation" << endl;
		for (auto p = dissipated->begin(); p != dissipated->end(); p++) {
			out << p->first << endl;
		}

		out << "Creation" << endl;
		for (auto p = created->begin(); p != created->end(); p++) {
			out << p->first << endl;
		}

		out << "Continuation" << endl;
		for (auto p = continuated->begin(); p != continuated->end(); p++) {
			out << p->first << " to " << p->second.at(0).id << endl;
		}
			
		out << "Amalgamation" << endl;
		for (auto p = merged->begin(); p != merged->end();	p++) {
			out << p->first << " to " <<  p->second.at(0).id << endl;
		}

		out << "Bifurcation" << endl;
		for (auto p = splitted->begin(); p != splitted->end(); p++) {
			out << p->first << " to ";
			for(unsigned int i = 0; i < p->second.size(); i++) 
				out << p->second.at(i).id << " ";
			out << endl;
		}
	
		out.close();
	}

	// building the fast-access vectors ====================================================================
	// copy the results using the output computed by the main computation(at top) to 
	// get all possibilities due to the algorithm erasing some

	// adding erased values to dissipated
	for (auto it = dissipated->begin();
		it != dissipated->end(); it++) {
			if(uToV->at(it->first - 1).second.size() > it->second.size()) {
				// adding erased values
				for (auto c = uToV->at(it->first - 1).second.begin(); c != uToV->at(it->first - 1).second.end(); c++) {
					it->second.push_back(*new iAFeatureTrackingCorrespondence(*c));
				}
			}
	}

	// adding erased values to continuated
	for (auto it = continuated->begin();
		it != continuated->end(); it++) {
			if(uToV->at(it->first - 1).second.size() > 1) {
				// adding erased values
				for (auto c = uToV->at(it->first - 1).second.begin(); c != uToV->at(it->first - 1).second.end(); c++) {
					bool alreadyContaining = false;
					for (auto c1 = it->second.begin(); c1 != it->second.end(); c1++)
						if(c1->id == c->id) {
							alreadyContaining = true;
							break;
						}
					if(!alreadyContaining)
						it->second.push_back(*new iAFeatureTrackingCorrespondence(*c));
				}
			}
	}
				
	// set usedForCurrentIteration before adding erased values to remember the difference
	for (auto it = merged->begin();
		it != merged->end(); it++) {
		for (auto it2 = it->second.begin();
				it2 != it->second.end(); it2++) {
					it2->isTakenForCurrentIteration = true;
			}
	}

	// adding erased values to merged
	for (auto it = merged->begin();
		it != merged->end(); it++) {
			if(uToV->at(it->first - 1).second.size() > it->second.size()) {
				// adding erased values
				for (auto c = uToV->at(it->first - 1).second.begin(); c != uToV->at(it->first - 1).second.end(); c++) {
					bool alreadyContaining = false;
					for (auto c1 = it->second.begin(); c1 != it->second.end(); c1++)
						if(c1->id == c->id) {
							alreadyContaining = true;
							break;
						}
					if(!alreadyContaining) 
						it->second.push_back(*new iAFeatureTrackingCorrespondence(*c));
				}
			}
	}

	// set usedForCurrentIteration before adding erased values to remember the difference
	for (auto it = splitted->begin();
		it != splitted->end(); it++) {
		for (auto it2 = it->second.begin();
				it2 != it->second.end(); it2++) {
					it2->isTakenForCurrentIteration = true;
			}
	}
	
	// adding erased values to splitted
	for (auto it = splitted->begin();
		it != splitted->end(); it++) {
			if(uToV->at(it->first - 1).second.size() > it->second.size()) {
				// adding erased values
				for (auto c = uToV->at(it->first - 1).second.begin(); c != uToV->at(it->first - 1).second.end(); c++) {
					bool alreadyContaining = false;
					for (auto c1 = it->second.begin(); c1 != it->second.end(); c1++)
						if(c1->id == c->id) {
							alreadyContaining = true;
							break;
						}
					if(!alreadyContaining)
						it->second.push_back(*new iAFeatureTrackingCorrespondence(*c));
				}
			}
	}
	// end of copying erased values...

	// building the fast-access vectors ============================================================================
	auto temp = new std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > >();
	temp->reserve(dissipated->size() + continuated->size() + merged->size() + splitted->size());
	temp->insert(temp->end(), dissipated->begin(), dissipated->end());
	temp->insert(temp->end(), continuated->begin(), continuated->end());
	temp->insert(temp->end(), merged->begin(), merged->end());
	temp->insert(temp->end(), splitted->begin(), splitted->end());

	// make allUtoV initialization
	allUtoV->reserve(uToV->size());
	for(unsigned int i = 0; i < uToV->size(); i++) {
		auto vec = new std::vector<iAFeatureTrackingCorrespondence>();
		auto p = std::make_pair(i + 1, *vec);
		allUtoV->push_back(p);
	}

	// insert sorted
	for (auto p = temp->begin(); p != temp->end(); p++)
		for (auto c = p->second.begin(); c != p->second.end(); c++)
			allUtoV->at(p->first - 1).second.push_back(*new iAFeatureTrackingCorrespondence(*c));

	// make the mirroring version allVtoU -----------------------------------------------------------------------
	allVtoU->reserve(vToU->size());
	for(unsigned int i = 0; i < vToU->size(); i++) {
		auto vec = new std::vector<iAFeatureTrackingCorrespondence>();
		auto p = std::make_pair(i + 1, *vec);
		allVtoU->push_back(p);
	}

	// just adding valid values and not all of the computed ones
	for (auto p = allUtoV->begin(); p != allUtoV->end(); p++) {
		for (auto c = p->second.begin(); c != p->second.end(); c++) {
			if(c->featureEvent == Dissipation) {
				// do nothing
			} else if(c->featureEvent == Continuation) {
				// Continuation
				if(c->isTakenForCurrentIteration)
					allVtoU->at(c->id - 1).second.push_back(
					*new iAFeatureTrackingCorrespondence(p->first, c->overlap, 1 / c->volumeRatio, true, 1.f, Continuation));
			} else if(c->featureEvent == Amalgamation) {
				// add splitted
				if(c->isTakenForCurrentIteration)
					allVtoU->at(c->id - 1).second.push_back(
					*new iAFeatureTrackingCorrespondence(p->first, c->overlap, 1 / c->volumeRatio, true, 1.f, Bifurcation));
			} else if(c->featureEvent == Bifurcation) {
				// add merge
				if(c->isTakenForCurrentIteration)
					allVtoU->at(c->id - 1).second.push_back(
					*new iAFeatureTrackingCorrespondence(p->first, c->overlap, 1 / c->volumeRatio, true, 1.f, Amalgamation));
			}
		}
	}

	for (auto p = created->begin(); p != created->end(); p++) {
		allVtoU->at(p->first - 1).second = p->second;
	}

	// compute the percentages =====================================================================
	for (auto it = allUtoV->begin();
		it != allUtoV->end(); it++) {

			float total = 0.f;
			for (auto it2 = it->second.begin();
				it2 != it->second.end();
				it2++) {

					float overlap;
					if(it2->overlap > 1.f)
						overlap = 1.f;
					else
						overlap = it2->overlap;
					it2->likelyhood = it2->overlap * 2  + overlap;
					total += it2->likelyhood;
			}
			for (auto it2 = it->second.begin();
				it2 != it->second.end();
				it2++) {

					it2->likelyhood /= total;
			}
	}

	ComputeOverallMatchingPercentage();
}

std::vector<iAFeatureTrackingCorrespondence> iAFeatureTracking::FromUtoV(unsigned int uId) {
	std::vector<iAFeatureTrackingCorrespondence> result;
	if(allUtoV != 0 && uId > 0 && uId <= allUtoV->size() && allUtoV->at(uId - 1).second.size() > 0)
		for (auto c = allUtoV->at(uId - 1).second.begin(); c != allUtoV->at(uId - 1).second.end(); c++)
			result.push_back(*new iAFeatureTrackingCorrespondence(*c));
	return result;
}

std::vector<iAFeatureTrackingCorrespondence> iAFeatureTracking::FromVtoU(unsigned int vId) {
	std::vector<iAFeatureTrackingCorrespondence> result;
	if(allVtoU != 0 && vId > 0 && vId <= allVtoU->size() && allVtoU->at(vId - 1).second.size() > 0)
		for (auto c = allVtoU->at(vId - 1).second.begin(); c != allVtoU->at(vId - 1).second.end(); c++)
			result.push_back(*new iAFeatureTrackingCorrespondence(*c));
	return result;
}

size_t iAFeatureTracking::getNumberOfEventsInU()
{
	return uToV->size();
}

size_t iAFeatureTracking::getNumberOfEventsInV()
{
	return vToU->size();
}

vtkTable* iAFeatureTracking::getU()
{
	return u;
}

vtkTable* iAFeatureTracking::getV()
{
	return v;
}