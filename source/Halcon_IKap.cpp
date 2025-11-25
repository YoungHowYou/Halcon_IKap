#include "Halcon.h"
#include "Halcon_IKap.h"
#include "HalconCpp.h"
#include "IKapBoard.h"
#define H_IKapCamera_TAG 0xC0FFEE31
#define H_IKapCamera_SEM_TYPE "IKapCamera"
using namespace HalconCpp;

extern "C"
{
    typedef struct
    {
        HANDLE IKapHandle;
    } HUserHandleData;

    static Herror HUserHandleDestructor(Hproc_handle ph, HUserHandleData *data)
    {

        return HFree(ph, data);
    }
    // 句柄类型描述符
    const HHandleInfo HandleTypeUser = HANDLE_INFO_INITIALIZER_NOSER(H_IKapCamera_TAG, H_IKapCamera_SEM_TYPE, HUserHandleDestructor, NULL, NULL);
}
struct MyContext
{
    int 分时频闪数;
    int 设备序号;
    HTuple 海康相机采集队列; // 直接放对象
    HANDLE IKaphDev;

    MyContext(int flash, int nIndex, const HTuple &queue, const HANDLE &hDev)
        : 分时频闪数(flash), 设备序号(nIndex), 海康相机采集队列(queue), IKaphDev(hDev)
    {
    }
};

///* 注册的回调函数 */

void CALLBACK HIKapOnGrabStart(void *pContext)
{
}
void CALLBACK HIKapOnFrameReady(void *pContext)
{

    MyContext *context = (MyContext *)pContext;
    int nWidth = 512;
    int nHeight = 512;
    int ret = IKapGetInfo(context->IKaphDev, IKP_IMAGE_WIDTH, &nWidth);
    ret = IKapGetInfo(context->IKaphDev, IKP_IMAGE_HEIGHT, &nHeight);
    int nFrameSize = 0;
    int nFrameCount = 0;
    IKAPBUFFERSTATUS status;
    IKapGetInfo(context->IKaphDev, IKP_FRAME_COUNT, &nFrameCount);
    IKapGetBufferStatus(context->IKaphDev, 0, &status);
    int DATA_FORMAT = 0;
    IKapGetInfo(context->IKaphDev, IKP_DATA_FORMAT, &DATA_FORMAT);
    HObject ho_Image;
    HTuple hv_MessageHandle;

    if (status.uFull == 1)
    {
        IKapGetInfo(context->IKaphDev, IKP_FRAME_SIZE, &nFrameSize);
        unsigned char *pUserBuffer = NULL;
        IKapGetBufferAddress(context->IKaphDev, 0, (void **)&pUserBuffer);
        int64_t pContexs = (unsigned long long)pUserBuffer;
        if (DATA_FORMAT == IKP_DATA_FORMAT_VAL_8Bit)
        {
            GenImage1(&ho_Image, "byte", nWidth * (context->分时频闪数), nHeight / (context->分时频闪数), (int64_t)pContexs);
        }
        else
        {
            GenImage1(&ho_Image, "uint2", nWidth * (context->分时频闪数), nHeight / (context->分时频闪数), (int64_t)pContexs);
        }
        CreateMessage(&hv_MessageHandle);
        SetMessageObj(ho_Image, hv_MessageHandle, "image");
        SetMessageTuple(hv_MessageHandle, "nFrameNum", (__int64)nFrameCount);

        SetMessageTuple(hv_MessageHandle, "nDeviceID", context->设备序号);

        EnqueueMessage((context->海康相机采集队列), hv_MessageHandle, HTuple(), HTuple());
        ClearMessage(hv_MessageHandle);
    }
}

void CALLBACK HIKapOnFrameLost(void *pContext)
{
}
void CALLBACK HIKapOnTimeout(void *pContext)
{
}
void CALLBACK HIKapOnGrabStop(void *pContext)
{
}

