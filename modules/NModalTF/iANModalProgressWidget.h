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
#include <QThread>
#include <QMap>

class QGridLayout;
class QProgressBar;
class QLabel;
class QLayout;

class iANModalProgressWidget : public QWidget {
	Q_OBJECT

public:
	iANModalProgressWidget(QWidget *parent=nullptr);

	int addProgressBar(int max, QString title, bool autoUpdateText=true, QString name=QString());
	int addProgressBar(int max, QLabel *label, bool autoUpdateText=false, QString name= QString());

	void addWidget(QWidget *widget, QString title, int rowStretch=1);
	void addWidget(QWidget *widget, QLabel *label=nullptr, int rowStretch=1);

	void addSeparator();

	bool isFinished();
	bool isCanceled();
	void setCanceled(bool);
	bool exists(int pbid);

private:
	QGridLayout *m_layout;
	QList<QProgressBar*> m_bars;
	QList<QLabel*> m_barLabels;
	QList<QString> m_barTexts;
	QList<bool> m_barAutoUpdateText;
	QMap<QString, int> m_name2id;
	int m_nextRow = 0;

	bool m_canceled = false;

public slots:
	void showDialog(QWidget *parent = nullptr);
	void update(int pbid);
	void finish();
	void cancel();

	void setFirstValue(int value);

	void setValue(int pbid, int value);
	void setMax(int pbid, int max);
	void setAutoUpdateText(int pbid, bool autoUpdateText);
	void setText(int pbid, QString text);

	void setValue(QString name, int value);
	void setMax(QString name, int max);
	void setAutoUpdateText(QString name, bool autoUpdateText);
	void setText(QString name, QString text);

signals:
	void updated(int pbid, int value, int max);
	void finished();
	void canceled();
};

class iANModalProgressUpdater : public QThread {
	Q_OBJECT

public:
	iANModalProgressUpdater(iANModalProgressWidget*);

private:
	static const auto CONNECTION_TYPE = Qt::BlockingQueuedConnection;

signals:
	void finish();
	void cancel();

	void showDialog(QWidget *parent = nullptr);

	void setFirstValue(int value);
	void setValue(int pbid, int value);
	void setMax(int pbid, int max);
	void setAutoUpdateText(int pbid, bool autoUpdateText);
	void setText(int pbid, QString text);

	void setValue(QString name, int value);
	void setMax(QString name, int max);
	void setAutoUpdateText(QString name, bool autoUpdateText);
	void setText(QString name, QString text);
};