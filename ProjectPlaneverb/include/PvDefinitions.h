#pragma once

// Enforce OS requirement
#if !defined(_WIN32)
#error "Planeverb only supports Windows!"
#endif

// API export/import
#if defined(PV_BUILD)
#define PV_API __declspec(dllexport)
#else
#define PV_API __declspec(dllimport)
#endif

// Assert only defined for debug mode
#ifdef _DEBUG
#define PV_ASSERT(cond) if((cond)){} else { __debugbreak(); }
#else
#define PV_ASSERT(cond) 
#endif

// helper defines
#define INDEX(row, col, dim) ( (row) * ((unsigned)(dim).x) + (col) )
#define INDEX_TO_POS(ISET, JSET, i, dim) (ISET) = (i) / (unsigned)(dim).x; (JSET) = (i) % (unsigned)(dim).x
#define INDEX3(row, col, t, dim, maxT) ( (t) + (maxT) * (INDEX((row), (col), (dim))) ) 
#define INDEX3_2(x, y, z, xmax, ymax, zmax) ((x * (ymax) * (zmax)) + (y * (zmax)) + z)
#define PV_INLINE inline 
#define PV_FORCEINLINE __forceinline 
#define INDEX4(x, y, z, t, xmax, ymax, zmax, tmax)  ((x * ymax * zmax * tmax) + (zmax * tmax * y) + (tmax) * z + t)

// Redefine these to true to enable debug print information to stdout
#define PRINT_PROFILE false
#define PRINT_PROFILE_SECTION false
#define PRINT_GRID false

#if PRINT_PROFILE
#define PROFILE_TIME(line, tag)	\
	std::cout << tag << ": ";	\
	{							\
		ScopedTimer _t;			\
		(line);					\
	}							\
	std::cout << std::endl
#else
#define PROFILE_TIME(line, tag) line
#endif

#if PRINT_PROFILE_SECTION
#define PROFILE_SECTION(section, tag)	\
	std::cout << tag << ": ";	\
	{							\
		ScopedTimer _s;			\
		section					\
	}							\
	std::cout << std::endl
#else
#define PROFILE_SECTION(section, tag) section
#endif