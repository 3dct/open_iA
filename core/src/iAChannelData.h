/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "open_iA_Core_export.h"

#include <QColor>

#include <vtkSmartPointer.h>


class QString;

class vtkImageData;
class vtkPiecewiseFunction;
class vtkScalarsToColors;


class open_iA_Core_API iAChannelData
{
public:
	static const size_t Maximum3DChannels = 3;

	iAChannelData();
	iAChannelData(vtkSmartPointer<vtkImageData> image, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf=nullptr);
	virtual ~iAChannelData();

	virtual void reset();
	void setData(vtkSmartPointer<vtkImageData> image, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf);

	void setOpacity(double opacity);
	double getOpacity() const;

	bool isEnabled() const;
	void setEnabled(bool enabled);

	bool uses3D() const;
	void set3D(bool enabled);

	void setImage(vtkSmartPointer<vtkImageData> image);
	void setColorTF(vtkScalarsToColors* cTF);
	void setOpacityTF(vtkPiecewiseFunction* oTF);

	void setName(QString name);
	QString getName() const;

	// check if this can be somehow refactored (not needed for each kind of channel):
	// begin
	void setColor(QColor const & col);
	QColor getColor() const;

	bool isSimilarityRenderingEnabled() const;
	void setSimilarityRenderingEnabled(bool enabled);
	// end

	vtkSmartPointer<vtkImageData> getImage() const;
	vtkPiecewiseFunction * getOTF() const;
	vtkScalarsToColors * getCTF() const;
private:
	bool m_enabled;
	double m_opacity;
	bool m_threeD;
	QColor m_color; // TODO: only used in XRF module, move there!
	bool m_similarityRenderingEnabled;
	vtkSmartPointer<vtkImageData>       m_image;
	vtkPiecewiseFunction*               m_piecewiseFunction;
	vtkScalarsToColors*                 m_colorTransferFunction;
	QString                             m_name;
};
