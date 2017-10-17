#ifndef DEFECTCLASSIFIER_H
#define DEFECTCLASSIFIER_H

#include <string>
#include <vector>
#include "iAFeature.h"
#include <QVector>

#include "iAFiberCharacteristics.h"

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
		Vec3d Direction;
		double Angle;
		double Elongation;
		Vec3d Endpoints[2];
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
		std::string FibersFile;
		std::string FeaturesFile;
		std::string OutputDir;
	};


public:
						iADefectClassifier( );
	//void				run( std::string fibersFile, std::string featuresFile, std::string outputDir );
	void				run( Parameters params );

	Statistic			m_stat;


private:
	FeatureList			readDefects( std::string defectFile ) const;
	void				classify( FibersData* fibers, FeatureList* defects );
	void				save( ) const;
	void				calcStatistic( FeatureList* defects );
	ExtendedDefectInfo	calcExtendedDefectInfo( iAFeature& def ) const;
	FibersData			findNeighboringFibers( FibersData& fibers, ExtendedDefectInfo& defInfo, double distance ) const;

	Classification		m_classification;
	Parameters			m_param;
};

#endif // DEFECTCLASSIFIER_H