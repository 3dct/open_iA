/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAWidgetSettingsMapper.h"

#include <iAConsole.h>
#include <io/iAFileChooserWidget.h>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QSpinBox>

void loadSettings(iASettings const& settings, iAWidgetMap const& settingsWidgetMap)
{
	for (QString key : settingsWidgetMap.keys())
	{
		if (settings.contains(key))
		{
			QObject* w = settingsWidgetMap[key];
			if (qobject_cast<QComboBox*>(w))
			{
				int idx = qobject_cast<QComboBox*>(w)->findText(settings.value(key).toString());
				if (idx != -1)
				{
					qobject_cast<QComboBox*>(w)->setCurrentIndex(idx);
				}
				else
				{
					DEBUG_LOG(QString("Invalid value '%1' for input '%2'").arg(settings.value(key).toString()).arg(key));
				}
			}
			else if (qobject_cast<QCheckBox*>(w))
			{
				qobject_cast<QCheckBox*>(w)->setChecked(settings.value(key).toBool());
			}
			else if (qobject_cast<QSlider*>(w))
			{
				bool ok;
				int value = settings.value(key).toInt(&ok);
				if (!ok)
				{
					DEBUG_LOG(QString("Invalid value '%1' for input '%2': cannot convert to int!").arg(settings.value(key).toString()).arg(key));
					continue;
				}
				qobject_cast<QSlider*>(w)->setValue(value);
			}
			else if (qobject_cast<QDoubleSpinBox*>(w))
			{
				bool ok;
				double value = settings.value(key).toDouble(&ok);
				if (!ok)
				{
					DEBUG_LOG(QString("Invalid value '%1' for input '%2': cannot convert to double!").arg(settings.value(key).toString()).arg(key));
					continue;
				}
				qobject_cast<QDoubleSpinBox*>(w)->setValue(value);
			}
			else if (qobject_cast<QSpinBox*>(w))
			{
				bool ok;
				int value = settings.value(key).toInt(&ok);
				if (!ok)
				{
					DEBUG_LOG(QString("Invalid value '%1' for input '%2': cannot convert to int!").arg(settings.value(key).toString()).arg(key));
					continue;
				}
				qobject_cast<QSpinBox*>(w)->setValue(value);
			}
			else if (qobject_cast<QLineEdit*>(w))
			{
				qobject_cast<QLineEdit*>(w)->setText(settings[key].toString());
			}
			else if (qobject_cast<iAFileChooserWidget*>(w))
			{
				// error check?
				qobject_cast<iAFileChooserWidget*>(w)->setText(settings[key].toString());
			}
			else if (dynamic_cast<iAQLineEditVector*>(w))
			{
				auto& lineEditVector = *dynamic_cast<iAQLineEditVector*>(w);
				QStringList values = settings.value(key).toString().split(",");
				if (values.size() != lineEditVector.size())
				{
					DEBUG_LOG(QString("Invalid value '%1' for key=%2 - should be able to split that into %3 values, but encountered %4")
						.arg(settings.value(key).toString())
						.arg(key)
						.arg(lineEditVector.size())
						.arg(values.size()));
				}
				for (int i = 0; i < lineEditVector.size() && i < values.size(); ++i)
					lineEditVector[i]->setText(values[i]);
			}
			else if (dynamic_cast<iAQCheckBoxVector*>(w))
			{
				auto& checkBoxVector = *dynamic_cast<iAQCheckBoxVector*>(w);
				QStringList values = settings.value(key).toString().split(",");
				// first uncheck all entries:
				for (auto checkbox : checkBoxVector)
				{
					checkbox->setChecked(false);
				}
				QString fullStr = settings.value(key).toString();
				if (fullStr.isEmpty())
				{
					continue;
				}
				// then check those mentioned in settings:
				for (QString v : values)
				{
					bool ok;
					int idx = v.toInt(&ok);
					if (!ok || idx < 0 || idx > checkBoxVector.size())
					{
						DEBUG_LOG(QString("Invalid value '%1' for key=%2; entry %3 is either not convertible to int or outside of valid range 0..%4.")
							.arg(settings.value(key).toString())
							.arg(key)
							.arg(idx)
							.arg(checkBoxVector.size()));
					}
					else
					{
						checkBoxVector[idx]->setChecked(true);
					}
				}
			}
			else
			{
				DEBUG_LOG(QString("Widget type for key=%1 unknown!").arg(key));
			}
		}
		else
		{
			DEBUG_LOG(QString("No value found for key=%1 in settings.").arg(key));
		}
	}
}

