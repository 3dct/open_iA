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

#include <QWidget>
#include <QList>
#include <QSharedPointer>
#include <QDialog>

class iAModality;
class iASlicer;
enum iASlicerMode;
class MdiChild;

class QAbstractButton;
class QButtonGroup;
class QStatusBar;

class iANModalDisplay : public QWidget {
	Q_OBJECT

public:
	// numOfRows must be at least 1; smaller values will be clamped
	// maxSelection: how many modalities can be selected at maximum. <= 0 means there is no limit
	// minSelection: how many modalities can be selected at minimum. <= 0 means it acceptable to make no selections
	iANModalDisplay(QWidget *parent, MdiChild *mdiChild, QList<QSharedPointer<iAModality>> modalities, int maxSelection = 0, int minSelection = 1, int numOfRows = 1);

	QList<QSharedPointer<iAModality>> modalities() { return m_modalities; }

	QList<QSharedPointer<iAModality>> selection() { return m_selectedModalities; };
	QSharedPointer<iAModality> singleSelection() { return selection()[0]; }

	bool isSingleSelection() {
		return m_minSelection == 1 && m_maxSelection == 1;
	}

	iASlicer* createSlicer(QSharedPointer<iAModality> modality);


	// Result can be null! That means that the selection was cancelled
	static QList<QSharedPointer<iAModality>> selectModalities(
		QWidget *widget,
		iANModalDisplay *display,
		int maxSelection = 0, // how many modalities can be selected at maximum. <= 0 means there is no limit
		int minSelection = 1, // how many modalities can be selected at minimum. <= 0 means it acceptable to make no selections
		QWidget *dialogParent = nullptr
	);

	static QSharedPointer<iAModality> selectModality(QWidget *widget, iANModalDisplay *display, QWidget* dialogParent) {
		return selectModalities(widget, display, 1, 1, dialogParent)[0];
	};

private:
	QList<QSharedPointer<iAModality>> m_modalities;
	QList<QSharedPointer<iAModality>> m_selectedModalities;
	QList<iASlicer*> m_slicers;

	int m_maxSelection;
	int m_minSelection;
	iASlicerMode m_slicerMode;

	MdiChild *m_mdiChild;

	QWidget* _createSlicerContainer(iASlicer* slicer, QSharedPointer<iAModality> mod, QButtonGroup* group, bool checked);

	void setModalitySelected(QSharedPointer<iAModality> mod, QAbstractButton *button);
	bool isSelectionValid();
	bool validateSelection();

	static const uint CHANNEL_MAIN = 0;
	static const uint CHANNEL_MASK = 1;

	// Source: https://www.qtcentre.org/threads/8048-Validate-Data-in-QDialog
	class SelectionDialog : public QDialog {
	public:
		SelectionDialog(iANModalDisplay *display);
		void done(int r) override;
		iANModalDisplay *m_display;
	};

signals:
	void selectionChanged();
	void selectionRejected(const QString &message);
};