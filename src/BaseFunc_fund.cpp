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


//���APP������Ϣ
long  FUNC_Check_AppidInfo(void * pMem, string sAppID, string &sErrmsg)
{
#if 0
	CStdRecord * pRecAppIDInfo = pMem->GetMemRecord("TFC_APPIDINFO");
	long nRetCode = -1L;

    //��ȡ�ڴ�����ָ��ʧ��
    if (pRecAppIDInfo == NULL)
    {
		sErrmsg = "��ȡAppid��Ϣ�쳣";
        return nRetCode;
    }
	
    int nCount = pRecAppIDInfo->GetCount();
    if (nCount < 1)
    {
		sErrmsg = "��Ӧ��δ��Ȩ����ֹ����";
        return nRetCode;
    }
	
	//�Ҳ�����Ӧ���ݣ���������ҵ�����
	int i;
	i	=	QuickFind(pRecAppIDInfo,	pRecAppIDInfo->FindIndex("RQ"),	sAppID.c_str());
	if (i >=nCount || i < 0)
	{
		sErrmsg = "��֧�ִ˽���";
		return nRetCode;
	}
	//ҵ���ж�

	sErrmsg = "������";
#endif
    
    return 1L;
}
