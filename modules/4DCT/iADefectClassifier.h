// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAVec3.h>

#include <QVector>

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
