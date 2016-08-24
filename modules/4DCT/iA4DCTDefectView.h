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
 
#ifndef IADEFECTVIEW_H
#define IADEFECTVIEW_H
// Ui
#include "ui_iA4DCTDefectView.h"
// iA
#include "iA4DCTDefects.h"
#include "iAChanData.h"
//#include "iASlicer.h"
#include "iASlicerMode.h"
// VTK
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkImageData.h>
// Qt
#include <QScopedPointer>
#include <QVector>
// std
#include <bitset>

class iASlicer;
class vtkColorTransferFunction;

class iA4DCTDefectView : public QDockWidget, public Ui::iA4DCTDefectView
{
	Q_OBJECT

public:
					iA4DCTDefectView(QWidget* parent = 0);
					~iA4DCTDefectView();

	//void			initializeSlicer(vtkImageData* image);
	void			initializeSlicer(QString path);
	void			setDefects(QString labeledImg, QString pullouts, QString cracks, QString breakages, QString debondings);

protected slots:
	void			setSliceScrollBar(int sn);
	void			toggleXY(bool checked);
	void			toggleXZ(bool checked);
	void			toggleYZ(bool checked);
	void			enablePullouts(int state);
	void			enableCracks(int state);
	void			enableDebondings(int state);
	void			enableBrekages(int state);

protected:
	void			initializeChannel(iAChanData* ch, vtkImageData* image);
	void			initializeTransferFunction(iAChanData* ch);
	void			directionChanged(iASlicerMode mode);
	vtkImageData*	loadImage(QString path);
	template<typename T>
	void			labledImageToMask(vtkImageData* img, iA4DCTDefects::VectorDataType list);
	template<typename T>
	void			prepareLabeledImage(vtkImageData* img, QVector<iA4DCTDefects::VectorDataType> lists);
	void			enableDefect(bool& defect, int state);
	void			updateChannelTF();

	iASlicer*									m_slicer;
	QScopedPointer<iAChanData>					m_channel;
	vtkSmartPointer<vtkColorTransferFunction>	m_colorTF;
	vtkSmartPointer<vtkTransform>				m_transform;
	vtkSmartPointer<vtkImageData>				m_intensityImg;
	vtkSmartPointer<vtkImageData>				m_labeledImg;

	bool										m_showPullouts;
	bool										m_showCracks;
	bool										m_showDebondings;
	bool										m_showBreakages;
};

template<typename T>
void iA4DCTDefectView::labledImageToMask(vtkImageData* img, iA4DCTDefects::VectorDataType list)
{
	iA4DCTDefects::HashDataType hash = iA4DCTDefects::DefectDataToHash(list);

	int* dims = img->GetDimensions();
	for (int x = 0; x < dims[0]; x++) {
		for (int y = 0; y < dims[1]; y++) {
			for (int z = 0; z < dims[2]; z++) {
				T* pixel = static_cast<T*>(img->GetScalarPointer(x, y, z));
				if (hash.contains(pixel[0])) {
					pixel[0] = 1;
				} else {
					pixel[0] = 0;
				}
			}
		}
	}
}

template<typename T>
void iA4DCTDefectView::prepareLabeledImage(vtkImageData* img, QVector<iA4DCTDefects::VectorDataType> lists)
{
	QVector<iA4DCTDefects::HashDataType> hashes;
	for (auto l : lists) {
		iA4DCTDefects::HashDataType hash = iA4DCTDefects::DefectDataToHash(l);
		hashes.push_back(hash);
	}

	std::bitset<32> mask;

	int* dims = img->GetDimensions();
	for (int x = 0; x < dims[0]; x++) {
		for (int y = 0; y < dims[1]; y++) {
			for (int z = 0; z < dims[2]; z++) {
				T* pixel = static_cast<T*>(img->GetScalarPointer(x, y, z));
				mask.reset();
				for (int i = 0; i < hashes.size(); i++) {
					if (hashes[i].contains(pixel[0])) {
						mask[i] = 1;
					}
				}
				pixel[0] = (T)mask.to_ulong();
			}
		}
	}
}

#endif // IADEFECTVIEW_H
