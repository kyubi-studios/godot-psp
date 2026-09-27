#pragma once

// PSP (pspdev newlib + GCC) varsayılanı: int32_t = long, uint32_t = unsigned long.
// Godot, int32_t ile int'in aynı tip olduğunu varsayar (GetTypeInfo<int>, PtrToArg<int>, Variant
// kurucuları). MIPS32'de int ve long ikisi de 32 bit olduğundan, 32-bit sabit genişlikli tipleri
// derleyici öntanımlarını değiştirerek int'e yönlendiriyoruz. -include ile her dosyadan önce gelir.
// C ABI değişmez; yalnızca C++ ad süslemesi (mangling) değişir.

#undef __INT32_TYPE__
#define __INT32_TYPE__ int
#undef __UINT32_TYPE__
#define __UINT32_TYPE__ unsigned int
#undef __INT_LEAST32_TYPE__
#define __INT_LEAST32_TYPE__ int
#undef __UINT_LEAST32_TYPE__
#define __UINT_LEAST32_TYPE__ unsigned int
#undef __INT32_C
#define __INT32_C(c) c
#undef __UINT32_C
#define __UINT32_C(c) c##U
#undef __INT32_MAX__
#define __INT32_MAX__ 0x7fffffff
#undef __UINT32_MAX__
#define __UINT32_MAX__ 0xffffffffU
#undef __INT_LEAST32_MAX__
#define __INT_LEAST32_MAX__ 0x7fffffff
#undef __UINT_LEAST32_MAX__
#define __UINT_LEAST32_MAX__ 0xffffffffU
