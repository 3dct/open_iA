// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iANModalObjects.h"

#include <vector>

class iATransferFunction;

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class iANModalTFManager
{
public:
	iANModalTFManager(iATransferFunction* transfer);

	void addControlPoint(unsigned int x, const iANModalLabel& label);
	void addControlPoint(unsigned int x, const double (&rgba)[4]);

	void removeControlPoint(unsigned int x);
	void removeControlPoints(int labelId);
	void removeAllControlPoints();

	void updateLabels(const std::vector<iANModalLabel>& labels);

	void update();

	unsigned int minx()
	{
		return 0;
	}
	unsigned int maxx()
	{
		return static_cast<unsigned int>(m_cps.size() - 1);
	}
	iATransferFunction* tf()
	{
		return m_tf;
	}

private:
	struct CP
	{
		CP() : x(0), r(0), g(0), b(0), a(0), labelId(-1)
		{
		}
		CP(unsigned int X, float R, float G, float B, float A) : x(X), r(R), g(G), b(B), a(A), labelId(-1)
		{
		}
		CP(unsigned int X, float R, float G, float B, float A, int LabelId) :
			x(X), r(R), g(G), b(B), a(A), labelId(LabelId)
		{
		}

		unsigned int x;
		float r, g, b, a;
		int labelId;

		bool operator==(const CP& other) const
		{
			return (null() && other.null()) || (null() == other.null() && labelId == other.labelId);
		}

		bool operator!=(const CP& other) const
		{
			return !operator==(other);
		}

		bool null() const
		{
			return labelId < 0;
		}
	};

	iATransferFunction* m_tf;
	std::vector<CP> m_cps;

	inline void addControlPointToTfs(const CP& cp);
	inline void removeControlPointFromTfs(unsigned int x);
};
