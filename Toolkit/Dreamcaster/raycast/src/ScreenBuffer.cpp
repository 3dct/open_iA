#include "../include/common.h"
#include "../include/ScreenBuffer.h"
//#include "../include/stdio.h"
#include "string.h"

//namespace Raytracer {

ScreenBuffer::ScreenBuffer( int a_Width, int a_Height ) :
	m_Width( a_Width ),
	m_Height( a_Height )
{
	m_Buffer = new unsigned int[a_Width * a_Height];
	buffSize = m_Width * m_Height * sizeof(m_Buffer[0]);
	Clear();
	//memset(m_Buffer, 0, a_Width * a_Height);
}

ScreenBuffer::~ScreenBuffer()
{
	if(m_Buffer)
		delete [] m_Buffer;
}

void ScreenBuffer::Clear()
{
	memset(m_Buffer, 0, buffSize);
}

//}; // namespace Raytracer
