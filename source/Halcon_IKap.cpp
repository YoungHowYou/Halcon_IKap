#include "Halcon.h"
#include "Halcon_IKap.h"
#include "HalconCpp.h"
#include "IKapBoard.h"
#define H_IKapCamera_TAG 0xC0FFEE11   
#define H_IKapCamera_SEM_TYPE "IKapCamera"
using namespace HalconCpp;

extern "C"
{
    typedef struct
    {
        void *IKapHandle;
    } HUserHandleData;

    static Herror HUserHandleDestructor(Hproc_handle ph, HUserHandleData *data)
    {
        
        return HFree(ph, data);
    }
    // 句柄类型描述符
    const HHandleInfo HandleTypeUser = HANDLE_INFO_INITIALIZER_NOSER(H_IKapCamera_TAG, H_IKapCamera_SEM_TYPE, HUserHandleDestructor, NULL, NULL);
}





Herror HIKapOpenBoard(Hproc_handle proc_handle)
{
	Hcpar filename;
	Hcpar nDeviceIndex;
	Hcpar timeout;
	Hcpar TimeSharingFlicker;
    void    *IKaphDev;
	HAllocStringMem(proc_handle, 128);
	HGetSPar(proc_handle, 1, LONG_PAR, &nDeviceIndex, 1);
	HGetSPar(proc_handle, 2, STRING_PAR, &filename, 1);
	HGetSPar(proc_handle, 3, LONG_PAR, &timeout, 1);
	HGetSPar(proc_handle, 4, LONG_PAR, &TimeSharingFlicker, 1);

	int ret = IK_RTN_OK;
	IKaphDev = IKapOpen(IKBoardPCIE, nDeviceIndex.par.l);
	ret = IKapLoadConfigurationFromFile(IKaphDev , filename.par.s);
	if (ret != 1) { return ret; }
	ret = IKapSetInfo(IKaphDev , IKP_FRAME_COUNT, 1);//缓存修改为1
	if (ret != 1) { return ret; }
	ret = IKapSetInfo(IKaphDev , IKP_TIME_OUT, timeout.par.l);
	if (ret != 1) { return ret; }
	ret = IKapSetInfo(IKaphDev , IKP_GRAB_MODE, IKP_GRAB_NON_BLOCK);    /* Configure grab start mode. */
	if (ret != 1) { return ret; }
	ret = IKapSetInfo(IKaphDev , IKP_FRAME_TRANSFER_MODE, IKP_FRAME_TRANSFER_SYNCHRONOUS_NEXT_EMPTY_WITH_PROTECT);
	if (ret != 1){return ret;}

	ret = IKapRegisterCallback(IKaphDev , IKEvent_GrabStart, HIKapOnGrabStart, ptr);
	ret = IKapRegisterCallback(IKaphDev , IKEvent_FrameReady, HIKapOnFrameReady, ptr);
	ret = IKapRegisterCallback(IKaphDev , IKEvent_FrameLost, HIKapOnFrameLost, ptr);
	ret = IKapRegisterCallback(IKaphDev , IKEvent_TimeOut, HIKapOnTimeout, ptr);
	ret = IKapRegisterCallback(IKaphDev , IKEvent_GrabStop, HIKapOnGrabStop, ptr);
	
	if (ret == 0) { return 10000 + 7; }
	return H_MSG_TRUE;
}
Herror HIKapStartGrab(Hproc_handle proc_handle)
{
	Hcpar nFrameCount, nDeviceIndex;
	HAllocStringMem(proc_handle, 1024);
	HGetSPar(proc_handle, 1, LONG_PAR, &nDeviceIndex, 1);
	HGetSPar(proc_handle, 2, LONG_PAR, &nFrameCount, 1);
	int ret = IKapStartGrab(IKaphDev , nFrameCount.par.l);
	if (ret == 0) { return 10000 + 7; }
	return H_MSG_TRUE;

}
Herror HIKapStopGrab(Hproc_handle proc_handle)
{
	Hcpar  nDeviceIndex;
	HAllocStringMem(proc_handle, 1024);
	HGetSPar(proc_handle, 1, LONG_PAR, &nDeviceIndex, 1);
	int ret = IKapStopGrab(IKaphDev );
	if (ret == 0) { return 10000 + 7; }
	return H_MSG_TRUE;

}