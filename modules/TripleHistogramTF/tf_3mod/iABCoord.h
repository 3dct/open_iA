// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class iABarycentricTriangle;

class iABCoord
{
public:
	iABCoord(double alpha, double beta);
	iABCoord() : iABCoord((double)1 / (double)3, (double)1 / (double)3)
	{}
	iABCoord(iABarycentricTriangle triangle, double x, double y);

	double getAlpha() const;
	double getBeta() const;
	double getGamma() const;
	bool isInside() const;

	double operator[] (int x)
	{
		switch (x)
		{
		case 0: return getAlpha();
		case 1: return getBeta();
		case 2: return getGamma();
		default: return 0;
		}
	}

	bool operator== (const iABCoord that)
	{
		return getAlpha() == that.getAlpha() && getBeta() == that.getBeta();
	}

	bool operator!= (const iABCoord that)
	{
		return getAlpha() != that.getAlpha() || getBeta() != that.getBeta();
	}

private:
	double m_alpha;
	double m_beta;

};
