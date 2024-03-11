// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFeatureTrackingCorrespondence.h"

#include <vtkSmartPointer.h>

#include <QString>

#include <string>
#include <vector>

class vtkTable;
class vtkVariantArray;

class iAFeatureTracking
{
private:
	QString file1, file2, outputFilename;
	int lineOffset;
	vtkSmartPointer<vtkTable> u, v;
	float dissipationThreshold;
	float overlapThreshold;
	float volumeThreshold;
	float overallMatchingPercentage;
	int m_maxSearchValue;
	std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > > *uToV;
	std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > > *vToU;
	std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > > *allUtoV;
	std::vector<std::pair<vtkIdType, std::vector<iAFeatureTrackingCorrespondence> > > *allVtoU;
	std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
	std::vector<std::string> split(const std::string &s, char delim);
	vtkSmartPointer<vtkTable> readTableFromFile(const QString &filename, int dataLineOffset);
	void sortCorrespondencesByOverlap(std::vector<iAFeatureTrackingCorrespondence> &correspondences);
	std::vector<iAFeatureTrackingCorrespondence> getCorrespondences(const vtkVariantArray &row, vtkTable &table, int maxSearchValue, bool useZ);
	void ComputeOverallMatchingPercentage();

public:
	iAFeatureTracking(QString const & fileName1, QString const & fileName2, int lineOffset, QString const & outputFilename, float dissipationThreshold,
		float overlapThreshold, float volumeThreshold, int maxSearchValue);
	void TrackFeatures();
	std::vector<iAFeatureTrackingCorrespondence> FromUtoV(unsigned int uId);
	std::vector<iAFeatureTrackingCorrespondence> FromVtoU(unsigned int vId);
	float GetOverallMatchingPercentage();

	size_t getNumberOfEventsInU();
	size_t getNumberOfEventsInV();
	vtkSmartPointer<vtkTable> getU();
	vtkSmartPointer<vtkTable> getV();
};
