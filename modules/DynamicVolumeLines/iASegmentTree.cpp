/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iASegmentTree.h"

#include <iAMathUtility.h>

#include <itkImage.h>
#include <itkImportImageFilter.h>
#include <itkImageToHistogramFilter.h>

#include <iterator>

// Resource: http://codeforces.com/blog/entry/18051

iASegmentTree::iASegmentTree(const std::vector<int> &input, int binCnt, int lowerBnd, int upperBnd) :
	m_inputElemCnt(0)
{
	//NOTE: Currently only hist is needed, others can be disabled
	m_inputElemCnt = input.size();
	/*m_avg.resize(2 * m_inputElemCnt);
	m_min.resize(2 * m_inputElemCnt);
	m_max.resize(2 * m_inputElemCnt);*/
	m_hist.resize(2 * m_inputElemCnt);

	for (int i = 0; i < m_inputElemCnt; ++i)
	{
		/*m_avg[m_inputElemCnt + i] = input[i];
		m_min[m_inputElemCnt + i] = input[i];
		m_max[m_inputElemCnt + i] = input[i];*/
		std::vector<int> v(binCnt);
		std::fill(v.begin(), v.end(), 0);
		v[clamp(0, binCnt - 1, mapValue(lowerBnd, upperBnd, 0, binCnt, input[i]))]++;
		m_hist[m_inputElemCnt + i] = v;
	}

	hist_build();
	/*sum_build();
	min_build();
	max_build();*/
}

iASegmentTree::~iASegmentTree()
{
}

void iASegmentTree::hist_build()
{
	for (int i = m_inputElemCnt - 1; i > 0; --i)
		std::transform(m_hist[i << 1].begin(), m_hist[i << 1].end(), m_hist[i << 1 | 1].begin(),
			std::back_inserter(m_hist[i]), std::plus<int>());
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
			std::transform(a.begin(), a.end(), b.begin(), std::back_inserter(c), std::plus<int>());
			histVec = c;
		}
		if (r & 1)
		{
			std::vector<int> a; std::vector<int> b; std::vector<int> c;
			a = m_hist[--r];
			b = histVec;
			std::transform(a.begin(), a.end(), b.begin(), std::back_inserter(c), std::plus<int>());
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
