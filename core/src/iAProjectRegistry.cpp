#include "iAProjectRegistry.h"

template <typename ProjectType> using iAProjectFactory = iASpecificFactory<ProjectType, iAProjectBase>;

template <typename ProjectType>
void iAProjectRegistry::addProject(QString const & projectIdentifier)
{
    m_projectTypes.insert(projectIdentifier, QSharedPointer<iAIProjectFactory>(new iAProjectFactory<ProjectType>()));
}
