/*
* Cedarx framework.
* Copyright (c) 2008-2015 Allwinner Technology Co. Ltd.
* Copyright (c) 2014 Ning Fang <fangning@allwinnertech.com>
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

/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file omx_vdec.cpp
  This module contains the implementation of the OpenMAX core & component.

*//*========================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

//#define CONFIG_LOG_LEVEL 3
#include "log.h"

#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "omx_vdec.h"
#include <fcntl.h>
#include "AWOMX_VideoIndexExtension.h"
#include "transform_color_format.h"

#include "memoryAdapter.h"
#include "vdecoder.h"

#if CONFIG_OS == OPTION_OS_ANDROID
    #include <binder/IPCThreadState.h>
    #include <media/stagefright/foundation/ADebug.h>
    #include <ui/GraphicBufferMapper.h>
    #include <ui/Rect.h>
    #include <HardwareAPI.h>
#endif

#define debug logi("LINE %d, FUNCTION %s", __LINE__, __FUNCTION__);

#define OPEN_STATISTICS (0)
#define OPEN_LINUX_STATISTICS (1)

#if CONFIG_OS == OPTION_OS_ANDROID
    using namespace android;
#endif

/* H.263 Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedH263ProfileLevels[] = {
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level40},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level50},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level60},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70},
  {-1, -1}};

/* MPEG4 Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedMPEG4ProfileLevels[] ={
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0b},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level1},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level2},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level3},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4a},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level5},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0b},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level1},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level2},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level3},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5},
  {-1,-1}};

/* AVC Supported Levels & profiles for cts */
VIDEO_PROFILE_LEVEL_TYPE CTSSupportedAVCProfileLevels[] ={
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32},

  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2 },
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3 },
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32},

  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1 },
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2 },
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3 },
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32},

  {-1,-1}};

/* AVC Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedAVCProfileLevels[] ={
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevelMax},

  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2 },
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3 },
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel41},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel42},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel5},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevelMax},
  
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1 },        
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b},      
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11},     
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12},     
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13},    
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2 },    
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21},   
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22},  
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3 },  
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel41},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel42},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel5},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevelMax},
  {-1,-1}};
/*
 *     M A C R O S
 */

/*
 * Initializes a data structure using a pointer to the structure.
 * The initialization of OMX structures always sets up the nSize and nVersion fields
 *   of the structure.
 */
#define OMX_CONF_INIT_STRUCT_PTR(_s_, _name_)	\
    memset((_s_), 0x0, sizeof(_name_));	\
    (_s_)->nSize = sizeof(_name_);		\
    (_s_)->nVersion.s.nVersionMajor = 0x1;	\
    (_s_)->nVersion.s.nVersionMinor = 0x1;	\
    (_s_)->nVersion.s.nRevision = 0x0;		\
    (_s_)->nVersion.s.nStep = 0x0



static VIDDEC_CUSTOM_PARAM sVideoDecCustomParams[] =
{
	{VIDDEC_CUSTOMPARAM_ENABLE_ANDROID_NATIVE_BUFFER, (OMX_INDEXTYPE)AWOMX_IndexParamVideoEnableAndroidNativeBuffers},
	{VIDDEC_CUSTOMPARAM_GET_ANDROID_NATIVE_BUFFER_USAGE, (OMX_INDEXTYPE)AWOMX_IndexParamVideoGetAndroidNativeBufferUsage},
	{VIDDEC_CUSTOMPARAM_USE_ANDROID_NATIVE_BUFFER2, (OMX_INDEXTYPE)AWOMX_IndexParamVideoUseAndroidNativeBuffer2},
    {VIDDEC_CUSTOMPARAM_STORE_META_DATA_IN_BUFFER,(OMX_INDEXTYPE)AWOMX_IndexParamVideoUseStoreMetaDataInBuffer},
	{VIDDEC_CUSTOMPARAM_PREPARE_FOR_ADAPTIVE_PLAYBACK,(OMX_INDEXTYPE)AWOMX_IndexParamVideoUsePrepareForAdaptivePlayback}
};


static void* ComponentThread(void* pThreadData);
static void* ComponentVdrvThread(void* pThreadData);

#if CONFIG_OS == OPTION_OS_ANDROID
    #define GET_CALLING_PID	(IPCThreadState::self()->getCallingPid())
    void getCallingProcessName(char *name)
    {
    	char proc_node[128];

    	if (name == 0)
    	{
    		logd("error in params");
    		return;
    	}
    	
    	memset(proc_node, 0, sizeof(proc_node));
    	sprintf(proc_node, "/proc/%d/cmdline", GET_CALLING_PID);
    	int fp = ::open(proc_node, O_RDONLY);
    	if (fp > 0) 
    	{
    		memset(name, 0, 128);
    		::read(fp, name, 128);
    		::close(fp);
    		fp = 0;
    		logd("Calling process is: %s", name);
    	}
    	else 
    	{
    		logd("Obtain calling process failed");
    	}
    }
#endif

//* factory function executed by the core to create instances
void *get_omx_component_factory_fn(void)
{
	return (new omx_vdec);
}

#if OPEN_LINUX_STATISTICS
static int64_t OMX_GetNowUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000ll + tv.tv_usec;
}
#endif

typedef enum OMX_VDRV_COMMANDTYPE
{
    OMX_VdrvCommand_PrepareVdecLib,
    OMX_VdrvCommand_CloseVdecLib,
    OMX_VdrvCommand_FlushVdecLib,
    OMX_VdrvCommand_Stop,
    OMX_VdrvCommand_EndOfStream,
    OMX_VdrvCommand_Max = 0X7FFFFFFF,
} OMX_VDRV_COMMANDTYPE;

void post_message_to_vdrv(omx_vdec *omx, OMX_S32 id)
{
      int ret_value;
      logi("omx_vdec: post_message %d pipe out%d to vdrv\n", (int)id,omx->m_vdrv_cmdpipe[1]);
      ret_value = write(omx->m_vdrv_cmdpipe[1], &id, sizeof(OMX_S32));
      logi("post_message to pipe done %d\n",ret_value);
}

