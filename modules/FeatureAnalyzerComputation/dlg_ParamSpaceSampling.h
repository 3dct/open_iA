// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_ParamSpaceSampling.h"

#include <qcustomplot.h>

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

// TODO: avoid duplication with iAParameterDlg
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
