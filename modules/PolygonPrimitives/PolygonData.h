#pragma once


#include <QLineF>; 


class point3d {
public:
	point3d() {
		initPoint(0.0, 0.0, 0.0); 
	}

	void initPoint(double x, double y, double z) {
		m_X = x; 
		m_y = y;
		m_z = z; 
	}

	double getX() {
		return this->m_X;

	}

	double getY() {
		return this->m_y; 
	}

	double getZ() {
		return this->m_z; 
	}


private:
	double m_X;
	double m_y; 
	double m_z; 
	
};

class sphere  : public point3d {
	sphere();

	double getRadius() {
		return raduis; 
	};

private:
	double raduis; 

};


class Line {
public:
	Line(double x1, double y1, double z1, double x2, double y2, double z2) {
		this->m_p1.initPoint(x1, y1, z1);
		this->m_p2.initPoint(x2, y2, z2); 
		
	}

	const point3d& getP1() {
		return this->m_p1;

	}


	const point3d& getP2() {
		return this->m_p2; 
	}



private: 


	point3d m_p1;
	point3d m_p2; 
};




