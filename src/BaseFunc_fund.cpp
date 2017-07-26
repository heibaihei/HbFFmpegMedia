//#include "StdAfx.h"
//#include "sysdef.h"
//#include "fixdef.h"
//#include <axcom/CommFunc.h>
#include <math.h>
#include<string>
#include<algorithm>
#include<functional>
#include<stdio.h>
using namespace std;


//检查APP接入信息
long  FUNC_Check_AppidInfo(void * pMem, string sAppID, string &sErrmsg)
{
#if 0
	CStdRecord * pRecAppIDInfo = pMem->GetMemRecord("TFC_APPIDINFO");
	long nRetCode = -1L;

    //获取内存数据指针失败
    if (pRecAppIDInfo == NULL)
    {
		sErrmsg = "获取Appid信息异常";
        return nRetCode;
    }
	
    int nCount = pRecAppIDInfo->GetCount();
    if (nCount < 1)
    {
		sErrmsg = "该应用未授权，禁止交易";
        return nRetCode;
    }
	
	//找不到对应数据，允许所有业务操作
	int i;
	i	=	QuickFind(pRecAppIDInfo,	pRecAppIDInfo->FindIndex("RQ"),	sAppID.c_str());
	if (i >=nCount || i < 0)
	{
		sErrmsg = "不支持此交易";
		return nRetCode;
	}
	//业务判断

	sErrmsg = "允许交易";
#endif
    
    return 1L;
}
