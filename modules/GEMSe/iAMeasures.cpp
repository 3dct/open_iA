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

	delete[] actTot;
	delete[] refTot;
}
