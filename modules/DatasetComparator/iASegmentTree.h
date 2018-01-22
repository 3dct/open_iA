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

#include "itkImage.h"
#include "itkImportImageFilter.h"
#include "itkImageToHistogramFilter.h"

#include "iAConsole.h"
#include <QDebug>
#include "iAMathUtility.h"


// Resource: http://codeforces.com/blog/entry/18051

class iASegmentTree
{
//TODO: check true false
//TODO: bounds + binCnt in constructor
//TODO: functions into cpp
//TODO: to unsigned int or other more general type?

public:
	iASegmentTree(const std::vector<int> &input, int binCnt, int lowerBnd, int upperBnd);
	std::vector<int> iASegmentTree::hist_query(int l, int r);
	double avg_query(int l, int r);
	int min_query(int l, int r);
	int max_query(int l, int r);

private:
	int m_inputElemCnt;
	std::vector<int> m_input;
	std::vector<std::vector<int>> m_hist;
	std::vector<double> m_avg;
	std::vector<int> m_min;
	std::vector<int> m_max;
	void hist_build();
	void sum_build();
	void min_build();
	void max_build();
	std::vector<int> calcHist(std::vector<int> data);
};

iASegmentTree::iASegmentTree(const std::vector<int> &input, int binCnt, int lowerBnd, int upperBnd)
{
	m_inputElemCnt = input.size();
	m_avg.resize(2 * m_inputElemCnt);
	m_min.resize(2 * m_inputElemCnt);
	m_max.resize(2 * m_inputElemCnt);
	m_hist.resize(2 * m_inputElemCnt);
	
	// version 1
	/*for (int i = 0; i < m_inputElemCnt; ++i)
	{
		m_avg[m_inputElemCnt + i] = m_input[i];
		m_min[m_inputElemCnt + i] = m_input[i];
		m_max[m_inputElemCnt + i] = m_input[i];
		std::vector<int> v = { m_input[i] };
		m_hist[m_inputElemCnt + i] = calcHist(v);
		qDebug() << (QString("calcHist: %1 of %2").arg(i).arg(m_inputElemCnt));
	}*/

	/*typedef int PixelType;
	const int Dim = 1;
	const int dataElemCnt = 1;

	typedef itk::Image<PixelType, Dim> ImageType;
	typedef itk::ImportImageFilter<PixelType, Dim> ImportFilterType;
	ImportFilterType::Pointer importFilter = ImportFilterType::New();
	ImportFilterType::SizeType size; size[0] = dataElemCnt;
	ImportFilterType::IndexType start; start[0] = 0;
	ImportFilterType::RegionType region;
	region.SetIndex(start);
	region.SetSize(size);
	importFilter->SetRegion(region);

	const unsigned int binNumber = 8;
	const unsigned int MeasurementVectorSize = 1;
	typedef itk::Statistics::ImageToHistogramFilter<ImageType> ImageToHistogramFilterType;
	ImageToHistogramFilterType::HistogramType::MeasurementVectorType lowerBound(binNumber);
	lowerBound.Fill(0);
	ImageToHistogramFilterType::HistogramType::MeasurementVectorType upperBound(binNumber);
	upperBound.Fill(65535);
	ImageToHistogramFilterType::HistogramType::SizeType histSize(MeasurementVectorSize);
	histSize.Fill(binNumber);
	ImageToHistogramFilterType::Pointer imageToHistogramFilter = ImageToHistogramFilterType::New();
	imageToHistogramFilter->SetHistogramBinMinimum(lowerBound);
	imageToHistogramFilter->SetHistogramBinMaximum(upperBound);
	imageToHistogramFilter->SetHistogramSize(histSize);
	imageToHistogramFilter->SetAutoMinimumMaximum(false);*/

	for (int i = 0; i < m_inputElemCnt; ++i)
	{
		m_avg[m_inputElemCnt + i] = input[i];
		m_min[m_inputElemCnt + i] = input[i];
		m_max[m_inputElemCnt + i] = input[i];

		//std::vector<int> v = { m_input[i] };

		//PixelType * localBuffer = new PixelType[dataElemCnt];
		//PixelType * it = localBuffer;
		//for (int i = 0; i < dataElemCnt; ++i)
		//	*it++ = v[i];

		//importFilter->SetImportPointer(localBuffer, dataElemCnt, false);	//check true -> segment tree destructor?
		//importFilter->Update();
		//imageToHistogramFilter->SetInput(importFilter->GetOutput());
		//imageToHistogramFilter->Update();
		//ImageToHistogramFilterType::HistogramType* histogram = imageToHistogramFilter->GetOutput();
		//std::vector<int> histo;
		//for (unsigned int i = 0; i < histogram->GetSize()[0]; ++i)
		//	histo.push_back(histogram->GetFrequency(i));
		//m_hist[m_inputElemCnt + i] = histo;
		//qDebug() << (QString("calcHist: %1 of %2").arg(i).arg(m_inputElemCnt));



		int bin = clamp(0, binCnt - 1, mapValue(lowerBnd, upperBnd, 0, binCnt, input[i]));
		std::vector<int> v(binCnt);
		std::fill(v.begin(), v.end(), 0);
		v[bin]++;
		m_hist[m_inputElemCnt + i] = v;
	}





	hist_build();
	sum_build();
	min_build();
	max_build();
}

