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

#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

#include "iAEnergySpectrum.h"

#define LINES_IN_HEADER 11

/**
 * \class	iAElementSpectralInfo
 *
 * \brief	Loads the spectral function and info from the correspondingly formatted txt file
 *
 */

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
			QRegExp splitter("\\s+");
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

	/**
	* Get energies in keV of the spectrum
	*/
	const QVector<float> & GetEnergyData() const
	{
		return m_energyData;
	}

	/**
	* Get counts of the spectrum
	*/
	const iAEnergySpectrum & GetCountsData() const
	{
		return m_countsData;
	}

	/**
	* Get count uncertainties of the spectrum
	*/
	const QVector<float> & GetUncertData() const
	{
		return m_uncertData;
	}

	/**
	* Get name of the chemical element
	*/
	const QString & GetName() const
	{
		return m_name;
	}

	const QString & GetSymbol() const
	{
		return m_symbol;
	}

private:
	QString						m_name;		///< name of the chemical element
	QString						m_symbol;	///< the symbol for this element
	QVector<float>			m_energyData;	///< energies in keV for measured spectrum points
	iAEnergySpectrum		m_countsData;	///< number of counted photons for measured spectrum points
	QVector<float>			m_uncertData;	///< uncertainty of the measured number of counts
};
