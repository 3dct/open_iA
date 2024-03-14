// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVectorDistanceImpl.h"

#include "iASimpleTester.h"
#include "iAVectorArray.h"
#include "iAVectorTypeImpl.h"

class TestVectorArray: public iAVectorArray
{
private:
	std::vector<std::vector<iAVectorDataType> > m_data;
public:
	size_t size() const override
	{
		return m_data.size();
	}
	size_t channelCount() const override
	{
		return m_data[0].size();
	}
	std::shared_ptr<iAVectorType const> get(size_t voxelIdx) const override
	{
		return std::shared_ptr<iAVectorType const>(new iAPixelVector(*this, voxelIdx));
	}
	iAVectorDataType get(size_t voxelIdx, size_t channelIdx) const override
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
