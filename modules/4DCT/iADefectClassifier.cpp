/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iADefectClassifier.h"

#include "iAFeature.h"
#include "iA4DCTDefects.h"

#include <vtkMath.h>

#include <fstream>
#include <map>
#include <chrono>


iADefectClassifier::iADefectClassifier( )
{
	/*m_param.Spacing = 2.5;
	m_param.ElongationP = 1.5;
	m_param.ElongationD = 3.;
	m_param.LengthRangeP[0] = 3.;
	m_param.LengthRangeP[1] = 12.;
	m_param.WidthRangeP[0] = 3.;
	m_param.WidthRangeP[1] = 12.;
	m_param.AngleP = 30.;
	m_param.AngleB = 20.;
	m_param.AngleD = 45;
	m_param.NeighborhoodDist = 10.;
	m_param.BigVolumeThreshold = 9999.;*/
}

//void DefectClassifier::run( std::string fibersFile, std::string featuresFile, std::string outputDir )
void iADefectClassifier::run( Parameters params )
{
	std::cout << "Defect classification started" << std::endl;

	m_param = params;

	FibersData fibers = Fiber::ReadFromCSV( m_param.FibersFile, m_param.Spacing );
	FeatureList defects = readDefects( m_param.FeaturesFile );
	classify( &fibers, &defects );
	calcStatistic( &defects );
	save( );

	std::cout << "Defect classification finished" << std::endl;
}

iADefectClassifier::FeatureList iADefectClassifier::readDefects( std::string defectFile ) const
{
	FeatureList result;
	std::ifstream file;
	file.open( defectFile );
	iAFeature f;
	while( file >> f ) result.push_back( f );
	return result;
}

void iADefectClassifier::classify( FibersData* fibers, FeatureList* defects )
{
	std::cout << "Classifying defects 0%... ";

	std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now( );
	unsigned int currIter = 0;

	for( auto def : *defects )
	{
		// progress reporting
		++currIter;
		std::chrono::duration<double> elapsed = std::chrono::system_clock::now( ) - currentTime;
		if( elapsed.count() > 10. )
		{
			std::cout << ( 100. / defects->size( ) ) * currIter << "%... ";
			currentTime = std::chrono::system_clock::now( );
		}

		ExtendedDefectInfo defInfo = calcExtendedDefectInfo( def );
		FibersData neighborFibersP = findNeighboringFibers( *fibers, defInfo, m_param.NeighborhoodDistP );
		FibersData neighborFibersFF = findNeighboringFibers( *fibers, defInfo, m_param.NeighborhoodDistFF );

		enum DefectNames { Fracture, Pulloout, Debonding, Breakage };
		DefectNames looksLike = DefectNames::Fracture;

		// pull-outs
		if( def.volume > m_param.BigVolumeThreshold )
		{
			if( defInfo.Elongation > m_param.ElongationP
				&& def.obbSize[1] > m_param.LengthRangeP[0]
				&& def.obbSize[1] < m_param.LengthRangeP[1]
				&& def.obbSize[2] > m_param.WidthRangeP[0]
				&& def.obbSize[2] < m_param.WidthRangeP[1]
				&& defInfo.Angle < m_param.AngleP * vtkMath::Pi() / 180
				&& neighborFibersP.size( ) >= 1 )
			{
				looksLike = DefectNames::Pulloout;
			}
		}
		else
		{
			if( neighborFibersP.size( ) >= 1 )
			{
				looksLike = DefectNames::Pulloout;
			}
		}

		// debondings
		if( defInfo.Elongation > m_param.ElongationD
			&& defInfo.Angle > m_param.AngleD * vtkMath::Pi() / 180 )
		{
			looksLike = DefectNames::Debonding;
		}

		// breakages
		if( looksLike == DefectNames::Pulloout
			&& neighborFibersFF.size( ) >= 2 )
		{
			double minAngle = 2 * vtkMath::Pi(); // maximum possible angle
			for( int i = 0; i < neighborFibersFF.size( ); ++i )
			{				
				for( int j = i + 1; j < neighborFibersFF.size( ); ++j )
				{
					double max[2], min[2];
					max[0] = std::max( neighborFibersFF[i].startPoint[2], neighborFibersFF[i].endPoint[2] );
					max[1] = std::max( neighborFibersFF[j].startPoint[2], neighborFibersFF[j].endPoint[2] );
					min[0] = std::min( neighborFibersFF[i].startPoint[2], neighborFibersFF[i].endPoint[2] );
					min[1] = std::min( neighborFibersFF[j].startPoint[2], neighborFibersFF[j].endPoint[2] );
					if( min[0] < max[1] && min[1] < max[0] ) continue;	// fibers are overlapped

					Vec3d dir[2];
					dir[0] = Vec3d( neighborFibersFF[i].endPoint ) - Vec3d( neighborFibersFF[i].startPoint );
					dir[1] = Vec3d( neighborFibersFF[j].endPoint ) - Vec3d( neighborFibersFF[j].startPoint );
					double angle = Vec3d::angle( dir[0], dir[1] );
					angle = angle > (vtkMath::Pi()/2) ? vtkMath::Pi() - angle : angle;
					if( minAngle > angle ) minAngle = angle;
				}
			}

			if( minAngle < m_param.AngleB * vtkMath::Pi() / 180 ) looksLike = DefectNames::Breakage;
		}

		switch( looksLike )
		{
		case DefectNames::Fracture:
			m_classification.Fractures.push_back( def.id );
			break;
		case DefectNames::Pulloout:
			m_classification.Pullouts.push_back( def.id );
			break;
		case DefectNames::Debonding:
			m_classification.Debondings.push_back( def.id );
			break;
		case DefectNames::Breakage:
			m_classification.Breakages.push_back( def.id );
			break;
		}
	}
	std::cout << "100%\n";
}

