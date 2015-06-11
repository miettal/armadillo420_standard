#if defined(CONFIG_MACH_ARMADILLO410) || defined(CONFIG_MACH_ARMADILLO420) || defined(CONFIG_MACH_ARMADILLO440) || defined(CONFIG_MACH_ARMADILLO460)
#include "board-armadillo400.h"
#if defined(CONFIG_ARMADILLO410_CON2_AIOTG_STD)
#include "board-armadillo-iotg-std.h"
#endif
#else
#error no board is selected
#endif
