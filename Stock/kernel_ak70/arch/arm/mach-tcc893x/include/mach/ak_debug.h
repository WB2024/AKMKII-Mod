/*
 *
 */
#ifndef __AK_DEBUG__H__
#define __AK_DEBUG__H__

asmlinkage int printk2(const char *fmt, ...);

#include <linux/string.h>
#define __FILENAME__ (strrchr((__FILE__),('/'))+1)



#define ENABLE_DEBUG


#ifdef ENABLE_DEBUG
#define	DBG_PRINT(tag,fmt, args...)	printk2(tag " %-12s %4d %s  " fmt ,__FILENAME__,__LINE__, __FUNCTION__, ## args)
#define	DBG_CALLER(fmt, args...)	printk2("\x1b[1;33m" " %-12s %4d %s %pS  " fmt "\x1b[0m\n",__FILENAME__,__LINE__, __FUNCTION__,__builtin_return_address(0), ## args)
#define	DBG_ERR(tag,fmt, args...)	printk2(tag " %-12s %4d %s " "\x1b[1;33m" fmt "\x1b[0m" ,__FILENAME__,__LINE__, __FUNCTION__, ## args)
#define	CPRINT(fmt, args...)	printk2("\x1b[1;33m" fmt "\x1b[0m\n" ,## args)

#else
#define	DBG_PRINT(tag,fmt, args...)	
#define	DBG_ERR(tag,fmt, args...)	
#define	CPRINT(fmt, args...)	
#endif



#define DEBUG_ALSA_MAPPING


#endif	
