/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <iAMathUtility.h>

#include <QPointF>

#include <QString>

#include <stdexcept>
#include <vector>

namespace threshold_defs
{
	const double dblInf_min = -std::numeric_limits<double>::infinity();
	const double fltInf_min = -std::numeric_limits<float>::infinity();

	//storing lokal and global maximum
	class iAGreyThresholdPeaks
	{
	public:
		iAGreyThresholdPeaks() :
			m_lokalMax(dblInf_min), m_globalMax(dblInf_min) {}

		void init(double minThr, double maxThr)
		{
			m_lokalMax = minThr;
			m_globalMax = maxThr;
		}

		double getLocalMax() const
		{
			return m_lokalMax;
		}

		double getGlobalMax() const
		{
			return m_globalMax;
		}
	private:
		double m_lokalMax; //<!Air peak
		double m_globalMax;
	};

	//Ranges in XY direction
	class iAParametersRanges {

	public:
		iAParametersRanges()
		{
			x_vals.reserve(1000);
			y_vals.reserve(1000);
		}

		iAParametersRanges(const std::vector<double> &xVals, const std::vector<double> &yVals):
			x_vals(xVals), y_vals(yVals)
		{}

		void insertElem(double x, double y)
		{
			x_vals.push_back(x);
			y_vals.push_back(y);
		}

		void clearElemems()
		{
			x_vals.clear();
			y_vals.clear();
			x_vals.reserve(1000);
			y_vals.reserve(1000);
		}

		const std::vector<double>& getXRange() const
		{
			return x_vals;
		}
		const std::vector<double>& getYRange() const
		{
			return y_vals;
		}

		double getXMin() const
		{
			double min = *std::min_element(std::begin(x_vals), std::end(x_vals));
			return min;
		}

		double getXMax() const
		{
			double xmax = *std::max_element(std::begin(x_vals), std::end(x_vals));
			return xmax;
		}

		void setXVals(const std::vector<double> &vals)
		{
			x_vals = vals;
		}

		bool isXEmpty() const
		{
			return x_vals.empty();
		}
		bool isYEmpty() const
		{
			return y_vals.empty();
		}

	private:
		std::vector<double> x_vals;
		std::vector<double> y_vals;
	};

	//storing xInd and threshold
	struct iAThresIndx
	{
		iAThresIndx():
			thrIndx(-std::numeric_limits<long int>::infinity()),
			value(-std::numeric_limits<double>::infinity())
		{
		}
		long int thrIndx;
		double value;
	};

	class iAThresMinMax {
	public:
		iAThresMinMax():
			freqPeakMinY(dblInf_min),
			peakMinXThreshold(dblInf_min),
			fPeakHalf(dblInf_min),
			iso50ValueThr(dblInf_min),
			freqPeakLokalMaxY(dblInf_min),
			lokalPeakAirThrX(dblInf_min),
			determinedThreshold(dblInf_min),
			intersectionPoint(fltInf_min, fltInf_min)
		{
		}

		QPointF createLokalMaxHalfPoint()
		{
			return QPointF((float)LokalMaxPeakThreshold_X(), (float)fAirPeakHalf());
		}

		QString MinMaxToString()
		{
			QString res = QString("First Min above air %1 \t %2\n First Max Peak %3 \t %4")
				.arg(PeakMinXThreshold()).arg(FreqPeakMinY()).arg(LokalMaxPeakThreshold_X()).arg(FreqPeakLokalMaxY());
			return res;
		}

		void normalizeXValues(double min, double max)
		{
			peakMinXThreshold = minMaxNormalize(min, max, peakMinXThreshold);
			iso50ValueThr = minMaxNormalize(min, max, iso50ValueThr);
			lokalPeakAirThrX = minMaxNormalize(min, max, lokalPeakAirThrX);
			MaterialPeakThrX = minMaxNormalize(min, max, MaterialPeakThrX);
		}

