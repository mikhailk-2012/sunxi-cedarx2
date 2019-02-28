/*
 * Copyright (C) 2008-2015 Allwinner Technology Co. Ltd. 
 * Author: Ning Fang <fangning@allwinnertech.com>
 *         Caoyuan Yang <yangcaoyuan@allwinnertech.com>
 * 
 * This software is confidential and proprietary and may be used
 * only as expressly authorized by a licensing agreement from 
 * Softwinner Products. 
 *
 * The entire notice above must be reproduced on all copies 
 * and should not be removed. 
 */
 
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "vencoder.h"
#include "FrameBufferManager.h"
#include "venc_device.h"
#include "EncAdapter.h"
#include "libcedar_plugin_venc.h"

#define FRAME_BUFFER_NUM 4

typedef struct VencContext
{
   VENC_DEVICE*         pVEncDevice;
   void *               pEncoderHandle;
   FrameBufferManager*  pFBM;
   VencBaseConfig    	baseConfig;
   unsigned int		    nFrameBufferNum;
   VencHeaderData		headerData;
   VencInputBuffer      curEncInputbuffer;
   VENC_CODEC_TYPE 		codecType;
   unsigned int         ICVersion;
   int                  bInit;
}VencContext;

VideoEncoder* VideoEncCreate(VENC_CODEC_TYPE eCodecType)
{
	VencContext* venc_ctx = NULL;

    if(EncAdapterInitialize() != 0)
    {
        loge("can not set up video engine runtime environment.");
        return NULL;
    }
	venc_ctx = (VencContext*)malloc(sizeof(VencContext));
	if(!venc_ctx){
		loge("malloc VencContext fail!");
		return NULL;
	}

	memset(venc_ctx, 0,sizeof(VencContext));

	venc_ctx->nFrameBufferNum = FRAME_BUFFER_NUM;
	venc_ctx->codecType = eCodecType;
	venc_ctx->ICVersion = EncAdapterGetICVersion();
	venc_ctx->bInit = 0;
	
	venc_ctx->pVEncDevice = VencoderDeviceCreate(eCodecType);
	if(venc_ctx->pVEncDevice == NULL)
	{
		free(venc_ctx);
		return NULL;
	}

	venc_ctx->pEncoderHandle = venc_ctx->pVEncDevice->open();

	if(!venc_ctx->pEncoderHandle)
	{
		VencoderDeviceDestroy(venc_ctx->pVEncDevice);
		venc_ctx->pVEncDevice = NULL;
		free(venc_ctx);
		return NULL;
	}

	return (VideoEncoder*)venc_ctx;	
}

void VideoEncDestroy(VideoEncoder* pEncoder)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;

	VideoEncUnInit(pEncoder);

	if(venc_ctx->pVEncDevice)
	{
		venc_ctx->pVEncDevice->close(venc_ctx->pEncoderHandle);
		VencoderDeviceDestroy(venc_ctx->pVEncDevice);
		venc_ctx->pVEncDevice = NULL;
		venc_ctx->pEncoderHandle = NULL;
	}

	EncAdpaterRelease();

	if(venc_ctx)
	{
		free(venc_ctx);
	}
}

int VideoEncInit(VideoEncoder* pEncoder, VencBaseConfig* pConfig)
{
	int result = 0;
	VencContext* venc_ctx = (VencContext*)pEncoder;
	
	if(pEncoder == NULL || pConfig == NULL || venc_ctx->bInit)
	{
		loge("InitVideoEncoder, param is NULL");
		return -1;
	}
	venc_ctx->pFBM = FrameBufferManagerCreate(venc_ctx->nFrameBufferNum);

	if(venc_ctx->pFBM == NULL)
	{

		loge("venc_ctx->pFBM == NULL");
		return -1;
	}

	logd("(f:%s, l:%d)", __FUNCTION__, __LINE__);

	if(venc_ctx->ICVersion == 0x1639)
	{
		if(pConfig->nDstWidth >= 3840 || pConfig->nDstHeight>= 2160)
		{
			VeInitEncoderPerformance(1);

		}
		else
		{
			VeInitEncoderPerformance(0);
			logd("VeInitEncoderPerformance");
		}
    } 
    else if (venc_ctx->ICVersion == 0x1625 && venc_ctx->codecType == VENC_CODEC_H264) {
        H264Ctx  * h264ctx = (H264Ctx *) venc_ctx->pEncoderHandle;
        h264ctx->rMePara.deblk_to_dram = 1;
    }

	logd("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	loge("ICVersion:0x%x",  venc_ctx->ICVersion);


	memcpy(&venc_ctx->baseConfig, pConfig, sizeof(VencBaseConfig));

	EncAdapterLockVideoEngine();
	result = venc_ctx->pVEncDevice->init(venc_ctx->pEncoderHandle, &venc_ctx->baseConfig);
	EncAdapterUnLockVideoEngine();

	venc_ctx->bInit = 1;
	return result;
}

int VideoEncUnInit(VideoEncoder* pEncoder)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;

	if(!venc_ctx->bInit)
	{
		return -1;
	}

 	venc_ctx->pVEncDevice->uninit(venc_ctx->pEncoderHandle);

	if(venc_ctx->ICVersion == 0x1639)
	{
		if(venc_ctx->baseConfig.nDstWidth >= 3840 || venc_ctx->baseConfig.nDstHeight >= 2160)
		{
			VeUninitEncoderPerformance(1);

		}
		else
		{
			VeUninitEncoderPerformance(0);
			logd("VeUninitEncoderPerformance");
		}
	}

	if(venc_ctx->pFBM)
	{
		FrameBufferManagerDestroy(venc_ctx->pFBM);
		venc_ctx->pFBM = NULL;
	}

	venc_ctx->bInit = 0;
	return 0;
}

