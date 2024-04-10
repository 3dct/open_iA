// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMeasures.h"

#include <iALog.h>

#include <itkImageRegionConstIterator.h>
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
		if (r < 0 || c < 0 || r >= rowCount || c >= colCount)
		{
			LOG(lvlError, QString("Invalid Matrix access (%1, %2), matrix dimensions (%3, %4)").arg(r).arg(c).arg(rowCount).arg(colCount));
		}
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
	QVector<double> & measures, bool reportUndecided)
{
	auto filter = itk::LabelOverlapMeasuresImageFilter<LabelImageType>::New();
	filter->SetSourceImage(refImg);
	filter->SetTargetImage(curImg);
	filter->Update();
	measures.push_back(filter->GetDiceCoefficient());

	// row      column
	Matrix errorMatrix(labelCount, labelCount);

	typedef itk::ImageRegionConstIterator<LabelImageType> ImgConstIter;
	ImgConstIter sampleIt(curImg, curImg->GetLargestPossibleRegion());
	ImgConstIter gtIt(refImg, refImg->GetLargestPossibleRegion());
	sampleIt.GoToBegin();
	gtIt.GoToBegin();
	int outsideValues = 0;
	while (!gtIt.IsAtEnd())
	{
		int refValue = gtIt.Get();
		if (refValue != -1)
		{
			int sampleValue = sampleIt.Get();
			if (sampleValue < labelCount)
			{
				errorMatrix.inc(sampleValue, refValue);
			}
			else
			{
				outsideValues++;
			}
		}
		++sampleIt;
		++gtIt;
	}
	int * actTot = rowTotals(errorMatrix);
	int * refTot = colTotals(errorMatrix);

	int diagSum = 0;
	int totalSum = 0;

	double precision = 0;
	double recall = 0;

	double chanceAgreement = 0;

	for (int i = 0; i<labelCount; ++i)
	{
		diagSum += errorMatrix.get(i, i);
		totalSum += actTot[i];
		precision += (actTot[i] == 0) ? 0 : errorMatrix.get(i, i) / static_cast<double>(actTot[i]); // could be equivalent to oa, have to check!
		recall += (refTot[i] == 0) ? 0 : errorMatrix.get(i, i) / static_cast<double>(refTot[i]);

		chanceAgreement = actTot[i] * refTot[i];
	}
	chanceAgreement /= static_cast<double>(std::pow(totalSum, 2));
	double oa = diagSum / static_cast<double>(totalSum);
	double kappa = (oa - chanceAgreement) / (1 - chanceAgreement);

	measures.push_back(kappa);
	measures.push_back(oa);
	measures.push_back(precision);
	measures.push_back(recall);
	for (int l = 0; l < labelCount; ++l)
	{
		measures.push_back(filter->GetDiceCoefficient(l));
	}

	if (reportUndecided)
	{
		// LOG(lvlWarn, QString("Encountered %1 pixel values out of valid range (0, %2)").arg(outsideValues).arg(labelCount));
		measures.push_back(outsideValues);
	}
	delete[] actTot;
	delete[] refTot;
}