void iADefectClassifier::calcStatistic( FeatureList* defects )
{
	std::cout << "Calculating statistic......\n";

	m_stat = Statistic( );
	m_stat.fracturesCount = m_classification.Fractures.count( );
	m_stat.pulloutsCount = m_classification.Pullouts.count( );
	m_stat.debondingsCount = m_classification.Debondings.count( );
	m_stat.breakagesCount = m_classification.Breakages.count( );

	// create a map from id to feature
	std::map<unsigned long, iAFeature> idToFeature;
	for( auto def : *defects ) idToFeature[def.id] = def;

	for( auto i : m_classification.Fractures )	m_stat.fracturesVolume += idToFeature[i].volume;
	for( auto i : m_classification.Pullouts ) m_stat.pulloutVolume += idToFeature[i].volume;
	for( auto i : m_classification.Debondings ) m_stat.debondingsVolume += idToFeature[i].volume;
	for( auto i : m_classification.Breakages ) m_stat.breakagesVolume += idToFeature[i].volume;
}

iADefectClassifier::ExtendedDefectInfo iADefectClassifier::calcExtendedDefectInfo( iAFeature& def ) const
{
	ExtendedDefectInfo defInfo;
	defInfo.Direction = def.eigenvectors[2].normalized( );
	double angle = Vec3d::angle( defInfo.Direction, Vec3d( 0, 0, 1 ) );
	if( angle > vtkMath::Pi()/2) angle = vtkMath::Pi() - angle;
	defInfo.Angle = angle;
	defInfo.Elongation = def.obbSize[0] / def.obbSize[1];
	defInfo.Endpoints[0] = def.centroid + defInfo.Direction * def.obbSize[0] / 2;
	defInfo.Endpoints[1] = def.centroid - defInfo.Direction * def.obbSize[0] / 2.;
	return defInfo;
}

FibersData iADefectClassifier::findNeighboringFibers( FibersData& fibers, ExtendedDefectInfo& defInfo, double distance ) const
{
	FibersData neighborFibers;
	for( auto fib : fibers )
	{
		Vec3d fibEndpoints[2];
		fibEndpoints[0] = Vec3d( fib.startPoint );
		fibEndpoints[1] = Vec3d( fib.endPoint );
		bool isNeighbor = false;
		for( int i = 0; i < 2; i++ )
		{
			for( int j = 0; j < 2; j++ )
			{
				if( ( fibEndpoints[j] - defInfo.Endpoints[i] ).magnitude( ) < distance )
				{
					isNeighbor = true;
					break;
				}
			}
			if( isNeighbor ) break;
		}
		if( isNeighbor ) neighborFibers.push_back( fib );
	}
	return neighborFibers;
}

void iADefectClassifier::save( ) const
{
	std::cout << "Saving results......\n";

	QString qOutputDir = QString::fromStdString( m_param.OutputDir ) + '\\';
	iA4DCTDefects::save( m_classification.Fractures, qOutputDir + "ids_matrix_fractures.txt" );
	iA4DCTDefects::save( m_classification.Pullouts, qOutputDir + "ids_fiber_pull_outs.txt" );
	iA4DCTDefects::save( m_classification.Debondings, qOutputDir + "ids_fiber_matrix_debondings.txt" );
	iA4DCTDefects::save( m_classification.Breakages, qOutputDir + "ids_fiber_fractures.txt" );

	std::ofstream ofs;
	ofs.open( ( qOutputDir + "statistics.txt" ).toStdString( ) );
	ofs << "Parameters:\n";
	ofs << "Spacing = " << m_param.Spacing << std::endl;
	ofs << "Fiber fracture angle = " << m_param.AngleB << std::endl;
	ofs << "Fiber pull-out angle = " << m_param.AngleP << std::endl;
	ofs << "Fiber/matrix debonding angle = " << m_param.AngleD << std::endl;
	ofs << "Big volume threshold = " << m_param.BigVolumeThreshold << std::endl;
	ofs << "Fiber/matrix debonding elongation = " << m_param.ElongationD << std::endl;
	ofs << "Fiber pull-out elongation = " << m_param.ElongationP << std::endl;
	ofs << "Fiber pull-out length range start = " << m_param.LengthRangeP[0] << std::endl;
	ofs << "Fiber pull-out length range end = " << m_param.LengthRangeP[1] << std::endl;
	ofs << "Fiber pull-out width range start = " << m_param.WidthRangeP[0] << std::endl;
	ofs << "Fiber pull-out width range end = " << m_param.WidthRangeP[1] << std::endl;
	ofs << "Fiber pull-out neighborhood distance = " << m_param.NeighborhoodDistP << std::endl;
	ofs << "Fiber fracture neighborhood distance = " << m_param.NeighborhoodDistFF << std::endl;
	ofs << std::endl;

	ofs << "Number of matrix fractures = " << m_stat.fracturesCount << std::endl;
	ofs << "Volume of matrix fractures = " << m_stat.fracturesVolume << std::endl;
	ofs << "Number of fiber pull-outs = " << m_stat.pulloutsCount << std::endl;
	ofs << "Volume of fiber pull-outs = " << m_stat.pulloutVolume << std::endl;
	ofs << "Number of fiber/matrix debondings = " << m_stat.debondingsCount << std::endl;
	ofs << "Volume of fiber/matrix debondings = " << m_stat.debondingsVolume << std::endl;
	ofs << "Number of fiber fractures = " << m_stat.breakagesCount << std::endl;
	ofs << "Volume of fiber fractures = " << m_stat.breakagesVolume << std::endl;
	ofs.close( );
}
