// =============================================================================
// Header file to include all OpenGL related headers
// =============================================================================

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
#else // Release
	#define ASSERT(x) ;
	#define ASSERTM(exp, msg) ((void)0)
	#define GLCall(x) x
#endif

static std::string ErrorCodeToString(GLenum errorCode)
{
	std::string error = "";
	switch (errorCode)
	{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		// undefined in Glad 4.1 OpenGL headers
		// case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		// case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
	}
	return error;
}

static void GLClearError(const char* function, const char* file, int line)
{	
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error = ErrorCodeToString(errorCode);
		std::cout << "CLEARING: [OpenGL Error: " << errorCode << "] (" << error << ")" << 
			function << " " << file << ":" << line <<
			std::endl;

	}
}

static bool GLLogErrors(const char* function, const char* file, int line)
{
	while (GLenum errorCode = glGetError())
	{
		std::string error = ErrorCodeToString(errorCode);
		std::cerr << "[OpenGL Error: " << errorCode << "] (" << error << ")" << 
			function << " " << file << ":" << line <<
			std::endl;
		return false;
	}
	return true;
}

// namespace Util
// {
// 	// A hash function used to hash a pair of any kind
// 	struct hash_pair {
// 		template <class T1, class T2>
// 		size_t operator()(const std::pair<T1, T2>& p) const
// 		{
// 			auto hash1 = std::hash<T1>{}(p.first);
// 			auto hash2 = std::hash<T2>{}(p.second);
	
// 			if (hash1 != hash2)
// 				return hash1 ^ hash2;             
			
// 			// If hash1 == hash2, their XOR is zero.
// 			return hash1;
// 		}
// 	};

// }


