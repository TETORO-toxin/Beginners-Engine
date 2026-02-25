#pragma once

#ifdef _WIN32
#ifdef MINIZ_DLL
#define MINIZ_EXPORT __declspec(dllexport)
#else
#define MINIZ_EXPORT
#endif
#else
#define MINIZ_EXPORT
#endif
