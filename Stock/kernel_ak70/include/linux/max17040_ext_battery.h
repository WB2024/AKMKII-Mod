
#ifndef __MAX17040_EXT_BATTERY_H_
#define __MAX17040_EXT_BATTERY_H_

struct max17040_ext_platform_data {
	int (*battery_online)(void);
	int (*charger_online)(void);
	int (*charger_enable)(void);
   int (*charger_done)(void);
   void (*charger_disable)(void);
};

#endif
