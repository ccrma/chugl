#pragma once
#include <glad/glad.h>
#include <cassert>
#include <iostream>


#ifdef _DEBUG
	#define ASSERTM(exp, msg) assert(((void)msg, exp))

	#ifdef _MSC_VER
	#define ASSERT(x) if (!(x)) __debugbreak();
	#else
	#define ASSERT(x) assert(x);
	#endif

#define GLCall(x) GLClearError(#x, __FILE__, __LINE__); x; ASSERT(GLLogErrors(#x, __FILE__, __LINE__));
	//#define GLCall(x) ASSERT(GLLogErrors(#x, __FILE__, __LINE__)); x;
#else
#define ASSERT(x) ;
#define ASSERTM(exp, msg) ((void)0)
#define GLCall(x) x
#endif


static void GLClearError(const char* function, const char* file, int line)
{	
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		std::cout << "CLEARING: [OpenGL Error] (" << error << ")" << 
			function << " " << file << ":" << line <<
			std::endl;

	}
}

static bool GLLogErrors(const char* function, const char* file, int line)
{
	while (GLenum error = glGetError())
	{
		std::cout << "[OpenGL Error] (" << error << ")" << 
			function << " " << file << ":" << line <<
			std::endl;
		return false;
	}
	return true;
}
namespace Util
{
	inline void println(const std::string& str) {
		std::cout << str << std::endl;
	}

	inline void printErr(const std::string& str) {
		std::cerr << str << std::endl;
	}
}


