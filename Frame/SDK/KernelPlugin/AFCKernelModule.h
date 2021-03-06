/*
* This source file is part of ArkGameFrame
* For the latest info, see https://github.com/ArkGame
*
* Copyright (c) 2013-2018 ArkGame authors.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#pragma once

#include "SDK/Core/AFIEntity.h"
#include "SDK/Core/Base/AFCDataList.h"
#include "SDK/Core/AFDataTable.h"
#include "SDK/Core/Base/AFGUID.h"
#include "SDK/Interface/AFIGUIDModule.h"
#include "SDK/Interface/AFILogModule.h"
#include "SDK/Interface/AFIKernelModule.h"
#include "SDK/Interface/AFIClassModule.h"
#include "SDK/Interface/AFIElementModule.h"
#include "SDK/Interface/AFISceneModule.h"
#include "SDK/Core/Base/AFPlatform.hpp"
#include "SDK/Core/Base/AFMapEx.h"

class AFCKernelModule
    : public AFIKernelModule,
      public AFMapEx<AFGUID, AFIEntity> //TODO:put data as a seperate data
{
public:
    AFCKernelModule(AFIPluginManager* p);
    virtual ~AFCKernelModule();

    virtual bool Init();
    virtual bool Shut();

    virtual bool BeforeShut();
    virtual bool AfterInit();

    virtual void Update();

    ///////////////////////////////////////////////////////////////////////

    virtual bool FindHeartBeat(const AFGUID& self, const std::string& name);
    virtual bool RemoveHeartBeat(const AFGUID& self, const std::string& name);

    virtual bool IsContainer(const AFGUID& self);
    virtual bool ExistContainer(const int nSceneID);

    virtual ARK_SHARE_PTR<AFIEntity> GetEntity(const AFGUID& ident);
    virtual ARK_SHARE_PTR<AFIEntity> CreateEntity(const AFGUID& self, const int nSceneID, const int nGroupID, const std::string& strClassName, const std::string& strConfigIndex, const AFIDataList& arg);

    virtual bool DestroyAll();
    virtual bool DestroySelf(const AFGUID& self);
    virtual bool DestroyEntity(const AFGUID& self);

    //////////////////////////////////////////////////////////////////////////
    virtual bool FindNode(const AFGUID& self, const std::string& name);

    virtual bool SetNodeBool(const AFGUID& self, const std::string& name, const bool value);
    virtual bool SetNodeInt(const AFGUID& self, const std::string& name, const int32_t value);
    virtual bool SetNodeInt64(const AFGUID& self, const std::string& name, const int64_t value);
    virtual bool SetNodeFloat(const AFGUID& self, const std::string& name, const float value);
    virtual bool SetNodeDouble(const AFGUID& self, const std::string& name, const double value);
    virtual bool SetNodeString(const AFGUID& self, const std::string& name, const std::string& value);
    virtual bool SetNodeObject(const AFGUID& self, const std::string& name, const AFGUID& value);

    virtual bool GetNodeBool(const AFGUID& self, const std::string& name);
    virtual int32_t GetNodeInt(const AFGUID& self, const std::string& name);
    virtual int64_t GetNodeInt64(const AFGUID& self, const std::string& name);
    virtual float GetNodeFloat(const AFGUID& self, const std::string& name);
    virtual double GetNodeDouble(const AFGUID& self, const std::string& name);
    virtual const char* GetNodeString(const AFGUID& self, const std::string& name);
    virtual const AFGUID GetNodeObject(const AFGUID& self, const std::string& name);
    //////////////////////////////////////////////////////////////////////////
    virtual AFDataTable* FindTable(const AFGUID& self, const std::string& name);
    virtual bool ClearTable(const AFGUID& self, const std::string& name);

    virtual bool SetTableBool(const AFGUID& self, const std::string& name, const int row, const int col, const bool value);
    virtual bool SetTableInt(const AFGUID& self, const std::string& name, const int row, const int col, const int32_t value);
    virtual bool SetTableInt64(const AFGUID& self, const std::string& name, const int row, const int col, const int64_t value);
    virtual bool SetTableFloat(const AFGUID& self, const std::string& name, const int row, const int col, const float value);
    virtual bool SetTableDouble(const AFGUID& self, const std::string& name, const int row, const int col, const double value);
    virtual bool SetTableString(const AFGUID& self, const std::string& name, const int row, const int col, const std::string& value);
    virtual bool SetTableObject(const AFGUID& self, const std::string& name, const int row, const int col, const AFGUID& value);

    virtual bool GetTableBool(const AFGUID& self, const std::string& name, const int row, const int col);
    virtual int32_t GetTableInt(const AFGUID& self, const std::string& name, const int row, const int col);
    virtual int64_t GetTableInt64(const AFGUID& self, const std::string& name, const int row, const int col);
    virtual float GetTableFloat(const AFGUID& self, const std::string& name, const int row, const int col);
    virtual double GetTableDouble(const AFGUID& self, const std::string& name, const int row, const int col);
    virtual const char* GetTableString(const AFGUID& self, const std::string& name, const int row, const int col);
    virtual const AFGUID GetTableObject(const AFGUID& self, const std::string& name, const int row, const int col);

    //////////////////////////////////////////////////////////////////////////
    virtual bool SwitchScene(const AFGUID& self, const int nTargetSceneID, const int nTargetGroupID, const float fX, const float fY, const float fZ, const float fOrient, const AFIDataList& arg);

    virtual bool CreateScene(const int nSceneID);
    virtual bool DestroyScene(const int nSceneID);

    virtual int GetOnLineCount();
    virtual int GetMaxOnLineCount();
    virtual int GetSceneOnLineCount(const int nSceneID);
    virtual int GetSceneOnLineCount(const int nSceneID, const int nGroupID);

    virtual int GetSceneOnLineList(const int nSceneID, AFIDataList& var);

    virtual int RequestGroupScene(const int nSceneID);
    virtual bool ReleaseGroupScene(const int nSceneID, const int nGroupID);
    virtual bool ExitGroupScene(const int nSceneID, const int nGroupID);

    virtual bool GetGroupEntityList(const int nSceneID, const int nGroupID, AFIDataList& list);
    virtual int GetEntityByDataNode(const int nSceneID, const std::string& strNodeName, const AFIDataList& valueArgArg, AFIDataList& list);

    virtual void Random(int nStart, int nEnd, int nCount, AFIDataList& valueList);

    //////////////////////////////////////////////////////////////////////////
    virtual bool LogStack();
    virtual bool LogInfo(const AFGUID& ident);
    virtual bool LogSelfInfo(const AFGUID& ident);

    //////////////////////////////////////////////////////////////////////////

    virtual bool DoEvent(const AFGUID& self, const std::string& strClassName, CLASS_OBJECT_EVENT eEvent, const AFIDataList& valueList);
    virtual bool DoEvent(const AFGUID& self, const int nEventID, const AFIDataList& valueList);

protected:

    //注册通用Class事件
    virtual bool RegCommonClassEvent(const CLASS_EVENT_FUNCTOR_PTR& cb);

    //注册通用DataNode事件
    virtual bool RegCommonDataNodeEvent(const DATA_NODE_EVENT_FUNCTOR_PTR& cb);

    //注册通用DataTable事件
    virtual bool RegCommonDataTableEvent(const DATA_TABLE_EVENT_FUNCTOR_PTR& cb);

protected:

    virtual bool AddEventCallBack(const AFGUID& self, const int nEventID, const EVENT_PROCESS_FUNCTOR_PTR& cb);
    virtual bool AddClassCallBack(const std::string& strClassName, const CLASS_EVENT_FUNCTOR_PTR& cb);

    void InitRandom();

    int OnCommonClassEvent(const AFGUID& self, const std::string& name, const CLASS_OBJECT_EVENT eClassEvent, const AFIDataList& var);
    int OnCommonNodeEvent(const AFGUID& self, const std::string& name, const AFIData& oldVar, const AFIData& newVar);
    int OnCommonTableEvent(const AFGUID& self, const DATA_TABLE_EVENT_DATA& xEventData, const AFIData& oldVar, const AFIData& newVar);

    void ProcessMemFree();

protected:

    std::list<AFGUID> mtDeleteSelfList;

    //////////////////////////////////////////////////////////////////////////
    std::list<CLASS_EVENT_FUNCTOR_PTR> mxCommonClassCBList;
    std::list<DATA_NODE_EVENT_FUNCTOR_PTR> mxCommonNodeCBList;
    std::list<DATA_TABLE_EVENT_FUNCTOR_PTR> mxCommonTableCBList;

private:
    std::vector<float> mvRandom;
    int mnRandomPos;

    AFGUID mnCurExeEntity;
    int64_t nLastTime;

    AFISceneModule* m_pSceneModule;
    AFILogModule* m_pLogModule;
    AFIClassModule* m_pClassModule;
    AFIElementModule* m_pElementModule;
    AFIGUIDModule* m_pGUIDModule;
};