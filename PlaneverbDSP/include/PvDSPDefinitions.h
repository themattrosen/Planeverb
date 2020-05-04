#pragma once

// enforce OS requirement
#if !defined(_WIN32)
#error "PlaneverbDSP only supports Windows!"
#endif

// API export/import
#if defined(PV_DSP_BUILD)
#define PV_DSP_API __declspec(dllexport)
#else
#define PV_DSP_API __declspec(dllimport)
#endif

// Assert only defined for debug mode
#ifdef _DEBUG
#define PV_DSP_ASSERT(cond) if((cond)){} else { __debugbreak(); }
#else
#define PV_DSP_ASSERT(cond) 
#endif

// helper defines
#define PV_DSP_INLINE inline

#define PV_DSP_SAFE_DELETE(ptr)	\
if((ptr))	\
{	\
	delete (ptr);	\
	(ptr) = nullptr;	\
}

#define PV_DSP_SAFE_ARRAY_DELETE(ptr)	\
if((ptr))	\
{	\
	delete[] (ptr);	\
	(ptr) = nullptr;	\
}

#define PV_DSP_ALIGN(num_bytes) __declspec(align( (num_bytes) ))

#define PV_DSP_SWAP_BUFFERS(ptr, bA, bB)	\
if(ptr == bA)	\
{	\
	ptr = bB;	\
}	\
else	\
{	\
	ptr = bA;	\
}

#define LERP_FLOAT(start, end, factor) (((start * (1.f - factor)) + (end * factor)))
