// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkPolyDataAlgorithm.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkLineSource.h>
#include <vtkRotationalExtrusionFilter.h>
#include <vtkMath.h>
#include <vtkSmartPointer.h>
#include <vtkCellArray.h>

class iAPieSource : public vtkPolyDataAlgorithm
{
public:
	static iAPieSource * New()
	{
		vtkObject* ret = vtkObjectFactory::CreateInstance("iAPieSource");
		if(ret)
			return static_cast<iAPieSource*>(ret);
		return new iAPieSource;
	}
	vtkTypeMacro(iAPieSource, vtkPolyDataAlgorithm);

	void PrintSelf(ostream& os, vtkIndent indent) override
	{
		this->Superclass::PrintSelf(os, indent);

		os << indent << "Radius: " << this->m_radius << "\n";
		os << indent << "ZCoord: " << this->m_z << "\n";
		os << indent << "Resolution: " << this->m_resolution << "\n";
		os << indent << "StartAngle: " << this->m_startAngle << "\n";
		os << indent << "EndAngle: " << this->m_endAngle << "\n";
	}

	void	SetRadius(double radius)			{ m_radius = radius; }
	double	GetRadius()							{ return m_radius; }

	void	SetZ(double z)						{ m_z = z; }
	double	GetZ()								{ return m_z; }

	void	SetResulution(double resolution)	{ m_resolution = resolution; }
	double	GetResolution()						{ return m_resolution; }

	void	SetStartAngle(double startAngle)	{ m_startAngle = startAngle; }
	double	GetStartAngle()						{ return m_startAngle; }

	void	SetEndAngle(double endAngle)		{ m_endAngle = endAngle; }
	double	GetEndAngle()						{ return m_endAngle; }

protected:
	iAPieSource()
	{
		m_radius = 0.5;
		m_z = 0.0;
		m_startAngle = 0.0;
		m_endAngle = 90.0;
		m_resolution = 50;
		this->SetNumberOfInputPorts(0);
	}

	~iAPieSource() {};

	int RequestData(vtkInformation *vtkNotUsed(request),
					vtkInformationVector **vtkNotUsed(inputVector),
					vtkInformationVector *outputVector) override
	{
		//Get the info object
		vtkInformation *outInfo = outputVector->GetInformationObject(0);
		//Get the ouptut
		vtkPolyData *output = vtkPolyData::SafeDownCast( outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

		int numberOfSegments = (int) (m_resolution * (m_endAngle - m_startAngle) / 360.0);
		if(numberOfSegments < 2)
			numberOfSegments = 2;
		//Set things up; allocate memory
		vtkIdType numPts = numberOfSegments + 1;
		vtkIdType numPolys = numberOfSegments;
		vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
		newPoints->Allocate(numPts);
		vtkSmartPointer<vtkCellArray> newPolys = vtkSmartPointer<vtkCellArray>::New();
		newPolys->Allocate(newPolys->EstimateSize(numPolys, 3));

		//Create pie
		double startAngleRad = vtkMath::RadiansFromDegrees( m_startAngle );
		double endAngleRad = vtkMath::RadiansFromDegrees( m_endAngle );
		double theta = (endAngleRad - startAngleRad) / (numberOfSegments - 1);
		double x[3] = { 0.0, 0.0, 0.0 };
		newPoints->InsertNextPoint(x);
		for (int i=0; i < numberOfSegments; i++)
		{
			double currentAngle = startAngleRad + i*theta;

			x[0] = m_radius * std::cos(currentAngle);
			x[1] = m_radius * std::sin(currentAngle);
			x[2] = 0.0;
			newPoints->InsertNextPoint(x);
		}

		//Connectivity
		vtkIdType pts[3];
		pts[0] = 0;
		for (int i=1; i < numberOfSegments; i++)
		{
			pts[1] = i;
			pts[2] = i + 1;
			newPolys->InsertNextCell(3, pts);
		}

		//Setup output
		output->SetPoints(newPoints);
		output->SetPolys(newPolys);

		return 1;
	}

	double m_radius;
	double m_z;
	int m_resolution;
	double m_startAngle;
	double m_endAngle;

private:
	iAPieSource(const iAPieSource&);  // Not implemented.
	void operator=(const iAPieSource&);  // Not implemented.
};
