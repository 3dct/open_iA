// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QTableWidgetItem>

class QFile;

class vtkImageData;

class iAFoamCharacterizationTable;

class iADataSet;

class iAFoamCharacterizationItem : public QObject, public QTableWidgetItem
{
	Q_OBJECT

public:
	enum EItemType
	{
		itBinarization,
		itDistanceTransform,
		itFilter,
		itWatershed
	};

public:
	explicit iAFoamCharacterizationItem(iAFoamCharacterizationTable* _pTable, const EItemType& _eItemType);
	explicit iAFoamCharacterizationItem(iAFoamCharacterizationItem* _pItem);
	virtual ~iAFoamCharacterizationItem();

	double executeTime() const;
	bool executing() const;

	QString executeTimeString() const;

	static QColor itemIconColor(EItemType eItemType);
	static QIcon itemButtonIcon(EItemType eItemType);

	bool itemEnabled() const;

	EItemType itemType() const;
	QString itemTypeStr() const;

	bool modified() const;

	QString name() const;

	int progress() const;

	void reset();
	void setItemEnabled(const bool& _bEnabled);
	void setModified(const bool& _bModified);
	void setName(const QString& _sName);

	iAFoamCharacterizationTable* table();

	virtual void dialog() = 0;
	virtual std::shared_ptr<iADataSet> execute(std::shared_ptr<iADataSet> input) = 0;
	virtual void open(QFile* _pFileOpen) = 0;
	virtual void save(QFile* _pFileSave) = 0;

private:
	bool m_bExecuting = false;

	int m_iProgress = 0;

	void setItemIcon();

protected:
	bool m_bItemEnabled = true;
	bool m_bModified = false;

	double m_dExecuteTime = 0.0;

	iAFoamCharacterizationTable* m_pTable = nullptr;

	EItemType m_eItemType = itFilter;

	QString m_sName;

	QString fileRead(QFile* _pFileOpen);
	void fileWrite(QFile* _pFileSave, const QString& _sText);

	void setExecuting(const bool& _bExecuting);
	void setProgress(const unsigned int& _uiProgress);

	virtual void setItemText();

protected slots:
	void slotObserver(const int&);
};
