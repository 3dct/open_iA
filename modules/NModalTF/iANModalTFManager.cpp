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

#include "iANModalTFManager.h"

#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "iAPerformanceHelper.h"

#include "vtkSmartPointer.h"
#include "vtkImageData.h"

#include <assert.h>

iANModalTFManager::iANModalTFManager(QSharedPointer<iAModality> modality) 
	: m_colorTf(modality->transfer()->colorTF()), m_opacityTf(modality->transfer()->opacityTF()) {

	double *range = modality->image()->GetScalarRange();
	double rangeMax = range[1];
	assert(rangeMax < 65536);

	m_cps.resize(rangeMax + 1);
}

void iANModalTFManager::addControlPoint(unsigned int x, const iANModalLabel &label) {
	assert(label.id >= 0);
	auto c = label.color;
	m_cps[x] = CP(x, c.redF(), c.greenF(), c.blueF(), label.opacity, label.id);
}

void iANModalTFManager::addControlPoint(unsigned int x, const double(&rgba)[4]) {
	addControlPointToTfs(CP(x, rgba[0], rgba[1], rgba[2], rgba[3]));
}

void iANModalTFManager::removeControlPoint(unsigned int x) {
	m_cps[x] = CP();
	removeControlPointFromTfs(x);
}

void iANModalTFManager::update() {
	assert(m_colorTf->GetSize() == m_opacityTf->GetSize());

	std::vector<CP> unmanaged_cps;
	double c[6], o[4];
	for (int i = 0; i < m_colorTf->GetSize(); ++i) {
		m_opacityTf->GetNodeValue(i, o);
		unsigned int x = o[0];
		if (m_cps[x].null()) {
			m_colorTf->GetNodeValue(i, c);
			assert(o[0] == c[0]);
			unmanaged_cps.push_back(CP(x, c[1], c[2], c[3], o[1]));
		}
	}
	unmanaged_cps.shrink_to_fit();
	m_colorTf->RemoveAllPoints();
	m_opacityTf->RemoveAllPoints();

	assert(m_colorTf->GetSize() == m_opacityTf->GetSize());

	// Naive parallelization of this loop here.
	// Because of that, the result may contain more than two adjacent control points with the same color/opacity.
	// That's ok though :)
	iATimeGuard *tg = new iATimeGuard("Go through all 65536 control points");
#pragma omp parallel
	{
		bool repeated = false;
		CP prev;
#pragma omp for
		for (int i = 0; i < m_cps.size(); ++i) {
			const CP &cp = m_cps[i];
			if (!cp.null()) {
				if (prev == cp) {
					prev.x = cp.x;
					repeated = true;
				} else {
					if (repeated) {
#pragma omp critical
						addControlPointToTfs(prev);
						repeated = false;
					}
#pragma omp critical
					addControlPointToTfs(cp);
					prev = cp;
				}
			}
		}
		if (repeated) {
#pragma omp critical
			addControlPointToTfs(prev);
		}
	} // end of parallel block
	delete tg;

	assert(m_colorTf->GetSize() == m_opacityTf->GetSize());

	for (int i = 0; i < unmanaged_cps.size(); ++i) {
		addControlPointToTfs(unmanaged_cps[i]);
	}

	assert(m_colorTf->GetSize() == m_opacityTf->GetSize());
}

void iANModalTFManager::updateLabels(const std::vector<iANModalLabel> &labels) {
	// Put labels in an indexed array for faster access
	int maxId = -1;
	for (auto label : labels) {
		if (label.id > maxId) {
			maxId = label.id;
		}
	}
	std::vector<iANModalLabel> labels_indexed(maxId + 1);
	for (auto label : labels) {
		labels_indexed[label.id] = label;
	}

	// Now update control points :)
	iANModalLabel *labelPtr = labels_indexed.data();
	CP *cpPtr = m_cps.data();
#pragma omp parallel for
	for (int i = 0; i < m_cps.size(); ++i) {
		if (!cpPtr[i].null()) {
			int labelId = cpPtr[i].labelId;
			if (labelId <= maxId) {
				auto label = labelPtr[labelId];
				if (!label.null()) {
					cpPtr[i].r = label.color.redF();
					cpPtr[i].g = label.color.greenF();
					cpPtr[i].b = label.color.blueF();
					cpPtr[i].a = label.opacity;
				}
			}
		}
	}
}

void iANModalTFManager::removeControlPoints(int labelId) {
	assert(m_colorTf->GetSize() == m_opacityTf->GetSize());

	// Remove control points from TF first
	std::vector<unsigned int> to_be_removed;
	double c[6], o[4];
	for (int i = 0; i < m_opacityTf->GetSize(); ++i) {
		m_opacityTf->GetNodeValue(i, o);
		unsigned int x = o[0];
		if (m_cps[x].labelId == labelId) {
			to_be_removed.push_back(x);
		}
	}
#pragma omp parallel sections
	{
#pragma omp section
		for (unsigned int x : to_be_removed) {
			m_colorTf->RemovePoint(x);
		}
#pragma omp section
		for (unsigned int x : to_be_removed) {
			m_opacityTf->RemovePoint(x);
		}
	}

	// Then remove control points from our data structure
	CP *ptr = m_cps.data();
#pragma omp parallel for
	for (int i = 0; i < m_cps.size(); ++i) {
		if (ptr[i].labelId == labelId) {
			ptr[i] = CP();
		}
	}
}

void iANModalTFManager::removeAllControlPoints() {
	CP *ptr = m_cps.data();
#pragma omp parallel
	{
#pragma omp sections
		{
#pragma omp section
			for (int i = 0; i < m_cps.size(); ++i) {
				if (!ptr[i].null()) {
					m_colorTf->RemovePoint(i);
				}
			}
#pragma omp section
			for (int i = 0; i < m_cps.size(); ++i) {
				if (!ptr[i].null()) {
					m_opacityTf->RemovePoint(i);
				}
			}
		} // end of sections
#pragma omp for
		for (int i = 0; i < m_cps.size(); ++i) {
			ptr[i] = CP();
		}
	} // end of parallel block
}

inline void iANModalTFManager::addControlPointToTfs(const CP &cp) {
	double mul = cp.a == 0 ? 0 : 1;
	m_colorTf->AddRGBPoint(cp.x , cp.r * mul, cp.g * mul, cp.b * mul);
	m_opacityTf->AddPoint(cp.x, cp.a);
}

inline void iANModalTFManager::removeControlPointFromTfs(unsigned int x) {
	m_colorTf->RemovePoint(x);
	m_opacityTf->RemovePoint(x);
}