int VideoEncReInit(VideoEncoder* pEncoder)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;
	int result = 0;

	if(!venc_ctx->bInit)
	{
		return -1;
	}
	EncAdapterLockVideoEngine();
	venc_ctx->pVEncDevice->uninit(venc_ctx->pEncoderHandle);
	venc_ctx->bInit = 0;
	EncAdapterUnLockVideoEngine();

	EncAdapterLockVideoEngine();
	result = venc_ctx->pVEncDevice->init(venc_ctx->pEncoderHandle, &venc_ctx->baseConfig);
	EncAdapterUnLockVideoEngine();

	venc_ctx->bInit = 1;
	return result;
}

int AllocInputBuffer(VideoEncoder* pEncoder, VencAllocateBufferParam *pBufferParam)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;
	
	if(pEncoder == NULL || pBufferParam == NULL)
	{
		loge("InitVideoEncoder, param is NULL");
		return -1;
	}

	if(venc_ctx->pFBM == NULL)
	{

		loge("venc_ctx->pFBM == NULL, must call InitVideoEncoder firstly");
		return -1;
	}

	if(AllocateInputBuffer(venc_ctx->pFBM, pBufferParam)!=0)
	{

		loge("allocat inputbuffer failed");
		return -1;		
	}

	return 0;
}

int GetOneAllocInputBuffer(VideoEncoder* pEncoder, VencInputBuffer* pInputbuffer)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;
	
	if(pEncoder == NULL)
	{
		loge("pEncoder == NULL");
		return -1;
	}

	if(GetOneAllocateInputBuffer(venc_ctx->pFBM, pInputbuffer) != 0)
	{

		loge("get one allocate inputbuffer failed");
		return -1;		
	}

	return 0;
}

int FlushCacheAllocInputBuffer(VideoEncoder* pEncoder,  VencInputBuffer *pInputbuffer)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;
	
	if(venc_ctx == NULL)
	{
		loge("pEncoder == NULL");
		return -1;
	}

	FlushCacheAllocateInputBuffer(venc_ctx->pFBM, pInputbuffer);

	return 0;
}

int ReturnOneAllocInputBuffer(VideoEncoder* pEncoder,  VencInputBuffer *pInputbuffer)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;

	if(pEncoder == NULL)
	{
		loge("pEncoder == NULL");
		return -1;
	}

	if(ReturnOneAllocateInputBuffer(venc_ctx->pFBM, pInputbuffer) != 0)
	{

		loge("get one allocate inputbuffer failed");
		return -1;		
	}

	return 0;
}


int ReleaseAllocInputBuffer(VideoEncoder* pEncoder)
{
	if(pEncoder == NULL)
	{
		loge("ReleaseAllocInputBuffer, pEncoder is NULL");
		return -1;
	}
	return 0;
}

int AddOneInputBuffer(VideoEncoder* pEncoder, VencInputBuffer* pBuffer)
{
	int result = 0;
	VencContext* venc_ctx = (VencContext*)pEncoder;
	
	if(venc_ctx == NULL || pBuffer == NULL)
	{
		loge("AddInputBuffer, param is NULL");
		return -1;
	}

	result = AddInputBuffer(venc_ctx->pFBM, pBuffer);
	
	return result;
}