omx_vdec::omx_vdec()
{
    logi("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
	m_state              = OMX_StateLoaded;
	m_cRole[0]           = 0;
	m_cName[0]           = 0;
	m_eCompressionFormat = OMX_VIDEO_CodingUnused;
	m_pAppData           = NULL;
	m_thread_id          = 0;
    m_vdrv_thread_id     = 0;
	m_cmdpipe[0]         = 0;
	m_cmdpipe[1]         = 0;
	m_cmddatapipe[0]     = 0;
	m_cmddatapipe[1]     = 0;
    m_vdrv_cmdpipe[0]    = 0;
    m_vdrv_cmdpipe[1]    = 0;
	m_InputNum           = 0;
	m_OutputNum          = 0;
    m_maxWidth           = 0;
    m_maxHeight          = 0;
	m_decoder            = NULL;

    m_storeOutputMetaDataFlag = OMX_FALSE;
    m_useAndroidBuffer        = OMX_FALSE;
    m_JumpFlag                = OMX_FALSE;
    mIsFromCts                = OMX_FALSE;
    m_nInportPrevTimeStamp    = 0;
    mFirstInputDataFlag       = OMX_TRUE;
    mVp9orH265SoftDecodeFlag  = OMX_FALSE;
    mResolutionChangeFlag     = OMX_FALSE;

    memset(mCallingProcess,0,sizeof(mCallingProcess));
    
    
#if CONFIG_OS == OPTION_OS_ANDROID    
	getCallingProcessName(mCallingProcess);
	if((strcmp(mCallingProcess, "com.android.cts.media") == 0) || (strcmp(mCallingProcess, "com.android.cts.videoperf") == 0) || (strcmp(mCallingProcess, "com.android.pts.videoperf") == 0))
	{
		mIsFromCts           = true;
	}
#endif

	memset(&m_Callbacks, 0, sizeof(m_Callbacks));
	memset(&m_sInPortDef, 0, sizeof(m_sInPortDef));
	memset(&m_sOutPortDef, 0, sizeof(m_sOutPortDef));
	memset(&m_sInPortFormat, 0, sizeof(m_sInPortFormat));
	memset(&m_sOutPortFormat, 0, sizeof(m_sOutPortFormat));
	memset(&m_sPriorityMgmt, 0, sizeof(m_sPriorityMgmt));
	memset(&m_sInBufSupplier, 0, sizeof(m_sInBufSupplier));
	memset(&m_sOutBufSupplier, 0, sizeof(m_sOutBufSupplier));
	memset(&m_sInBufList, 0, sizeof(m_sInBufList));
	memset(&m_sOutBufList, 0, sizeof(m_sOutBufList));
	memset(&m_streamInfo, 0, sizeof(m_streamInfo));


	pthread_mutex_init(&m_inBufMutex, NULL);
	pthread_mutex_init(&m_outBufMutex, NULL);
    pthread_mutex_init(&m_pipeMutex, NULL);
    
    sem_init(&m_vdrv_cmd_lock,0,0);
    sem_init(&m_sem_vbs_input,0,0);
    sem_init(&m_sem_frame_output,0,0);
}


omx_vdec::~omx_vdec()
{
	OMX_S32 nIndex;
    // In case the client crashes, check for nAllocSize parameter.
    // If this is greater than zero, there are elements in the list that are not free'd.
    // In that case, free the elements.
    logi("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
	pthread_mutex_lock(&m_inBufMutex);

    for(nIndex=0; nIndex<m_sInBufList.nBufArrSize; nIndex++)
    {
        if(m_sInBufList.pBufArr==NULL)
        {
            break;
        }
        
        if(m_sInBufList.pBufArr[nIndex].pBuffer != NULL)
        {
            if(m_sInBufList.nAllocBySelfFlags & (1<<nIndex))
            {
                free(m_sInBufList.pBufArr[nIndex].pBuffer);
                m_sInBufList.pBufArr[nIndex].pBuffer = NULL;
            }
        }
    }

    if (m_sInBufList.pBufArr != NULL)
    {
    	free(m_sInBufList.pBufArr);
    }

    if (m_sInBufList.pBufHdrList != NULL)
    {
    	free(m_sInBufList.pBufHdrList);
    }

	memset(&m_sInBufList, 0, sizeof(struct _BufferList));
	m_sInBufList.nBufArrSize = m_sInPortDef.nBufferCountActual;

	pthread_mutex_unlock(&m_inBufMutex);


	pthread_mutex_lock(&m_outBufMutex);

	for(nIndex=0; nIndex<m_sOutBufList.nBufArrSize; nIndex++)
	{
		if(m_sOutBufList.pBufArr[nIndex].pBuffer != NULL)
		{
			if(m_sOutBufList.nAllocBySelfFlags & (1<<nIndex))
			{
				free(m_sOutBufList.pBufArr[nIndex].pBuffer);
				m_sOutBufList.pBufArr[nIndex].pBuffer = NULL;
			}
		}
	}

    if (m_sOutBufList.pBufArr != NULL)
    	free(m_sOutBufList.pBufArr);

    if (m_sOutBufList.pBufHdrList != NULL)
    {
    	free(m_sOutBufList.pBufHdrList);
    }

	memset(&m_sOutBufList, 0, sizeof(struct _BufferList));
	m_sOutBufList.nBufArrSize = m_sOutPortDef.nBufferCountActual;

	pthread_mutex_unlock(&m_outBufMutex);

    if(m_decoder != NULL)
    {
    	DestroyVideoDecoder(m_decoder);
    	m_decoder = NULL;
    }

    pthread_mutex_destroy(&m_inBufMutex);
    pthread_mutex_destroy(&m_outBufMutex);

    pthread_mutex_destroy(&m_pipeMutex);
    sem_destroy(&m_vdrv_cmd_lock);
    sem_destroy(&m_sem_vbs_input);
    sem_destroy(&m_sem_frame_output);

    if(m_streamInfo.pCodecSpecificData)
        free(m_streamInfo.pCodecSpecificData);
    
#if (OPEN_STATISTICS)
    if(mDecodeFrameTotalCount!=0 && mConvertTotalCount!=0)
    {
        mDecodeFrameSmallAverageDuration = (mDecodeFrameTotalDuration + mDecodeOKTotalDuration)/(mDecodeFrameTotalCount);
        mDecodeFrameBigAverageDuration = (mDecodeFrameTotalDuration + mDecodeOKTotalDuration + mDecodeNoFrameTotalDuration + mDecodeNoBitstreamTotalDuration + mDecodeOtherTotalDuration)/(mDecodeFrameTotalCount);
        if(mDecodeNoFrameTotalCount > 0)
        {
            mDecodeNoFrameAverageDuration = (mDecodeNoFrameTotalDuration)/(mDecodeNoFrameTotalCount);
        }
        else
        {
            mDecodeNoFrameAverageDuration = 0;
        }
        if(mDecodeNoBitstreamTotalCount > 0)
        {
            mDecodeNoBitstreamAverageDuration = (mDecodeNoBitstreamTotalDuration)/(mDecodeNoBitstreamTotalCount);
        }
        else
        {
            mDecodeNoBitstreamAverageDuration = 0;
        }
        mConvertAverageDuration = mConvertTotalDuration/mConvertTotalCount;
        logd("decode and convert statistics:\
            \n mDecodeFrameTotalDuration[%lld]ms, mDecodeOKTotalDuration[%lld]ms, mDecodeNoFrameTotalDuration[%lld]ms, mDecodeNoBitstreamTotalDuration[%lld]ms, mDecodeOtherTotalDuration[%lld]ms,\
            \n mDecodeFrameTotalCount[%lld], mDecodeOKTotalCount[%lld], mDecodeNoFrameTotalCount[%lld], mDecodeNoBitstreamTotalCount[%lld], mDecodeOtherTotalCount[%lld],\
            \n mDecodeFrameSmallAverageDuration[%lld]ms, mDecodeFrameBigAverageDuration[%lld]ms, mDecodeNoFrameAverageDuration[%lld]ms, mDecodeNoBitstreamAverageDuration[%lld]ms\
            \n mConvertTotalDuration[%lld]ms, mConvertTotalCount[%lld], mConvertAverageDuration[%lld]ms",
            mDecodeFrameTotalDuration/1000, mDecodeOKTotalDuration/1000, mDecodeNoFrameTotalDuration/1000, mDecodeNoBitstreamTotalDuration/1000, mDecodeOtherTotalDuration/1000,
            mDecodeFrameTotalCount, mDecodeOKTotalCount, mDecodeNoFrameTotalCount, mDecodeNoBitstreamTotalCount, mDecodeOtherTotalCount,
            mDecodeFrameSmallAverageDuration/1000, mDecodeFrameBigAverageDuration/1000, mDecodeNoFrameAverageDuration/1000, mDecodeNoBitstreamAverageDuration/1000,
            mConvertTotalDuration/1000, mConvertTotalCount, mConvertAverageDuration/1000);
    }
#endif

    logd("~omx_dec done!");
}


OMX_ERRORTYPE omx_vdec::component_init(OMX_STRING name)
{
	OMX_ERRORTYPE eRet = OMX_ErrorNone;
    int           err;
    OMX_U32       nIndex;

    logi("(f:%s, l:%d) name = %s", __FUNCTION__, __LINE__, name);
	strncpy((char*)m_cName, name, OMX_MAX_STRINGNAME_SIZE);

    if (!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg1", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.mpeg1", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat      = OMX_VIDEO_CodingMPEG1;
        m_streamInfo.eCodecFormat = VIDEO_CODEC_FORMAT_MPEG1;
	}
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mjpeg", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.mjpeg", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat      = OMX_VIDEO_CodingMJPEG;
		m_streamInfo.eCodecFormat = VIDEO_CODEC_FORMAT_MJPEG;
	}
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg2", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.mpeg2", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat      = OMX_VIDEO_CodingMPEG2;
        m_streamInfo.eCodecFormat = VIDEO_CODEC_FORMAT_MPEG2;
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char*)m_cRole, "video_decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat      = OMX_VIDEO_CodingMPEG4;
		m_streamInfo.eCodecFormat = VIDEO_CODEC_FORMAT_XVID;
	}
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.xvid", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char*)m_cRole, "video_decoder.xvid", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat      = OMX_VIDEO_CodingXVID;
		m_streamInfo.eCodecFormat = VIDEO_CODEC_FORMAT_XVID;
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.h263", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.h263", OMX_MAX_STRINGNAME_SIZE);
		logi("\n H263 Decoder selected");
		m_eCompressionFormat      = OMX_VIDEO_CodingH263;
		m_streamInfo.eCodecFormat = VIDEO_CODEC_FORMAT_H263;
	}
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.avc", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.avc", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat      = OMX_VIDEO_CodingAVC;
		m_streamInfo.eCodecFormat = VIDEO_CODEC_FORMAT_H264;
	}
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vc1", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.vc1", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat      = OMX_VIDEO_CodingWMV;
		m_streamInfo.eCodecFormat = VIDEO_CODEC_FORMAT_WMV3;
	}
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vp8", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.vp8", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat      = OMX_VIDEO_CodingVP8; //OMX_VIDEO_CodingVPX
        m_streamInfo.eCodecFormat = VIDEO_CODEC_FORMAT_VP8;
	}
	else
	{
		logi("ERROR:Unknown Component");
	    eRet = OMX_ErrorInvalidComponentName;
	    return eRet;
	}

    // Initialize component data structures to default values
    OMX_CONF_INIT_STRUCT_PTR(&m_sPortParam, OMX_PORT_PARAM_TYPE);
    m_sPortParam.nPorts           = 0x2;
    m_sPortParam.nStartPortNumber = 0x0;

    // Initialize the video parameters for input port
    OMX_CONF_INIT_STRUCT_PTR(&m_sInPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    m_sInPortDef.nPortIndex 					 = 0x0;
    m_sInPortDef.bEnabled 						 = OMX_TRUE;
    m_sInPortDef.bPopulated 					 = OMX_FALSE;
    m_sInPortDef.eDomain 						 = OMX_PortDomainVideo;
    m_sInPortDef.format.video.nFrameWidth 		 = 0;
    m_sInPortDef.format.video.nFrameHeight 		 = 0;
    m_sInPortDef.eDir 							 = OMX_DirInput;
    m_sInPortDef.nBufferCountMin 				 = NUM_IN_BUFFERS;
    m_sInPortDef.nBufferCountActual 			 = NUM_IN_BUFFERS;
    m_sInPortDef.nBufferSize 					 = OMX_VIDEO_DEC_INPUT_BUFFER_SIZE;
    m_sInPortDef.format.video.eCompressionFormat = m_eCompressionFormat;
    m_sInPortDef.format.video.cMIMEType 		 = (OMX_STRING)"";

    // Initialize the video parameters for output port
    OMX_CONF_INIT_STRUCT_PTR(&m_sOutPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    m_sOutPortDef.nPortIndex 				     = 0x1;
    m_sOutPortDef.bEnabled 					     = OMX_TRUE;
    m_sOutPortDef.bPopulated 				     = OMX_FALSE;
    m_sOutPortDef.eDomain 					     = OMX_PortDomainVideo;
    m_sOutPortDef.format.video.cMIMEType 	     = (OMX_STRING)"YUV420";
    m_sOutPortDef.format.video.nFrameWidth 	     = 176;
    m_sOutPortDef.format.video.nFrameHeight      = 144;
    m_sOutPortDef.eDir 						     = OMX_DirOutput;
    m_sOutPortDef.nBufferCountMin 			     = NUM_OUT_BUFFERS;
    m_sOutPortDef.nBufferCountActual 		     = NUM_OUT_BUFFERS;
    m_sOutPortDef.nBufferSize 				     = (OMX_U32)(m_sOutPortDef.format.video.nFrameWidth*m_sOutPortDef.format.video.nFrameHeight*3/2);
    m_sOutPortDef.format.video.eColorFormat      = OMX_COLOR_FormatYUV420Planar;

    // Initialize the video compression format for input port
    OMX_CONF_INIT_STRUCT_PTR(&m_sInPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    m_sInPortFormat.nPortIndex         = 0x0;
    m_sInPortFormat.nIndex             = 0x0;
    m_sInPortFormat.eCompressionFormat = m_eCompressionFormat;

    // Initialize the compression format for output port
    OMX_CONF_INIT_STRUCT_PTR(&m_sOutPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    m_sOutPortFormat.nPortIndex        = 0x1;
    m_sOutPortFormat.nIndex            = 0x0;
    m_sOutPortFormat.eColorFormat      = OMX_COLOR_FormatYUV420Planar;

    OMX_CONF_INIT_STRUCT_PTR(&m_sPriorityMgmt, OMX_PRIORITYMGMTTYPE);

    OMX_CONF_INIT_STRUCT_PTR(&m_sInBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE );
    m_sInBufSupplier.nPortIndex  = 0x0;

    OMX_CONF_INIT_STRUCT_PTR(&m_sOutBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE );
    m_sOutBufSupplier.nPortIndex = 0x1;

    // Initialize the input buffer list
    memset(&(m_sInBufList), 0x0, sizeof(BufferList));

    m_sInBufList.pBufArr = (OMX_BUFFERHEADERTYPE*)malloc(sizeof(OMX_BUFFERHEADERTYPE) * m_sInPortDef.nBufferCountActual);
    if(m_sInBufList.pBufArr == NULL)
    {
    	eRet = OMX_ErrorInsufficientResources;
    	goto EXIT;
    }

    memset(m_sInBufList.pBufArr, 0, sizeof(OMX_BUFFERHEADERTYPE) * m_sInPortDef.nBufferCountActual);
    for (nIndex = 0; nIndex < m_sInPortDef.nBufferCountActual; nIndex++)
    {
        OMX_CONF_INIT_STRUCT_PTR (&m_sInBufList.pBufArr[nIndex], OMX_BUFFERHEADERTYPE);
    }


    m_sInBufList.pBufHdrList = (OMX_BUFFERHEADERTYPE**)malloc(sizeof(OMX_BUFFERHEADERTYPE*) * m_sInPortDef.nBufferCountActual);
    if(m_sInBufList.pBufHdrList == NULL)
    {
    	eRet = OMX_ErrorInsufficientResources;
    	goto EXIT;
    }

    m_sInBufList.nSizeOfList       = 0;
    m_sInBufList.nAllocSize        = 0;
    m_sInBufList.nWritePos         = 0;
    m_sInBufList.nReadPos          = 0;
    m_sInBufList.nAllocBySelfFlags = 0;
    m_sInBufList.nSizeOfList       = 0;
    m_sInBufList.nBufArrSize       = m_sInPortDef.nBufferCountActual;
    m_sInBufList.eDir              = OMX_DirInput;

    // Initialize the output buffer list
    memset(&m_sOutBufList, 0x0, sizeof(BufferList));

    m_sOutBufList.pBufArr = (OMX_BUFFERHEADERTYPE*)malloc(sizeof(OMX_BUFFERHEADERTYPE) * m_sOutPortDef.nBufferCountActual);
    if(m_sOutBufList.pBufArr == NULL)
    {
    	eRet = OMX_ErrorInsufficientResources;
    	goto EXIT;
    }

    memset(m_sOutBufList.pBufArr, 0, sizeof(OMX_BUFFERHEADERTYPE) * m_sOutPortDef.nBufferCountActual);
    for (nIndex = 0; nIndex < m_sOutPortDef.nBufferCountActual; nIndex++)
    {
        OMX_CONF_INIT_STRUCT_PTR(&m_sOutBufList.pBufArr[nIndex], OMX_BUFFERHEADERTYPE);
    }

    m_sOutBufList.pBufHdrList = (OMX_BUFFERHEADERTYPE**)malloc(sizeof(OMX_BUFFERHEADERTYPE*) * m_sOutPortDef.nBufferCountActual);
    if(m_sOutBufList.pBufHdrList == NULL)
    {
    	eRet = OMX_ErrorInsufficientResources;
    	goto EXIT;
    }

    m_sOutBufList.nSizeOfList       = 0;
    m_sOutBufList.nAllocSize        = 0;
    m_sOutBufList.nWritePos         = 0;
    m_sOutBufList.nReadPos          = 0;
    m_sOutBufList.nAllocBySelfFlags = 0;
    m_sOutBufList.nSizeOfList       = 0;
    m_sOutBufList.nBufArrSize       = m_sOutPortDef.nBufferCountActual;
    m_sOutBufList.eDir              = OMX_DirOutput;

    // Create the pipe used to send commands to the thread
    err = pipe(m_cmdpipe);
    if (err)
    {
    	eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    // Create the pipe used to send command to the vdrv_thread
    err = pipe(m_vdrv_cmdpipe);
    if (err)
    {
    	eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    // Create the pipe used to send command data to the thread
    err = pipe(m_cmddatapipe);
    if (err)
    {
    	eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    //* create a decoder.

    /*
    m_decoder = CreateVideoDecoder();
    if(m_decoder == NULL)
    {
    	logi(" can not create video decoder.");
    	eRet = OMX_ErrorInsufficientResources;
    	goto EXIT;
    }
    */
    
	//*set omx cts flag to flush the last frame in h264
	//m_decoder->ioctrl(m_decoder, CEDARV_COMMAND_SET_OMXCTS_DECODER, 1);
	
    // Create the component thread
    err = pthread_create(&m_thread_id, NULL, ComponentThread, this);
    if( err || !m_thread_id )
    {
    	eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    
    // Create vdrv thread
    err = pthread_create(&m_vdrv_thread_id, NULL, ComponentVdrvThread, this);
    if( err || !m_vdrv_thread_id )
    {
    	eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    mDecodeFrameTotalDuration           = 0;
    mDecodeOKTotalDuration              = 0;
    mDecodeNoFrameTotalDuration         = 0;
    mDecodeNoBitstreamTotalDuration     = 0;
    mDecodeOtherTotalDuration           = 0;
    mDecodeFrameTotalCount              = 0;
    mDecodeOKTotalCount                 = 0;
    mDecodeNoFrameTotalCount            = 0;
    mDecodeNoBitstreamTotalCount        = 0;
    mDecodeOtherTotalCount              = 0;
    mDecodeFrameSmallAverageDuration    = 0;
    mDecodeFrameBigAverageDuration      = 0;
    mDecodeNoFrameAverageDuration       = 0;
    mDecodeNoBitstreamAverageDuration   = 0;
    
    mConvertTotalDuration               = 0;
    mConvertTotalCount                  = 0;
    mConvertAverageDuration             = 0;
    
EXIT:
    return eRet;
}


OMX_ERRORTYPE  omx_vdec::get_component_version(OMX_IN OMX_HANDLETYPE hComp,
                                               OMX_OUT OMX_STRING componentName,
                                               OMX_OUT OMX_VERSIONTYPE* componentVersion,
                                               OMX_OUT OMX_VERSIONTYPE* specVersion,
                                               OMX_OUT OMX_UUIDTYPE* componentUUID)
{
    logi("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
    if (!hComp || !componentName || !componentVersion || !specVersion)
    {
        return OMX_ErrorBadParameter;
    }

    strcpy((char*)componentName, (char*)m_cName);

    componentVersion->s.nVersionMajor = 1;
    componentVersion->s.nVersionMinor = 1;
    componentVersion->s.nRevision     = 0;
    componentVersion->s.nStep         = 0;

    specVersion->s.nVersionMajor = 1;
    specVersion->s.nVersionMinor = 1;
    specVersion->s.nRevision     = 0;
    specVersion->s.nStep         = 0;

	return OMX_ErrorNone;
}


OMX_ERRORTYPE  omx_vdec::send_command(OMX_IN OMX_HANDLETYPE  hComp,
                                      OMX_IN OMX_COMMANDTYPE cmd,
                                      OMX_IN OMX_U32         param1,
                                      OMX_IN OMX_PTR         cmdData)
{
    ThrCmdType    eCmd;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    logi("(f:%s, l:%d) ", __FUNCTION__, __LINE__);

    if(m_state == OMX_StateInvalid)
    {
    	logd("ERROR: Send Command in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if (cmd == OMX_CommandMarkBuffer && cmdData == NULL)
    {
    	logd("ERROR: Send OMX_CommandMarkBuffer command but pCmdData invalid.");
    	return OMX_ErrorBadParameter;
    }

    switch (cmd)
    {
        case OMX_CommandStateSet:
        	logi(" COMPONENT_SEND_COMMAND: OMX_CommandStateSet");
            eCmd = SetState;
	        break;

        case OMX_CommandFlush:
        	logi(" COMPONENT_SEND_COMMAND: OMX_CommandFlush");
	        eCmd = Flush;
	        if ((int)param1 > 1 && (int)param1 != -1)
	        {
	        	logd("Error: Send OMX_CommandFlush command but param1 invalid.");
	        	return OMX_ErrorBadPortIndex;
	        }
	        break;

        case OMX_CommandPortDisable:
        	logi(" COMPONENT_SEND_COMMAND: OMX_CommandPortDisable");
	        eCmd = StopPort;
	        break;

        case OMX_CommandPortEnable:
        	logi(" COMPONENT_SEND_COMMAND: OMX_CommandPortEnable");
	        eCmd = RestartPort;
	        break;

        case OMX_CommandMarkBuffer:
        	logi(" COMPONENT_SEND_COMMAND: OMX_CommandMarkBuffer");
	        eCmd = MarkBuf;
 	        if (param1 > 0)
	        {
	        	logd("Error: Send OMX_CommandMarkBuffer command but param1 invalid.");
	        	return OMX_ErrorBadPortIndex;
	        }
            break;
            
        default:
            logw("(f:%s, l:%d) ignore other command[0x%x]", __FUNCTION__, __LINE__, cmd);
            return OMX_ErrorBadParameter;
    }
    
    post_event(eCmd, param1, cmdData);

    return eError;
}


OMX_ERRORTYPE  omx_vdec::get_parameter(OMX_IN OMX_HANDLETYPE hComp,
                                       OMX_IN OMX_INDEXTYPE  paramIndex,
                                       OMX_INOUT OMX_PTR     paramData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    logi("(f:%s, l:%d) paramIndex = 0x%x", __FUNCTION__, __LINE__, paramIndex);
    if(m_state == OMX_StateInvalid)
    {
    	logi("Get Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if(paramData == NULL)
    {
    	logi("Get Param in Invalid paramData \n");
        return OMX_ErrorBadParameter;
    }
    
    switch(paramIndex)
    {
    	case OMX_IndexParamVideoInit:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoInit");
	        memcpy(paramData, &m_sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
    		break;
    	}

    	case OMX_IndexParamPortDefinition:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamPortDefinition");
	        if (((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nPortIndex == m_sInPortDef.nPortIndex)
	        {
	            memcpy(paramData, &m_sInPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                logi(" get_OMX_IndexParamPortDefinition: m_sInPortDef.nPortIndex[%d]", (int)m_sInPortDef.nPortIndex);
	        }
	        else if (((OMX_PARAM_PORTDEFINITIONTYPE*)(paramData))->nPortIndex == m_sOutPortDef.nPortIndex)
	        {
	            //logd("*** before , m_sOutPortDef.nBufferSize = %lu",m_sOutPortDef.nBufferSize);
	            //m_sOutPortDef.nBufferSize  = (OMX_U32)(m_sOutPortDef.format.video.nFrameWidth*m_sOutPortDef.format.video.nFrameHeight*3/2);//* test
                //logd("*** after , m_sOutPortDef.nBufferSize = %lu",m_sOutPortDef.nBufferSize);
                memcpy(paramData, &m_sOutPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	            logi("(omx_vdec, f:%s, l:%d) OMX_IndexParamPortDefinition, width = %d, height = %d, nPortIndex[%d], nBufferCountActual[%d], nBufferCountMin[%d], nBufferSize[%d]", __FUNCTION__, __LINE__, 
                    (int)m_sOutPortDef.format.video.nFrameWidth, (int)m_sOutPortDef.format.video.nFrameHeight,
                    (int)m_sOutPortDef.nPortIndex, (int)m_sOutPortDef.nBufferCountActual, (int)m_sOutPortDef.nBufferCountMin, (int)m_sOutPortDef.nBufferSize);
	        }
	        else
	        {
	            eError = OMX_ErrorBadPortIndex;
                logw(" get_OMX_IndexParamPortDefinition: error. paramData->nPortIndex=[%d]", (int)((OMX_PARAM_PORTDEFINITIONTYPE*)(paramData))->nPortIndex);
	        }

    		break;
    	}

    	case OMX_IndexParamVideoPortFormat:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoPortFormat");
 	        if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)(paramData))->nPortIndex == m_sInPortFormat.nPortIndex)
 	        {
 	            if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(paramData))->nIndex > m_sInPortFormat.nIndex)
		            eError = OMX_ErrorNoMore;
		        else
		            memcpy(paramData, &m_sInPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
		    }
	        else if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(paramData))->nPortIndex == m_sOutPortFormat.nPortIndex)
	        {
	            if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(paramData))->nIndex > m_sOutPortFormat.nIndex)
		            eError = OMX_ErrorNoMore;
		        else
		            memcpy(paramData, &m_sOutPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
		    }
		    else
		        eError = OMX_ErrorBadPortIndex;

            logi("OMX_IndexParamVideoPortFormat, eError[0x%x]", eError);
	        break;
    	}

    	case OMX_IndexParamStandardComponentRole:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamStandardComponentRole");
    		OMX_PARAM_COMPONENTROLETYPE* comp_role;

    		comp_role                    = (OMX_PARAM_COMPONENTROLETYPE *) paramData;
    		comp_role->nVersion.nVersion = OMX_SPEC_VERSION;
    		comp_role->nSize             = sizeof(*comp_role);

    		strncpy((char*)comp_role->cRole, (const char*)m_cRole, OMX_MAX_STRINGNAME_SIZE);
    		break;
    	}

    	case OMX_IndexParamPriorityMgmt:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamPriorityMgmt");
	        memcpy(paramData, &m_sPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
    		break;
    	}

    	case OMX_IndexParamCompBufferSupplier:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamCompBufferSupplier");
            OMX_PARAM_BUFFERSUPPLIERTYPE* pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE*)paramData;

            if (pBuffSupplierParam->nPortIndex == 1)
            {
                pBuffSupplierParam->eBufferSupplier = m_sOutBufSupplier.eBufferSupplier;
            }
            else if (pBuffSupplierParam->nPortIndex == 0)
            {
                pBuffSupplierParam->eBufferSupplier = m_sInBufSupplier.eBufferSupplier;
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }

    		break;
    	}

    	case OMX_IndexParamAudioInit:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamAudioInit");
    		OMX_PORT_PARAM_TYPE *audioPortParamType = (OMX_PORT_PARAM_TYPE *) paramData;

    		audioPortParamType->nVersion.nVersion = OMX_SPEC_VERSION;
    		audioPortParamType->nSize             = sizeof(OMX_PORT_PARAM_TYPE);
    		audioPortParamType->nPorts            = 0;
    		audioPortParamType->nStartPortNumber  = 0;

    		break;
    	}

    	case OMX_IndexParamImageInit:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamImageInit");
    		OMX_PORT_PARAM_TYPE *imagePortParamType = (OMX_PORT_PARAM_TYPE *) paramData;

    		imagePortParamType->nVersion.nVersion = OMX_SPEC_VERSION;
    		imagePortParamType->nSize             = sizeof(OMX_PORT_PARAM_TYPE);
    		imagePortParamType->nPorts            = 0;
    		imagePortParamType->nStartPortNumber  = 0;

    		break;
    	}

    	case OMX_IndexParamOtherInit:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamOtherInit");
    		OMX_PORT_PARAM_TYPE *otherPortParamType = (OMX_PORT_PARAM_TYPE *) paramData;

    		otherPortParamType->nVersion.nVersion = OMX_SPEC_VERSION;
    		otherPortParamType->nSize             = sizeof(OMX_PORT_PARAM_TYPE);
    		otherPortParamType->nPorts            = 0;
    		otherPortParamType->nStartPortNumber  = 0;

    		break;
    	}

    	case OMX_IndexParamVideoAvc:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoAvc");
        	logi("get_parameter: OMX_IndexParamVideoAvc, do nothing.\n");
            break;
    	}

    	case OMX_IndexParamVideoH263:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoH263");
        	logi("get_parameter: OMX_IndexParamVideoH263, do nothing.\n");
            break;
    	}

    	case OMX_IndexParamVideoMpeg4:
    	{
    		logi(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoMpeg4");
        	logi("get_parameter: OMX_IndexParamVideoMpeg4, do nothing.\n");
            break;
    	}
        case OMX_IndexParamVideoProfileLevelQuerySupported:
        {
        	VIDEO_PROFILE_LEVEL_TYPE* pProfileLevel = NULL;
        	OMX_U32 nNumberOfProfiles = 0;
        	OMX_VIDEO_PARAM_PROFILELEVELTYPE *pParamProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)paramData;

        	pParamProfileLevel->nPortIndex = m_sInPortDef.nPortIndex;

        	/* Choose table based on compression format */
        	switch(m_sInPortDef.format.video.eCompressionFormat)
        	{
        	case OMX_VIDEO_CodingH263:
        		pProfileLevel = SupportedH263ProfileLevels;
        		nNumberOfProfiles = sizeof(SupportedH263ProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
        		break;

        	case OMX_VIDEO_CodingMPEG4:
        		pProfileLevel = SupportedMPEG4ProfileLevels;
        		nNumberOfProfiles = sizeof(SupportedMPEG4ProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
        		break;

        	case OMX_VIDEO_CodingAVC:
			if (mIsFromCts == true){
				pProfileLevel = CTSSupportedAVCProfileLevels;
				nNumberOfProfiles = sizeof(CTSSupportedAVCProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
			}else{
				pProfileLevel = SupportedAVCProfileLevels;
				nNumberOfProfiles = sizeof(SupportedAVCProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
			}
			break;

        	default:
                logw("OMX_IndexParamVideoProfileLevelQuerySupported, Format[0x%x] not support", m_sInPortDef.format.video.eCompressionFormat);
        		return OMX_ErrorBadParameter;
        	}

        	if(((int)pParamProfileLevel->nProfileIndex < 0) || (pParamProfileLevel->nProfileIndex >= (nNumberOfProfiles - 1)))
        	{
                logw("pParamProfileLevel->nProfileIndex[0x%x] error!", (unsigned int)pParamProfileLevel->nProfileIndex);
        		return OMX_ErrorBadParameter;
        	}

        	/* Point to table entry based on index */
        	pProfileLevel += pParamProfileLevel->nProfileIndex;

        	/* -1 indicates end of table */
        	if(pProfileLevel->nProfile != -1)
        	{
        		pParamProfileLevel->eProfile = pProfileLevel->nProfile;
        		pParamProfileLevel->eLevel = pProfileLevel->nLevel;
        		eError = OMX_ErrorNone;
        	}
        	else
        	{
                logw("pProfileLevel->nProfile error!");
        		eError = OMX_ErrorNoMore;
        	}

        	break;
        }
    	default:
    	{
    		if((AW_VIDEO_EXTENSIONS_INDEXTYPE)paramIndex == AWOMX_IndexParamVideoGetAndroidNativeBufferUsage)
    		{
        		logi(" COMPONENT_GET_PARAMETER: AWOMX_IndexParamVideoGetAndroidNativeBufferUsage");
                break;
    		}
    		else
    		{
        		logi("get_parameter: unknown param %08x\n", paramIndex);
        		eError =OMX_ErrorUnsupportedIndex;
        		break;
    		}
    	}
    }

    return eError;
}

OMX_ERRORTYPE  omx_vdec::set_parameter(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_INDEXTYPE paramIndex,  OMX_IN OMX_PTR paramData)
{
    OMX_ERRORTYPE         eError      = OMX_ErrorNone;
    unsigned int          alignment   = 0;
    unsigned int          buffer_size = 0;
    int                   i;

    logi("(f:%s, l:%d) paramIndex = 0x%x", __FUNCTION__, __LINE__, paramIndex);
    if(m_state == OMX_StateInvalid)
    {
        logi("Set Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if(paramData == NULL)
    {
    	logi("Get Param in Invalid paramData \n");
    	return OMX_ErrorBadParameter;
    }

    switch(paramIndex)
    {
	    case OMX_IndexParamPortDefinition:
	    {
	    	logi(" COMPONENT_SET_PARAMETER: OMX_IndexParamPortDefinition");
            if (((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nPortIndex == m_sInPortDef.nPortIndex)
            {
                logi("set_OMX_IndexParamPortDefinition, m_sInPortDef.nPortIndex=%d", (int)m_sInPortDef.nPortIndex);
	        	if(((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nBufferCountActual != m_sInPortDef.nBufferCountActual)
	        	{
	        		int nBufCnt;
	        		int nIndex;


	        		pthread_mutex_lock(&m_inBufMutex);

	        		if(m_sInBufList.pBufArr != NULL)
	        			free(m_sInBufList.pBufArr);

	        		if(m_sInBufList.pBufHdrList != NULL)
	        			free(m_sInBufList.pBufHdrList);

	        		nBufCnt = ((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nBufferCountActual;
	        		logi("x allocate %d buffers.", nBufCnt);

	        	    m_sInBufList.pBufArr = (OMX_BUFFERHEADERTYPE*)malloc(sizeof(OMX_BUFFERHEADERTYPE)* nBufCnt);
	        	    m_sInBufList.pBufHdrList = (OMX_BUFFERHEADERTYPE**)malloc(sizeof(OMX_BUFFERHEADERTYPE*)* nBufCnt);
	        	    for (nIndex = 0; nIndex < nBufCnt; nIndex++)
	        	    {
	        	        OMX_CONF_INIT_STRUCT_PTR (&m_sInBufList.pBufArr[nIndex], OMX_BUFFERHEADERTYPE);
	        	    }

	        	    m_sInBufList.nSizeOfList       = 0;
	        	    m_sInBufList.nAllocSize        = 0;
	        	    m_sInBufList.nWritePos         = 0;
	        	    m_sInBufList.nReadPos          = 0;
	        	    m_sInBufList.nAllocBySelfFlags = 0;
	        	    m_sInBufList.nSizeOfList       = 0;
	        	    m_sInBufList.nBufArrSize       = nBufCnt;
	        	    m_sInBufList.eDir              = OMX_DirInput;

	        		pthread_mutex_unlock(&m_inBufMutex);
	        	}

	            memcpy(&m_sInPortDef, paramData, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

                m_streamInfo.nWidth  = m_sInPortDef.format.video.nFrameWidth;
                m_streamInfo.nHeight = m_sInPortDef.format.video.nFrameHeight;
            }
	        else if (((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nPortIndex == m_sOutPortDef.nPortIndex)
	        {
                logi("set_OMX_IndexParamPortDefinition, m_sOutPortDef.nPortIndex=%d", (int)m_sOutPortDef.nPortIndex);
	        	if(((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nBufferCountActual != m_sOutPortDef.nBufferCountActual)
	        	{
	        		int nBufCnt;
	        		int nIndex;

	        		pthread_mutex_lock(&m_outBufMutex);

	        		if(m_sOutBufList.pBufArr != NULL)
	        			free(m_sOutBufList.pBufArr);

	        		if(m_sOutBufList.pBufHdrList != NULL)
	        			free(m_sOutBufList.pBufHdrList);

	        		nBufCnt = ((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nBufferCountActual;
	        		logi("x allocate %d buffers.", nBufCnt);

                    //*Initialize the output buffer list
	        	    m_sOutBufList.pBufArr = (OMX_BUFFERHEADERTYPE*) malloc(sizeof(OMX_BUFFERHEADERTYPE) * nBufCnt);
	        	    m_sOutBufList.pBufHdrList = (OMX_BUFFERHEADERTYPE**) malloc(sizeof(OMX_BUFFERHEADERTYPE*) * nBufCnt);
	        	    for (nIndex = 0; nIndex < nBufCnt; nIndex++)
	        	    {
	        	        OMX_CONF_INIT_STRUCT_PTR (&m_sOutBufList.pBufArr[nIndex], OMX_BUFFERHEADERTYPE);
	        	    }

	        	    m_sOutBufList.nSizeOfList       = 0;
	        	    m_sOutBufList.nAllocSize        = 0;
	        	    m_sOutBufList.nWritePos         = 0;
	        	    m_sOutBufList.nReadPos          = 0;
	        	    m_sOutBufList.nAllocBySelfFlags = 0;
	        	    m_sOutBufList.nSizeOfList       = 0;
	        	    m_sOutBufList.nBufArrSize       = nBufCnt;
	        	    m_sOutBufList.eDir              = OMX_DirOutput;

	        		pthread_mutex_unlock(&m_outBufMutex);
	        	}
                
                OMX_BOOL bTmpPopulate = m_sOutPortDef.bPopulated;
                OMX_BOOL bTmpEnabled  = m_sOutPortDef.bEnabled;
                //logd("********* 0  m_sOutPortDef.format.video.eColorFormat=%d,m_sOutPortFormat.eColorFormat=%d",  m_sOutPortDef.format.video.eColorFormat, m_sOutPortFormat.eColorFormat);
	            memcpy(&m_sOutPortDef, paramData, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                //logd("********* 1  m_sOutPortDef.format.video.eColorFormat=%d,m_sOutPortFormat.eColorFormat =%d",  m_sOutPortDef.format.video.eColorFormat, m_sOutPortFormat.eColorFormat );
                m_sOutPortDef.bPopulated = bTmpPopulate;
                m_sOutPortDef.bEnabled   = bTmpEnabled;
                
                
                //check some key parameter
                if(m_sOutPortDef.format.video.nFrameWidth * m_sOutPortDef.format.video.nFrameHeight * 3 / 2 != m_sOutPortDef.nBufferSize)
                {
					logw("set_parameter, OMX_IndexParamPortDefinition, OutPortDef : change nBufferSize[%d] to [%d] to suit frame width[%d] and height[%d]",
                        (int)m_sOutPortDef.nBufferSize, 
                        (int)(m_sOutPortDef.format.video.nFrameWidth * m_sOutPortDef.format.video.nFrameHeight * 3 / 2), 
                        (int)m_sOutPortDef.format.video.nFrameWidth, 
                        (int)m_sOutPortDef.format.video.nFrameHeight);
                    m_sOutPortDef.nBufferSize = m_sOutPortDef.format.video.nFrameWidth * m_sOutPortDef.format.video.nFrameHeight * 3 / 2;
                }

                logi("(omx_vdec, f:%s, l:%d) OMX_IndexParamPortDefinition, width = %d, height = %d, nPortIndex[%d], nBufferCountActual[%d], nBufferCountMin[%d], nBufferSize[%d]", __FUNCTION__, __LINE__, 
                    (int)m_sOutPortDef.format.video.nFrameWidth, (int)m_sOutPortDef.format.video.nFrameHeight,
                    (int)m_sOutPortDef.nPortIndex, (int)m_sOutPortDef.nBufferCountActual, (int)m_sOutPortDef.nBufferCountMin, (int)m_sOutPortDef.nBufferSize);
	        }
	        else
	        {
                logw("set_OMX_IndexParamPortDefinition, error, paramPortIndex=%d", (int)((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nPortIndex);
	            eError = OMX_ErrorBadPortIndex;
	        }

	       break;
	    }

	    case OMX_IndexParamVideoPortFormat:
	    {
	    	logi(" COMPONENT_SET_PARAMETER: OMX_IndexParamVideoPortFormat");

	        if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)(paramData))->nPortIndex == m_sInPortFormat.nPortIndex)
	        {
	            if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)(paramData))->nIndex > m_sInPortFormat.nIndex)
		            eError = OMX_ErrorNoMore;
		        else
		            memcpy(&m_sInPortFormat, paramData, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	        }
	        else if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(paramData))->nPortIndex == m_sOutPortFormat.nPortIndex)
	        {
	            if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(paramData))->nIndex > m_sOutPortFormat.nIndex)
		            eError = OMX_ErrorNoMore;
		        else
		            memcpy(&m_sOutPortFormat, paramData, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	        }
	        else
	            eError = OMX_ErrorBadPortIndex;
	        break;
	    }

	    case OMX_IndexParamStandardComponentRole:
	    {
	    	logi(" COMPONENT_SET_PARAMETER: OMX_IndexParamStandardComponentRole");
    		OMX_PARAM_COMPONENTROLETYPE *comp_role;
    		comp_role = (OMX_PARAM_COMPONENTROLETYPE *) paramData;
    		logi("set_parameter: OMX_IndexParamStandardComponentRole %s\n", comp_role->cRole);

    		if((m_state == OMX_StateLoaded)/* && !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING)*/)
    		{
    			logi("Set Parameter called in valid state");
    		}
    		else
    		{
    			logi("Set Parameter called in Invalid State\n");
    			return OMX_ErrorIncorrectStateOperation;
    		}

            if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mjpeg", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.mjpeg",OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.mjpeg",OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg1", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((char*)comp_role->cRole, "video_decoder.mpeg1", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.mpeg1", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
                  	logi("Setparameter: unknown Index %s\n", comp_role->cRole);
                  	eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg2", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((char*)comp_role->cRole, "video_decoder.mpeg2", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.mpeg2", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
                  	logi("Setparameter: unknown Index %s\n", comp_role->cRole);
                  	eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
    		else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError = OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.msmpeg4v1", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.msmpeg4v1",OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.msmpeg4v1",OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError = OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.msmpeg4v2", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.msmpeg4v2",OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.msmpeg4v2",OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError = OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.divx", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.divx",OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError = OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.xvid", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.xvid",OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.xvid",OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError = OMX_ErrorUnsupportedSetting;
    			}
    		}
    		else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.h263", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.h263", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.h263", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.s263", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.s263", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.s263", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.rxg2", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.rxg2", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.rxg2", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.wmv1", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.wmv1",OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.wmv1",OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.wmv2", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.wmv2",OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.wmv2",OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vc1", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vp6", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((char*)comp_role->cRole, "video_decoder.vp6", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.vp6", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
                  	logi("Setparameter: unknown Index %s\n", comp_role->cRole);
                  	eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vp8", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((char*)comp_role->cRole, "video_decoder.vp8", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.vp8", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
                  	logi("Setparameter: unknown Index %s\n", comp_role->cRole);
                  	eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vp9", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((char*)comp_role->cRole, "video_decoder.vp9", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.vp9", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
                  	logi("Setparameter: unknown Index %s\n", comp_role->cRole);
                  	eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.avc", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((char*)comp_role->cRole, "video_decoder.avc", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.avc", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
                  	logi("Setparameter: unknown Index %s\n", comp_role->cRole);
                  	eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.hevc", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.hevc", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.hevc", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				logi("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
    		else
    		{
    			logi("Setparameter: unknown param %s\n", m_cName);
    			eError = OMX_ErrorInvalidComponentName;
    		}

    		break;
	    }

	    case OMX_IndexParamPriorityMgmt:
	    {
	    	logi(" COMPONENT_SET_PARAMETER: OMX_IndexParamPriorityMgmt");
            if(m_state != OMX_StateLoaded)
            {
            	logi("Set Parameter called in Invalid State\n");
            	return OMX_ErrorIncorrectStateOperation;
            }

            OMX_PRIORITYMGMTTYPE *priorityMgmtype = (OMX_PRIORITYMGMTTYPE*) paramData;

            m_sPriorityMgmt.nGroupID = priorityMgmtype->nGroupID;
            m_sPriorityMgmt.nGroupPriority = priorityMgmtype->nGroupPriority;

            break;
	    }

	    case OMX_IndexParamCompBufferSupplier:
	    {
	    	logi(" COMPONENT_SET_PARAMETER: OMX_IndexParamCompBufferSupplier");
    		OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;

            logi("set_parameter: OMX_IndexParamCompBufferSupplier %d\n", bufferSupplierType->eBufferSupplier);
            if(bufferSupplierType->nPortIndex == 0)
            	m_sInBufSupplier.eBufferSupplier = bufferSupplierType->eBufferSupplier;
            else if(bufferSupplierType->nPortIndex == 1)
            	m_sOutBufSupplier.eBufferSupplier = bufferSupplierType->eBufferSupplier;
        	else
        		eError = OMX_ErrorBadPortIndex;

        	break;
	    }

	    case OMX_IndexParamVideoAvc:
	    {
	    	logi(" COMPONENT_SET_PARAMETER: OMX_IndexParamVideoAvc");
      		logi("set_parameter: OMX_IndexParamVideoAvc, do nothing.\n");
	    	break;
	    }

	    case OMX_IndexParamVideoH263:
	    {
	    	logi(" COMPONENT_SET_PARAMETER: OMX_IndexParamVideoH263");
     		logi("set_parameter: OMX_IndexParamVideoH263, do nothing.\n");
	    	break;
	    }

	    case OMX_IndexParamVideoMpeg4:
	    {
	    	logi(" COMPONENT_SET_PARAMETER: OMX_IndexParamVideoMpeg4");
     		logi("set_parameter: OMX_IndexParamVideoMpeg4, do nothing.\n");
	    	break;
	    }

	    default:
	    {
#if CONFIG_OS == OPTION_OS_ANDROID            
	    	if((AW_VIDEO_EXTENSIONS_INDEXTYPE)paramIndex == AWOMX_IndexParamVideoUseAndroidNativeBuffer2)
	    	{
		    	logi(" COMPONENT_SET_PARAMETER: AWOMX_IndexParamVideoUseAndroidNativeBuffer2");
	     		logi("set_parameter: AWOMX_IndexParamVideoUseAndroidNativeBuffer2, do nothing.\n");
	     		m_useAndroidBuffer = OMX_TRUE;
		    	break;
	    	}
	    	else if((AW_VIDEO_EXTENSIONS_INDEXTYPE)paramIndex == AWOMX_IndexParamVideoEnableAndroidNativeBuffers)
	    	{
		    	logi(" COMPONENT_SET_PARAMETER: AWOMX_IndexParamVideoEnableAndroidNativeBuffers");
	     		logi("set_parameter: AWOMX_IndexParamVideoEnableAndroidNativeBuffers, set m_useAndroidBuffer to OMX_TRUE\n");

                EnableAndroidNativeBuffersParams *EnableAndroidBufferParams =  (EnableAndroidNativeBuffersParams*) paramData;
                logi(" enbleParam = %d\n",EnableAndroidBufferParams->enable);
                if(1==EnableAndroidBufferParams->enable)
                {
                    m_useAndroidBuffer = OMX_TRUE;
                }         
		    	break;
	    	}
            else if((AW_VIDEO_EXTENSIONS_INDEXTYPE)paramIndex==AWOMX_IndexParamVideoUseStoreMetaDataInBuffer)
            {
                logi(" COMPONENT_SET_PARAMETER: AWOMX_IndexParamVideoUseStoreMetaDataInBuffer");

                StoreMetaDataInBuffersParams *pStoreMetaData = (StoreMetaDataInBuffersParams*)paramData;
                if(pStoreMetaData->nPortIndex != 1)
                {
                    logd("error: not support set AWOMX_IndexParamVideoUseStoreMetaDataInBuffer for inputPort");
                    eError = OMX_ErrorUnsupportedIndex;
                }
                if(pStoreMetaData->nPortIndex==1 && pStoreMetaData->bStoreMetaData==OMX_TRUE)
                {
                    logi("***set m_storeOutputMetaDataFlag to TRUE");
                    m_storeOutputMetaDataFlag = OMX_TRUE;
                }
            }
            #if(CONFIG_OS_VERSION >= OPTION_OS_VERSION_ANDROID_4_4)  
            else if((AW_VIDEO_EXTENSIONS_INDEXTYPE)paramIndex==AWOMX_IndexParamVideoUsePrepareForAdaptivePlayback)
            {
                logi(" COMPONENT_SET_PARAMETER: AWOMX_IndexParamVideoUsePrepareForAdaptivePlayback");

                PrepareForAdaptivePlaybackParams *pPlaybackParams;
                pPlaybackParams = (PrepareForAdaptivePlaybackParams *)paramData;

                if(pPlaybackParams->nPortIndex==1 && pPlaybackParams->bEnable==OMX_TRUE)
                {
                    logi("set adaptive playback ,maxWidth = %d, maxHeight = %d",
                           (int)pPlaybackParams->nMaxFrameWidth,
                           (int)pPlaybackParams->nMaxFrameHeight);
                    
                    m_maxWidth  = pPlaybackParams->nMaxFrameWidth;
                    m_maxHeight = pPlaybackParams->nMaxFrameHeight;
                }
            }
            #endif            
	    	else
	    	{
	    		logi("Setparameter: unknown param %d\n", paramIndex);
	    		eError = OMX_ErrorUnsupportedIndex;
	    		break;
	    	}
#else
            logi("Setparameter: unknown param %d\n", paramIndex);
    		eError = OMX_ErrorUnsupportedIndex;
    		break;
#endif
	    }
    }

    return eError;
}

OMX_ERRORTYPE  omx_vdec::get_config(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_INDEXTYPE configIndex, OMX_INOUT OMX_PTR configData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

    logi("(f:%s, l:%d) index = %d", __FUNCTION__, __LINE__, configIndex);
	if (m_state == OMX_StateInvalid)
	{
		logi("get_config in Invalid State\n");
		return OMX_ErrorInvalidState;
	}

	switch (configIndex)
	{
        case OMX_IndexConfigCommonOutputCrop:
        {
            OMX_CONFIG_RECTTYPE *pRect = (OMX_CONFIG_RECTTYPE *)configData;

            OMX_CONFIG_RECTTYPE    mVideoRect;    //* for display crop

            mVideoRect.nLeft = 0;
            mVideoRect.nTop = 0;
            mVideoRect.nHeight = m_sOutPortDef.format.video.nFrameHeight;
            mVideoRect.nWidth = m_sOutPortDef.format.video.nFrameWidth;
            
            logi("+++++ get display crop: top[%d],left[%d],width[%d],height[%d]",
                  (int)mVideoRect.nTop,(int)mVideoRect.nLeft,
                  (int)mVideoRect.nWidth,(int)mVideoRect.nHeight);
            
            if(mVideoRect.nHeight != 0)
            {
                memcpy(pRect,&mVideoRect,sizeof(OMX_CONFIG_RECTTYPE));
            }
            else
            {
                logw("the crop is invalid!");
                eError = OMX_ErrorUnsupportedIndex;
            }
            break;
        }
    	default:
    	{
    		logi("get_config: unknown param %d\n",configIndex);
    		eError = OMX_ErrorUnsupportedIndex;
    	}
	}

	return eError;
}

OMX_ERRORTYPE omx_vdec::set_config(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_INDEXTYPE configIndex, OMX_IN OMX_PTR configData)
{

	logi("(f:%s, l:%d) index = %d", __FUNCTION__, __LINE__, configIndex);

	if(m_state == OMX_StateInvalid)
	{
		logi("set_config in Invalid State\n");
		return OMX_ErrorInvalidState;
	}

	OMX_ERRORTYPE eError = OMX_ErrorNone;

	if (m_state == OMX_StateExecuting)
	{
		logi("set_config: Ignore in Executing state\n");
		return eError;
	}

	switch(configIndex)
	{
		default:
		{
			eError = OMX_ErrorUnsupportedIndex;
		}
	}

	return eError;
}


OMX_ERRORTYPE  omx_vdec::get_extension_index(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_STRING paramName, OMX_OUT OMX_INDEXTYPE* indexType)
{
	unsigned int  nIndex;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    logi("(f:%s, l:%d) param name = %s", __FUNCTION__, __LINE__, paramName);
    if(m_state == OMX_StateInvalid)
    {
    	logi("Get Extension Index in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if(hComp == NULL)
    	return OMX_ErrorBadParameter;

    for(nIndex = 0; nIndex < sizeof(sVideoDecCustomParams)/sizeof(VIDDEC_CUSTOM_PARAM); nIndex++)
    {
        if(strcmp((char *)paramName, (char *)&(sVideoDecCustomParams[nIndex].cCustomParamName)) == 0)
        {
            *indexType = sVideoDecCustomParams[nIndex].nCustomParamIndex;
            eError = OMX_ErrorNone;
            break;
        }
    }

    return eError;
}



OMX_ERRORTYPE omx_vdec::get_state(OMX_IN OMX_HANDLETYPE hComp, OMX_OUT OMX_STATETYPE* state)
{
	logi("(f:%s, l:%d) ", __FUNCTION__, __LINE__);

	if(hComp == NULL || state == NULL)
		return OMX_ErrorBadParameter;

	*state = m_state;
    logi("COMPONENT_GET_STATE, state[0x%x]", m_state);
	return OMX_ErrorNone;
}


OMX_ERRORTYPE omx_vdec::component_tunnel_request(OMX_IN    OMX_HANDLETYPE       hComp,
                                                 OMX_IN    OMX_U32              port,
                                                 OMX_IN    OMX_HANDLETYPE       peerComponent,
                                                 OMX_IN    OMX_U32              peerPort,
                                                 OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup)
{
	logi(" COMPONENT_TUNNEL_REQUEST");

	logw("Error: component_tunnel_request Not Implemented\n");
	return OMX_ErrorNotImplemented;
}



OMX_ERRORTYPE  omx_vdec::use_buffer(OMX_IN    OMX_HANDLETYPE          hComponent,
                  			        OMX_INOUT OMX_BUFFERHEADERTYPE**  ppBufferHdr,
                  			        OMX_IN    OMX_U32                 nPortIndex,
                  			        OMX_IN    OMX_PTR                 pAppPrivate,
                  			        OMX_IN    OMX_U32                 nSizeBytes,
                  			        OMX_IN    OMX_U8*                 pBuffer)
{
    OMX_PARAM_PORTDEFINITIONTYPE*   pPortDef;
    OMX_U32                         nIndex = 0x0;

    logi("(f:%s, l:%d) PortIndex[%d], nSizeBytes[%d], pBuffer[%p]", __FUNCTION__, __LINE__, (int)nPortIndex, (int)nSizeBytes, pBuffer);
	if(hComponent == NULL || ppBufferHdr == NULL || pBuffer == NULL)
	{
		return OMX_ErrorBadParameter;
	}

    if (nPortIndex == m_sInPortDef.nPortIndex)
        pPortDef = &m_sInPortDef;
    else if (nPortIndex == m_sOutPortDef.nPortIndex)
        pPortDef = &m_sOutPortDef;
    else
    	return OMX_ErrorBadParameter;

    if (m_state!=OMX_StateLoaded && m_state!=OMX_StateWaitForResources && pPortDef->bEnabled!=OMX_FALSE)
    {
        logw("pPortDef[%d]->bEnabled=%d, m_state=0x%x, Can't use_buffer!", (int)nPortIndex, pPortDef->bEnabled, m_state);
    	return OMX_ErrorIncorrectStateOperation;
    }
    logi("pPortDef[%d]->bEnabled=%d, m_state=0x%x, can use_buffer.", (int)nPortIndex, pPortDef->bEnabled, m_state);
    
    

    if(pPortDef->bPopulated)
        return OMX_ErrorBadParameter;

    // Find an empty position in the BufferList and allocate memory for the buffer header.
    // Use the buffer passed by the client to initialize the actual buffer
    // inside the buffer header.
    if (nPortIndex == m_sInPortDef.nPortIndex)
    {
        if (nSizeBytes != pPortDef->nBufferSize)
          	return OMX_ErrorBadParameter;
    
        logi("use_buffer, m_sInPortDef.nPortIndex=[%d]", (int)m_sInPortDef.nPortIndex);
    	pthread_mutex_lock(&m_inBufMutex);

    	if((OMX_S32)m_sInBufList.nAllocSize >= m_sInBufList.nBufArrSize)
        {
        	pthread_mutex_unlock(&m_inBufMutex);
        	return OMX_ErrorInsufficientResources;
        }

    	nIndex = m_sInBufList.nAllocSize;
    	m_sInBufList.nAllocSize++;

    	m_sInBufList.pBufArr[nIndex].pBuffer          = pBuffer;
    	m_sInBufList.pBufArr[nIndex].nAllocLen        = nSizeBytes;
    	m_sInBufList.pBufArr[nIndex].pAppPrivate      = pAppPrivate;
    	m_sInBufList.pBufArr[nIndex].nInputPortIndex  = nPortIndex;
    	m_sInBufList.pBufArr[nIndex].nOutputPortIndex = 0xFFFFFFFE;
        *ppBufferHdr = &m_sInBufList.pBufArr[nIndex];
        if (m_sInBufList.nAllocSize == pPortDef->nBufferCountActual)
        	pPortDef->bPopulated = OMX_TRUE;

    	pthread_mutex_unlock(&m_inBufMutex);
    }
    else
    {
#if(CONFIG_OS == OPTION_OS_ANDROID && CONFIG_OS_VERSION >= OPTION_OS_VERSION_ANDROID_4_4)       
        if(m_storeOutputMetaDataFlag==OMX_TRUE)
        {
            if(nSizeBytes != sizeof(VideoDecoderOutputMetaData))
                return OMX_ErrorBadParameter;
        }
        else
        {
            if(nSizeBytes != pPortDef->nBufferSize)
          	    return OMX_ErrorBadParameter;
        }
#else
        if(nSizeBytes != pPortDef->nBufferSize)
          	    return OMX_ErrorBadParameter;
#endif
        
        logi("use_buffer, m_sOutPortDef.nPortIndex=[%d]", (int)m_sOutPortDef.nPortIndex);
    	pthread_mutex_lock(&m_outBufMutex);

    	if((OMX_S32)m_sOutBufList.nAllocSize >= m_sOutBufList.nBufArrSize)
        {
        	pthread_mutex_unlock(&m_outBufMutex);
        	return OMX_ErrorInsufficientResources;
        }
        pPortDef->nBufferSize = nSizeBytes;
    	nIndex = m_sOutBufList.nAllocSize;
    	m_sOutBufList.nAllocSize++;

    	m_sOutBufList.pBufArr[nIndex].pBuffer          = pBuffer;
    	m_sOutBufList.pBufArr[nIndex].nAllocLen        = nSizeBytes;
    	m_sOutBufList.pBufArr[nIndex].pAppPrivate      = pAppPrivate;
    	m_sOutBufList.pBufArr[nIndex].nInputPortIndex  = 0xFFFFFFFE;
    	m_sOutBufList.pBufArr[nIndex].nOutputPortIndex = nPortIndex;
        *ppBufferHdr = &m_sOutBufList.pBufArr[nIndex];
        if (m_sOutBufList.nAllocSize == pPortDef->nBufferCountActual)
        	pPortDef->bPopulated = OMX_TRUE;

    	pthread_mutex_unlock(&m_outBufMutex);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_vdec::allocate_buffer(OMX_IN    OMX_HANDLETYPE         hComponent,
        								OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        								OMX_IN    OMX_U32                nPortIndex,
        								OMX_IN    OMX_PTR                pAppPrivate,
        								OMX_IN    OMX_U32                nSizeBytes)
{
	OMX_S8                        nIndex = 0x0;
	OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;

	//logi(" COMPONENT_ALLOCATE_BUFFER");
    logi("(f:%s, l:%d) nPortIndex[%d], nSizeBytes[%d]", __FUNCTION__, __LINE__, (int)nPortIndex, (int)nSizeBytes);
	if(hComponent == NULL || ppBufferHdr == NULL)
    {   
        loge("hComponent=%p, ppBufferHdr=%p", hComponent, ppBufferHdr);
		return OMX_ErrorBadParameter;
    }
    if (nPortIndex == m_sInPortDef.nPortIndex)
        pPortDef = &m_sInPortDef;
    else
    {
        if (nPortIndex == m_sOutPortDef.nPortIndex)
	        pPortDef = &m_sOutPortDef;
        else
        {
            loge("nPortIndex = %lu", nPortIndex);
        	return OMX_ErrorBadParameter;
        } 
    }

//    if (!pPortDef->bEnabled)
//    	return OMX_ErrorIncorrectStateOperation;

    if (m_state!=OMX_StateLoaded && m_state!=OMX_StateWaitForResources && pPortDef->bEnabled!=OMX_FALSE) // 
    {
        logw("pPortDef[%d]->bEnabled=%d, m_state=0x%x, Can't allocate_buffer! pPortDef(%p)", (int)nPortIndex, pPortDef->bEnabled, m_state, pPortDef);
    	return OMX_ErrorIncorrectStateOperation;
    }
    logi("pPortDef[%d]->bEnabled=%d, m_state=0x%x, can allocate_buffer.", (int)nPortIndex, pPortDef->bEnabled, m_state);

    //if (nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated) 
    if(pPortDef->bPopulated) //* 
    {
        loge("nSizeBytes=%lu, pPortDef->bPopulated=%d", nSizeBytes,  pPortDef->bPopulated);
    	return OMX_ErrorBadParameter;
    }

    // Find an empty position in the BufferList and allocate memory for the buffer header
    // and the actual buffer
    if (nPortIndex == m_sInPortDef.nPortIndex)
    {
        logi("allocate_buffer, m_sInPortDef.nPortIndex[%d]", (int)m_sInPortDef.nPortIndex);
    	pthread_mutex_lock(&m_inBufMutex);

    	if((OMX_S32)m_sInBufList.nAllocSize >= m_sInBufList.nBufArrSize)
        {
        	pthread_mutex_unlock(&m_inBufMutex);
        	return OMX_ErrorInsufficientResources;
        }

    	nIndex = m_sInBufList.nAllocSize;

        m_sInBufList.pBufArr[nIndex].pBuffer = (OMX_U8*)malloc(nSizeBytes);

        if (!m_sInBufList.pBufArr[nIndex].pBuffer)
        {
        	pthread_mutex_unlock(&m_inBufMutex);
            return OMX_ErrorInsufficientResources;
        }

        m_sInBufList.nAllocBySelfFlags |= (1<<nIndex);

    	m_sInBufList.pBufArr[nIndex].nAllocLen        = nSizeBytes;
    	m_sInBufList.pBufArr[nIndex].pAppPrivate      = pAppPrivate;
    	m_sInBufList.pBufArr[nIndex].nInputPortIndex  = nPortIndex;
    	m_sInBufList.pBufArr[nIndex].nOutputPortIndex = 0xFFFFFFFE;
        *ppBufferHdr = &m_sInBufList.pBufArr[nIndex];

        m_sInBufList.nAllocSize++;
        
        if (m_sInBufList.nAllocSize == pPortDef->nBufferCountActual)
        	pPortDef->bPopulated = OMX_TRUE;

    	pthread_mutex_unlock(&m_inBufMutex);
    }
    else
    {
        logi("allocate_buffer, m_sOutPortDef.nPortIndex[%d]", (int)m_sOutPortDef.nPortIndex);
    	pthread_mutex_lock(&m_outBufMutex);

    	if((OMX_S32)m_sOutBufList.nAllocSize >= m_sOutBufList.nBufArrSize)
        {
        	pthread_mutex_unlock(&m_outBufMutex);
        	return OMX_ErrorInsufficientResources;
        }

    	nIndex = m_sOutBufList.nAllocSize;

    	m_sOutBufList.pBufArr[nIndex].pBuffer = (OMX_U8*)malloc(nSizeBytes);

        if (!m_sOutBufList.pBufArr[nIndex].pBuffer)
        {
        	pthread_mutex_unlock(&m_outBufMutex);
            return OMX_ErrorInsufficientResources;
        }

        m_sOutBufList.nAllocBySelfFlags |= (1<<nIndex);

        m_sOutBufList.pBufArr[nIndex].nAllocLen        = nSizeBytes;
        m_sOutBufList.pBufArr[nIndex].pAppPrivate      = pAppPrivate;
        m_sOutBufList.pBufArr[nIndex].nInputPortIndex  = 0xFFFFFFFE;
        m_sOutBufList.pBufArr[nIndex].nOutputPortIndex = nPortIndex;
        *ppBufferHdr = &m_sOutBufList.pBufArr[nIndex];

        m_sOutBufList.nAllocSize++;
        
        if (m_sOutBufList.nAllocSize == pPortDef->nBufferCountActual)
        	pPortDef->bPopulated = OMX_TRUE;

    	pthread_mutex_unlock(&m_outBufMutex);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_vdec::free_buffer(OMX_IN  OMX_HANDLETYPE        hComponent,
        							OMX_IN  OMX_U32               nPortIndex,
        							OMX_IN  OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;
    OMX_S32                       nIndex;

	logi("(f:%s, l:%d) nPortIndex = %d, pBufferHdr = %p, m_state=0x%x", __FUNCTION__, __LINE__, (int)nPortIndex, pBufferHdr, m_state);
    if(hComponent == NULL || pBufferHdr == NULL)
    	return OMX_ErrorBadParameter;

    // Match the pBufferHdr to the appropriate entry in the BufferList
    // and free the allocated memory
    if (nPortIndex == m_sInPortDef.nPortIndex)
    {
        pPortDef = &m_sInPortDef;

    	pthread_mutex_lock(&m_inBufMutex);

    	for(nIndex = 0; nIndex < m_sInBufList.nBufArrSize; nIndex++)
    	{
    		if(pBufferHdr == &m_sInBufList.pBufArr[nIndex])
    			break;
    	}

    	if(nIndex == m_sInBufList.nBufArrSize)
    	{
    		pthread_mutex_unlock(&m_inBufMutex);
    		return OMX_ErrorBadParameter;
    	}

    	if(m_sInBufList.nAllocBySelfFlags & (1<<nIndex))
    	{
    		free(m_sInBufList.pBufArr[nIndex].pBuffer);
    		m_sInBufList.pBufArr[nIndex].pBuffer = NULL;
    		m_sInBufList.nAllocBySelfFlags &= ~(1<<nIndex);
    	}

    	m_sInBufList.nAllocSize--;
        
    	if(m_sInBufList.nAllocSize == 0)
    		pPortDef->bPopulated = OMX_FALSE;

    	pthread_mutex_unlock(&m_inBufMutex);
    }
    else if (nPortIndex == m_sOutPortDef.nPortIndex)
    {
	    pPortDef = &m_sOutPortDef;

    	pthread_mutex_lock(&m_outBufMutex);

    	for(nIndex = 0; nIndex < m_sOutBufList.nBufArrSize; nIndex++)
    	{
    		logi("pBufferHdr = %p, &m_sOutBufList.pBufArr[%d] = %p", pBufferHdr, (int)nIndex, &m_sOutBufList.pBufArr[nIndex]);
    		if(pBufferHdr == &m_sOutBufList.pBufArr[nIndex])
    			break;
    	}

    	logi("index = %d", (int)nIndex);

    	if(nIndex == m_sOutBufList.nBufArrSize)
    	{
    		pthread_mutex_unlock(&m_outBufMutex);
    		return OMX_ErrorBadParameter;
    	}

    	if(m_sOutBufList.nAllocBySelfFlags & (1<<nIndex))
    	{
    		free(m_sOutBufList.pBufArr[nIndex].pBuffer);
    		m_sOutBufList.pBufArr[nIndex].pBuffer = NULL;
    		m_sOutBufList.nAllocBySelfFlags &= ~(1<<nIndex);
    	}

    	m_sOutBufList.nAllocSize--;
        logv("*** m_sOutBufList.nAllocSize = %lu, nBufArrSize = %ld, nBufferCountActual = %lu, nBufferSize = %lu",
             m_sOutBufList.nAllocSize,
             m_sOutBufList.nBufArrSize,
             pPortDef->nBufferCountActual,
             pPortDef->nBufferSize);
        
    	if(m_sOutBufList.nAllocSize == 0)
    		pPortDef->bPopulated = OMX_FALSE;

    	pthread_mutex_unlock(&m_outBufMutex);
    }
    else
        return OMX_ErrorBadParameter;

    return OMX_ErrorNone;
}




OMX_ERRORTYPE  omx_vdec::empty_this_buffer(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    logv("***emptyThisBuffer: pts = %lld , videoFormat = %d",
         pBufferHdr->nTimeStamp,
         m_eCompressionFormat);
    
    ThrCmdType eCmd   = EmptyBuf;
    if(hComponent == NULL || pBufferHdr == NULL)
    	return OMX_ErrorBadParameter;

    if (!m_sInPortDef.bEnabled)
    	return OMX_ErrorIncorrectStateOperation;

    if (pBufferHdr->nInputPortIndex != 0x0  || pBufferHdr->nOutputPortIndex != OMX_NOPORT)
        return OMX_ErrorBadPortIndex;

    if (m_state != OMX_StateExecuting && m_state != OMX_StatePause)
        return OMX_ErrorIncorrectStateOperation;

    post_event(eCmd, 0, (OMX_PTR)pBufferHdr);

    return OMX_ErrorNone;
}



OMX_ERRORTYPE  omx_vdec::fill_this_buffer(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    ThrCmdType eCmd = FillBuf;

    logi("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
    if(hComponent == NULL || pBufferHdr == NULL)
    	return OMX_ErrorBadParameter;

    if (!m_sOutPortDef.bEnabled)
    	return OMX_ErrorIncorrectStateOperation;

    if (pBufferHdr->nOutputPortIndex != 0x1 || pBufferHdr->nInputPortIndex != OMX_NOPORT)
        return OMX_ErrorBadPortIndex;

    if (m_state != OMX_StateExecuting && m_state != OMX_StatePause)
        return OMX_ErrorIncorrectStateOperation;

    post_event(eCmd, 0, (OMX_PTR)pBufferHdr);
    return OMX_ErrorNone;
}



OMX_ERRORTYPE  omx_vdec::set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
                                           OMX_IN OMX_CALLBACKTYPE* callbacks,
                                           OMX_IN OMX_PTR           appData)
{
	logi("(f:%s, l:%d) ", __FUNCTION__, __LINE__);

    if(hComp == NULL || callbacks == NULL || appData == NULL)
    	return OMX_ErrorBadParameter;
    memcpy(&m_Callbacks, callbacks, sizeof(OMX_CALLBACKTYPE));
    m_pAppData = appData;

    return OMX_ErrorNone;
}



OMX_ERRORTYPE  omx_vdec::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
	//logi(" COMPONENT_DEINIT");
    logi("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    ThrCmdType    eCmd   = Stop;
    OMX_S32       nIndex = 0;

    // In case the client crashes, check for nAllocSize parameter.
    // If this is greater than zero, there are elements in the list that are not free'd.
    // In that case, free the elements.
    if (m_sInBufList.nAllocSize > 0)
    {
    	for(nIndex=0; nIndex<m_sInBufList.nBufArrSize; nIndex++)
    	{
    		if(m_sInBufList.pBufArr[nIndex].pBuffer != NULL)
    		{
    			if(m_sInBufList.nAllocBySelfFlags & (1<<nIndex))
    			{
    				free(m_sInBufList.pBufArr[nIndex].pBuffer);
    				m_sInBufList.pBufArr[nIndex].pBuffer = NULL;
    			}
    		}
    	}

        if (m_sInBufList.pBufArr != NULL)
        	free(m_sInBufList.pBufArr);

        if (m_sInBufList.pBufHdrList != NULL)
        	free(m_sInBufList.pBufHdrList);

    	memset(&m_sInBufList, 0, sizeof(struct _BufferList));
    	m_sInBufList.nBufArrSize = m_sInPortDef.nBufferCountActual;
    }

    if (m_sOutBufList.nAllocSize > 0)
    {
    	for(nIndex=0; nIndex<m_sOutBufList.nBufArrSize; nIndex++)
    	{
    		if(m_sOutBufList.pBufArr[nIndex].pBuffer != NULL)
    		{
    			if(m_sOutBufList.nAllocBySelfFlags & (1<<nIndex))
    			{
    				free(m_sOutBufList.pBufArr[nIndex].pBuffer);
    				m_sOutBufList.pBufArr[nIndex].pBuffer = NULL;
    			}
    		}
    	}

        if (m_sOutBufList.pBufArr != NULL)
        	free(m_sOutBufList.pBufArr);

        if (m_sOutBufList.pBufHdrList != NULL)
        	free(m_sOutBufList.pBufHdrList);

    	memset(&m_sOutBufList, 0, sizeof(struct _BufferList));
    	m_sOutBufList.nBufArrSize = m_sOutPortDef.nBufferCountActual;
    }

    post_event(eCmd, eCmd, NULL);

    // Wait for thread to exit so we can get the status into "error"
    pthread_join(m_thread_id, (void**)&eError);
    pthread_join(m_vdrv_thread_id, (void**)&eError);
    
    logd("(f:%s, l:%d) two threads exit!", __FUNCTION__, __LINE__);
    
    // close the pipe handles
    close(m_cmdpipe[0]);
    close(m_cmdpipe[1]);
    close(m_cmddatapipe[0]);
    close(m_cmddatapipe[1]);
    close(m_vdrv_cmdpipe[0]);
    close(m_vdrv_cmdpipe[1]);

    if(m_decoder != NULL)
    {
    	DestroyVideoDecoder(m_decoder);
    	m_decoder = NULL;
    }
    
    return eError;
}


OMX_ERRORTYPE  omx_vdec::use_EGL_image(OMX_IN OMX_HANDLETYPE               hComp,
                                          OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                          OMX_IN OMX_U32                   port,
                                          OMX_IN OMX_PTR                   appData,
                                          OMX_IN void*                     eglImage)
{
	logw("Error : use_EGL_image:  Not Implemented \n");
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE  omx_vdec::component_role_enum(OMX_IN  OMX_HANDLETYPE hComp,
                                             OMX_OUT OMX_U8*        role,
                                             OMX_IN  OMX_U32        index)
{
	//logi(" COMPONENT_ROLE_ENUM");
    logi("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
	OMX_ERRORTYPE eRet = OMX_ErrorNone;

	if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE);
			logi("component_role_enum: role %s\n", role);
		}
		else
		{
			eRet = OMX_ErrorNoMore;
		}
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.h263", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
			logi("component_role_enum: role %s\n",role);
		}
		else
		{
			logi("\n No more roles \n");
			eRet = OMX_ErrorNoMore;
		}
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.avc", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
			logi("component_role_enum: role %s\n",role);
		}
		else
		{
			logi("\n No more roles \n");
			eRet = OMX_ErrorNoMore;
		}
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vc1", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
			logi("component_role_enum: role %s\n",role);
		}
		else
		{
			logi("\n No more roles \n");
			eRet = OMX_ErrorNoMore;
		}
	}
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vp8", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.vp8",OMX_MAX_STRINGNAME_SIZE);
			logi("component_role_enum: role %s\n",role);
		}
		else
		{
			logi("\n No more roles \n");
			eRet = OMX_ErrorNoMore;
		}
	}
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg1", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.mpeg1",OMX_MAX_STRINGNAME_SIZE);
			logi("component_role_enum: role %s\n",role);
		}
		else
		{
			logi("\n No more roles \n");
			eRet = OMX_ErrorNoMore;
		}
	}
    
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg2", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE);
			logi("component_role_enum: role %s\n",role);
		}
		else
		{
			logi("\n No more roles \n");
			eRet = OMX_ErrorNoMore;
		}
	}	
	else
	{
		logd("\nERROR:Querying Role on Unknown Component\n");
		eRet = OMX_ErrorInvalidComponentName;
	}

	return eRet;
}

OMX_ERRORTYPE omx_vdec::send_vdrv_feedback_msg(OMX_IN OMX_VDRV_FEEDBACK_MSGTYPE nMsg,
                                                   OMX_IN OMX_U32         param1,
                                                   OMX_IN OMX_PTR         cmdData)
{
    ThrCmdType    eCmd;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    logi("(f:%s, l:%d) ", __FUNCTION__, __LINE__);

    if(m_state == OMX_StateInvalid)
    {
    	logd("ERROR: Send Command in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    switch (nMsg)
    {
        case OMX_VdrvFeedbackMsg_NotifyEos:
        	logi("(omx_vdec, f:%s, l:%d) send OMX_VdrvFeedbackMsg_NotifyEos", __FUNCTION__, __LINE__);
            eCmd = VdrvNotifyEos;
	        break;

        case OMX_VdrvFeedbackMsg_ResolutionChange:
        	logi("(omx_vdec, f:%s, l:%d) send OMX_VdrvFeedbackMsg_ResolutionChange", __FUNCTION__, __LINE__);
            eCmd = VdrvResolutionChange;
            break;

        default:
            logw("(omx_vdec, f:%s, l:%d) send unknown feedback message[0x%x]", __FUNCTION__, __LINE__, nMsg);
            return OMX_ErrorUndefined;
    }
    
    post_event(eCmd, param1, cmdData);

    return eError;
}

OMX_ERRORTYPE omx_vdec::post_event(OMX_IN ThrCmdType eCmd,
                                   OMX_IN OMX_U32         param1,
                                   OMX_IN OMX_PTR         cmdData)
{
    ssize_t ret;
    pthread_mutex_lock(&m_pipeMutex);
    ret = write(m_cmdpipe[1], &eCmd, sizeof(eCmd));
    if(ret < 0)
    {
       logd("error: write data to pipe failed!,ret = %d",(int)ret);
       return OMX_ErrorNone;
    }
    // In case of MarkBuf, the pCmdData parameter is used to carry the data.
    // In other cases, the nParam1 parameter carries the data.
    if(eCmd == MarkBuf || eCmd == EmptyBuf || eCmd == FillBuf)
    {
        ret = write(m_cmddatapipe[1], &cmdData, sizeof(OMX_PTR));
        if(ret < 0)
        {
           logd("error: write data to pipe failed!,ret = %d",(int)ret);
           return OMX_ErrorNone;
        }
    }
    else
    {
        ret = write(m_cmddatapipe[1], &param1, sizeof(param1));
        if(ret < 0)
        {
           logd("error: write data to pipe failed!,ret = %d",(int)ret);
           return OMX_ErrorNone;
        }
    }
    
    pthread_mutex_unlock(&m_pipeMutex);

    return OMX_ErrorNone;
}

/****************************************************************************
physical address to virtual address
****************************************************************************/
int convertAddress_Phy2Vir(VideoPicture *picture)
{
    if(picture->pData0)
    {
        picture->pData0 = (char*)MemAdapterGetVirtualAddress((void*)picture->pData0);
    }
    /*
    if(picture->u)
    {
        picture->u = (u8*)cedarv_address_phy2vir(picture->u);
    }
    if(picture->v)
    {
        picture->v = (u8*)cedarv_address_phy2vir(picture->v);
    }
    */
    return 0;
}

/*******************************************************************************
Function name: detectPipeDataToRead
Description: 
    
Parameters: 
    
Return: 
    1:data ready
    0:no data
Time: 2013/9/30
*******************************************************************************/
int waitPipeDataToRead(int nPipeFd, int nTimeUs)
{
    int                     i;
    fd_set                  rfds;
    struct timeval          timeout;
    FD_ZERO(&rfds);
    FD_SET(nPipeFd, &rfds);

    // Check for new command
    timeout.tv_sec  = 0;
    timeout.tv_usec = nTimeUs;

    i = select(nPipeFd+1, &rfds, NULL, NULL, &timeout);

    if (FD_ISSET(nPipeFd, &rfds))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
/*
 *  Component Thread
 *    The ComponentThread function is exeuted in a separate pThread and
 *    is used to implement the actual component functions.
 */
 /*****************************************************************************/
static int count=0;
static void* ComponentThread(void* pThreadData)
{
	int                     decodeResult;
    VideoPicture*           picture;

    int                     i;
    int                     fd1;
    fd_set                  rfds;
    OMX_U32                 cmddata;
    ThrCmdType              cmd;
    ssize_t                 readRet = -1;

    // Variables related to decoder buffer handling
    OMX_BUFFERHEADERTYPE*   pInBufHdr   = NULL;
    OMX_BUFFERHEADERTYPE*   pOutBufHdr  = NULL;
    OMX_MARKTYPE*           pMarkBuf    = NULL;
    OMX_U8*                 pInBuf      = NULL;
    OMX_U32                 nInBufSize;

    // Variables related to decoder timeouts
    OMX_U32                 nTimeout;
    OMX_BOOL                port_setting_match;
    OMX_BOOL                nInBufEos;
    OMX_BOOL                nVdrvNotifyEosFlag;
    
    OMX_PTR                 pMarkData;
    OMX_HANDLETYPE          hMarkTargetComponent;
    struct timeval          timeout;

    nInBufEos            = OMX_FALSE;
    nVdrvNotifyEosFlag   = OMX_FALSE;
    port_setting_match   = OMX_TRUE;
    pMarkData            = NULL;
    hMarkTargetComponent = NULL;


    int64_t         nTimeUs1;
    int64_t         nTimeUs2;

    int nSemVal;
    int nRetSemGetValue;
    
    // Recover the pointer to my component specific data
    omx_vdec* pSelf = (omx_vdec*)pThreadData;

    while (1)
    {
        fd1 = pSelf->m_cmdpipe[0];
        FD_ZERO(&rfds);
        FD_SET(fd1, &rfds);

        //*Check for new command
        timeout.tv_sec  = 0;
        timeout.tv_usec = 0;

        i = select(pSelf->m_cmdpipe[0]+1, &rfds, NULL, NULL, &timeout);

        if (FD_ISSET(pSelf->m_cmdpipe[0], &rfds))
        {
            //*retrieve command and data from pipe
            readRet = read(pSelf->m_cmdpipe[0], &cmd, sizeof(cmd));
            if(readRet<0)
            {
                logd("error: read pipe data failed!,ret = %d",(int)readRet);
                goto EXIT;
            }
            
	        readRet = read(pSelf->m_cmddatapipe[0], &cmddata, sizeof(cmddata));
            if(readRet<0)
            {
                logd("error: read pipe data failed!,ret = %d",(int)readRet);
                goto EXIT;
            }
            
            //*State transition command
	        if (cmd == SetState)
	        {
	        	logi(" set state command, cmd = %d, cmddata = %d.", (int)cmd, (int)cmddata);
                //*If the parameter states a transition to the same state
                // raise a same state transition error.
                if (pSelf->m_state == (OMX_STATETYPE)(cmddata))
                {
                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorSameState, 0 , NULL);
                }
                else
                {
	                //*transitions/callbacks made based on state transition table
                    // cmddata contains the target state
	                switch ((OMX_STATETYPE)(cmddata))
	                {
             	        case OMX_StateInvalid:
             	        	pSelf->m_state = OMX_StateInvalid;
             	        	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorInvalidState, 0 , NULL);
             	        	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);
			                break;

		                case OMX_StateLoaded:
                            if (pSelf->m_state == OMX_StateIdle || pSelf->m_state == OMX_StateWaitForResources)
                            {
			                    nTimeout = 0x0;
			                    while (1)
			                    {
                                    //*Transition happens only when the ports are unpopulated
			                        if (!pSelf->m_sInPortDef.bPopulated && !pSelf->m_sOutPortDef.bPopulated)
			                        {
			                        	pSelf->m_state = OMX_StateLoaded;
			                        	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);
                                        
 			                            //* close decoder
			                        	//* TODO.

				                        break;
 	                                }
				                    else if (nTimeout++ > OMX_MAX_TIMEOUTS)
				                    {
				                    	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorInsufficientResources, 0 , NULL);
 	                                    logw("Transition to loaded failed\n");
				                        break;
				                    }

			                        usleep(OMX_TIMEOUT*1000);
			                    }

                                post_message_to_vdrv(pSelf, OMX_VdrvCommand_CloseVdecLib);
                                logi("(f:%s, l:%d) wait for OMX_VdrvCommand_CloseVdecLib", __FUNCTION__, __LINE__);
                                sem_wait(&pSelf->m_vdrv_cmd_lock);
                                logi("(f:%s, l:%d) wait for OMX_VdrvCommand_CloseVdecLib done!", __FUNCTION__, __LINE__);
		                    }
			                else
			                {
			                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
			                }

			                break;

                        case OMX_StateIdle:
		                    if (pSelf->m_state == OMX_StateInvalid)
		                    	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
		                    else
		                    {
			                    //*Return buffers if currently in pause and executing
			                    if (pSelf->m_state == OMX_StatePause || pSelf->m_state == OMX_StateExecuting)
			                    {
			                    	pthread_mutex_lock(&pSelf->m_inBufMutex);

                                    while (pSelf->m_sInBufList.nSizeOfList > 0)
                                    {
                                    	pSelf->m_sInBufList.nSizeOfList--;
                                    	pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp,
                                    			                           pSelf->m_pAppData,
                                    			                           pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos++]);

                                    	if (pSelf->m_sInBufList.nReadPos >= pSelf->m_sInBufList.nBufArrSize)
                                    		pSelf->m_sInBufList.nReadPos = 0;
                                    }

                                    pSelf->m_sInBufList.nReadPos  = 0;
                                    pSelf->m_sInBufList.nWritePos = 0;

			                    	pthread_mutex_unlock(&pSelf->m_inBufMutex);


			                    	pthread_mutex_lock(&pSelf->m_outBufMutex);

                                    while (pSelf->m_sOutBufList.nSizeOfList > 0)
                                    {
                                    	pSelf->m_sOutBufList.nSizeOfList--;
                                    	pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp,
                                    			                           pSelf->m_pAppData,
                                    			                           pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++]);

                                    	if (pSelf->m_sOutBufList.nReadPos >= pSelf->m_sOutBufList.nBufArrSize)
                                    		pSelf->m_sOutBufList.nReadPos = 0;
                                    }

                                    pSelf->m_sOutBufList.nReadPos  = 0;
                                    pSelf->m_sOutBufList.nWritePos = 0;

			                    	pthread_mutex_unlock(&pSelf->m_outBufMutex);
			                    }
			                    else    //OMX_StateLoaded -> OMX_StateIdle
			                    {
                                    //*we not create decoder here
                                    /*
                                    post_message_to_vdrv(pSelf, OMX_VdrvCommand_PrepareVdecLib);
                                    logd("(f:%s, l:%d) wait for OMX_VdrvCommand_PrepareVdecLib", __FUNCTION__, __LINE__);
                                    sem_wait(&pSelf->m_vdrv_cmd_lock);
                                    logd("(f:%s, l:%d) wait for OMX_VdrvCommand_PrepareVdecLib done!", __FUNCTION__, __LINE__);
                                    */
                                }

			                    nTimeout = 0x0;
			                    while (1)
			                    {
			                        //*Ports have to be populated before transition completes
			                        if ((!pSelf->m_sInPortDef.bEnabled && !pSelf->m_sOutPortDef.bEnabled)   ||
                                        (pSelf->m_sInPortDef.bPopulated && pSelf->m_sOutPortDef.bPopulated))
                                    {
			                        	pSelf->m_state = OMX_StateIdle;
                                        //*wake up vdrvThread
                                        sem_post(&pSelf->m_sem_vbs_input);
                                        sem_post(&pSelf->m_sem_frame_output);
			                        	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);

                                        //* Open decoder
			                        	//* TODO.
 		                                break;
			                        }
			                        else if (nTimeout++ > OMX_MAX_TIMEOUTS)
			                        {
			                        	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorInsufficientResources, 0 , NULL);
                		                logw("Idle transition failed\n");
	                                    break;
				                    }

			                        usleep(OMX_TIMEOUT*1000);
			                    }
		                    }

		                    break;

		                case OMX_StateExecuting:
                            //*Transition can only happen from pause or idle state
                            if (pSelf->m_state == OMX_StateIdle || pSelf->m_state == OMX_StatePause)
                            {
                                //*Return buffers if currently in pause
			                    if (pSelf->m_state == OMX_StatePause)
			                    {
			                    	pthread_mutex_lock(&pSelf->m_inBufMutex);

                                    while (pSelf->m_sInBufList.nSizeOfList > 0)
                                    {
                                    	pSelf->m_sInBufList.nSizeOfList--;
                                    	pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp,
                                    			                           pSelf->m_pAppData,
                                    			                           pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos++]);

                                    	if (pSelf->m_sInBufList.nReadPos >= pSelf->m_sInBufList.nBufArrSize)
                                    		pSelf->m_sInBufList.nReadPos = 0;
                                    }

                                    pSelf->m_sInBufList.nReadPos  = 0;
                                    pSelf->m_sInBufList.nWritePos = 0;

			                    	pthread_mutex_unlock(&pSelf->m_inBufMutex);

			                    	pthread_mutex_lock(&pSelf->m_outBufMutex);

                                    while (pSelf->m_sOutBufList.nSizeOfList > 0)
                                    {
                                    	pSelf->m_sOutBufList.nSizeOfList--;
                                    	pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp,
                                    			                           pSelf->m_pAppData,
                                    			                           pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++]);

                                    	if (pSelf->m_sOutBufList.nReadPos >= pSelf->m_sOutBufList.nBufArrSize)
                                    		pSelf->m_sOutBufList.nReadPos = 0;
                                    }

                                    pSelf->m_sOutBufList.nReadPos  = 0;
                                    pSelf->m_sOutBufList.nWritePos = 0;

			                    	pthread_mutex_unlock(&pSelf->m_outBufMutex);
			                    }

			                    pSelf->m_state = OMX_StateExecuting;
			                    pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);
			                }
			                else
			                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);

                            nInBufEos            = OMX_FALSE;
                            pMarkData            = NULL;
                            hMarkTargetComponent = NULL;

			                break;

                        case OMX_StatePause:
                            // Transition can only happen from idle or executing state
		                    if (pSelf->m_state == OMX_StateIdle || pSelf->m_state == OMX_StateExecuting)
		                    {
		                    	pSelf->m_state = OMX_StatePause;
		                    	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);
		                    }
			                else
			                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);

		                    break;

                        case OMX_StateWaitForResources:
		                    if (pSelf->m_state == OMX_StateLoaded)
		                    {
		                    	pSelf->m_state = OMX_StateWaitForResources;
		                    	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);
			                }
			                else
			                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);

		                    break;

                        default:
		                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
		                	break;

		            }
                }
	        }
	        else if (cmd == StopPort)
	        {
	        	logd(" stop port command, cmddata = %d.", (int)cmddata);
                
	            //*Stop Port(s)
	            // cmddata contains the port index to be stopped.
                // It is assumed that 0 is input and 1 is output port for this component
                // The cmddata value -1 means that both input and output ports will be stopped.
	            if (cmddata == 0x0 || (OMX_S32)cmddata == -1)
	            {
	                //*Return all input buffers
                	pthread_mutex_lock(&pSelf->m_inBufMutex);

                    while (pSelf->m_sInBufList.nSizeOfList > 0)
                    {
                    	pSelf->m_sInBufList.nSizeOfList--;
                    	pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp,
                    			                           pSelf->m_pAppData,
                    			                           pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos++]);

                    	if (pSelf->m_sInBufList.nReadPos >= pSelf->m_sInBufList.nBufArrSize)
                    		pSelf->m_sInBufList.nReadPos = 0;
                    }

                    pSelf->m_sInBufList.nReadPos  = 0;
                    pSelf->m_sInBufList.nWritePos = 0;
                    
                	pthread_mutex_unlock(&pSelf->m_inBufMutex);

 		            //*Disable port
					pSelf->m_sInPortDef.bEnabled = OMX_FALSE;
		        }

	            if (cmddata == 0x1 || (OMX_S32)cmddata == -1)
	            {
		            //*Return all output buffers
                	pthread_mutex_lock(&pSelf->m_outBufMutex);

                    while (pSelf->m_sOutBufList.nSizeOfList > 0)
                    {
                    	pSelf->m_sOutBufList.nSizeOfList--;
                    	pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp,
                    			                           pSelf->m_pAppData,
                    			                           pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++]);

                    	if (pSelf->m_sOutBufList.nReadPos >= pSelf->m_sOutBufList.nBufArrSize)
                    		pSelf->m_sOutBufList.nReadPos = 0;
                    }

                    pSelf->m_sOutBufList.nReadPos  = 0;
                    pSelf->m_sOutBufList.nWritePos = 0;
                	pthread_mutex_unlock(&pSelf->m_outBufMutex);

       	            // Disable port
					pSelf->m_sOutPortDef.bEnabled = OMX_FALSE;
		        }

		        // Wait for all buffers to be freed
		        nTimeout = 0x0;
		        while (1)
		        {
		            if (cmddata == 0x0 && !pSelf->m_sInPortDef.bPopulated)
		            {
		                //*Return cmdcomplete event if input unpopulated
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x0, NULL);
			            break;
		            }

		            if (cmddata == 0x1 && !pSelf->m_sOutPortDef.bPopulated)
		            {
		                //*Return cmdcomplete event if output unpopulated
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x1, NULL);
			            break;
		            }

		            if ((OMX_S32)cmddata == -1 &&  !pSelf->m_sInPortDef.bPopulated && !pSelf->m_sOutPortDef.bPopulated)
		            {
            		    //*Return cmdcomplete event if inout & output unpopulated
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x0, NULL);
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x1, NULL);
			            break;
		            }

                    //* Here , we sleep 500 ms.                                         
                    //* In the duration , if not all buffers be freed, we also return OMX_EventCmdComplete
		            if (nTimeout++ > OMX_MAX_TIMEOUTS_STOP_PORT)
		            {
                        logw("timeOut when wait for all buffer to be freed in stopPort command! But we also return OMX_EventCmdComplete!");
                        if(cmddata == 0x0)
                        {
                            pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x0, NULL);
			                break;
                        }
                        else if(cmddata == 0x1)
                        {
                            pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x1, NULL);
			                break;
                        }
                        else if((OMX_S32)cmddata == -1)
                        {
                            pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x0, NULL);
    		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x1, NULL);
    			            break;
                        }
                        
		            	//pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorPortUnresponsiveDuringDeallocation, 0 , NULL);
       		            //break;
		            }

                    usleep(OMX_TIMEOUT*1000);
		        }
	        }
	        else if (cmd == RestartPort)
	        {
	        	logd(" restart port command. pSelf->m_state[%d] , cmddata=%d", pSelf->m_state, (int)cmddata);
                
                //*Restart Port(s)
                // cmddata contains the port index to be restarted.
                // It is assumed that 0 is input and 1 is output port for this component.
                // The cmddata value -1 means both input and output ports will be restarted.

                /*
                if (cmddata == 0x0 || (OMX_S32)cmddata == -1)
                	pSelf->m_sInPortDef.bEnabled = OMX_TRUE;

	            if (cmddata == 0x1 || (OMX_S32)cmddata == -1)
	            	pSelf->m_sOutPortDef.bEnabled = OMX_TRUE;
                */
                
 	            // Wait for port to be populated
		        nTimeout = 0x0;
		        while (1)
		        {
                    // Return cmdcomplete event if input port populated
		            if (cmddata == 0x0 && (pSelf->m_state == OMX_StateLoaded || pSelf->m_sInPortDef.bPopulated))
		            {
                        pSelf->m_sInPortDef.bEnabled = OMX_TRUE;
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortEnable, 0x0, NULL);
			            break;
		            }
                    // Return cmdcomplete event if output port populated
		            else if (cmddata == 0x1 && (pSelf->m_state == OMX_StateLoaded || pSelf->m_sOutPortDef.bPopulated))
		            {
                        pSelf->m_sOutPortDef.bEnabled = OMX_TRUE;
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortEnable, 0x1, NULL);
                        break;
		            }
                    // Return cmdcomplete event if input and output ports populated
		            else if ((OMX_S32)cmddata == -1 && (pSelf->m_state == OMX_StateLoaded || (pSelf->m_sInPortDef.bPopulated && pSelf->m_sOutPortDef.bPopulated)))
		            {
                        pSelf->m_sInPortDef.bEnabled = OMX_TRUE;
                        pSelf->m_sOutPortDef.bEnabled = OMX_TRUE;
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortEnable, 0x0, NULL);
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortEnable, 0x1, NULL);
			            break;
		            }
		            else if (nTimeout++ > OMX_MAX_TIMEOUTS)
		            {
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorPortUnresponsiveDuringAllocation, 0, NULL);
                        break;
	                }

                    usleep(OMX_TIMEOUT*1000);
		        }

		        if(port_setting_match == OMX_FALSE)
		        	port_setting_match = OMX_TRUE;
	        }
	        else if (cmd == Flush)
	        {
	        	logd(" flush command.");
                
	            //*Flush port(s)
                // cmddata contains the port index to be flushed.
                // It is assumed that 0 is input and 1 is output port for this component
                // The cmddata value -1 means that both input and output ports will be flushed.
                
                if(cmddata == OMX_ALL || cmddata == 0x1 || (OMX_S32)cmddata == -1)   //if request flush input and output port, we reset decoder!
                {
                    logd(" flush all port! we reset decoder!");
                    post_message_to_vdrv(pSelf, OMX_VdrvCommand_FlushVdecLib);
                    sem_post(&pSelf->m_sem_vbs_input);
                    sem_post(&pSelf->m_sem_frame_output);
                    logd("(f:%s, l:%d) wait for OMX_VdrvCommand_FlushVdecLib", __FUNCTION__, __LINE__);
                    sem_wait(&pSelf->m_vdrv_cmd_lock);
                    logd("(f:%s, l:%d) wait for OMX_VdrvCommand_FlushVdecLib done!", __FUNCTION__, __LINE__);
                }
	            if (cmddata == 0x0 || (OMX_S32)cmddata == -1)
	            {
	                // Return all input buffers and send cmdcomplete
                	pthread_mutex_lock(&pSelf->m_inBufMutex);

                    while (pSelf->m_sInBufList.nSizeOfList > 0)
                    {
                    	pSelf->m_sInBufList.nSizeOfList--;
                    	pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp,
                    			                           pSelf->m_pAppData,
                    			                           pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos++]);

                    	if (pSelf->m_sInBufList.nReadPos >= pSelf->m_sInBufList.nBufArrSize)
                    		pSelf->m_sInBufList.nReadPos = 0;
                    }

                    pSelf->m_sInBufList.nReadPos  = 0;
                    pSelf->m_sInBufList.nWritePos = 0;

                	pthread_mutex_unlock(&pSelf->m_inBufMutex);

					pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandFlush, 0x0, NULL);
		        }

	            if (cmddata == 0x1 || (OMX_S32)cmddata == -1)
	            {
	                // Return all output buffers and send cmdcomplete
                	pthread_mutex_lock(&pSelf->m_outBufMutex);

                    while (pSelf->m_sOutBufList.nSizeOfList > 0)
                    {
                    	pSelf->m_sOutBufList.nSizeOfList--;
                    	pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp,
                    			                           pSelf->m_pAppData,
                    			                           pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++]);

                    	if (pSelf->m_sOutBufList.nReadPos >= pSelf->m_sOutBufList.nBufArrSize)
                    		pSelf->m_sOutBufList.nReadPos = 0;
                    }

                    pSelf->m_sOutBufList.nReadPos  = 0;
                    pSelf->m_sOutBufList.nWritePos = 0;

                	pthread_mutex_unlock(&pSelf->m_outBufMutex);

					pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandFlush, 0x1, NULL);
		        }
	        }
	        else if (cmd == Stop)
	        {
	        	logd(" stop command. pSelf->m_state[%d]", pSelf->m_state);
                post_message_to_vdrv(pSelf, OMX_VdrvCommand_Stop);
                sem_post(&pSelf->m_sem_vbs_input);
                sem_post(&pSelf->m_sem_frame_output);
                logd("(f:%s, l:%d) wait for OMX_VdrvCommand_Stop", __FUNCTION__, __LINE__);
                sem_wait(&pSelf->m_vdrv_cmd_lock);
                logd("(f:%s, l:%d) wait for OMX_VdrvCommand_Stop done!", __FUNCTION__, __LINE__);
  		        //*Kill thread
 	            goto EXIT;
	        }
	        else if (cmd == FillBuf)
	        {
	            //*Fill buffer
	        	pthread_mutex_lock(&pSelf->m_outBufMutex);
	        	if (pSelf->m_sOutBufList.nSizeOfList < pSelf->m_sOutBufList.nAllocSize)
	        	{
	        		pSelf->m_sOutBufList.nSizeOfList++;
	        		pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nWritePos++] = ((OMX_BUFFERHEADERTYPE*) cmddata);

	                if (pSelf->m_sOutBufList.nWritePos >= (OMX_S32)pSelf->m_sOutBufList.nAllocSize)
	                	pSelf->m_sOutBufList.nWritePos = 0;
	        	}
	        	pthread_mutex_unlock(&pSelf->m_outBufMutex);
            }
	        else if (cmd == EmptyBuf)
	        {
                OMX_BUFFERHEADERTYPE* pTmpInBufHeader = (OMX_BUFFERHEADERTYPE*) cmddata;
                OMX_TICKS   nInterval;

                //*Empty buffer
	    	    pthread_mutex_lock(&pSelf->m_inBufMutex);
                if(0 == pTmpInBufHeader->nTimeStamp)
                {
                    logi("why cur vbs input nTimeStamp=0? prevPts=%lld", pSelf->m_nInportPrevTimeStamp);
                }
                else
                {
                    if(pSelf->m_nInportPrevTimeStamp)
                    {
                        //*compare pts to decide if jump
                        nInterval = pTmpInBufHeader->nTimeStamp - pSelf->m_nInportPrevTimeStamp;
                        if(nInterval < -AWOMX_PTS_JUMP_THRESH || nInterval > AWOMX_PTS_JUMP_THRESH)
                        {
                            logd("pts jump [%lld]us, prev[%lld],cur[%lld],need reset vdeclib", nInterval, pSelf->m_nInportPrevTimeStamp, pTmpInBufHeader->nTimeStamp);
                            pSelf->m_JumpFlag = OMX_TRUE;
                        }
                        
                        pSelf->m_nInportPrevTimeStamp = pTmpInBufHeader->nTimeStamp;
                    }
                    else //*first InBuf data
                    {
                        pSelf->m_nInportPrevTimeStamp = pTmpInBufHeader->nTimeStamp;
                    }
                }
				
                if (pSelf->m_sInBufList.nSizeOfList < pSelf->m_sInBufList.nAllocSize)
	        	{
	        		pSelf->m_sInBufList.nSizeOfList++;
	        		pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nWritePos++] = ((OMX_BUFFERHEADERTYPE*) cmddata);

	                if (pSelf->m_sInBufList.nWritePos >= (OMX_S32)pSelf->m_sInBufList.nAllocSize)
	                	pSelf->m_sInBufList.nWritePos = 0;
	        	}
                
                logi("(omx_vdec, f:%s, l:%d) nTimeStamp[%lld], nAllocLen[%d], nFilledLen[%d], nOffset[%d], nFlags[0x%x], nOutputPortIndex[%d], nInputPortIndex[%d]", __FUNCTION__, __LINE__, 
                    pTmpInBufHeader->nTimeStamp, 
                    (int)pTmpInBufHeader->nAllocLen, 
                    (int)pTmpInBufHeader->nFilledLen, 
                    (int)pTmpInBufHeader->nOffset, 
                    (int)pTmpInBufHeader->nFlags, 
                    (int)pTmpInBufHeader->nOutputPortIndex, 
                    (int)pTmpInBufHeader->nInputPortIndex);
                
	    	    pthread_mutex_unlock(&pSelf->m_inBufMutex);
                
        	    //*Mark current buffer if there is outstanding command
		        if (pMarkBuf)
		        {
		            ((OMX_BUFFERHEADERTYPE *)(cmddata))->hMarkTargetComponent = &pSelf->m_cmp;
		            ((OMX_BUFFERHEADERTYPE *)(cmddata))->pMarkData = pMarkBuf->pMarkData;
		            pMarkBuf = NULL;
		        }
		    }
	        else if (cmd == MarkBuf)
	        {
	            if (!pMarkBuf)
	                pMarkBuf = (OMX_MARKTYPE *)(cmddata);
	        }
            else if(cmd == VdrvNotifyEos)
            {
                nVdrvNotifyEosFlag = OMX_TRUE;
            }
            else if(cmd == VdrvResolutionChange)
            {
                pSelf->mResolutionChangeFlag = (OMX_BOOL)cmddata;
                logv("***pSelf->mResolutionChangeFlag = %d",(int)pSelf->mResolutionChangeFlag);
            }
            else
            {
                logw("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
            }
	    }

        //*Buffer processing
        // Only happens when the component is in executing state.
        if (pSelf->m_state == OMX_StateExecuting  &&
        	pSelf->m_sInPortDef.bEnabled          &&
        	pSelf->m_sOutPortDef.bEnabled         &&
        	port_setting_match                    &&
        	(pSelf->mResolutionChangeFlag == OMX_FALSE))
        {
            if(OMX_TRUE == pSelf->m_JumpFlag)
            {
                logd("reset vdeclib for jump!");
                //pSelf->m_decoder->ioctrl(pSelf->m_decoder, CEDARV_COMMAND_JUMP, 0);
                pSelf->m_JumpFlag = OMX_FALSE;
            }

            //*if the first-input-data is configure-data, we should cpy to decoder
            // as init-data. here we send commad to VdrvThread to create decoder
            if(pSelf->mFirstInputDataFlag==OMX_TRUE && pSelf->m_sInBufList.nSizeOfList>0)
            {
                pInBufHdr = pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos];

                if(pInBufHdr == NULL)
                {
                    logd("(f:%s, l:%d) fatal error! pInBufHdr is NULL, check code!", __FUNCTION__, __LINE__);
                	break;
                }
                
                pSelf->mFirstInputDataFlag = OMX_FALSE;
                logv("*** the first pInBufHdr->nFlags = 0x%x, pInBufHdr->nFilledLen=%lu",(int)pInBufHdr->nFlags, pInBufHdr->nFilledLen);
                if (pInBufHdr->nFlags & OMX_BUFFERFLAG_CODECCONFIG) //*test 1 extradata
                {
                    if(pSelf->m_streamInfo.pCodecSpecificData)
                        free(pSelf->m_streamInfo.pCodecSpecificData);
                    pSelf->m_streamInfo.nCodecSpecificDataLen = pInBufHdr->nFilledLen;
                    pSelf->m_streamInfo.pCodecSpecificData    = (char*)malloc(pSelf->m_streamInfo.nCodecSpecificDataLen);
                    memset(pSelf->m_streamInfo.pCodecSpecificData,0,pSelf->m_streamInfo.nCodecSpecificDataLen);
                    memcpy(pSelf->m_streamInfo.pCodecSpecificData,pInBufHdr->pBuffer,pSelf->m_streamInfo.nCodecSpecificDataLen);

                    post_message_to_vdrv(pSelf, OMX_VdrvCommand_PrepareVdecLib);
                    logd("(f:%s, l:%d) wait for OMX_VdrvCommand_PrepareVdecLib", __FUNCTION__, __LINE__);
                    sem_wait(&pSelf->m_vdrv_cmd_lock);
                    logd("(f:%s, l:%d) wait for OMX_VdrvCommand_PrepareVdecLib done!", __FUNCTION__, __LINE__);

                    pSelf->m_sInBufList.nSizeOfList--;
                    pSelf->m_sInBufList.nReadPos++;
                	if (pSelf->m_sInBufList.nReadPos >= (OMX_S32)pSelf->m_sInBufList.nAllocSize)
                		pSelf->m_sInBufList.nReadPos = 0;
                    
                    pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp, pSelf->m_pAppData, pInBufHdr);
                }
                else
                {
                    post_message_to_vdrv(pSelf, OMX_VdrvCommand_PrepareVdecLib);
                    logd("(f:%s, l:%d) wait for OMX_VdrvCommand_PrepareVdecLib", __FUNCTION__, __LINE__);
                    sem_wait(&pSelf->m_vdrv_cmd_lock);
                    logd("(f:%s, l:%d) wait for OMX_VdrvCommand_PrepareVdecLib done!", __FUNCTION__, __LINE__);

                }
            }
        
            if(pSelf->m_decoder!=NULL)
            {
                //*fill buffer first
       		    while(0==ValidPictureNum(pSelf->m_decoder,0) && pSelf->m_sInBufList.nSizeOfList>0)
                {
                	char* pBuf0;
                	char* pBuf1;
                	int   size0;
                	int   size1;
                	int   require_size;
                	unsigned char*   pData;
                    VideoStreamDataInfo DataInfo;

                    memset(&DataInfo, 0, sizeof(VideoStreamDataInfo));
        			pInBufHdr = pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos];

                    if(pInBufHdr == NULL)
                    {
                        logd("(f:%s, l:%d) fatal error! pInBufHdr is NULL, check code!", __FUNCTION__, __LINE__);
                    	break;
                    }

                    //*add stream to decoder.
                    require_size = pInBufHdr->nFilledLen;
                    if(RequestVideoStreamBuffer(pSelf->m_decoder, require_size, &pBuf0, &size0, &pBuf1, &size1,0) != 0)
                    {
                        logi("(f:%s, l:%d)req vbs fail! maybe vbs buffer is full! require_size[%d]", __FUNCTION__, __LINE__, require_size);
        				//*report a fatal error.
                        //pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorHardware, 0, NULL);
                        break;
                    }
                    
                    DataInfo.nLength      = require_size;
                    DataInfo.bIsFirstPart = 1;
                    DataInfo.bIsLastPart  = 1;
                    DataInfo.pData        = pBuf0;
                    if(pInBufHdr->nTimeStamp >= 0)
                    {
                        DataInfo.nPts   = pInBufHdr->nTimeStamp;
                        DataInfo.bValid = 1;
                    }
                    else
                    {
                        DataInfo.nPts   = -1;
                        DataInfo.bValid = 0;
                    }
                    
                    if(require_size <= size0)
                    {
                    	pData = pInBufHdr->pBuffer + pInBufHdr->nOffset;
                    	memcpy(pBuf0, pData, require_size);
                    }
                    else
                    {
                    	pData = pInBufHdr->pBuffer + pInBufHdr->nOffset;
                    	memcpy(pBuf0, pData, size0);
                    	pData += size0;
                    	memcpy(pBuf1, pData, require_size - size0);
                    }
                    SubmitVideoStreamData(pSelf->m_decoder, &DataInfo,0);

                    //*Check for EOS flag
                    if (pInBufHdr)
                    {
                        if (pInBufHdr->nFlags & OMX_BUFFERFLAG_EOS)
                        {
        	                //*Copy flag to output buffer header
                        	nInBufEos = OMX_TRUE;
                            usleep(5*1000);
                            post_message_to_vdrv(pSelf, OMX_VdrvCommand_EndOfStream);
                            sem_post(&pSelf->m_sem_vbs_input);
                            sem_post(&pSelf->m_sem_frame_output);
                            logd("(f:%s, l:%d) wait for OMX_VdrvCommand_EndOfStream", __FUNCTION__, __LINE__);
                            sem_wait(&pSelf->m_vdrv_cmd_lock);
                            logd("(f:%s, l:%d) wait for OMX_VdrvCommand_EndOfStream done!", __FUNCTION__, __LINE__);

            	        	logd(" set up nInBufEos flag.");

         	                //*Trigger event handler
                            pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventBufferFlag, 0x1, pInBufHdr->nFlags, NULL);

         		            //*Clear flag
        		            pInBufHdr->nFlags = 0;
        	            }

        	            //*Check for mark buffers
        	            if (pInBufHdr->pMarkData)
        	            {
                            //*Copy mark to output buffer header
        	                if (pOutBufHdr)
        	                {
        	                    pMarkData = pInBufHdr->pMarkData;
                                //*Copy handle to output buffer header
                                hMarkTargetComponent = pInBufHdr->hMarkTargetComponent;
                            }
         		        }

        		        //*Trigger event handler
        	            if (pInBufHdr->hMarkTargetComponent == &pSelf->m_cmp && pInBufHdr->pMarkData)
        	            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventMark, 0, 0, pInBufHdr->pMarkData);
                    }
                    
                    pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp, pSelf->m_pAppData, pInBufHdr);

                    //*notify vdrvThread vbs input
                    nRetSemGetValue=sem_getvalue(&pSelf->m_sem_vbs_input, &nSemVal);
                    if(0 == nRetSemGetValue)
                    {
                        if(0 == nSemVal)
                        {
                            sem_post(&pSelf->m_sem_vbs_input);
                        }
                        else
                        {
                            logi("(f:%s, l:%d) Be careful, sema vbs_input[%d]!=0", __FUNCTION__, __LINE__, nSemVal);
                        }
                    }
                    else
                    {
                        logw("(f:%s, l:%d) fatal error, why sem getvalue of m_sem_vbs_input[%d] fail!", __FUNCTION__, __LINE__, nRetSemGetValue);
                    }                      	        

                    //* check if there is a input bit stream.
                    pthread_mutex_lock(&pSelf->m_inBufMutex);
    				
                	pSelf->m_sInBufList.nSizeOfList--;
                    pSelf->m_sInBufList.nReadPos++;
                	if (pSelf->m_sInBufList.nReadPos >= (OMX_S32)pSelf->m_sInBufList.nAllocSize)
                	{
                		pSelf->m_sInBufList.nReadPos = 0;
                	}

    				/*for cts, using for synchronization between inputbuffer and outputbuffer*/
    				pSelf->m_InputNum ++;
    				if ((pSelf->mIsFromCts == true)/* && ((pSelf->m_InputNum - pSelf->m_OutputNum) > 3)*/) 
                    {
    					usleep(20000);
    				}
    				else
    				{
    					usleep(10000); 
    				}
                    pthread_mutex_unlock(&pSelf->m_inBufMutex);

        		}
            
        	    //*check if there is a picture to output.
        	    if(ValidPictureNum(pSelf->m_decoder,0))
            	{
                    int width_align  = 0;
                    int height_align = 0;
                    
                    //* check if the picture size changed.
                    picture = NextPictureInfo(pSelf->m_decoder,0);

                    //height_align = (picture.display_height + 7) & (~7);
                    //width_align  = (picture.display_width + 15) & (~15);

                    //* we use offset to compute width and height
                    width_align  = picture->nRightOffset - picture->nLeftOffset;
                    height_align = picture->nBottomOffset - picture->nTopOffset;

                    //* if the offset is not right, we should not use them to compute width and height
                    if(width_align <= 0 || height_align <= 0)
                    {
                        width_align  = picture->nWidth;
                        height_align = picture->nHeight;
                    }
                    
                    //* make the height to 2 align
                    height_align = (height_align + 1) & ~1;
                    
                	if((OMX_U32)width_align != pSelf->m_sOutPortDef.format.video.nFrameWidth || (OMX_U32)height_align != pSelf->m_sOutPortDef.format.video.nFrameHeight)
                	{
                		logw(" port setting changed., org_height = %d, org_width = %d, new_height = %d, new_width = %d.",
                				(int)pSelf->m_sOutPortDef.format.video.nFrameHeight,
                				(int)pSelf->m_sOutPortDef.format.video.nFrameWidth,
                				(int)height_align, (int)width_align);

                		pSelf->m_sOutPortDef.format.video.nFrameHeight = height_align;
                		pSelf->m_sOutPortDef.format.video.nFrameWidth = width_align;
                        pSelf->m_sOutPortDef.nBufferSize = height_align*width_align *3/2;
                		port_setting_match = OMX_FALSE;
    					pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventPortSettingsChanged, 0x01/*pSelf->m_sOutPortDef.nPortIndex*/, 0, NULL);
    					continue;
                	}


                    pthread_mutex_lock(&pSelf->m_outBufMutex);

                    if(pSelf->m_sOutBufList.nSizeOfList > 0)
                    {
                    	pSelf->m_sOutBufList.nSizeOfList--;
                    	pOutBufHdr = pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++];
                    	if (pSelf->m_sOutBufList.nReadPos >= (OMX_S32)pSelf->m_sOutBufList.nAllocSize)
                    		pSelf->m_sOutBufList.nReadPos = 0;
                    }
                    else
                    	pOutBufHdr = NULL;

                    pthread_mutex_unlock(&pSelf->m_outBufMutex);

                    //* if no output buffer, wait for some time.
                	if(pOutBufHdr == NULL)
                	{
                        if(0 == waitPipeDataToRead(pSelf->m_cmdpipe[0], 40*1000))
                        {
                            //logv("(f:%s, l:%d) no output frame, wait 40ms for msg, interval=[%lld]ms", __FUNCTION__, __LINE__, (tm2-tm1)/1000);
                        }
                        else
                        {
                            //logv("(f:%s, l:%d) no output frame, wait <40ms for msg coming, interval=[%lld]ms", __FUNCTION__, __LINE__, (tm3-tm1)/1000);
                        }
                        continue;
                    }

                	picture = RequestPicture(pSelf->m_decoder, 0);
                    //convertAddress_Phy2Vir(picture);

                	//*transform picture color format.
                	
                	logv("(omx_vdec, f:%s, l:%d) m_useAndroidBuffer = %d", __FUNCTION__, __LINE__,(int)pSelf->m_useAndroidBuffer);
                	if(pSelf->m_useAndroidBuffer == OMX_FALSE)
                	{
                		
#if OPEN_LINUX_STATISTICS
                        int64_t start, end;                            
                        start = OMX_GetNowUs();
#endif
                        MemAdapterFlushCache(picture->pData0, picture->nWidth* picture->nHeight* 2);

                        TransformToYUVPlaner(picture, pOutBufHdr->pBuffer);
                        
#if OPEN_LINUX_STATISTICS
                        end = OMX_GetNowUs();
                        pSelf->mConvertTotalDuration += (end-start);
                        pSelf->mConvertTotalCount++;
                        logd("xxxxxxxxxxx cur TransformYV12ToYUV420 time:%.3f, avg time:%.3f", (float)(end-start)/1000.0, (float)pSelf->mConvertTotalDuration/pSelf->mConvertTotalCount/1000);
#endif
                	}

                	pOutBufHdr->nTimeStamp = picture->nPts;
                	pOutBufHdr->nOffset    = 0;
                    
