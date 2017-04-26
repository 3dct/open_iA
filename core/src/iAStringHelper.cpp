#include "iAStringHelper.h"

#include <QRegularExpression>
#include <QStringList>

QStringList SplitPossiblyQuotedString(QString const & additionalArguments)
{
	QStringList result;
	QRegularExpression exp("\\s*([^\"]\\S*|\".+?\")\\s*");
	int offset = 0;
	QRegularExpressionMatch match = exp.match(additionalArguments, offset);
	while (match.hasMatch())
	{
		QString argument = match.captured(1);
		if (argument.startsWith("\"") && argument.endsWith("\""))
		{
			argument = argument.mid(1, argument.length() - 2);
		}
		result.append(argument);
		offset = match.capturedEnd(0);
		match = exp.match(additionalArguments, offset);
	}
	return result;
}


bool Str2Vec3D(QString const & str, double vec[3])
{
	QStringList list = str.split(" ");
	if (list.size() != 3)
	{
		return false;
	}
	for (int i = 0; i < 3; ++i)
	{
		bool ok;
		vec[i] = list[i].toDouble(&ok);
		if (!ok)
			return false;
	}
	return true;
}


QString Vec3D2String(double* vec)
{
	return QString("%1 %2 %3").arg(vec[0]).arg(vec[1]).arg(vec[2]);
}
