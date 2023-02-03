// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAChannelData.h>
#include <iASlicerMode.h>

#include <QDialog>
#include <QList>
#include <QSharedPointer>
#include <QWidget>

class iAImageData;
class iASlicer;
class iAMdiChild;

class QAbstractButton;
class QButtonGroup;
class QStatusBar;

class iANModalDisplay : public QWidget
{
	Q_OBJECT

public:
	// numOfRows must be at least 1; smaller values will be clamped
	// maxSelection: how many datasets can be selected at maximum. <= 0 means there is no limit
	// minSelection: how many datasets can be selected at minimum. <= 0 means it acceptable to make no selections
	iANModalDisplay(QWidget* parent, iAMdiChild* mdiChild, const QList<std::shared_ptr<iAImageData>>& dataSets,
		int maxSelection = 0, int minSelection = 1, int numOfRows = 1);

	QList<std::shared_ptr<iAImageData>> dataSets()
	{
		return m_dataSets;
	}

	QList<std::shared_ptr<iAImageData>> selection()
	{
		return m_selectedDataSets;
	};
	std::shared_ptr<iAImageData> singleSelection()
	{
		return selection()[0];
	}

	bool isSingleSelection()
	{
		return m_minSelection == 1 && m_maxSelection == 1;
	}

	iASlicer* createSlicer(std::shared_ptr<iAImageData> dataset);

	uint createChannel();
	void setChannelData(uint channelId, iAChannelData channelData);
	static const uint MAIN_CHANNEL_ID = 0;

	// Result can be null! That means that the selection was cancelled
	static QList<std::shared_ptr<iAImageData>> selectDataSets(
		iANModalDisplay* display, QWidget* footer = nullptr, QWidget* dialogParent = nullptr);

	static std::shared_ptr<iAImageData> selectDataSet(
		iANModalDisplay* display, QWidget* footer = nullptr, QWidget* dialogParent = nullptr)
	{
		return selectDataSets(display, footer, dialogParent)[0];
	};

	class Footer : public QWidget
	{
	public:
		Footer(QDialog* parent) : QWidget(parent)
		{
		}
		QString m_textOfButtonClicked;
	};

	static Footer* createOkCancelFooter(QDialog* dialog)
	{
		return createFooter(dialog, {"OK"}, {"Cancel"});
	}
	static Footer* createOkSkipFooter(QDialog* dialog)
	{
		return createFooter(dialog, {"OK"}, {"Skip"});
	}
	static Footer* createFooter(QDialog* dialog, const QList<QString>& acceptTexts, const QList<QString>& rejectTexts);

private:
	QList<std::shared_ptr<iAImageData>> m_dataSets;
	QList<std::shared_ptr<iAImageData>> m_selectedDataSets;
	QList<iASlicer*> m_slicers;

	int m_maxSelection;
	int m_minSelection;
	iASlicerMode m_slicerMode;

	iAMdiChild* m_mdiChild;

	QWidget* createSlicerContainer(
		iASlicer* slicer, std::shared_ptr<iAImageData> mod, QButtonGroup* group /*, bool checked*/);

	void setModalitySelected(std::shared_ptr<iAImageData> mod, QAbstractButton* button);
	bool isSelectionValid();
	bool validateSelection();

	uint m_nextChannelId;

	// Source: https://www.qtcentre.org/threads/8048-Validate-Data-in-QDialog
	class SelectionDialog : public QDialog
	{
	public:
		SelectionDialog(iANModalDisplay* display, QWidget* parent);
		void done(int r) override;
		iANModalDisplay* m_display;
	};

signals:
	void selectionChanged();
	void selectionRejected(const QString& message);
};
