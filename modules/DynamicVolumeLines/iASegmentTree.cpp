/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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
#include "iASegmentTree.h"
#include "iAMathUtility.h"

#include <itkImage.h>
#include <itkImportImageFilter.h>
#include <itkImageToHistogramFilter.h>

#include <iterator>

// Resource: http://codeforces.com/blog/entry/18051

iASegmentTree::iASegmentTree(const vector<int> &input, int binCnt, int lowerBnd, int upperBnd) :
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
		vector<int> v(binCnt);
		fill(v.begin(), v.end(), 0);
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
		transform(m_hist[i << 1].begin(), m_hist[i << 1].end(), m_hist[i << 1 | 1].begin(),
			back_inserter(m_hist[i]), plus<int>());
}

void iASegmentTree::sum_build()
{
	for (int i = m_inputElemCnt - 1; i > 0; --i)
		m_avg[i] = m_avg[i << 1] + m_avg[i << 1 | 1];
}

void iASegmentTree::min_build()
{  
	for (int i = m_inputElemCnt - 1; i > 0; --i)
		m_min[i] = min(m_min[i << 1], m_min[i << 1 | 1]);
}

void iASegmentTree::max_build()
{ 
	for (int i = m_inputElemCnt - 1; i > 0; --i)
		m_max[i] = max(m_max[i << 1], m_max[i << 1 | 1]);
}

vector<int> iASegmentTree::hist_query(int l, int r)
{
	vector<int> histVec(m_hist.back().size());
	fill(histVec.begin(), histVec.end(), 0);
	for (l += m_inputElemCnt, r += m_inputElemCnt; l < r; l >>= 1, r >>= 1)
	{
		if (l & 1)
		{
			vector<int> a; vector<int> b; vector<int> c;
			a = m_hist[l++];
			b = histVec;
			transform(a.begin(), a.end(), b.begin(), back_inserter(c), plus<int>());
			histVec = c;
		}
		if (r & 1)
		{
			vector<int> a; vector<int> b; vector<int> c;
			a = m_hist[--r];
			b = histVec;
			transform(a.begin(), a.end(), b.begin(), back_inserter(c), plus<int>());
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
			minVal = min(minVal, m_min[l++]);
		if (r & 1)
			minVal = min(m_min[--r], minVal);
	}
	return minVal;
}

int iASegmentTree::max_query(int l, int r)
{  
	int maxVal = INT_MIN;
	for (l += m_inputElemCnt, r += m_inputElemCnt; l < r; l >>= 1, r >>= 1)
	{
		if (l & 1)
			maxVal = max(maxVal, m_max[l++]);
		if (r & 1)
			maxVal = max(m_max[--r], maxVal);
	}
	return maxVal;
}
