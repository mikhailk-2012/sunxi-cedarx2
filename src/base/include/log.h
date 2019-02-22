/*
* Cedarx framework.
* Copyright (c) 2008-2015 Allwinner Technology Co. Ltd.
* Copyright (c) 2014 BZ Chen <bzchen@allwinnertech.com>
*
* This file is part of Cedarx.
*
* Cedarx is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*/
#ifndef LOG_H
#define LOG_H

#include "CdxConfig.h"
#include <CdxUtils.h>

#ifndef LOG_TAG
#define LOG_TAG "CedarX"
#endif

#if CONFIG_OS == OPTION_OS_ANDROID
    #include <cutils/log.h>

    #define LOG_LEVEL_ERROR     ANDROID_LOG_ERROR
    #define LOG_LEVEL_WARNING   ANDROID_LOG_WARN
    #define LOG_LEVEL_INFO      ANDROID_LOG_INFO
    #define LOG_LEVEL_VERBOSE   ANDROID_LOG_VERBOSE
    #define LOG_LEVEL_DEBUG     ANDROID_LOG_DEBUG
    
    #define AWLOG(level, fmt, arg...)  \
        LOG_PRI(level, LOG_TAG, "<%s:%u>: "fmt, strrchr(__FILE__, '/')+1, __LINE__, ##arg)
        
#elif CONFIG_OS == OPTION_OS_LINUX
    #include <stdio.h>
    #include <string.h>
    
    #define LOG_LEVEL_ERROR     "error  "
    #define LOG_LEVEL_WARNING   "warning"
    #define LOG_LEVEL_INFO      "info   "
    #define LOG_LEVEL_VERBOSE   "verbose"
    #define LOG_LEVEL_DEBUG     "debug  "
    /*
    #define AWLOG(level, fmt, arg...)  \
        fprintf(stderr, "%s: %s <%s:%u>: "fmt"\n", level, LOG_TAG, __FILE__, __LINE__, ##arg)
        */
	#define AWLOG(level, fmt, arg...)
#else
    #error "invalid configuration of os."
#endif

#define loge(fmt, arg...) \
    do { \
        AWLOG(LOG_LEVEL_ERROR, "\033[40;31m"fmt"\033[0m", ##arg) ; \
        CdxBTDump(); \
    } while (0)
    
#define logi(fmt, arg...) AWLOG(LOG_LEVEL_INFO, fmt, ##arg)
#if CONFIG_LOG_LEVEL >= OPTION_LOG_LEVEL_WARNING
#define logw(fmt, arg...) AWLOG(LOG_LEVEL_WARNING, fmt, ##arg)
#else
#define logw(fmt, arg...) 
#endif
#if CONFIG_LOG_LEVEL >= OPTION_LOG_LEVEL_DEFAULT
#define logd(fmt, arg...) AWLOG(LOG_LEVEL_DEBUG, fmt, ##arg)
#else
#define logd(fmt, arg...) 
#endif
#if CONFIG_LOG_LEVEL >= OPTION_LOG_LEVEL_DETAIL
#define logv(fmt, arg...) AWLOG(LOG_LEVEL_VERBOSE, fmt, ##arg)
#else
#define logv(fmt, arg...)
#endif

#define CEDARX_UNUSE(param) (void)param

#endif

