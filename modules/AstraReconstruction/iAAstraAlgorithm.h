// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAFilterDefault.h>
IAFILTER_DEFAULT_CLASS(iAAstraForwardProject);
IAFILTER_DEFAULT_CLASS(iAAstraReconstruct);

enum AstraReconstructionMethods
{
	BP3D,
	FDK3D,
	SIRT3D,
	CGLS3D
};

//! Container for constant names for parameters to Astra algorithms
class AstraParameters
{
public:
	// names of all parameters (to avoid ambiguous strings)
	static const QString ProjGeometry;
	static const QString DetSpcX;
	static const QString DetSpcY;
	static const QString ProjAngleStart;
	static const QString ProjAngleEnd;
	static const QString ProjAngleCnt;
	static const QString DetRowCnt;
	static const QString DetColCnt;
	static const QString ProjAngleDim;
	static const QString DetRowDim;
	static const QString DetColDim;
	static const QString DstOrigDet;
	static const QString DstOrigSrc;
	static const QString CenterOfRotCorr;
	static const QString CenterOfRotOfs;
	static const QString InitWithFDK;
	static const QString VolDimX;
	static const QString VolDimY;
	static const QString VolDimZ;
	static const QString VolSpcX;
	static const QString VolSpcY;
	static const QString VolSpcZ;
	static const QString AlgoType;
	static const QString NumberOfIterations;

	static QStringList algorithmStrings();
	static int mapAlgoStringToIndex(QString const& algo);
	static QString mapAlgoIndexToString(int astraIndex);
};

