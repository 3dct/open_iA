// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMap>
#include <QString>
#include <QVector>

class QSettings;

class iAEnsembleDescriptorFile
{
public:

	static const QString DefaultSMPFileName;
	static const QString DefaultSPSFileName;
	static const QString DefaultCHRFileName;

	iAEnsembleDescriptorFile(QSettings const& metaFile, QString const & ensembleFileName);
	/*
	iAEnsembleDescriptorFile(
		int labelCount,
		QMap<int, QString> const & samplings,
		QString const & layoutName,
		QString const & refImg,
		QString const & hiddenCharts,
		QString const & colorThemeName,
		QString const & labelNames
	);
	*/
	void store(QSettings& metaFile, QString const & ensembleFileName);
	QString const & fileName() const;
	int labelCount() const;
	QMap<int, QString> const & samplings() const;
	QString const & layoutName() const;
	QString const & referenceImage() const;
	QString const & hiddenCharts() const;
	QString const & labelNames() const;
	QString const & colorTheme() const;

	qsizetype subEnsembleCount() const;
	QVector<int> subEnsemble(qsizetype idx) const;
	int subEnsembleID(qsizetype idx) const;

	void addSubEnsemble(int id, QVector<int> const & members);

	bool good() const;
private:
	int m_LabelCount;
	QMap<int, QString> m_Samplings;
	QString m_LayoutName;
	QString m_fileName;
	QString m_RefImg;
	QString m_HiddenCharts;
	QString m_ColorTheme;
	QString m_LabelNames;

	QVector<QVector<int> > m_subEnsembles;
	QVector<int> m_subEnsembleID;

	bool m_good;
};