#if( CONFIG_OS == OPTION_OS_ANDROID && CONFIG_OS_VERSION>=OPTION_OS_VERSION_ANDROID_4_4)              
                    if(pSelf->m_storeOutputMetaDataFlag==OMX_TRUE)
                        pOutBufHdr->nFilledLen = sizeof(VideoDecoderOutputMetaData);
                    else
                        pOutBufHdr->nFilledLen = pSelf->m_sOutPortDef.format.video.nFrameWidth * pSelf->m_sOutPortDef.format.video.nFrameHeight * 3 / 2;
#else
                    pOutBufHdr->nFilledLen = pSelf->m_sOutPortDef.format.video.nFrameWidth * pSelf->m_sOutPortDef.format.video.nFrameHeight * 3 / 2;
#endif
                    ReturnPicture(pSelf->m_decoder, picture);

                    //*notify vdrvThread free frame
                    nRetSemGetValue=sem_getvalue(&pSelf->m_sem_frame_output, &nSemVal);
                    if(0 == nRetSemGetValue)
                    {
                        if(0 == nSemVal)
                        {
                            sem_post(&pSelf->m_sem_frame_output);
                        }
                        else
                        {
                            logv("(f:%s, l:%d) Be careful, sema frame_output[%d]!=0", __FUNCTION__, __LINE__, nSemVal);
                        }
                    }
                    else
                    {
                        logw("(f:%s, l:%d) fatal error, why sem getvalue of m_sem_frame_output[%d] fail!", __FUNCTION__, __LINE__, nRetSemGetValue);
                    }

    	            //*Check for mark buffers
    	            if (pMarkData != NULL && hMarkTargetComponent != NULL)
    	            {
                    	if(!ValidPictureNum(pSelf->m_decoder,0))
                    	{
                    		//*Copy mark to output buffer header
                    		pOutBufHdr->pMarkData = pInBufHdr->pMarkData;
                    		//*Copy handle to output buffer header
                    		pOutBufHdr->hMarkTargetComponent = pInBufHdr->hMarkTargetComponent;

                    		pMarkData = NULL;
                    		hMarkTargetComponent = NULL;
                    	}
     		        }

                    logi("****FillBufferDone is called, nSizeOfList[%d], pts[%lld], nAllocLen[%d], nFilledLen[%d], nOffset[%d], nFlags[0x%x], nOutputPortIndex[%d], nInputPortIndex[%d]",
                        (int)pSelf->m_sOutBufList.nSizeOfList, 
                             pOutBufHdr->nTimeStamp, 
                        (int)pOutBufHdr->nAllocLen, 
                        (int)pOutBufHdr->nFilledLen, 
                        (int)pOutBufHdr->nOffset, 
                        (int)pOutBufHdr->nFlags, 
                        (int)pOutBufHdr->nOutputPortIndex, 
                        (int)pOutBufHdr->nInputPortIndex);

    				pSelf->m_OutputNum ++;
                    pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp, pSelf->m_pAppData, pOutBufHdr);
                    pOutBufHdr = NULL;

                    continue;
            	}
                else
                {
                	logi("LINE %d, nInBufEos %d, nVdrvNotifyEosFlag %d", __LINE__,  nInBufEos, nVdrvNotifyEosFlag);
                	if (/*nInBufEos || */nVdrvNotifyEosFlag) {
                        
                        //*set eos flag, MediaCodec use this flag
                    	// to determine whether a playback is finished.
                    	pthread_mutex_lock(&pSelf->m_outBufMutex);

                    	while (pSelf->m_sOutBufList.nSizeOfList > 0) {
                    		pSelf->m_sOutBufList.nSizeOfList--;
                    		pOutBufHdr = pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++];
                    		if (pSelf->m_sOutBufList.nReadPos >= (OMX_S32)pSelf->m_sOutBufList.nAllocSize) {
                    			pSelf->m_sOutBufList.nReadPos = 0;
                    		}

                            if(pOutBufHdr==NULL)
                            {
                                continue;
                            }
                            
                    		if (pOutBufHdr->nFilledLen != 0) {
                        		pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp, pSelf->m_pAppData, pOutBufHdr);
                        		pOutBufHdr = NULL;
                    		}
                    		else {
                    			pOutBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
                    			pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp, pSelf->m_pAppData, pOutBufHdr);
                        		pOutBufHdr = NULL;
                        		nInBufEos = OMX_FALSE;
                        		nVdrvNotifyEosFlag = OMX_FALSE;
                        		break;
                    		}
                    	}
                    	pthread_mutex_unlock(&pSelf->m_outBufMutex);
                    }
                        
                    if(0 == waitPipeDataToRead(pSelf->m_cmdpipe[0], 20*1000))
                    {
                        //logv("(f:%s, l:%d) no vdeclib frame, wait 20ms for msg, interval=[%lld]ms", __FUNCTION__, __LINE__, (tm2-tm1)/1000);
                    }
                    else
                    {
                        //logv("(f:%s, l:%d) no vdeclib frame, wait <20ms for msg coming, interval=[%lld]ms", __FUNCTION__, __LINE__, (tm3-tm1)/1000);
                    }
                }
            }
        }
    }

