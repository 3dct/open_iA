/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "iAMeasures.h"

#include <itkLabelOverlapMeasuresImageFilter.h>

class Matrix
{
private:
	int * data;
	int rowCount, colCount;
public:
	Matrix(int rowCount, int colCount) :
		rowCount(rowCount), colCount(colCount)
	{
		data = new int[rowCount * colCount];
		for (int i = 0; i<rowCount * colCount; ++i)
		{
			data[i] = 0;
		}
	}
	~Matrix()
	{
		delete[] data;
	}
	int & get(int r, int c)
	{
		return data[r*colCount + c];
	}
	void inc(int r, int c)
	{
		get(r, c)++;
	}
	int RowCount() const { return rowCount; }
	int ColCount() const { return colCount; }
};

int * rowTotals(Matrix & m)
{
	int * result = new int[m.RowCount()];
	for (int r = 0; r<m.RowCount(); ++r)
	{
		result[r] = 0;
		for (int c = 0; c<m.ColCount(); ++c)
		{
			result[r] += m.get(r, c);
		}
	}
	return result;
}
int * colTotals(Matrix & m)
{
	int * result = new int[m.ColCount()];
	for (int c = 0; c<m.ColCount(); ++c)
	{
		result[c] = 0;
		for (int r = 0; r<m.RowCount(); ++r)
		{
			result[c] += m.get(r, c);
		}
	}
	return result;
}

void CalculateMeasures(LabelImagePointer refImg, LabelImagePointer curImg, int labelCount,
	double & dice, double & kappa, double & oa, double &precision, double &recall)
{
	typedef itk::LabelOverlapMeasuresImageFilter<LabelImageType > FilterType;
	FilterType::Pointer filter = FilterType::New();
	filter->SetSourceImage(refImg);
	filter->SetTargetImage(curImg);
	filter->Update();
	dice = filter->GetMeanOverlap();


	// row      column
	Matrix errorMatrix(labelCount, labelCount);
	LabelImageType::RegionType reg = refImg->GetLargestPossibleRegion();
	LabelImageType::SizeType size = reg.GetSize();
	LabelImageType::IndexType idx;
	for (idx[0] = 0; idx[0] < size[0]; ++idx[0])
	{
		for (idx[1] = 0; idx[1] < size[1]; ++idx[1])
		{
			for (idx[2] = 0; idx[2] < size[2]; ++idx[2])
			{
				int refValue = refImg->GetPixel(idx);
				int curValue = curImg->GetPixel(idx);
				errorMatrix.inc(curValue, refValue);
			}
		}
	}
	int * actTot = rowTotals(errorMatrix);
	int * refTot = colTotals(errorMatrix);

	int diagSum = 0;
	int totalSum = 0;

	precision = 0;
	recall = 0;

	double chanceAgreement = 0;

	for (int i = 0; i<labelCount; ++i)
	{
		diagSum += errorMatrix.get(i, i);
		totalSum += actTot[i];
		precision += (actTot[i] == 0) ? 0 : errorMatrix.get(i, i) / static_cast<double>(actTot[i]); // could be equivalent to oa, have to check!
		recall += (refTot[i] == 0) ? 0 : errorMatrix.get(i, i) / static_cast<double>(refTot[i]);

		chanceAgreement = actTot[i] * refTot[i];
	}
	chanceAgreement /= static_cast<double>(pow(totalSum, 2));


	oa = diagSum / static_cast<double>(totalSum);

	kappa = (oa - chanceAgreement) / (1 - chanceAgreement);

	/*

	typedef itk::ImageFileReader<DoubleImageType> DoubleReaderType;
	DoubleReaderType::Pointer DoubleReader = DoubleReaderType::New();
	DoubleReader->SetFileName(patientMHAs[nrModals].c_str());	// ground_truth
	DoubleReader->Update();
	DoubleImageType::Pointer m_gtImage = dynamic_cast<DoubleImageType *>(DoubleReader->GetOutput());

	cout << "1.1" << endl;

	std::string output;
	int nrOferrorOverall = 0, nrOfVoxelsOverall = 0;
	LabelImageType* m_LabelImage = dynamic_cast<LabelImageType*>(m_result->GetLabelledImage().GetPointer());

	cout << "1.2" << endl;

	itk::ImageRegionIterator<LabelImageType> sampleIter(m_LabelImage, m_LabelImage->GetLargestPossibleRegion());
	itk::ImageRegionIterator<DoubleImageType> gtIter(m_gtImage, m_gtImage->GetLargestPossibleRegion());
	sampleIter.GoToBegin();
	gtIter.GoToBegin();

	cout << "1.3" << endl;

	while (!gtIter.IsAtEnd())
	{
		if (gtIter.Get() != -1) {
			if (gtIter.Get() != sampleIter.Get()) {
				nrOferrorOverall++;
			}
			nrOfVoxelsOverall++;
		}
		++sampleIter;
		++gtIter;
	}

	cout << "1.4" << endl;

	double successRate = 100.0 - (nrOferrorOverall / (nrOfVoxelsOverall / 100.0));

	cout << "1.5" << endl;

	typedef itk::CastImageFilter< DoubleImageType, IntImageType > CastFilterType;
	CastFilterType::Pointer castFilterGroundTruth = CastFilterType::New();
	castFilterGroundTruth->SetInput(m_gtImage);

	cout << "1.6" << endl;

	typedef itk::LabelOverlapMeasuresImageFilter<LabelImageType> DiceType;
	DiceType::Pointer DiceCompare = DiceType::New();
	DiceCompare->SetSourceImage(m_LabelImage);
	DiceCompare->SetTargetImage(castFilterGroundTruth->GetOutput());
	DiceCompare->Update();

	cout << "1.7" << endl;

	double diceValue = DiceCompare->GetDiceCoefficient();

	m_result->SetAttribute(m_objCountIdx + 2, successRate);
	m_result->SetAttribute(m_objCountIdx + 3, diceValue);
	m_result->SetAttribute(m_objCountIdx + 4, DiceCompare->GetDiceCoefficient(0));
	m_result->SetAttribute(m_objCountIdx + 5, DiceCompare->GetDiceCoefficient(1));
	m_result->SetAttribute(m_objCountIdx + 6, DiceCompare->GetDiceCoefficient(2));
	m_result->SetAttribute(m_objCountIdx + 7, DiceCompare->GetDiceCoefficient(3));
	m_result->SetAttribute(m_objCountIdx + 8, DiceCompare->GetDiceCoefficient(4));

	*/


	delete[] actTot;
	delete[] refTot;
}
