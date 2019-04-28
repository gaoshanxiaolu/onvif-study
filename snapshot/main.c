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
**函数：ONVIF_GetSnapshotUri
**功能：获取设备图像抓拍地址(HTTP)
**参数：
        [in]  MediaXAddr    - 媒体服务地址
        [in]  ProfileToken  - the media profile token
        [out] uri           - 返回的地址
        [in]  sizeuri       - 地址缓存大小
**返回：
        0表明成功，非0表明失败
**备注：
    1). 并非所有的ProfileToken都支持图像抓拍地址。举例：XXX品牌的IPC有如下三个配置
    profile0/profile1/TestMediaProfile，其中TestMediaProfile返回的图像抓拍地址就是
    空指针。
************************************************************************/
int ONVIF_GetSnapshotUri(const char *MediaXAddr, char *ProfileToken, char *uri, unsigned int sizeuri)
{
    int result = 0;
    struct soap *soap = NULL;
    struct _trt__GetSnapshotUri         req;
    struct _trt__GetSnapshotUriResponse rep;

    SOAP_ASSERT(NULL != MediaXAddr);
    SOAP_ASSERT(NULL != uri);
    memset(uri, 0x00, sizeuri);

    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);

    memset(&req, 0x00, sizeof(req));
    memset(&rep, 0x00, sizeof(rep));
    req.ProfileToken = ProfileToken;
    result = soap_call___trt__GetSnapshotUri(soap, MediaXAddr, NULL, &req, &rep);
    SOAP_CHECK_ERROR(result, soap, "GetSnapshotUri");

    dump_trt__GetSnapshotUriResponse(&rep);

    result = -1;
    if (NULL != rep.MediaUri) {
        if (NULL != rep.MediaUri->Uri) {
            if (sizeuri > strlen(rep.MediaUri->Uri)) {
                strcpy(uri, rep.MediaUri->Uri);
                result = 0;
            } else {
                SOAP_DBGERR("Not enough cache!\n");
            }
        }
    }

EXIT:

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

    char cmd[256];
    char uri[ONVIF_ADDRESS_SIZE] = {0};                                         // 不带认证信息的URI地址
    char uri_auth[ONVIF_ADDRESS_SIZE + 50] = {0};                               // 带有认证信息的URI地址

    ONVIF_GetCapabilities(DeviceXAddr, &capa);                                  // 获取设备能力信息（获取媒体服务地址）
    
    profile_cnt = ONVIF_GetProfiles(capa.MediaXAddr, &profiles);                // 获取媒体配置信息（主/辅码流配置信息）

    if (profile_cnt > stmno) {
        ONVIF_GetSnapshotUri(capa.MediaXAddr, profiles[stmno].token, uri, sizeof(uri)); // 获取图像抓拍URI

        make_uri_withauth(uri, USERNAME, PASSWORD, uri_auth, sizeof(uri_auth)); // 生成带认证信息的URI（有的IPC要求认证）

        sprintf(cmd, "wget -O out.jpeg '%s'", uri_auth);                        // 使用wget下载图片
        system(cmd);
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