int VideoEncodeOneFrame(VideoEncoder* pEncoder)
{
	int result = 0;
	VencContext* venc_ctx = (VencContext*)pEncoder;

	if(!venc_ctx) {
		return -1;
	}
	
	if(GetInputBuffer(venc_ctx->pFBM, &venc_ctx->curEncInputbuffer) != 0)
	{
		return VENC_RESULT_NO_FRAME_BUFFER;
	}

	if(venc_ctx->ICVersion == 0x1639)
	{
		if((unsigned long)(venc_ctx->curEncInputbuffer.pAddrPhyY) >= 0x20000000)
		{
			venc_ctx->curEncInputbuffer.pAddrPhyY -= 0x20000000;
		}
		else
		{
			logw("venc_ctx->curEncInputbuffer.pAddrPhyY: %p, maybe not right", venc_ctx->curEncInputbuffer.pAddrPhyY);
		}

		if((unsigned long)venc_ctx->curEncInputbuffer.pAddrPhyC >= 0x20000000)
		{
			venc_ctx->curEncInputbuffer.pAddrPhyC -= 0x20000000;
		}
	}
	else
	{
		if((unsigned long)venc_ctx->curEncInputbuffer.pAddrPhyY >= 0x40000000)
		{
			venc_ctx->curEncInputbuffer.pAddrPhyY -= 0x40000000;
		}
		else
		{
			logv("venc_ctx->curEncInputbuffer.pAddrPhyY: %p, maybe not right", venc_ctx->curEncInputbuffer.pAddrPhyY);
		}

		if((unsigned long)venc_ctx->curEncInputbuffer.pAddrPhyC >= 0x40000000)
		{
			venc_ctx->curEncInputbuffer.pAddrPhyC -= 0x40000000;
		}
	}

	EncAdapterLockVideoEngine();
	result = venc_ctx->pVEncDevice->encode(venc_ctx->pEncoderHandle, &venc_ctx->curEncInputbuffer);
	EncAdapterUnLockVideoEngine();

	AddUsedInputBuffer(venc_ctx->pFBM, &venc_ctx->curEncInputbuffer);
	
	return result;
}


int VideoEncodeInputBuffer(VideoEncoder* pEncoder, VencInputBuffer* pInBuffer)
{
	int result = 0;
	VencContext* venc_ctx = (VencContext*)pEncoder;

	if(!venc_ctx) {
		return -1;
	}


	//memcpy(&venc_ctx->curEncInputbuffer, pInBuffer, sizeof(VencInputBuffer));


	if(venc_ctx->ICVersion == 0x1639)
	{
		if ((unsigned long)pInBuffer->pAddrPhyY >= 0x20000000)
		{
			pInBuffer->pAddrPhyY -= 0x20000000;
		}
		else
		{
			logw("pInBuffer->pAddrPhyY: %p, maybe not right", pInBuffer->pAddrPhyY);
		}

		if((unsigned long)pInBuffer->pAddrPhyC >= 0x20000000)
		{
			pInBuffer->pAddrPhyC -= 0x20000000;
		}
	}
	else
	{
		if((unsigned long)pInBuffer->pAddrPhyY >= 0x40000000)
		{
			pInBuffer->pAddrPhyY -= 0x40000000;
		}
		else
		{
			logv("vpInBuffer->pAddrPhyY: %p, maybe not right", pInBuffer->pAddrPhyY);
		}

		if((unsigned long)pInBuffer->pAddrPhyC >= 0x40000000)
		{
			pInBuffer->pAddrPhyC -= 0x40000000;
		}
	}

	EncAdapterLockVideoEngine();
	result = venc_ctx->pVEncDevice->encode(venc_ctx->pEncoderHandle, pInBuffer);
	EncAdapterUnLockVideoEngine();

	return result;
}

int AlreadyUsedInputBuffer(VideoEncoder* pEncoder, VencInputBuffer* pBuffer)
{
	int result = 0;
	VencContext* venc_ctx = (VencContext*)pEncoder;

	if(venc_ctx == NULL || pBuffer == NULL)
	{
		loge("AddInputBuffer, param is NULL");
		return -1;
	}

	result = GetUsedInputBuffer(venc_ctx->pFBM, pBuffer);
	return result;
}

int ValidBitstreamFrameNum(VideoEncoder* pEncoder)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;
	return venc_ctx->pVEncDevice->ValidBitStreamFrameNum(venc_ctx->pEncoderHandle);
}

int GetOneBitstreamFrame(VideoEncoder* pEncoder, VencOutputBuffer* pBuffer)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;

	if(!venc_ctx) {
		return -1;
	}
	
	if(venc_ctx->pVEncDevice->GetOneBitStreamFrame(venc_ctx->pEncoderHandle, pBuffer)!=0)
	{
		return -1;

	}
	
	return 0;
}

