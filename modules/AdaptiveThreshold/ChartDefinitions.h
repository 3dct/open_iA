#pragma once

namespace chartV {
	enum axisMode {
		x,
		y
	};

	enum plotMode
	{
		scatter,
		lines
	};

	class AxisParams
	{

	public:

		AxisParams() :m_xMin(0), m_xMax(100), m_yMin(0), m_yMax(100)
		{
			setDefaultTicks();
		}

		AxisParams(double xmin, double xMax, double yMin, double yMax) :
			m_xMin(xmin), m_xMax(xMax), m_yMin(yMin), m_yMax(yMax)
		{
			setDefaultTicks();
		}


		void setTicks(unsigned short xTicks, unsigned short yTicks) {
			if ((xTicks > 0) && (yTicks > 0)) {
				ticks_x = xTicks;
				ticks_y = yTicks;
			}

		}

	private:
		inline void setDefaultTicks()
		{
			ticks_x = 8;
			ticks_y = 5;
		}


		double m_xMin;
		double m_xMax;
		double m_yMin;
		double m_yMax;
		int ticks_x;
		int ticks_y;
	};
}
//void AxisParams::setDefaultTicks()
//{
//
//}

