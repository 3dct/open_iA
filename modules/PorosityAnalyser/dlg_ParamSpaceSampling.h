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
#pragma once

#include "ui_ParamSpaceSampling.h"

#include <charts/qcustomplot.h>

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QList>

class QCheckBox;
class QSpinBox;
class QTextDocument;
class QTextEdit;
class QWidget;

const int MAX_PEAK = 8000;

class dlg_ParamSpaceSampling : public QDialog, public Ui_ParamSpaceSampling
{
	Q_OBJECT

public:
	dlg_ParamSpaceSampling( QWidget *parent, QString winTitel, int n, QStringList inList, QList<QVariant> inPara,
							QTextDocument * fDescr, QString datasetDir, QString datasetName, QStringList datasetInfo, QVector<double> keyData,
							QVector<double> valueData, QString filterName, bool modal = false );
	double getValue(int index) const;

public slots:
	void updateHistoPeaks( int value );
	void updateHistoSmooth( int value );
	void updateIsoXPeak( int value );
	void updateSHLine( int value );

private:

	int m_numPara;
	int m_delta, m_sigma, m_isoX, m_emi_count, m_absorp_count;
	int m_emi_peaks[MAX_PEAK];
	int m_absorp_peaks[MAX_PEAK];
	int m_isoXGrayValue;
	int m_airPoreGrayValue;
	double* m_data[2];
	double m_highestFreq;
	QTextDocument* m_description;
	QString m_datasetDir, m_datasetName;
	QString m_filterName;
	QStringList m_datasetInfo;
	QStringList m_widgetList;
	QList<QCPGraph *> m_peakGraphList;
	QList<QVariant> m_inPara;
	QVector<double> m_keyData, m_valueData;
	QVector<double> m_smoothKey;
	QVector<double> m_smoothValue;
	QCustomPlot * m_histoPlot;
	QWidget* m_container;
	QSpinBox* m_sbIsoX;
	QCheckBox* m_cbSHLine;

	void createDatasetPreview();
	void createDatasetInfo();
	void computeSmoothHisto();
	void computePeaks( QVector<double> key, QVector<double> value );
	void createHistoPlot();
	void createHistoSpinBoxes();
	void createHistoPeaks();
	void createSHLine();
	void createDescription();
	void updateLineEdits();
	void addUnits();
	void updateValues(QList<QVariant>);
};
