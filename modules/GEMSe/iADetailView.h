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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IA_DETAIL_VIEW_H
#define IA_DETAIL_VIEW_H

#include "iAImageTypes.h"

#include <vtkSmartPointer.h>

#include <QWidget>

class QPushButton;
class QSplitter;
class QStandardItemModel;
class QTextEdit;

class iAAttributes;
class iAChannelVisualizationData;
class iAColorTheme;
class iAImageClusterNode;
class iAImagePreviewWidget;
class iALabelInfo;
class iAModalityList;

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class iADetailView: public QWidget
{
	Q_OBJECT
public:
	iADetailView(iAImagePreviewWidget* prevWdgt,
		ClusterImageType nullImage,
		QSharedPointer<iAModalityList> modalities,
		QSharedPointer<iAAttributes> attributes,
		iALabelInfo const & labelInfo,
		int representativeType);
	void SetNode(iAImageClusterNode const * node);
	int GetSliceNumber() const;
	void UpdateLikeHate(bool isLike, bool isHate);
	bool IsShowingCluster() const;

	//  for updating the magic lens:
	void SetSliceNumber(int sliceNr);
	void SetSlicerMode(int mode,int sliceNr);
	void SetMagicLensOpacity(double opacity);
	void SetLabelInfo(iALabelInfo const & labelInfo);
	void SetRepresentativeType(int representativeType);
signals:
	void Like();
	void Hate();
	void GoToCluster();
	void ViewUpdated();
protected:
	virtual void paintEvent(QPaintEvent * );
private slots:
	void DblClicked();
	void ChangeModality(int);
	void ChangeMagicLensOpacity(int);
private:
	iAImageClusterNode const * m_node;
	iAImagePreviewWidget* m_previewWidget;
	QPushButton *m_pbLike, *m_pbHate, *m_pbGoto;
	QTextEdit* m_detailText;
	bool m_showingClusterRepresentative;
	ClusterImageType m_nullImage;
	QSharedPointer<iAModalityList> m_modalities;
	vtkSmartPointer<vtkColorTransferFunction> m_ctf;
	vtkSmartPointer<vtkPiecewiseFunction> m_otf;
	QSharedPointer<iAAttributes> m_attributes;
	QStandardItemModel* m_labelItemModel;
	int m_representativeType;
	iAChannelVisualizationData* m_magicLensData;
	int m_magicLensCurrentModality;

	void UpdateGeometry();
	void SetImage();
};

#endif // IA_DETAIL_VIEW_H