Herror HIKapOpenBoard(Hproc_handle proc_handle)
{
    Hcpar filename;
    Hcpar nDeviceIndex;
    Hcpar timeout;
    Hcpar TimeSharingFlicker;
    HANDLE IKaphDev;
    Hcpar *相机采集队列容器;
    INT4_8 参数个数; // 参数个数
    HAllocStringMem(proc_handle, 128);
    HGetSPar(proc_handle, 1, LONG_PAR, &nDeviceIndex, 1);
    HGetSPar(proc_handle, 2, STRING_PAR, &filename, 1);
    HGetSPar(proc_handle, 3, LONG_PAR, &timeout, 1);
    HGetSPar(proc_handle, 4, LONG_PAR, &TimeSharingFlicker, 1);
    HGetPPar(proc_handle, 3, &相机采集队列容器, &参数个数);
    HTuple 相机采集队列(相机采集队列容器, 1);

    int ret = IK_RTN_OK;
    IKaphDev = IKapOpen(IKBoardPCIE, nDeviceIndex.par.l);
    ret = IKapLoadConfigurationFromFile(IKaphDev, filename.par.s);
    if (ret != 1)
    {
        return 10000 *H__LINE__;
    }
    ret = IKapSetInfo(IKaphDev, IKP_FRAME_COUNT, 1); // 缓存修改为1
    if (ret != 1)
    {
        return 10000 *H__LINE__;
    }
    ret = IKapSetInfo(IKaphDev, IKP_TIME_OUT, timeout.par.l);
    if (ret != 1)
    {
        return 10000 *H__LINE__;
    }
    ret = IKapSetInfo(IKaphDev, IKP_GRAB_MODE, IKP_GRAB_NON_BLOCK); /* Configure grab start mode. */
    if (ret != 1)
    {
        return 10000 *H__LINE__;
    }
    ret = IKapSetInfo(IKaphDev, IKP_FRAME_TRANSFER_MODE, IKP_FRAME_TRANSFER_SYNCHRONOUS_NEXT_EMPTY_WITH_PROTECT);
    if (ret != 1)
    {
        return 10000 *H__LINE__;
    }
    MyContext *ptr = new MyContext(TimeSharingFlicker.par.l, nDeviceIndex.par.l, 相机采集队列,IKaphDev);

    ret = IKapRegisterCallback(IKaphDev, IKEvent_GrabStart, HIKapOnGrabStart, ptr);
    ret = IKapRegisterCallback(IKaphDev, IKEvent_FrameReady, HIKapOnFrameReady, ptr);
    ret = IKapRegisterCallback(IKaphDev, IKEvent_FrameLost, HIKapOnFrameLost, ptr);
    ret = IKapRegisterCallback(IKaphDev, IKEvent_TimeOut, HIKapOnTimeout, ptr);
    ret = IKapRegisterCallback(IKaphDev, IKEvent_GrabStop, HIKapOnGrabStop, ptr);

    if (ret == 0)
    {
        return 10000 *H__LINE__;
    }
    HUserHandleData **handle_data;
    // 分配输出句柄
    HCkP(HAllocOutputHandle(proc_handle, 1, &handle_data, &HandleTypeUser));
    // 分配并初始化用户数据
    HCkP(HAlloc(proc_handle, sizeof(HUserHandleData), (void **)handle_data));
    (*handle_data)->IKapHandle = IKaphDev;

    return H_MSG_TRUE;
}
Herror HIKapStartGrab(Hproc_handle proc_handle)
{
    Hcpar nFrameCount;
    HUserHandleData *handle_data;
    HGetCElemH1(proc_handle, 1, &HandleTypeUser, &handle_data);
    HGetSPar(proc_handle, 2, LONG_PAR, &nFrameCount, 1);
    int ret = IKapStartGrab(handle_data, nFrameCount.par.l);
    if (ret == 0)
    {
        return 10000 *H__LINE__;
    }
    return H_MSG_TRUE;
}
Herror HIKapStopGrab(Hproc_handle proc_handle)
{
    HUserHandleData *handle_data;
    HGetCElemH1(proc_handle, 1, &HandleTypeUser, &handle_data);
    int ret = IKapStopGrab(handle_data->IKapHandle);
    if (ret == 0)
    {
        return 10000 *H__LINE__;
    }
    return H_MSG_TRUE;
}