std::vector<int> iASegmentTree::calcHist(std::vector<int> data)
{
	typedef int PixelType;
	const int Dim = 1;
	const int dataElemCnt = data.size();
	PixelType * localBuffer = new PixelType[dataElemCnt];
	PixelType * it = localBuffer;
	for (int i = 0; i < dataElemCnt; ++i)
		*it++ = data[i];

	typedef itk::Image<PixelType, Dim> ImageType;
	typedef itk::ImportImageFilter<PixelType, Dim> ImportFilterType;
	ImportFilterType::Pointer importFilter = ImportFilterType::New();
	ImportFilterType::SizeType size; size[0] = dataElemCnt;
	ImportFilterType::IndexType start; start[0] = 0;
	ImportFilterType::RegionType region;
	region.SetIndex(start);
	region.SetSize(size);
	importFilter->SetRegion(region);
	importFilter->SetImportPointer(localBuffer, dataElemCnt, false);	//check true -> segment tree destructor?
	importFilter->Update();
	
	const unsigned int binNumber = 8;
	const unsigned int MeasurementVectorSize = 1;
	typedef itk::Statistics::ImageToHistogramFilter<ImageType> ImageToHistogramFilterType;
	ImageToHistogramFilterType::HistogramType::MeasurementVectorType lowerBound(binNumber);
	lowerBound.Fill(0);
	ImageToHistogramFilterType::HistogramType::MeasurementVectorType upperBound(binNumber);
	upperBound.Fill(65535);
	ImageToHistogramFilterType::HistogramType::SizeType histSize(MeasurementVectorSize);
	histSize.Fill(binNumber);
	ImageToHistogramFilterType::Pointer imageToHistogramFilter = ImageToHistogramFilterType::New();
	imageToHistogramFilter->SetInput(importFilter->GetOutput());
	imageToHistogramFilter->SetHistogramBinMinimum(lowerBound);
	imageToHistogramFilter->SetHistogramBinMaximum(upperBound);
	imageToHistogramFilter->SetHistogramSize(histSize);
	imageToHistogramFilter->SetAutoMinimumMaximum(false);
	imageToHistogramFilter->Update();
	ImageToHistogramFilterType::HistogramType* histogram = imageToHistogramFilter->GetOutput();
		
	//QString outStr;
	//for (unsigned int i = 0; i < histogram->GetSize()[0]; ++i)
	//{
	//	/*outStr.append(QString::number(histogram->GetFrequency(i)));
	//	if (i < histogram->GetSize()[0] - 1)
	//		outStr.append(", ");*/
	//}
	//DEBUG_LOG(outStr);

	//for (unsigned int i = 0; i < histogram->GetSize()[0]; ++i)
	//	DEBUG_LOG(QString("Frequency of %1 (%2 to %3) = %4").arg(i).arg(histogram->GetBinMin(0, i))
	//		.arg(histogram->GetBinMax(0, i)).arg(histogram->GetFrequency(i)));

	std::vector<int> histo;
	for (unsigned int i = 0; i < histogram->GetSize()[0]; ++i)
		histo.push_back(histogram->GetFrequency(i));
	return histo;
}

