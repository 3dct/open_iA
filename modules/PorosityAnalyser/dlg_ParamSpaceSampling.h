/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QList>
#include <QTextDocument>
#include <QTextEdit>

#include "qcustomplot.h"

#include "ui_ParamSpaceSampling.h"

class MdiChild;
class QWidget;
class QErrorMessage;
class QLabel;
class QScrollArea;

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

	int numPara;
	int m_sbDelta, m_sbSigma, m_sbIsoX, m_emi_count, m_absorp_count;
	int emi_peaks[MAX_PEAK];
	int absorp_peaks[MAX_PEAK];
	int m_isoXGrayValue;
	int m_airPoreGrayValue;
	double* m_data[2];
	double m_highestFreq;
	QTextDocument* m_description;
	QString m_datasetDir, m_datasetName;
	QString tStr;
	QString m_filterName;
	QStringList m_datasetInfo;
	QStringList widgetList;
	QList<QCPGraph *> m_peakGraphList;
	QList<QVariant> m_inPara;
	QVector<double> m_keyData, m_valueData;
	QVector<double> m_smoothKey;
	QVector<double> m_smoothValue;
	QCustomPlot * m_histoPlot;
	QErrorMessage * eMessage;
	QScrollArea *scrollArea;
	QWidget *container;
	QGridLayout *containerLayout;
	QTextEdit * info;
	QWidget* histoBtnContainer;
	QHBoxLayout* histogramBtnContainer_HBLayout;
	QLabel* sbDelta_Label;
	QSpinBox* sbDelta;
	QLabel* sbSigma_Label;
	QSpinBox* sbSigma;
	QLabel* sbIsoX_Label;
	QSpinBox* sbIsoX;
	QLabel* cbSHLine_Label;
	QCheckBox* cbSHLine;

	void createDatasetPreview();
	void createDatasetInfo();
	void computeSmoothHisto( QVector<double> *m_smoothKey, QVector<double> *m_smoothValue );
	void computePeaks( QVector<double> key, QVector<double> value );
	void createHistoPlot();
	void createHistoSpinBoxes();
	void createHistoPeaks();
	void createSHLine();
	void createDescription();
	void updateLineEdits();
	void addUnits();
	void updateValues(QList<QVariant>);

protected:

	int detect_peak(
		const double*   data, /* the data */
		int             data_count, /* row count of data */
		int*            emi_peaks, /* emission peaks will be put here */
		int*            num_emi_peaks, /* number of emission peaks found */
		int             max_emi_peaks, /* maximum number of emission peaks */
		int*            absop_peaks, /* absorption peaks will be put here */
		int*            num_absop_peaks, /* number of absorption peaks found */
		int             max_absop_peaks, /* maximum number of absorption peaks */
		double          delta, /* delta used for distinguishing peaks */
		int             emi_first /* should we search emission peak first of absorption peak first? */
		);

};
