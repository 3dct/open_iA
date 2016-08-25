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