		void mapNormalizedBackToMinMax(double min, double max)
		{
			peakMinXThreshold = normalizedToMinMax(min, max, peakMinXThreshold);
			iso50ValueThr = normalizedToMinMax(min, max, iso50ValueThr);
			lokalPeakAirThrX = normalizedToMinMax(min, max, lokalPeakAirThrX);
			MaterialPeakThrX = normalizedToMinMax(min, max, MaterialPeakThrX);
		}

		//apply custom min max
		void updateMinMaxPeaks(double lokalMinX, double lokalMinY, double lokalMaxX, double lokalMaxY)
		{
			peakMinXThreshold = lokalMinX;
			freqPeakMinY = lokalMinY;
			freqPeakLokalMaxY = lokalMaxY;
			lokalPeakAirThrX = lokalMaxX;
		}

		double LokalMaxPeakThreshold_X() const { return lokalPeakAirThrX; }
		void LokalMaxPeakThreshold_X(double val) { lokalPeakAirThrX = val; }
		double DeterminedThreshold() const { return determinedThreshold; }
		void DeterminedThreshold(double val) { determinedThreshold = val; }
		double FreqPeakMinY() const { return freqPeakMinY; }
		void FreqPeakMinY(double val) { freqPeakMinY = val; }
		double PeakMinXThreshold() const { return peakMinXThreshold; }
		void PeakMinXThreshold(double val) { peakMinXThreshold = val; }
		double fAirPeakHalf() const { return fPeakHalf; }
		void fAirPeakHalf(double val) { fPeakHalf = val; }
		double Iso50ValueThr() const { return iso50ValueThr; }
		void Iso50ValueThr(double val) { iso50ValueThr = val; }
		double FreqPeakLokalMaxY() const { return freqPeakLokalMaxY; }
		void FreqPeakLokalMaxY(double val) { freqPeakLokalMaxY = val; }

		void setIntersectionPoint(const QPointF& pt)
		{
			intersectionPoint = pt;
		}

		double getAirPeakThr() const
		{
			return lokalPeakAirThrX;
		}

		double getMaterialsThreshold() const
		{
			return MaterialPeakThrX;
		}

		void setMaterialsThreshold(double maxThr)
		{
			MaterialPeakThrX = maxThr;
		}

		const QPointF& getIntersectionPoint() const
		{
			return intersectionPoint;
		}

		QString resultsToString(bool printFinalResult)
		{
			QString resSt = QString("Min peak %1  %2\n").arg(peakMinXThreshold).arg(freqPeakMinY);
			resSt += QString("Lokal Air Peak %1 %2\n").arg(lokalPeakAirThrX).arg(freqPeakLokalMaxY);
			resSt += QString("Intersection point %1 %2\n").arg(intersectionPoint.x()).arg(intersectionPoint.y());
			resSt += QString("iso 50 %1\n").arg(Iso50ValueThr());
			resSt += QString("Maximum Peak(Material) - Grey Value: %1\n").arg(MaterialPeakThrX);

			if (printFinalResult)
			{
				resSt += QString("final resulting grey value %1").arg(DeterminedThreshold());
			}
			return resSt;
		}

		void setPeaksMinMax(double minThrPeakAir, double maxThrPeakMaterials)
		{
			m_peaks.init(minThrPeakAir, maxThrPeakMaterials);
		}

		iAGreyThresholdPeaks const& getGreyThresholdPeaks() const
		{
			return m_peaks;
		}

	private:
		double freqPeakMinY;
		double peakMinXThreshold;
		double fPeakHalf;
		double iso50ValueThr; //is this needed?
		double freqPeakLokalMaxY;
		double lokalPeakAirThrX;
		double MaterialPeakThrX; //Material value, at global maximum
		QPointF intersectionPoint;
		double determinedThreshold;
		iAGreyThresholdPeaks m_peaks;
	};


	struct iAPeakRanges
	{
		double XRangeMin;
		double XRangeMax;
		double HighPeakXMin;
		double HighPeakXMax;
	};

}