void iASegmentTree::hist_build()
{
	for (int i = m_inputElemCnt - 1; i > 0; --i)
	{
		std::transform(m_hist[i << 1].begin(), m_hist[i << 1].end(), m_hist[i << 1 | 1].begin(),
			std::back_inserter(m_hist[i]), std::plus<int>());
		//qDebug() << (QString("hist_build: %1 of %2").arg(i).arg(m_inputElemCnt));
	}
}

void iASegmentTree::sum_build()
{
	for (int i = m_inputElemCnt - 1; i > 0; --i)
		m_avg[i] = m_avg[i << 1] + m_avg[i << 1 | 1];
}

void iASegmentTree::min_build()
{  
	for (int i = m_inputElemCnt - 1; i > 0; --i)
		m_min[i] = std::min(m_min[i << 1], m_min[i << 1 | 1]);
}

void iASegmentTree::max_build()
{ 
	for (int i = m_inputElemCnt - 1; i > 0; --i)
		m_max[i] = std::max(m_max[i << 1], m_max[i << 1 | 1]);
}

std::vector<int> iASegmentTree::hist_query(int l, int r)
{
	std::vector<int> histVec(m_hist.back().size());
	std::fill(histVec.begin(), histVec.end(), 0);
	for (l += m_inputElemCnt, r += m_inputElemCnt; l < r; l >>= 1, r >>= 1)
	{
		if (l & 1)
		{
			std::vector<int> a; std::vector<int> b; std::vector<int> c;
			a = m_hist[l++];
			b = histVec;
			std::transform(a.begin(), a.end(), b.begin(),
				std::back_inserter(c), std::plus<int>());
			histVec = c;
		}
		if (r & 1)
		{
			std::vector<int> a; std::vector<int> b; std::vector<int> c;
			a = m_hist[--r];
			b = histVec;
			std::transform(a.begin(), a.end(), b.begin(),
				std::back_inserter(c), std::plus<int>());
			histVec = c;
		}
	}
	return histVec;
}

double iASegmentTree::avg_query(int l, int r)
{
	double avgVal = 0;
	int left = l, right = r, nbCnt = right - left;
	for (l += m_inputElemCnt, r += m_inputElemCnt; l < r; l >>= 1, r >>= 1)
	{
		if (l & 1)
			avgVal += m_avg[l++];
		if (r & 1)
			avgVal += m_avg[--r];
	}
	return avgVal / nbCnt;
}

int iASegmentTree::min_query(int l, int r)
{ 
	int minVal = INT_MAX;
	for (l += m_inputElemCnt, r += m_inputElemCnt; l < r; l >>= 1, r >>= 1)
	{
		if (l & 1)
			minVal = std::min(minVal, m_min[l++]);
		if (r & 1)
			minVal = std::min(m_min[--r], minVal);
	}
	return minVal;
}

int iASegmentTree::max_query(int l, int r)
{  
	int maxVal = INT_MIN;
	for (l += m_inputElemCnt, r += m_inputElemCnt; l < r; l >>= 1, r >>= 1)
	{
		if (l & 1)
			maxVal = std::max(maxVal, m_max[l++]);
		if (r & 1)
			maxVal = std::max(m_max[--r], maxVal);
	}
	return maxVal;
}