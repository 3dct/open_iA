// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include <vtkSmartPointer.h>

#include <QString>

class vtkImageData;
class vtkPiecewiseFunction;
class vtkScalarsToColors;


class iAguibase_API iAChannelData
{
public:
	static const int Maximum3DChannels = 3;

	iAChannelData();
	iAChannelData(QString const & name, vtkSmartPointer<vtkImageData> image, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf=nullptr);
	virtual ~iAChannelData();

	virtual void reset();
	void setData(vtkSmartPointer<vtkImageData> image, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf);

	void setOpacity(double opacity);
	double opacity() const;

	bool isEnabled() const;
	void setEnabled(bool enabled);

	bool uses3D() const;
	void set3D(bool enabled);

	void setImage(vtkSmartPointer<vtkImageData> image);
	void setColorTF(vtkScalarsToColors* cTF);
	void setOpacityTF(vtkPiecewiseFunction* oTF);

	void setName(QString name);
	QString const & name() const;

	vtkSmartPointer<vtkImageData> image() const;
	vtkPiecewiseFunction * opacityTF() const;
	vtkScalarsToColors * colorTF() const;
private:
	bool m_enabled;
	double m_opacity;
	bool m_threeD;
	vtkSmartPointer<vtkImageData>       m_image;
	vtkScalarsToColors*                 m_cTF;
	vtkPiecewiseFunction*               m_oTF;
	QString                             m_name;
};
