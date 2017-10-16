/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAFCMGUIRunner.h"

#include "iAFuzzyCMeans.h"

#include "iAConsole.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "mdichild.h"

QSharedPointer<iAFilterRunnerGUI> iAFCMGUIRunner::Create()
{
	return QSharedPointer<iAFCMGUIRunner>(new iAFCMGUIRunner());
}

void iAFCMGUIRunner::ConnectThreadSignals(MdiChild* mdiChild, iAFilterRunnerGUIThread* thread)
{
	QObject::connect(thread, SIGNAL(finished()), this, SLOT(FCMFinished()));
	iAFilterRunnerGUI::ConnectThreadSignals(mdiChild, thread);
}

void iAFCMGUIRunner::FCMFinished()
{
	auto thread = dynamic_cast<iAFilterRunnerGUIThread*>(sender());
	iAProbabilitySource* probSource = dynamic_cast<iAProbabilitySource*>(thread->Filter().data());
	if (!thread || !probSource)
	{
		DEBUG_LOG("Invalid FCM finished call!");
		return;
	}
	auto & probs = probSource->Probabilities();
	for (int p = 0; p < probs.size(); ++p)
	{
		qobject_cast<MdiChild*>(thread->parent())->GetModalities()->Add(QSharedPointer<iAModality>(
			new iAModality(QString("FCM Prob. %1").arg(p), "", -1, probs[p], 0)));
	}
}

iAFCMGUIRunner::iAFCMGUIRunner()
{}