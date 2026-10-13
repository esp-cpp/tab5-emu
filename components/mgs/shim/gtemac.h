/* The GTE inline forms the game uses that psyz does not provide. The upstream
 * ESP32 build took this from a private shim directory; port/include/gtemac.h
 * is its phase-0 ancestor. Pre-included into every game translation unit. */
#ifndef GTEMAC_SHIM_H
#define GTEMAC_SHIM_H
#include <libgte.h>
/* PSX 1KiB scratchpad at 0x1F800000: a plain array on any other target */
extern unsigned long psyz_scratchpad[256];
#define getScratchAddr(n) ((unsigned long*)&psyz_scratchpad[(n)])
/* PsyQ inline forms that differ from psyz's plain-C prototypes */
#define gte_NormalClip(a, b, c, out) (*(int*)(out) = NormalClip((a), (b), (c)))
/* port/psyz_port.c's CompMatrix: the full PSY-Q composition (rotation applied
 * to the translation too), which psyz's own does not do */
MATRIX* CompMatrix(MATRIX* m0, MATRIX* m1, MATRIX* m2);
#define gte_CompMatrix(a, b, c) CompMatrix((MATRIX*)(a), (MATRIX*)(b), (MATRIX*)(c))
VECTOR* ApplyMatrixLV(MATRIX* m, VECTOR* v0, VECTOR* v1);
#endif
