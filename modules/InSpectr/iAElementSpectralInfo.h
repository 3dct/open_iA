// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAEnergySpectrum.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

#define LINES_IN_HEADER 11

//! Loads the spectral function and info from the correspondingly formatted txt file
class iAElementSpectralInfo
{
public:
	iAElementSpectralInfo(QString & elementName, QString & fileName) : m_name(elementName)
	{
		QFileInfo fi(fileName);
		if(fi.exists())
		{
			QString symb = fi.baseName();
			m_symbol = symb[0].toUpper() + symb.mid(1);
			QFile file(fileName);
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				return;

			//skip the header
			for(int i=0; i<LINES_IN_HEADER; ++i)
				file.readLine();

			//read the data
			QStringList stringList;
			QRegularExpression splitter("\\s+");
			while (!file.atEnd())
			{
				stringList = QString(file.readLine()).trimmed().split(splitter);
				m_energyData.push_back( stringList[1].toFloat() );
				m_countsData.push_back( stringList[2].toUInt() );
				m_uncertData.push_back( stringList[3].toFloat() );
			}
			file.close(); //cleanup
		}
	}

	//! Get energies in keV of the spectrum
	const QVector<float> & GetEnergyData() const
	{
		return m_energyData;
	}

	//! Get counts of the spectrum
	const iAEnergySpectrum & GetCountsData() const
	{
		return m_countsData;
	}

	//! Get count uncertainties of the spectrum
	const QVector<float> & GetUncertData() const
	{
		return m_uncertData;
	}

	//! Get name of the chemical element
	const QString & name() const
	{
		return m_name;
	}

	//! Get symbol (abbreviation) for this chemical element
	const QString & GetSymbol() const
	{
		return m_symbol;
	}

private:
	QString          m_name;       //!< name of the chemical element
	QString          m_symbol;     //!< the symbol for this element
	QVector<float>   m_energyData; //!< energies in keV for measured spectrum points
	iAEnergySpectrum m_countsData; //!< number of counted photons for measured spectrum points
	QVector<float>   m_uncertData; //!< uncertainty of the measured number of counts
};
