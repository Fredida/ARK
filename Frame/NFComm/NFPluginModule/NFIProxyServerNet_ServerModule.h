// -------------------------------------------------------------------------
//    @FileName         ��    NFIProxyServerNet_ServerModule.h
//    @Author           ��    Ark Game Tech
//    @Date             ��    2012-12-15
//    @Module           ��    NFIProxyServerNet_ServerModule
//
// -------------------------------------------------------------------------

#ifndef NFI_PROXYNET_SERVERMODULE_H
#define NFI_PROXYNET_SERVERMODULE_H

#include <iostream>
#include "NFIModule.h"
#include "NFINetModule.h"

class NFIProxyServerNet_ServerModule
    :  public NFIModule
{

public:
    virtual int Transpond(const int nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID) = 0;
    virtual int EnterGameSuccessEvent(const AFGUID xClientID, const AFGUID xPlayerID) = 0;
};

#endif