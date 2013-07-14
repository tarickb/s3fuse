#define CONFIG(type, name, def, desc) PARAM%%%%type%%%%name%%%%def%%%%desc
#define CONFIG_CONSTRAINT(x, y)
#define CONFIG_KEY(x)
#define CONFIG_SECTION(x) SECTION%%%%x

#include "base/config.inc"

#undef CONFIG
#undef CONFIG_CONSTRAINT
#undef CONFIG_KEY
#undef CONFIG_SECTION
