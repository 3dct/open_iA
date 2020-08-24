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

#include "iANModalObjects.h"

#include "QSharedPointer.h"

#include "vtkSmartPointer.h"

#include <vector>

class iAModality;

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class iANModalTFManager {
public:
	iANModalTFManager(QSharedPointer<iAModality> modality);

	void addControlPoint(unsigned int x, const iANModalLabel &label);
	void addControlPoint(unsigned int x, const double (&rgba)[4]);

	void removeControlPoint(unsigned int x);
	void removeControlPoints(int labelId);
	void removeAllControlPoints();

	void updateLabels(const std::vector<iANModalLabel> &labels);

	void update();

	unsigned int minx() { return 0; }
	unsigned int maxx() { return m_cps.size() - 1; }

private:

	struct CP {
		CP() : x(0), r(0), g(0), b(0), a(0), labelId(-1) {}
		CP(unsigned int X, float R, float G, float B, float A) : x(X), r(R), g(G), b(B), a(A), labelId(-1) {}
		CP(unsigned int X, float R, float G, float B, float A, int LabelId) : x(X), r(R), g(G), b(B), a(A), labelId(LabelId) {}

		unsigned int x;
		float r, g, b, a;
		int labelId;

		bool operator==(const CP &other) const {
			return (null() && other.null()) || (null() == other.null() && labelId == other.labelId);
		}

		bool operator!=(const CP &other) const {
			return !operator==(other);
		}

		bool null() const {
			return labelId < 0;
		}
	};

	vtkSmartPointer<vtkColorTransferFunction> m_colorTf;
	vtkSmartPointer<vtkPiecewiseFunction> m_opacityTf;
	std::vector<CP> m_cps;

	inline void addControlPointToTfs(const CP &cp);
	inline void removeControlPointFromTfs(unsigned int x);

};