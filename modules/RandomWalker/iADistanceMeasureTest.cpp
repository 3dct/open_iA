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

#include "iAVectorDistanceImpl.h"

#include "iASimpleTester.h"
#include "iAVectorArray.h"
#include "iAVectorTypeImpl.h"

class TestVectorArray: public iAVectorArray
{
private:
	std::vector<std::vector<iAVectorDataType> > m_data;
public:
	virtual size_t size() const
	{
		return m_data.size();
	}
	virtual size_t channelCount() const
	{
		return m_data[0].size();
	}
	virtual QSharedPointer<iAVectorType const> get(size_t voxelIdx) const
	{
		return QSharedPointer<iAVectorType const>(new iAPixelVector(*this, voxelIdx));
	}
	virtual iAVectorDataType get(size_t voxelIdx, size_t channelIdx) const
	{
		return m_data[voxelIdx][channelIdx];
	}
	void set(size_t voxelIdx, size_t channelIdx, iAVectorDataType value)
	{
		if (m_data.size() <= voxelIdx)
		{
			m_data.resize(voxelIdx+1);
		}
		if (m_data[voxelIdx].size() <= channelIdx)
		{
			m_data[voxelIdx].resize(channelIdx+1);
		}
		m_data[voxelIdx][channelIdx] = value;
	}
};


BEGIN_TEST
	TestVectorArray fct3;
	fct3.set(0, 0, 0);
	fct3.set(0, 1, 1);
	fct3.set(0, 2, 0);
	fct3.set(1, 0, 1);
	fct3.set(1, 1, 3);
	fct3.set(1, 2, 2);

	TestVectorArray fct4;
	fct4.set(0, 0, 5);
	fct4.set(0, 1, 5);
	fct4.set(0, 2, 6);
	fct4.set(0, 3, 7);
	// len = 11.6189
	// sum = 23
	fct4.set(1, 0,  5);
	fct4.set(1, 1,  6);
	fct4.set(1, 2,  8);
	fct4.set(1, 3, 10);
	// len = 15
	// sum = 29

	iAL1NormDistance l1;
	iAL2NormDistance l2;
	iALInfNormDistance lid;
	iASpectralAngularDistance sad;
	iAJensenShannonDistance jsd;
	iAKullbackLeiblerDivergence kld;
	iAChiSquareDistance csd;
	iAEarthMoversDistance emd;

	TestEqualFloatingPoint( 5.0,        l1. GetDistance(fct3.get(0), fct3.get(1)) );
	TestEqualFloatingPoint( 3.0,        l2. GetDistance(fct3.get(0), fct3.get(1)) );
	TestEqualFloatingPoint( 2.0,        lid.GetDistance(fct3.get(0), fct3.get(1)) );
	TestEqualFloatingPoint( 0.80178372573727319,    sad.GetDistance(fct3.get(0), fct3.get(1)) );
	
	TestEqualFloatingPoint( 0.69314718, kld.GetDistance(fct3.get(0), fct3.get(1)) );
	TestEqualFloatingPoint( 0.41627731, jsd.GetDistance(fct3.get(0), fct3.get(1)) );
	TestEqualFloatingPoint( 0.33333333, csd.GetDistance(fct3.get(0), fct3.get(1)) );
	TestEqualFloatingPoint( 0.5,        emd.GetDistance(fct3.get(0), fct3.get(1)) );
	

	TestEqualFloatingPoint( 5.0,        l1. GetDistance(fct3.get(1), fct3.get(0)) );
	TestEqualFloatingPoint( 3.0,        l2. GetDistance(fct3.get(1), fct3.get(0)) );
	TestEqualFloatingPoint( 2.0,        lid.GetDistance(fct3.get(1), fct3.get(0)) );
	TestEqualFloatingPoint( 0.80178372573727319,    sad.GetDistance(fct3.get(1), fct3.get(0)) );

	// TestEqualFloatingPoint( 0.69314718, kld.GetDistance(fct3.get(0), fct3.get(1)) ); // kld is not symmetric!
	TestEqualFloatingPoint( 0.41627731, jsd.GetDistance(fct3.get(1), fct3.get(0)) );
	TestEqualFloatingPoint( 0.33333333, csd.GetDistance(fct3.get(1), fct3.get(0)) );
	TestEqualFloatingPoint( 0.5,        emd.GetDistance(fct3.get(1), fct3.get(0)) );



	TestEqualFloatingPoint( 6.0,               l1. GetDistance(fct4.get(0), fct4.get(1)) );
	TestEqualFloatingPoint( 3.74166,           l2. GetDistance(fct4.get(0), fct4.get(1)) );
	TestEqualFloatingPoint( 3.0,               lid.GetDistance(fct4.get(0), fct4.get(1)) );
	TestEqualFloatingPoint( 0.992631287250197225, sad.GetDistance(fct4.get(0), fct4.get(1)) );

	TestEqualFloatingPoint( 0.00856575,        kld.GetDistance(fct4.get(0), fct4.get(1)) );
	TestEqualFloatingPoint( 0.09175449,        jsd.GetDistance(fct4.get(0), fct4.get(1)) );
	TestEqualFloatingPoint( 0.004196113731758, csd.GetDistance(fct4.get(0), fct4.get(1)) );
	TestEqualFloatingPoint( 0.140929535232384, emd.GetDistance(fct4.get(0), fct4.get(1)) );
	
	TestEqualFloatingPoint( 6.0,               l1. GetDistance(fct4.get(1), fct4.get(0)) );
	TestEqualFloatingPoint( 3.74166,           l2. GetDistance(fct4.get(1), fct4.get(0)) );
	TestEqualFloatingPoint( 3.0,               lid.GetDistance(fct4.get(1), fct4.get(0)) );
	TestEqualFloatingPoint( 0.992631287250197225, sad.GetDistance(fct4.get(1), fct4.get(0)) );

	//TestEqualFloatingPoint( 0.00856575,        kld.GetDistance(fct4.get(0), fct4.get(1)) ); // kld is not symmetric!
	TestEqualFloatingPoint( 0.09175449,        jsd.GetDistance(fct4.get(1), fct4.get(0)) );
	TestEqualFloatingPoint( 0.004196113731758, csd.GetDistance(fct4.get(1), fct4.get(0)) );
	TestEqualFloatingPoint( 0.140929535232384, emd.GetDistance(fct4.get(1), fct4.get(0)) );

END_TEST
