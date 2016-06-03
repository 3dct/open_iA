#ifndef I_SURFACE_H
#define I_SURFACE_H

#include "common.h"

//namespace Raytracer {

/**	\class ScreenBuffer.
	\brief Representing screen surface.

	Contains image's pixel buffer and size data.	
*/
class ScreenBuffer
{
public:
	// constructor / destructors
	ScreenBuffer( int a_Width, int a_Height );
	~ScreenBuffer();
	// member data access
	/**
	* Get pixel buffer.
	*/
	unsigned int* GetBuffer() { return m_Buffer; }
	/**
	* Get screen's width.
	*/
	int GetWidth() { return m_Width; }
	/**
	* Get screen's height.
	*/
	int GetHeight() { return m_Height; }
	/**
	* Clear pixel buffer with black color.
	*/
	void Clear();
private:
	// Attributes
	unsigned int* m_Buffer;	
	int m_Width, m_Height;	
	int buffSize;
};

//}; // namespace Raytracer

#endif