int FreeOneBitStreamFrame(VideoEncoder* pEncoder, VencOutputBuffer* pBuffer)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;

	if(!venc_ctx) {
		return -1;
	}
	
	if(venc_ctx->pVEncDevice->FreeOneBitStreamFrame(venc_ctx->pEncoderHandle, pBuffer)!=0)
	{
		return -1;
	}
	return 0;	
}

int VideoEncGetParameter(VideoEncoder* pEncoder, VENC_INDEXTYPE indexType, void* paramData)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;
	return venc_ctx->pVEncDevice->GetParameter(venc_ctx->pEncoderHandle, indexType, paramData);
}

int VideoEncSetParameter(VideoEncoder* pEncoder, VENC_INDEXTYPE indexType, void* paramData)
{
	VencContext* venc_ctx = (VencContext*)pEncoder;
	return venc_ctx->pVEncDevice->SetParameter(venc_ctx->pEncoderHandle, indexType, paramData);
}

int AWJpecEnc(JpegEncInfo* pJpegInfo, EXIFInfo* pExifInfo, void* pOutBuffer, int* pOutBufferSize)
{
	VencAllocateBufferParam bufferParam;
	VideoEncoder* pVideoEnc = NULL;
	VencInputBuffer inputBuffer;
	VencOutputBuffer outputBuffer;
	int result = 0;

	pVideoEnc = VideoEncCreate(VENC_CODEC_JPEG);
	VideoEncSetParameter(pVideoEnc, VENC_IndexParamJpegExifInfo, pExifInfo);
	VideoEncSetParameter(pVideoEnc, VENC_IndexParamJpegQuality, &pJpegInfo->quality);
	
	if(VideoEncInit(pVideoEnc, &pJpegInfo->sBaseInfo)< 0)
	{
		result = -1;
		goto ERROR;
	}

	if(pJpegInfo->bNoUseAddrPhy)
	{
		bufferParam.nSizeY = pJpegInfo->sBaseInfo.nStride*pJpegInfo->sBaseInfo.nInputHeight;
		bufferParam.nSizeC = bufferParam.nSizeY>>1;
		bufferParam.nBufferNum = 1;

		if(AllocInputBuffer(pVideoEnc, &bufferParam)<0)
		{
			result = -1;
			goto ERROR;
		}

		GetOneAllocInputBuffer(pVideoEnc, &inputBuffer);
		memcpy(inputBuffer.pAddrVirY, pJpegInfo->pAddrPhyY, bufferParam.nSizeY);
		memcpy(inputBuffer.pAddrVirC, pJpegInfo->pAddrPhyC, bufferParam.nSizeC);

		FlushCacheAllocInputBuffer(pVideoEnc, &inputBuffer);
	}
	else
	{
		inputBuffer.pAddrPhyY = pJpegInfo->pAddrPhyY;
		inputBuffer.pAddrPhyC = pJpegInfo->pAddrPhyC;
	}

	inputBuffer.bEnableCorp         = pJpegInfo->bEnableCorp;
	inputBuffer.sCropInfo.nLeft     =  pJpegInfo->sCropInfo.nLeft;
	inputBuffer.sCropInfo.nTop		=  pJpegInfo->sCropInfo.nTop;
	inputBuffer.sCropInfo.nWidth    =  pJpegInfo->sCropInfo.nWidth;
	inputBuffer.sCropInfo.nHeight   =  pJpegInfo->sCropInfo.nHeight;

	AddOneInputBuffer(pVideoEnc, &inputBuffer);
	if(VideoEncodeOneFrame(pVideoEnc)!= 0)
	{
		loge("jpeg encoder error");
	}

	AlreadyUsedInputBuffer(pVideoEnc,&inputBuffer);
	
	if(pJpegInfo->bNoUseAddrPhy)
	{
		ReturnOneAllocInputBuffer(pVideoEnc, &inputBuffer);
	}

	GetOneBitstreamFrame(pVideoEnc, &outputBuffer);

	memcpy(pOutBuffer, outputBuffer.pData0, outputBuffer.nSize0);
	if(outputBuffer.nSize1)
	{
		memcpy(((unsigned char*)pOutBuffer + outputBuffer.nSize0), outputBuffer.pData1, outputBuffer.nSize1);

		*pOutBufferSize = outputBuffer.nSize0 + outputBuffer.nSize1;
	}
	else
	{
		*pOutBufferSize = outputBuffer.nSize0;
	}
	
	FreeOneBitStreamFrame(pVideoEnc, &outputBuffer);

ERROR:
	
	if(pVideoEnc)
	{
		VideoEncDestroy(pVideoEnc);
	}
	
	return result;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
