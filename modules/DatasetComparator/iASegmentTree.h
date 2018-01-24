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

#include <vector>

using namespace std;

// Resource: http://codeforces.com/blog/entry/18051

class iASegmentTree
{
//TODO: to unsigned int or other more general type?
public:
	iASegmentTree(const vector<int> &input, int binCnt, int lowerBnd, int upperBnd);
	~iASegmentTree();
	vector<int> iASegmentTree::hist_query(int l, int r);
	double avg_query(int l, int r);
	int min_query(int l, int r);
	int max_query(int l, int r);

private:
	int m_inputElemCnt;
	vector<int> m_input;
	vector<vector<int>> m_hist;
	vector<double> m_avg;
	vector<int> m_min;
	vector<int> m_max;
	void hist_build();
	void sum_build();
	void min_build();
	void max_build();
};