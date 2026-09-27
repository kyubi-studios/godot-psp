#pragma once

#include <alloca.h>

// PSP'de pthread_setname_np yok; threads=no derlemede thread adı verilmez.
#define PTHREAD_NO_RENAME
