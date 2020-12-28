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

#include "iAHistogramAbstract.h"

#include <QVector>
#include <QWidget>

class iAHistogramStackGrid;
class iATripleModalityWidget;

class iAMdiChild;

class QLabel;
class QSplitter;

class iAHistogramStack : public iAHistogramAbstract
{
public:
	iAHistogramStack(iATripleModalityWidget *tripleModalityWidget);

	// OVERRIDES
	void initialize(QString const names[3]) override;
	bool isSlicerInteractionEnabled() override { return true; }
	void updateModalityNames(QString const names[3]) override;

private:
	QSplitter *m_splitter;
	iAHistogramStackGrid *m_grid;
	QVector<QLabel*> m_labels;

	iATripleModalityWidget* m_tmw;
};