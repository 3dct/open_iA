/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <QSharedPointer>
#include <QStringList>
#include <QVector>

class iACsvTableCreator;
class iAFiberResultsCollection;

class iASensitivityGUI;

class QDockWidget;
class QMainWindow;

class iASensitivityInfo
{
public:
	static QSharedPointer<iASensitivityInfo> create(QString const& parameterFileName, QSharedPointer<iAFiberResultsCollection> data);
	void createGUI(QMainWindow* child, QDockWidget* nextToDW);

	iASensitivityInfo(QSharedPointer<iAFiberResultsCollection> data);
	QSharedPointer<iAFiberResultsCollection> m_data;
	QStringList paramNames;
	//! "points" in parameter space at which the sensitivity was computed
	//! first index: parameter set; second index: parameter
	QVector<QVector<double>> paramSetValues;
	//! all samples points (i.e. all values from paramSetValues + points sampled for STAR around these points)
	//! NOTE: due to legacy reasons, swapped index order in comparison to paramSetValues!
	// TODO: unify index order?
	std::vector<std::vector<double>> allParamValues;
	//! indices of features for which sensitivity was computed
	QVector<int> charactIndex;
	//! which difference measures were used for distribution comparison
	QVector<int> charDiffMeasure;
	//! for which dissimilarity measure sensitivity was computed
	QVector<int> dissimMeasure;

	QVector<                //! For each result,
		QVector<	        //! for each characteristic,
		QVector<double>>>   //! a histogram.
		resultCharacteristicHistograms;

	int numOfSTARSteps;
	// CURRENTLY UNUSED: QVector<double> paramStep;  //! per varied parameter, the size of step performed for the STAR

	QVector<int> variedParams;  //! indices of the parameters that were varied

	// for each characteristic
	//     for each varied parameter
	//         for each selected characteristics difference measure
	//             for variation aggregation (see iASensitivityInfo::create)
	//                 for each point in parameter space (of base sampling method)
	//                     compute local change by that difference measure

	//! "sensitivity field":
	//! characteristic / parameter space point / parameter / diff measure
	QVector<    // characteristic (index in charactIndex)
		QVector<    // parameter index (second index in paramSetValues / allParamValues)
		QVector<    // characteristics difference measure index (index in charDiffMeasure)
		QVector<    // variation aggregation (see iASensitivityInfo::create)
		QVector<    // parameter set index (first index in paramSetValues)
		double
	>>>>> sensitivityField;

	//! averages over all parameter-set of above field ("global sensitivity" for a parameter)
	QVector<		// characteristis
		QVector<    // parameter index
		QVector<    // difference measure
		QVector<    // variation aggregation
		double
	>>>> aggregatedSensitivities;

	// per-object sensitivity:
	// required: 1-1 match between fibers
	// compute on the fly? spatial subdivision structure required...

	// Questions:
	// options for characteristic comparison:
	//    1. compute characteristic distribution difference
	//        - advantage: dissimilarity measure independent
	//        - disadvantage: distribution could be same even if lots of differences for single fibers
	//    2. compute matching fibers; then compute characteristic difference; then average this
	//        - advantage: represents actual differences better
	//        - disadvantage: depending on dissimilarity measure (since best match could be computed per dissimiliarity measure
	//    example: compare
	//         - result 1 with fibers a (len=5), b (len=3) and c (len=2)
	//         - result 2 with fibers A (len 3), B (len=2) and C (len=5)
	//         - best matches between result1&2: a <-> A, b <-> B, c <-> C
	//         - option 1 -> exactly the same, 1x5, 1x3, 1x2
	//         - option 2 -> length differences: 2, 1, 3

	QSharedPointer<iASensitivityGUI> m_gui;
};

// Factor out as generic CSV reading class also used by iACsvIO?
bool readParameterCSV(QString const& fileName, QString const& encoding, QString const& columnSeparator, iACsvTableCreator& tblCreator, size_t resultCount);