EXIT:
    return (void*)OMX_ErrorNone;
}

static void* ComponentVdrvThread(void* pThreadData)
{
	int                     decodeResult;

    int                     i;
    int                     fd1;
    fd_set                  rfds;
    OMX_VDRV_COMMANDTYPE    cmd;
    OMX_BOOL                nEosFlag = OMX_FALSE;
    struct timeval          timeout;
    int64_t                 nTimeUs1;
    int64_t                 nTimeUs2;

    int nSemVal;
    int nRetSemGetValue;
    int nStopFlag = 0;
    int nVdrvResolutionChangeFlag = 0;
    
    // Recover the pointer to my component specific data
    omx_vdec* pSelf = (omx_vdec*)pThreadData;


    while (1)
    {
        fd1 = pSelf->m_vdrv_cmdpipe[0];
        FD_ZERO(&rfds);
        FD_SET(fd1, &rfds);
        //*Check for new command
        timeout.tv_sec  = 0;
        timeout.tv_usec = 0;

        i = select(pSelf->m_vdrv_cmdpipe[0]+1, &rfds, NULL, NULL, &timeout);

        if (FD_ISSET(pSelf->m_vdrv_cmdpipe[0], &rfds))
        {
            // retrieve command and data from pipe
            ssize_t readRet = read(pSelf->m_vdrv_cmdpipe[0], &cmd, sizeof(cmd));
            if(readRet<0)
            {
                logd("error: read pipe failed!,ret = %d",(int)readRet);
                goto EXIT;
            }
            logd("(f:%s, l:%d) vdrvThread receive cmd[0x%x]", __FUNCTION__, __LINE__, cmd);

            // State transition command
	        if (cmd == OMX_VdrvCommand_PrepareVdecLib)  //now omx_vdec's m_state = OMX_StateLoaded, OMX_StateLoaded->OMX_StateIdle
	        {
    	        	logd("(f:%s, l:%d)(OMX_VdrvCommand_PrepareVdecLib)", __FUNCTION__, __LINE__);

                    // If the parameter states a transition to the same state
                    // raise a same state transition error.
                    if (pSelf->m_state != OMX_StateExecuting)
                    {
                    	logw("fatal error! when vdrv OMX_VdrvCommand_PrepareVdecLib, m_state[0x%x] should be OMX_StateExecuting", pSelf->m_state);
                    }

                    //*if mdecoder had closed before, we should create it 
    				if(pSelf->m_decoder==NULL)
                        pSelf->m_decoder = CreateVideoDecoder();
                    
                	//* set video stream info.
                	VConfig m_videoConfig;
                    memset(&m_videoConfig,0,sizeof(VConfig));
                    
                    //*set yv12
                    m_videoConfig.eOutputPixelFormat = PIXEL_FORMAT_YUV_MB32_420; //* test MB32
                    //m_videoConfig.eOutputPixelFormat = PIXEL_FORMAT_DEFAULT;
#if (CONFIG_PRODUCT == OPTION_PRODUCT_TVBOX)					
					// omx decoder make out buffer no more than 1920x1080
					if (pSelf->m_streamInfo.nWidth > 1920
						&& pSelf->m_streamInfo.nHeight > 1088)
					{
						m_videoConfig.bScaleDownEn = 1;
						m_videoConfig.nHorizonScaleDownRatio = 1;
						m_videoConfig.nVerticalScaleDownRatio = 1;
					}
#endif
                	if(InitializeVideoDecoder(pSelf->m_decoder, &(pSelf->m_streamInfo),&m_videoConfig) != 0)
                	{
                    	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorHardware, 0 , NULL);
    	                logw("Idle transition failed, set_vstream_info() return fail.\n");
                        goto __EXECUTE_CMD_DONE;
                	}
                
	        }
	        else if (cmd == OMX_VdrvCommand_CloseVdecLib)// now omx_vdec's m_state = OMX_StateLoaded, OMX_StateIdle->OMX_StateLoaded
	        {
                logd("(f:%s, l:%d)(OMX_VdrvCommand_CloseVdecLib)", __FUNCTION__, __LINE__);

                //* stop and close decoder.
                if(pSelf->m_decoder != NULL)
                {
                	DestroyVideoDecoder(pSelf->m_decoder);
                    pSelf->m_decoder           = NULL;
                    pSelf->mFirstInputDataFlag = OMX_TRUE;
                }
	        }
            
	        else if (cmd == OMX_VdrvCommand_FlushVdecLib)
	        {
                logd("(f:%s, l:%d)(OMX_VdrvCommand_FlushVdecLib)", __FUNCTION__, __LINE__);
                if(pSelf->m_decoder)
                {
                    ResetVideoDecoder(pSelf->m_decoder);
                }
                else
                {
                    logw(" fatal error, m_decoder is not malloc when flush all ports!");
                }
	        }
	        else if (cmd == OMX_VdrvCommand_Stop)
	        {
                logd("(f:%s, l:%d)(OMX_VdrvCommand_Stop)", __FUNCTION__, __LINE__);
                nStopFlag = 1;
	        }
	        else if (cmd == OMX_VdrvCommand_EndOfStream)
	        {
                logd("(f:%s, l:%d)(OMX_VdrvCommand_EndOfStream)", __FUNCTION__, __LINE__);
                nEosFlag = OMX_TRUE;
	        }
	        else
	        {
                logw("(f:%s, l:%d)fatal error! unknown OMX_VDRV_COMMANDTYPE[0x%x]", __FUNCTION__, __LINE__, cmd);
	        }
            
__EXECUTE_CMD_DONE:
    
            nRetSemGetValue=sem_getvalue(&pSelf->m_vdrv_cmd_lock, &nSemVal);
            if(0 == nRetSemGetValue)
            {
                if(0 == nSemVal)
                {
                    sem_post(&pSelf->m_vdrv_cmd_lock);
                }
                else
                {
                    logw("(f:%s, l:%d) fatal error, why nSemVal[%d]!=0", __FUNCTION__, __LINE__, nSemVal);
                }
            }
            else
            {
                logw("(f:%s, l:%d) fatal error, why sem getvalue[%d] fail!", __FUNCTION__, __LINE__, nRetSemGetValue);
            }
	    }
        if(nStopFlag)
        {
            logd("vdrvThread detect nStopFlag[%d], exit!", nStopFlag);
            goto EXIT;
		}
        
        //*Buffer processing
        // Only happens when the component is in executing state.
        if ((pSelf->m_state == OMX_StateExecuting || pSelf->m_state == OMX_StatePause)
            &&(pSelf->m_decoder!=NULL)
            &&(nVdrvResolutionChangeFlag == 0))
        {
          #if (OPEN_STATISTICS)
			nTimeUs1 = OMX_GetNowUs();
          #endif
          
			decodeResult = DecodeVideoStream(pSelf->m_decoder,0,0,0,0);
            logi("***decodeResult = %d",decodeResult);
            
          #if (OPEN_STATISTICS)
            nTimeUs2 = OMX_GetNowUs();
          
            if(decodeResult == CEDARV_RESULT_FRAME_DECODED || decodeResult == CEDARV_RESULT_KEYFRAME_DECODED)
            {
                pSelf->mDecodeFrameTotalDuration += (nTimeUs2-nTimeUs1);
                pSelf->mDecodeFrameTotalCount++;
            }
            else if(decodeResult == CEDARV_RESULT_OK)
            {
                pSelf->mDecodeOKTotalDuration += (nTimeUs2-nTimeUs1);
                pSelf->mDecodeOKTotalCount++;
            }
            else if(decodeResult == CEDARV_RESULT_NO_FRAME_BUFFER)
            {
                pSelf->mDecodeNoFrameTotalDuration += (nTimeUs2-nTimeUs1);
                pSelf->mDecodeNoFrameTotalCount++;
            }
            else if(decodeResult == CEDARV_RESULT_NO_BITSTREAM)
            {
                pSelf->mDecodeNoBitstreamTotalDuration += (nTimeUs2-nTimeUs1);
                pSelf->mDecodeNoBitstreamTotalCount++;
            }
            else
            {
                pSelf->mDecodeOtherTotalDuration += (nTimeUs2-nTimeUs1);
                pSelf->mDecodeOtherTotalCount++;
            }
          #endif

			if(decodeResult == VDECODE_RESULT_KEYFRAME_DECODED ||
			   decodeResult == VDECODE_RESULT_FRAME_DECODED ||
			   decodeResult == VDECODE_RESULT_OK)
			{
			    //*do nothing
			}
            else if(decodeResult == VDECODE_RESULT_NO_FRAME_BUFFER)
            {
                logi("(f:%s, l:%d) no_frame_buffer, wait!", __FUNCTION__, __LINE__);
                sem_wait(&pSelf->m_sem_frame_output);
                logi("(f:%s, l:%d) no_frame_buffer, wait_done!", __FUNCTION__, __LINE__);
            }
			else if(decodeResult == VDECODE_RESULT_NO_BITSTREAM)
			{
                if(nEosFlag)
                {
                    nEosFlag = OMX_FALSE;

                    //*set eos to decoder ,decoder should flush all fbm
                    DecodeVideoStream(pSelf->m_decoder,1,0,0,0);
                    
                	//*set eof flag, MediaCodec use this flag
                	// to determine whether a playback is finished.
                	
                	logi("(f:%s, l:%d) when eos, vdrvtask meet no_bitstream, all frames have decoded, notify ComponentThread eos!", __FUNCTION__, __LINE__);
                    pSelf->send_vdrv_feedback_msg(OMX_VdrvFeedbackMsg_NotifyEos, 0, NULL);
                    logi("(f:%s, l:%d) no_vbsinput_buffer, eos wait!", __FUNCTION__, __LINE__);
                    sem_wait(&pSelf->m_sem_vbs_input);
                    logi("(f:%s, l:%d) no_vbsinput_buffer, eos wait_done!", __FUNCTION__, __LINE__);
                }
                else
                {
                    logi("(f:%s, l:%d) no_vbsinput_buffer, wait!", __FUNCTION__, __LINE__);
                    sem_wait(&pSelf->m_sem_vbs_input);
                    logi("(f:%s, l:%d) no_vbsinput_buffer, wait_done!", __FUNCTION__, __LINE__);
                }
			}
            else if(decodeResult == VDECODE_RESULT_RESOLUTION_CHANGE)
            {
                nVdrvResolutionChangeFlag = 1;
                pSelf->send_vdrv_feedback_msg(OMX_VdrvFeedbackMsg_ResolutionChange, 1, NULL);
            }
			else if(decodeResult < 0)
			{
                logw("decode fatal error[%d]", decodeResult);
				//* report a fatal error.
                pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorHardware, 0, NULL);
                //break;
			}
            else
            {
                logd("decode ret[%d], ignore? why?", decodeResult);
            }
        }
        else
        {
            if(pSelf->mResolutionChangeFlag==OMX_TRUE)
            {
                logv("***ReopenVideoEngine!");
                ReopenVideoEngine(pSelf->m_decoder);
                pSelf->mResolutionChangeFlag = OMX_FALSE;
                nVdrvResolutionChangeFlag = 0;
            }
            
            if(0 == waitPipeDataToRead(pSelf->m_vdrv_cmdpipe[0], 20*1000))
            {
                //logv("(f:%s, l:%d) vdrvThread m_state[0x%x] is not OMX_StateExecuting or OMX_StatePause, wait 20ms for msg, interval=[%lld]ms", __FUNCTION__, __LINE__, pSelf->m_state, (tm2-tm1)/1000);
            }
            else
            {
                //logv("(f:%s, l:%d) vdrvThread m_state[0x%x] is not OMX_StateExecuting or OMX_StatePause, wait <20ms for msg coming, interval=[%lld]ms", __FUNCTION__, __LINE__, pSelf->m_state, (tm3-tm1)/1000);
            }
        }
    }

EXIT:
    logd("***notify: exit the componentVdrvThread!");
    return (void*)OMX_ErrorNone;
}

static void CedarOmxPluginInit(void) __attribute__((constructor));
void CedarOmxPluginInit(void)
{
    logw("CedarOmxPluginInit");
}

