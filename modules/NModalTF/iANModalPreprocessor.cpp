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

#include "iANModalPreprocessor.h"
#include "iANModalModalityReducer.h"
#include "iANModalBackgroundRemover.h"

#include "iAModality.h"

#include <vtkImageData.h>

iANModalPreprocessor::iANModalPreprocessor(QSharedPointer<iANModalModalityReducer> modalityReducer, QSharedPointer<iANModalBackgroundRemover> backgroundRemover)
	:
	m_modalityReducer(modalityReducer),
	m_backgroundRemover(backgroundRemover)
{

}

namespace {
	inline bool promptReduceNumOfModalitiesFirst() {
		return false; // TODO
	}
	inline bool promptReduceNumOfModalitiesSecond() {
		return true; // TODO
	}
	inline bool promptRemoveBackground() {
		return true; // TODO
	}
}

iANModalPreprocessorOutput iANModalPreprocessor::preprocess(QList<QSharedPointer<iAModality>> modalities) {

	iANModalPreprocessorOutput output;

	QList<ModalitiesGroup> groups;
	groupModalities(modalities, groups);
	modalities = chooseGroup(groups);

	if (promptReduceNumOfModalitiesFirst()) {
		// if number of modalities is to be reduced before background removal
		modalities = m_modalityReducer->reduce(modalities);
		if (promptRemoveBackground()) {
			auto mask = m_backgroundRemover->removeBackground(modalities);
			if (mask != nullptr) {
				output.mask = mask;
			}
		}

	} else {
		// if number of modalities is to be reduced after background removal
		if (promptRemoveBackground()) {
			auto mask = m_backgroundRemover->removeBackground(modalities);
			if (mask != nullptr) {
				output.mask = mask;
			}
		}

		if (modalities.size() > m_modalityReducer->maxOutputLength() || promptReduceNumOfModalitiesSecond()) {
			// if we have more modalities than allowed, force reduction
			modalities = m_modalityReducer->reduce(modalities);
		}
	}

	output.modalities = modalities;

	return output;
}

bool iANModalPreprocessor::areModalitiesCompatible(QSharedPointer<iAModality> m1, QSharedPointer<iAModality> m2) {
	return true; // TODO
}

void iANModalPreprocessor::groupModalities(QList<QSharedPointer<iAModality>> modalitiesToGroup, QList<ModalitiesGroup> &output) {
	// TODO
	// Currently returning same list as in input
	auto dims = modalitiesToGroup[0]->image()->GetDimensions();
	auto group = ModalitiesGroup();
	group.dimx = dims[0];
	group.dimy = dims[1];
	group.dimz = dims[2];
	group.modalities = modalitiesToGroup;
	output.append(group);
}

QList<QSharedPointer<iAModality>> iANModalPreprocessor::chooseGroup(QList<ModalitiesGroup> groups) {
	// TODO
	return groups[0].modalities;
}