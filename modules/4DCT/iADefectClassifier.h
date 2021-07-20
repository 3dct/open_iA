/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <iAVec3.h>

#include <QVector>

#include <string>
#include <vector>

class Fiber;
typedef std::vector<Fiber> FibersData;
struct iAFeature;

class iADefectClassifier
{
private:
	typedef std::vector<iAFeature> FeatureList;

	struct Statistic
	{
		Statistic( )
		{
			fracturesCount = 0;
			fracturesVolume = 0;
			pulloutsCount = 0;
			pulloutVolume = 0;
			debondingsCount = 0;
			debondingsVolume = 0;
			breakagesCount = 0;
			breakagesVolume = 0;
		}
		unsigned long fracturesCount;
		double fracturesVolume;
		unsigned long pulloutsCount;
		double pulloutVolume;
		unsigned long debondingsCount;
		double debondingsVolume;
		unsigned long breakagesCount;
		double breakagesVolume;
	};

	struct Classification
	{
		QVector<unsigned long> Fractures, Pullouts, Debondings, Breakages;
	};

	struct ExtendedDefectInfo
	{
		iAVec3d Direction;
		double Angle;
		double Elongation;
		iAVec3d Endpoints[2];
	};

public:
	struct Parameters
	{
		double Spacing;
		double ElongationP;
		double ElongationD;
		double LengthRangeP[2];
		double WidthRangeP[2];
		double AngleP;
		double AngleB;
		double AngleD;
		double NeighborhoodDistP;
		double NeighborhoodDistFF;
		double BigVolumeThreshold;
		QString FibersFile;
		QString FeaturesFile;
		QString OutputDir;
	};


public:
						iADefectClassifier( );
	//void				run( std::string fibersFile, std::string featuresFile, std::string outputDir );
	void				run( Parameters params );

	Statistic			m_stat;


private:
	FeatureList			readDefects( QString const & defectFile ) const;
	void				classify( FibersData* fibers, FeatureList* defects );
	void				save( ) const;
	void				calcStatistic( FeatureList* defects );
	ExtendedDefectInfo	calcExtendedDefectInfo( iAFeature& def ) const;
	FibersData			findNeighboringFibers( FibersData& fibers, ExtendedDefectInfo& defInfo, double distance ) const;

	Classification		m_classification;
	Parameters			m_param;
};
