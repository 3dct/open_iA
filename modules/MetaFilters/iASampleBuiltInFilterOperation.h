/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "iASampleOperation.h"

#include <QMap>
#include <QString>
#include <QVector>

class iAConnector;
class iALogger;

class iASampleBuiltInFilterOperation : public iASampleOperation
{
	Q_OBJECT
public:
	iASampleBuiltInFilterOperation(
		QString const& filterName,
		bool compressOutput,
		bool overwriteOutput,
		QMap<QString, QVariant> parameters,
		QVector<iAConnector*> input,
		QVector<QString> inputfileNames,
		QString const& outputFileName,
		iALogger * logger);
	QString output() const override;
private:
	void performWork() override;

	QString m_filterName;
	bool m_compressOutput, m_overwriteOutput;
	QMap<QString, QVariant> m_parameters;
	QVector<iAConnector*> m_input;
	QVector<QString> m_inputFileNames;
	QString m_outputFileName;
	iALogger * m_logger;
};
