/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAImageTreeNode.h"

#include <iASlicerMode.h>

#include <QVector>
#include <QWidget>

class iAImagePreviewWidget;
class iAPreviewWidgetPool;

class vtkCamera;

struct FavoriteData
{
	iAImageTreeNode * node;
	iAImagePreviewWidget* widget;
	FavoriteData():
		node(0), widget(0) {}
	FavoriteData(iAImageTreeNode *l, iAImagePreviewWidget* w):
		node(l), widget(w) {}
};

class iAFavoriteWidget : public QWidget
{
	Q_OBJECT
public:
	iAFavoriteWidget(iAPreviewWidgetPool* previewWidgetPool);
	bool ToggleLike(iAImageTreeNode * node);
	bool ToggleHate(iAImageTreeNode * node);
	bool HasAnyFavorite() const;
	QVector<iAImageTreeNode const *> GetFavorites(iAImageTreeNode::Attitude att) const;
signals:
	void ViewUpdated();
	void clicked(iAImageTreeNode * node);
	void rightClicked(iAImageTreeNode * node);
private slots:
	void FavoriteClicked();
	void FavoriteRightClicked();
private:
	void Add(iAImageTreeNode * node);
	void Remove(iAImageTreeNode const * node);
	int GetIndexForNode(iAImageTreeNode const* node);
	iAImageTreeNode * GetNodeForWidget(iAImagePreviewWidget* widget);
	QVector<FavoriteData> m_favorites;
	iAPreviewWidgetPool* m_previewPool;
	QLayout *m_likeLayout, *m_hateLayout;
};
