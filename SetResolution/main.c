/************************************************************************
**
** 作者：许振坪
** 日期：2017-05-03
** 博客：http://blog.csdn.net/benkaoya
** 描述：读取IPC音视频流数据示例代码
**
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "onvif_comm.h"
#include "onvif_dump.h"

/************************************************************************
**函数：ONVIF_GetVideoEncoderConfigurationOptions
**功能：获取指定视频编码器配置的参数选项集
**参数：
        [in] MediaXAddr   - 媒体服务地址
        [in] ConfigurationToken - 视频编码器配置的令牌字符串，如果为NULL，则获取所有视频编码器配置的选项集（会杂在一起，
                                  无法区分选项集是归属哪个视频编码配置器的）
**返回：
        0表明成功，非0表明失败
**备注：
    1). 有两种方式可以获取指定视频编码器配置的参数选项集：一种是根据ConfigurationToken，另一种
        是根据ProfileToken
************************************************************************/
int ONVIF_GetVideoEncoderConfigurationOptions(const char *MediaXAddr, char *ConfigurationToken)
{
    int result = 0;
    struct soap *soap = NULL;
    struct _trt__GetVideoEncoderConfigurationOptions          req;
    struct _trt__GetVideoEncoderConfigurationOptionsResponse  rep;

    SOAP_ASSERT(NULL != MediaXAddr);
    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    memset(&req, 0x00, sizeof(req));
    memset(&rep, 0x00, sizeof(rep));
    req.ConfigurationToken = ConfigurationToken;

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);
    result = soap_call___trt__GetVideoEncoderConfigurationOptions(soap, MediaXAddr, NULL, &req, &rep);
    SOAP_CHECK_ERROR(result, soap, "GetVideoEncoderConfigurationOptions");

    dump_trt__GetVideoEncoderConfigurationOptionsResponse(&rep);

EXIT:

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }

    return result;
}

/************************************************************************
**函数：ONVIF_GetVideoEncoderConfiguration
**功能：获取设备上指定的视频编码器配置信息
**参数：
        [in] MediaXAddr - 媒体服务地址
        [in] ConfigurationToken - 视频编码器配置的令牌字符串
**返回：
        0表明成功，非0表明失败
**备注：
************************************************************************/
int ONVIF_GetVideoEncoderConfiguration(const char *MediaXAddr, char *ConfigurationToken)
{
    int result = 0;
    struct soap *soap = NULL;
    struct _trt__GetVideoEncoderConfiguration          req;
    struct _trt__GetVideoEncoderConfigurationResponse  rep;

    SOAP_ASSERT(NULL != MediaXAddr);
    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);

    memset(&req, 0x00, sizeof(req));
    memset(&rep, 0x00, sizeof(rep));
    req.ConfigurationToken = ConfigurationToken;
    result = soap_call___trt__GetVideoEncoderConfiguration(soap, MediaXAddr, NULL, &req, &rep);
    SOAP_CHECK_ERROR(result, soap, "GetVideoEncoderConfiguration");

    dump_trt__GetVideoEncoderConfigurationResponse(&rep);

EXIT:

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }

    return result;
}

/************************************************************************
**函数：ONVIF_SetVideoEncoderConfiguration
**功能：修改指定的视频编码器配置信息
**参数：
        [in] MediaXAddr - 媒体服务地址
        [in] venc - 视频编码器配置信息
**返回：
        0表明成功，非0表明失败
**备注：
    1). 所设置的分辨率必须是GetVideoEncoderConfigurationOptions返回的“分辨率选项集”中的一种，
    否则调用SetVideoEncoderConfiguration会失败。
************************************************************************/
int ONVIF_SetVideoEncoderConfiguration(const char *MediaXAddr, struct tagVideoEncoderConfiguration *venc)
{
    int result = 0;
    struct soap *soap = NULL;

    struct _trt__GetVideoEncoderConfiguration           gVECfg_req;
    struct _trt__GetVideoEncoderConfigurationResponse   gVECfg_rep;

    struct _trt__SetVideoEncoderConfiguration           sVECfg_req;
    struct _trt__SetVideoEncoderConfigurationResponse   sVECfg_rep;

    SOAP_ASSERT(NULL != MediaXAddr);
    SOAP_ASSERT(NULL != venc);
    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    memset(&gVECfg_req, 0x00, sizeof(gVECfg_req));
    memset(&gVECfg_rep, 0x00, sizeof(gVECfg_rep));
    gVECfg_req.ConfigurationToken = venc->token;

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);
    result = soap_call___trt__GetVideoEncoderConfiguration(soap, MediaXAddr, NULL, &gVECfg_req, &gVECfg_rep);
    SOAP_CHECK_ERROR(result, soap, "GetVideoEncoderConfiguration");

    if (NULL == gVECfg_rep.Configuration) {
        SOAP_DBGERR("video encoder configuration is NULL.\n");
        goto EXIT;
    }

    memset(&sVECfg_req, 0x00, sizeof(sVECfg_req));
    memset(&sVECfg_rep, 0x00, sizeof(sVECfg_rep));

    sVECfg_req.ForcePersistence = xsd__boolean__true_;
    sVECfg_req.Configuration    = gVECfg_rep.Configuration;

    if (NULL != sVECfg_req.Configuration->Resolution) {
        sVECfg_req.Configuration->Resolution->Width  = venc->Width;
        sVECfg_req.Configuration->Resolution->Height = venc->Height;
    }

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);
    result = soap_call___trt__SetVideoEncoderConfiguration(soap, MediaXAddr, NULL, &sVECfg_req, &sVECfg_rep);
    SOAP_CHECK_ERROR(result, soap, "SetVideoEncoderConfiguration");

EXIT:

    if (SOAP_OK == result) {
        SOAP_DBGLOG("\nSetVideoEncoderConfiguration success!!!\n");
    }

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }

    return result;
}

void cb_discovery(char *DeviceXAddr)
{
    int stmno = 0;                                                              // 码流序号，0为主码流，1为辅码流
    int profile_cnt = 0;                                                        // 设备配置文件个数
    struct tagProfile *profiles = NULL;                                         // 设备配置文件列表
    struct tagCapabilities capa;                                                // 设备能力信息

    ONVIF_GetCapabilities(DeviceXAddr, &capa);                                  // 获取设备能力信息（获取媒体服务地址）
    
    profile_cnt = ONVIF_GetProfiles(capa.MediaXAddr, &profiles);                // 获取媒体配置信息（主/辅码流配置信息）

    if (profile_cnt > stmno) {
        struct tagVideoEncoderConfiguration venc;
        char *vencToken = profiles[stmno].venc.token;

        ONVIF_GetVideoEncoderConfigurationOptions(capa.MediaXAddr, vencToken);  // 获取该码流支持的视频编码器参数选项集

        ONVIF_GetVideoEncoderConfiguration(capa.MediaXAddr, vencToken);         // 获取该码流当前的视频编码器参数

        venc = profiles[stmno].venc;
        venc.Height = 960;
        venc.Width  = 1280;
        ONVIF_SetVideoEncoderConfiguration(capa.MediaXAddr, &venc);             // 设置该码流当前的视频编码器参数

        ONVIF_GetVideoEncoderConfiguration(capa.MediaXAddr, vencToken);         // 观察是否修改成功
    }

    if (NULL != profiles) {
        free(profiles);
        profiles = NULL;
    }
}

int main(int argc, char **argv)
{
    ONVIF_DetectDevice(cb_discovery);

    return 0;
}