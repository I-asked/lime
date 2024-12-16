#pragma once

#ifndef LIME_GLES
# define GL_API GLAPI
# define GL_APIENTRY GLAPIENTRY
#endif

#define _inline [[gnu::always_inline]] inline

#define safe_cast(T, V) reinterpret_cast<T>(V)

#ifndef DEPRECATED
# define DEPRECATED [[gnu::deprecated]]
#endif

#define _bit(N) (1 << (N))

#define _nop() asm volatile ("NOP")
