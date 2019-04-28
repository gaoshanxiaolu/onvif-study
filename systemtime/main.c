/************************************************************************
**
** 作者：许振坪
** 日期：2017-05-03
** 博客：http://blog.csdn.net/benkaoya
** 描述：IPC校时示例代码
**
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "onvif_comm.h"
#include "onvif_dump.h"

#ifdef WIN32
#include <windows.h>
#endif

/************************************************************************
**函数：ONVIF_GetSystemDateAndTime
**功能：获取设备的系统时间
**参数：
        [in] DeviceXAddr - 设备服务地址
**返回：
        0表明成功，非0表明失败
**备注：
    1). 对于IPC摄像头，OSD打印的时间是其LocalDateTime
************************************************************************/
int ONVIF_GetSystemDateAndTime(const char *DeviceXAddr)
{
    int result = 0;
    struct soap *soap = NULL;
    struct _tds__GetSystemDateAndTime         req;
    struct _tds__GetSystemDateAndTimeResponse rep;

    SOAP_ASSERT(NULL != DeviceXAddr);

    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);

    memset(&req, 0x00, sizeof(req));
    memset(&rep, 0x00, sizeof(rep));
    result = soap_call___tds__GetSystemDateAndTime(soap, DeviceXAddr, NULL, &req, &rep);
    SOAP_CHECK_ERROR(result, soap, "GetSystemDateAndTime");

    dump_tds__GetSystemDateAndTime(&rep);

EXIT:

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }

    return result;
}

/************************************************************************
**函数：ONVIF_GetHostTimeZone
**功能：获取主机的时区信息
**参数：
        [out] TZ    - 返回的时区信息
        [in] sizeTZ - TZ缓存大小
**返回：无
**备注：
************************************************************************/
static void ONVIF_GetHostTimeZone(char *TZ, int sizeTZ)
{
    char timezone[20] = {0};

#ifdef WIN32

    TIME_ZONE_INFORMATION TZinfo;
    GetTimeZoneInformation(&TZinfo);
    sprintf(timezone, "GMT%c%02d:%02d",  (TZinfo.Bias <= 0) ? '+' : '-', labs(TZinfo.Bias) / 60, labs(TZinfo.Bias) % 60);

#else

    FILE *fp = NULL;
    char time_fmt[32] = {0};

    fp = popen("date +%z", "r");
    fread(time_fmt, sizeof(time_fmt), 1, fp);
    pclose(fp);

    if( ((time_fmt[0] == '+') || (time_fmt[0] == '-')) &&
        isdigit(time_fmt[1]) && isdigit(time_fmt[2]) && isdigit(time_fmt[3]) && isdigit(time_fmt[4]) ) {
            sprintf(timezone, "GMT%c%c%c:%c%c", time_fmt[0], time_fmt[1], time_fmt[2], time_fmt[3], time_fmt[4]);
    } else {
        strcpy(timezone, "GMT+08:00");
    }

#endif

    if (sizeTZ > strlen(timezone)) {
        strcpy(TZ, timezone);
    }

    return;
}

/************************************************************************
**函数：ONVIF_SetSystemDateAndTime
**功能：根据客户端主机当前时间，校时IPC的系统时间
**参数：
        [in] DeviceXAddr - 设备服务地址
**返回：
        0表明成功，非0表明失败
**备注：
    1). 对于IPC摄像头，OSD打印的时间是其本地时间（本地时间跟时区息息相关），
        设置时间时一定要注意时区的正确性。
************************************************************************/
int ONVIF_SetSystemDateAndTime(const char *DeviceXAddr)
{
    int result = 0;
    struct soap *soap = NULL;
    struct _tds__SetSystemDateAndTime           req;
    struct _tds__SetSystemDateAndTimeResponse   rep;

    char TZ[20];                                                                // 用于获取客户端主机的时区信息（如"GMT+08:00"）
    time_t t;                                                                   // 用于获取客户端主机的UTC时间
    struct tm tm;

    SOAP_ASSERT(NULL != DeviceXAddr);
    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    ONVIF_GetHostTimeZone(TZ, DIM(TZ));                                         // 获取客户端主机的时区信息

    t = time(NULL);                                                             // 获取客户端主机的UTC时间
#ifdef WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif

    memset(&req, 0x00, sizeof(req));
    memset(&rep, 0x00, sizeof(rep));
    req.DateTimeType      = tt__SetDateTimeType__Manual;
    req.DaylightSavings   = xsd__boolean__false_;
    req.TimeZone          = (struct tt__TimeZone *)ONVIF_soap_malloc(soap, sizeof(struct tt__TimeZone));
    req.UTCDateTime       = (struct tt__DateTime *)ONVIF_soap_malloc(soap, sizeof(struct tt__DateTime));
    req.UTCDateTime->Date = (struct tt__Date *)ONVIF_soap_malloc(soap, sizeof(struct tt__Date));
    req.UTCDateTime->Time = (struct tt__Time *)ONVIF_soap_malloc(soap, sizeof(struct tt__Time));

    req.TimeZone->TZ              = TZ;                                         // 设置本地时区（IPC的OSD显示的时间就是本地时间）
    req.UTCDateTime->Date->Year   = tm.tm_year + 1900;                          // 设置UTC时间（注意不是本地时间）
    req.UTCDateTime->Date->Month  = tm.tm_mon + 1;
    req.UTCDateTime->Date->Day    = tm.tm_mday;
    req.UTCDateTime->Time->Hour   = tm.tm_hour;
    req.UTCDateTime->Time->Minute = tm.tm_min;
    req.UTCDateTime->Time->Second = tm.tm_sec;

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);
    result = soap_call___tds__SetSystemDateAndTime(soap, DeviceXAddr, NULL, &req, &rep);
    SOAP_CHECK_ERROR(result, soap, "SetSystemDateAndTime");

EXIT:

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }

    return result;
}

void cb_discovery(char *DeviceXAddr)
{
    ONVIF_GetSystemDateAndTime(DeviceXAddr);
    ONVIF_SetSystemDateAndTime(DeviceXAddr);
    ONVIF_GetSystemDateAndTime(DeviceXAddr);
}

int main(int argc, char **argv)
{
    ONVIF_DetectDevice(cb_discovery);

    return 0;
}