class iAInternalSettingsWrapper
{
public:
	virtual void setValue(QString const& key, QVariant const& value) = 0;
};

class iAQSettingsWrapper: public iAInternalSettingsWrapper
{
public:
	iAQSettingsWrapper(QSettings& qs):
		m_qs(qs)
	{}
	void setValue(QString const& key, QVariant const& value) override
	{
		m_qs.setValue(key, value);
	}
private:
	QSettings & m_qs;
};

class iASettingsWrapper : public iAInternalSettingsWrapper
{
public:
	iASettingsWrapper(iASettings& s):
		m_s(s)
	{}
	void setValue(QString const& key, QVariant const& value) override
	{
		m_s[key] = value;
	}
private:
	iASettings& m_s;
};

void internalSaveSettings(iAInternalSettingsWrapper& settings, iAWidgetMap const& settingsWidgetMap)
{
	for (QString key : settingsWidgetMap.keys())
	{
		QObject* w = settingsWidgetMap[key];
		if (qobject_cast<QComboBox*>(w))
		{
			settings.setValue(key, qobject_cast<QComboBox*>(w)->currentText());
		}
		else if (qobject_cast<QCheckBox*>(w))
		{
			settings.setValue(key, qobject_cast<QCheckBox*>(w)->isChecked());
		}
		else if (qobject_cast<QSlider*>(w))
		{
			settings.setValue(key, qobject_cast<QSlider*>(w)->value());
		}
		else if (qobject_cast<QDoubleSpinBox*>(w))
		{
			settings.setValue(key, qobject_cast<QDoubleSpinBox*>(w)->value());
		}
		else if (qobject_cast<QSpinBox*>(w))
		{
			settings.setValue(key, qobject_cast<QSpinBox*>(w)->value());
		}
		else if (qobject_cast<QLineEdit*>(w))
		{
			settings.setValue(key, qobject_cast<QLineEdit*>(w)->text());
		}
		else if (qobject_cast<iAFileChooserWidget*>(w))
		{
			settings.setValue(key, qobject_cast<iAFileChooserWidget*>(w)->text());
		}
		else if (dynamic_cast<iAQLineEditVector*>(w))
		{
			QStringList values;
			auto& list = *dynamic_cast<iAQLineEditVector*>(w);
			for (auto edit : list)
			{
				values << edit->text();
			}
			settings.setValue(key, values.join(","));
		}
		else if (dynamic_cast<iAQCheckBoxVector*>(w))
		{
			auto& list = *dynamic_cast<iAQCheckBoxVector*>(w);
			QStringList values;
			for (int i = 0; i < list.size(); ++i)
			{
				if (list[i]->isChecked())
				{
					values << QString::number(i);
				}
			}
			settings.setValue(key, values.join(","));
		}
		else
		{
			DEBUG_LOG(QString("Widget type for key=%1 unknown!").arg(key));
		}
	}
}

void saveSettings(QSettings& settings, iAWidgetMap const& settingsWidgetMap)
{
	iAQSettingsWrapper settingsWrapper(settings);
	internalSaveSettings(settingsWrapper, settingsWidgetMap);
}

void saveSettings(iASettings& settings, iAWidgetMap const& settingsWidgetMap)
{
	iASettingsWrapper settingsWrapper(settings);
	internalSaveSettings(settingsWrapper, settingsWidgetMap);
}
