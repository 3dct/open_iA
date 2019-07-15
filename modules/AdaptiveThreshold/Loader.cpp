#include "Loader.h"
#include "QTextStream"
#include "iAConsole.h"
#include <sstream>
#include <fstream>


Loader::Loader()
{
}


Loader::~Loader()
{
}

bool Loader::loadCSV(const QString &Fname)
{
	if (Fname.isNull() || Fname.isEmpty()) 
		return false;

	std::ifstream inputF(Fname.toStdString(), std::ios::in); 
	std::string line; 
	std::getline(inputF, line);
	if (!m_greyValues.empty())
		m_greyValues.clear();

	if (!m_frequencies.empty())
		m_frequencies.clear(); 

	if (inputF.is_open()) {
		DEBUG_LOG("Loading file"); 

		while (std::getline(inputF, line))
		{
			double x = 0; double y = 0; 
			std::istringstream ss(line);
			ss >> x; ss >> y;
			DEBUG_LOG(QString("x y %1 %2").arg(x).arg(y));
			m_greyValues.push_back(x);
			m_frequencies.push_back(y);

		}

		inputF.close(); 
		return true; 
	}
	else
		return false; 
}
