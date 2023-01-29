// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
