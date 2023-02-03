// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vector>

// Resource: http://codeforces.com/blog/entry/18051

class iASegmentTree
{
//TODO: to unsigned int or other more general type
public:
	iASegmentTree(const std::vector<int> &input, int binCnt, int lowerBnd, int upperBnd);
	~iASegmentTree();
	std::vector<int> hist_query(int l, int r);
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
};
