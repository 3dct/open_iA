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

#include "iANModalPCAModalityReducer.h"

// Input modalities (volumes) must have the exact same dimensions
QList<QSharedPointer<iAModality>> iANModalModalityReducer::reduce(QList<QSharedPointer<iAModality>> modalities) {

	// Assert if all modalities have the same dimensions


	// Flatten volumes
	// - all volumes have the same dimensions N1 x N2 x N3
	// - number of volumes: M
	// - flattened volumes: 2D matrix of size N1*N2*N3 x M


	// Calculate mean of each of the M vectors
	// ITK mean image filter: https://itk.org/Doxygen/html/classitk_1_1MeanImageFilter.html
	// ITK statistics image filter: https://itk.org/Doxygen/html/classitk_1_1StatisticsImageFilter.html


	// Subtract each element of the M vectors by their respective mean
	// ITK subtract image filter: https://itk.org/Doxygen/html/classitk_1_1SubtractImageFilter.html


	// Calculate covariance matrix
	// ITK covariance calculator: https://itk.org/Doxygen320/html/classitk_1_1Statistics_1_1CovarianceCalculator.html
	// ITK covariance sample filter: https://itk.org/Doxygen/html/classitk_1_1Statistics_1_1CovarianceSampleFilter.html


	// Perform eigen analysis on the covariance matrix
	// ITK symmetric eigen analysis: https://itk.org/Doxygen/html/classitk_1_1SymmetricEigenAnalysis.html


	// Reshape eigenvectors (each of size N1*N2*N3 x 1) into volumes
	// - we will now again have a number M of volumes with dimensions N1 x N2 x N3
	// - if  M  >  K = maxOutputLength()  then only necessary to reshape the K eigenvectors with largest eigenvalues


	// Ready to output :)
	// - length of output list <= maxOutputLength()
	auto output = modalities;
	assert(output.size() <= maxOutputLength());
	return output;
}