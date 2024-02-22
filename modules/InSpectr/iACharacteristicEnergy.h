// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <QVector>

#include <vector>

struct iACharacteristicEnergy
{
	QString symbol;
	// order of energies:
	enum
	{
		Kalpha1,
		Kalpha2,
		Kbeta1,
		Lalpha1,
		Lalpha2,
		Lbeta1,
		Lbeta2,
		Lgamma1,
		Malpha1
	};

	static const double NotAvailable;
	std::vector<double> energies;

};

class EnergyLoader
{
public:
	static void Load(QString const & filename, QVector<iACharacteristicEnergy>& output);
private:
	static iACharacteristicEnergy ConstructElement(QString line);
};
