// -------------------------------------------------------------------------
//    @FileName         :    NFCKernelModule.cpp
//    @Author           :    Ark Game Tech
//    @Date             :    2012-12-15
//    @Module           :    NFCKernelModule
//    @Desc             :
// -------------------------------------------------------------------------

#include "NFCKernelModule.h"
#include "SDK/Core/NFDefine.h"
#include "SDK/Core/NFCObject.h"
#include "SDK/Core/NFCRecord.h"
#include "SDK/Core/AFGUID.h"
#include "SDK/Core/NFCMemManger.h"
#include "SDK/MsgDef/NFProtocolDefine.hpp"

NFCKernelModule::NFCKernelModule(NFIPluginManager* p)
{
    pPluginManager = p;

    nLastTime = pPluginManager->GetNowTime();
    InitRandom();
}

NFCKernelModule::~NFCKernelModule()
{
    ClearAll();
}

void NFCKernelModule::InitRandom()
{
    mvRandom.clear();

    int nRandomMax = 100000;
    mnRandomPos = 0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0f, 1.0f);

    for(int i = 0; i < nRandomMax; i++)
    {
        mvRandom.push_back(dis(gen));
    }
}

bool NFCKernelModule::Init()
{
    mtDeleteSelfList.clear();

    m_pSceneModule = pPluginManager->FindModule<NFISceneModule>();
    m_pClassModule = pPluginManager->FindModule<NFIClassModule>();
    m_pElementModule = pPluginManager->FindModule<NFIElementModule>();
    m_pLogModule = pPluginManager->FindModule<NFILogModule>();
    m_pGUIDModule = pPluginManager->FindModule<AFIGUIDModule>();

    return true;
}

bool NFCKernelModule::Shut()
{
    return true;
}

bool NFCKernelModule::Execute()
{
    ProcessMemFree();

    mnCurExeObject = NULL_GUID;

    if(mtDeleteSelfList.size() > 0)
    {
        std::list<AFGUID>::iterator it = mtDeleteSelfList.begin();
        for(it; it != mtDeleteSelfList.end(); it++)
        {
            DestroyObject(*it);
        }
        mtDeleteSelfList.clear();
    }

    m_pSceneModule->Execute();

    NF_SHARE_PTR<NFIObject> pObject = First();
    while(pObject)
    {
        mnCurExeObject = pObject->Self();
        pObject->Execute();
        mnCurExeObject = NULL_GUID;

        pObject = Next();
    }

    return true;
}

bool NFCKernelModule::FindHeartBeat(const AFGUID& self, const std::string& strHeartBeatName)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(pObject)
    {
        return pObject->FindHeartBeat(strHeartBeatName);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::RemoveHeartBeat(const AFGUID& self, const std::string& strHeartBeatName)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(pObject)
    {
        return pObject->RemoveHeartBeat(strHeartBeatName);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);

    return false;
}

NF_SHARE_PTR<NFIObject> NFCKernelModule::CreateObject(const AFGUID& self, const int nSceneID, const int nGroupID, const std::string& strClassName, const std::string& strConfigIndex, const AFIDataList& arg)
{
    NF_SHARE_PTR<NFIObject> pObject;
    AFGUID ident = self;

    NF_SHARE_PTR<NFCSceneInfo> pContainerInfo = m_pSceneModule->GetElement(nSceneID);
    if(!pContainerInfo)
    {
        m_pLogModule->LogError(AFGUID(0, nSceneID), "There is no scene", nSceneID, __FUNCTION__, __LINE__);
        return pObject;
    }

    if(!pContainerInfo->GetElement(nGroupID))
    {
        m_pLogModule->LogError(AFGUID(0, nSceneID), "There is no group", nGroupID, __FUNCTION__, __LINE__);
        return pObject;
    }

    //  if (!m_pElementModule->ExistElement(strConfigIndex))
    //  {
    //      m_pLogModule->LogError(AFGUID(0, nSceneID), "There is no group", nGroupID, __FUNCTION__, __LINE__);
    //      return pObject;
    //  }

    //默认为1分组，0则是所有分组都看得见,-1则是容器
    if(ident.IsNull())
    {
        ident = m_pGUIDModule->CreateGUID();
    }

    if(GetElement(ident))
    {
        m_pLogModule->LogError(ident, "The object has Exists", NULL_STR, __FUNCTION__, __LINE__);
        return pObject;
    }

    NF_SHARE_PTR<NFIPropertyManager> pStaticClassPropertyManager = m_pClassModule->GetClassPropertyManager(strClassName);
    NF_SHARE_PTR<NFIRecordManager> pStaticClassRecordManager = m_pClassModule->GetClassRecordManager(strClassName);
    //NF_SHARE_PTR<NFIComponentManager> pStaticClasComponentManager = m_pClassModule->GetClassComponentManager(strClassName);
    if(pStaticClassPropertyManager && pStaticClassRecordManager/* && pStaticClasComponentManager*/)
    {

        pObject = NF_SHARE_PTR<NFIObject>(NF_NEW NFCObject(ident, pPluginManager));
        //是否是应该晚点等到事件2时才加入容器，这样能保证进入容器的对象都是有完整数据的，否则因为协程的原因，其他对象找到他时他却没数据或者部分数据
        AddElement(ident, pObject);
        pContainerInfo->AddObjectToGroup(nGroupID, ident, strClassName == NFrame::Player::ThisName() ? true : false);

        NF_SHARE_PTR<NFIPropertyManager> pPropertyManager = pObject->GetPropertyManager();
        NF_SHARE_PTR<NFIRecordManager> pRecordManager = pObject->GetRecordManager();
        //NF_SHARE_PTR<NFIComponentManager> pComponentManager = pObject->GetComponentManager();

        //默认属性
        NF_SHARE_PTR<NFIProperty> pStaticConfigPropertyInfo = pStaticClassPropertyManager->First();
        while(nullptr != pStaticConfigPropertyInfo)
        {
            NF_SHARE_PTR<NFIProperty> xProperty = pPropertyManager->AddProperty(ident, pStaticConfigPropertyInfo->GetKey(), pStaticConfigPropertyInfo->GetType());

            xProperty->SetPublic(pStaticConfigPropertyInfo->GetPublic());
            xProperty->SetPrivate(pStaticConfigPropertyInfo->GetPrivate());
            xProperty->SetSave(pStaticConfigPropertyInfo->GetSave());
            xProperty->SetCache(pStaticConfigPropertyInfo->GetCache());
            xProperty->SetRelationValue(pStaticConfigPropertyInfo->GetRelationValue());

            //通用回调，方便NET同步
            pObject->AddPropertyCallBack(pStaticConfigPropertyInfo->GetKey(), this, &NFCKernelModule::OnPropertyCommonEvent);

            pStaticConfigPropertyInfo = pStaticClassPropertyManager->Next();
        }

        NF_SHARE_PTR<NFIRecord> pConfigRecordInfo = pStaticClassRecordManager->First();
        while(nullptr != pConfigRecordInfo)
        {
            NF_SHARE_PTR<NFIRecord> xRecord =  pRecordManager->AddRecord(ident,
                                               pConfigRecordInfo->GetName(),
                                               pConfigRecordInfo->GetInitData(),
                                               pConfigRecordInfo->GetTag(),
                                               pConfigRecordInfo->GetRows());

            xRecord->SetPublic(pConfigRecordInfo->GetPublic());
            xRecord->SetPrivate(pConfigRecordInfo->GetPrivate());
            xRecord->SetSave(pConfigRecordInfo->GetSave());
            xRecord->SetCache(pConfigRecordInfo->GetCache());



            //通用回调，方便NET同步
            pObject->AddRecordCallBack(pConfigRecordInfo->GetName(), this, &NFCKernelModule::OnRecordCommonEvent);

            pConfigRecordInfo = pStaticClassRecordManager->Next();
        }

        //////////////////////////////////////////////////////////////////////////
        //配置属性
        NF_SHARE_PTR<NFIPropertyManager> pConfigPropertyManager = m_pElementModule->GetPropertyManager(strConfigIndex);
        NF_SHARE_PTR<NFIRecordManager> pConfigRecordManager = m_pElementModule->GetRecordManager(strConfigIndex);

        if(nullptr != pConfigPropertyManager && nullptr != pConfigRecordManager)
        {
            NF_SHARE_PTR<NFIProperty> pConfigPropertyInfo = pConfigPropertyManager->First();
            while(nullptr != pConfigPropertyInfo)
            {
                if(pConfigPropertyInfo->Changed())
                {
                    pPropertyManager->SetProperty(pConfigPropertyInfo->GetKey(), pConfigPropertyInfo->GetValue());
                }

                pConfigPropertyInfo = pConfigPropertyManager->Next();
            }
        }

        DoEvent(ident, strClassName, CLASS_OBJECT_EVENT::COE_CREATE_NODATA, arg);

        //传入的属性赋值
        for(int i = 0; i < arg.GetCount() - 1; i += 2)
        {
            const std::string& strPropertyName = arg.String(i);
            if(NFrame::IObject::ConfigID() != strPropertyName
                    && NFrame::IObject::ClassName() != strPropertyName
                    && NFrame::IObject::SceneID() != strPropertyName
                    && NFrame::IObject::GroupID() != strPropertyName)
            {
                NF_SHARE_PTR<NFIProperty> pArgProperty = pStaticClassPropertyManager->GetElement(strPropertyName);
                if(pArgProperty)
                {
                    switch(pArgProperty->GetType())
                    {
                    case DT_BOOLEAN:
                        pObject->SetPropertyBool(strPropertyName, arg.Bool(i + 1));
                        break;
                    case DT_INT:
                        pObject->SetPropertyInt(strPropertyName, arg.Int(i + 1));
                        break;
                    case DT_INT64:
                        pObject->SetPropertyInt64(strPropertyName, arg.Int64(i + 1));
                        break;
                    case DT_FLOAT:
                        pObject->SetPropertyFloat(strPropertyName, arg.Float(i + 1));
                        break;
                    case DT_DOUBLE:
                        pObject->SetPropertyDouble(strPropertyName, arg.Double(i + 1));
                        break;
                    case DT_STRING:
                        pObject->SetPropertyString(strPropertyName, arg.String(i + 1));
                        break;
                    case DT_OBJECT:
                        pObject->SetPropertyObject(strPropertyName, arg.Object(i + 1));
                        break;
                    //case TDATA_POINT:
                    //    pObject->SetPropertyPoint(strPropertyName, arg.Point(i + 1));
                    //    break;
                    default:
                        break;
                    }
                }
            }
        }

        //放进容器//先进入场景，再进入层
        pObject->SetPropertyString(NFrame::IObject::ConfigID(), strConfigIndex);
        pObject->SetPropertyString(NFrame::IObject::ClassName(), strClassName);
        pObject->SetPropertyInt(NFrame::IObject::SceneID(), nSceneID);
        pObject->SetPropertyInt(NFrame::IObject::GroupID(), nGroupID);

        DoEvent(ident, strClassName, COE_CREATE_LOADDATA, arg);
        DoEvent(ident, strClassName, COE_CREATE_BEFORE_EFFECT, arg);
        DoEvent(ident, strClassName, COE_CREATE_EFFECTDATA, arg);
        DoEvent(ident, strClassName, COE_CREATE_AFTER_EFFECT, arg);
        DoEvent(ident, strClassName, COE_CREATE_HASDATA, arg);
    }

    return pObject;
}

bool NFCKernelModule::DestroyObject(const AFGUID& self)
{
    if(self == mnCurExeObject
            && !self.IsNull())
    {
        //自己的循环过程中，不能删除自己，得等下一帧才行
        return DestroySelf(self);
    }

    //需要同时从容器中删掉
    int32_t nGroupID = GetPropertyInt(self, NFrame::IObject::GroupID());
    int32_t nSceneID = GetPropertyInt(self, NFrame::IObject::SceneID());

    NF_SHARE_PTR<NFCSceneInfo> pContainerInfo = m_pSceneModule->GetElement(nSceneID);
    if(nullptr != pContainerInfo)
    {
        const std::string& strClassName = GetPropertyString(self, NFrame::IObject::ClassName());

        pContainerInfo->RemoveObjectFromGroup(nGroupID, self, strClassName == NFrame::Player::ThisName() ? true : false);

        DoEvent(self, strClassName, COE_BEFOREDESTROY, AFXDataList());
        DoEvent(self, strClassName, COE_DESTROY, AFXDataList());

        RemoveElement(self);

        return true;

    }

    m_pLogModule->LogError(self, "There is no scene", nSceneID, __FUNCTION__, __LINE__);

    return false;
}

bool NFCKernelModule::FindProperty(const AFGUID& self, const std::string& strPropertyName)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->FindProperty(strPropertyName);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return false;
}

bool NFCKernelModule::SetPropertyBool(const AFGUID& self, const std::string& strPropertyName, const bool value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->SetPropertyBool(strPropertyName, value);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return false;
}

bool NFCKernelModule::SetPropertyInt(const AFGUID& self, const std::string& strPropertyName, const int32_t value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->SetPropertyInt(strPropertyName, value);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return false;
}

bool NFCKernelModule::SetPropertyInt64(const AFGUID& self, const std::string& strPropertyName, const int64_t value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->SetPropertyInt64(strPropertyName, value);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return false;
}

bool NFCKernelModule::SetPropertyFloat(const AFGUID& self, const std::string& strPropertyName, const float value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->SetPropertyFloat(strPropertyName, value);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return false;
}

bool NFCKernelModule::SetPropertyDouble(const AFGUID& self, const std::string& strPropertyName, const double value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->SetPropertyDouble(strPropertyName, value);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return false;
}

bool NFCKernelModule::SetPropertyString(const AFGUID& self, const std::string& strPropertyName, const std::string& value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->SetPropertyString(strPropertyName, value);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return false;
}

bool NFCKernelModule::SetPropertyObject(const AFGUID& self, const std::string& strPropertyName, const AFGUID& value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->SetPropertyObject(strPropertyName, value);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return false;
}

bool NFCKernelModule::GetPropertyBool(const AFGUID& self, const std::string& strPropertyName)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->GetPropertyBool(strPropertyName);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return NULL_BOOLEAN;
}

int32_t NFCKernelModule::GetPropertyInt(const AFGUID& self, const std::string& strPropertyName)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetPropertyInt(strPropertyName);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return NULL_INT;
}

int64_t NFCKernelModule::GetPropertyInt64(const AFGUID& self, const std::string& strPropertyName)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->GetPropertyInt64(strPropertyName);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return NULL_INT64;
}

float NFCKernelModule::GetPropertyFloat(const AFGUID& self, const std::string& strPropertyName)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->GetPropertyFloat(strPropertyName);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return NULL_FLOAT;
}

double NFCKernelModule::GetPropertyDouble(const AFGUID& self, const std::string& strPropertyName)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetPropertyDouble(strPropertyName);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return NULL_DOUBLE;
}

const std::string& NFCKernelModule::GetPropertyString(const AFGUID& self, const std::string& strPropertyName)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetPropertyString(strPropertyName);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return NULL_STR;
}

const AFGUID& NFCKernelModule::GetPropertyObject(const AFGUID& self, const std::string& strPropertyName)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetPropertyObject(strPropertyName);
    }

    m_pLogModule->LogError(self, strPropertyName, "| There is no object", __FUNCTION__, __LINE__);

    return NULL_GUID;
}

NF_SHARE_PTR<NFIRecord> NFCKernelModule::FindRecord(const AFGUID& self, const std::string& strRecordName)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetRecordManager()->GetElement(strRecordName);
    }

    m_pLogModule->LogError(self, strRecordName, "| There is no object", __FUNCTION__, __LINE__);

    return nullptr;
}

bool NFCKernelModule::ClearRecord(const AFGUID& self, const std::string& strRecordName)
{
    NF_SHARE_PTR<NFIRecord> pRecord =  FindRecord(self, strRecordName);
    if(nullptr != pRecord)
    {
        return pRecord->Clear();
    }

    m_pLogModule->LogError(self, strRecordName, "| There is no record", __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordBool(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol, const bool value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        if (!pObject->SetRecordBool(strRecordName, nRow, nCol, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error for row or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, strRecordName, "| There is no object", __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordBool(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag, const bool value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        if (!pObject->SetRecordBool(strRecordName, nRow, strColTag, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error for row or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, strRecordName, "| There is no object", __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordInt(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol, const int32_t value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        if(!pObject->SetRecordInt(strRecordName, nRow, nCol, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error for row or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, strRecordName, "| There is no object", __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordInt(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag, const int32_t value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        if(!pObject->SetRecordInt(strRecordName, nRow, strColTag, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error for row or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, strRecordName, "| There is no object", __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordInt64(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol, const int64_t value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        if (!pObject->SetRecordInt64(strRecordName, nRow, nCol, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error for row or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, strRecordName, "| There is no object", __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordInt64(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag, const int64_t value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        if (!pObject->SetRecordInt64(strRecordName, nRow, strColTag, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error for row or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, strRecordName, "| There is no object", __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordFloat(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol, const float value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        if (!pObject->SetRecordFloat(strRecordName, nRow, nCol, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error for row or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, strRecordName, "| There is no object", __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordFloat(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag, const float value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        if (!pObject->SetRecordFloat(strRecordName, nRow, strColTag, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error for row or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, strRecordName, "| There is no object", __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordDouble(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol, const double value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        if(!pObject->SetRecordDouble(strRecordName, nRow, nCol, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error SetRecordFloat for row  or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordDouble(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag, const double value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        if(!pObject->SetRecordDouble(strRecordName, nRow, strColTag, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error SetRecordFloat for row  or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordString(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol, const std::string& value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        if(!pObject->SetRecordString(strRecordName, nRow, nCol, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error SetRecordString for row  or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordString(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag, const std::string& value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        if(!pObject->SetRecordString(strRecordName, nRow, strColTag, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error SetRecordObject for row  or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordObject(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol, const AFGUID& value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        if(!pObject->SetRecordObject(strRecordName, nRow, nCol, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error SetRecordObject for row  or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::SetRecordObject(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag, const AFGUID& value)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        if(!pObject->SetRecordObject(strRecordName, nRow, strColTag, value))
        {
            m_pLogModule->LogError(self, strRecordName, "error SetRecordObject for row  or col", __FUNCTION__, __LINE__);
            return false;
        }

        return true;
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return false;
}

bool NFCKernelModule::GetRecordBool(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->GetRecordBool(strRecordName, nRow, nCol);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return 0;
}

bool NFCKernelModule::GetRecordBool(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->GetRecordBool(strRecordName, nRow, strColTag);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return 0;
}

int32_t NFCKernelModule::GetRecordInt(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetRecordInt(strRecordName, nRow, nCol);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return 0;
}

int32_t NFCKernelModule::GetRecordInt(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetRecordInt(strRecordName, nRow, strColTag);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return 0;
}

int64_t NFCKernelModule::GetRecordInt64(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->GetRecordInt64(strRecordName, nRow, nCol);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return 0;
}

int64_t NFCKernelModule::GetRecordInt64(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->GetRecordInt64(strRecordName, nRow, strColTag);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return 0;
}

float NFCKernelModule::GetRecordFloat(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->GetRecordFloat(strRecordName, nRow, nCol);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return 0;
}

float NFCKernelModule::GetRecordFloat(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if (nullptr != pObject)
    {
        return pObject->GetRecordFloat(strRecordName, nRow, strColTag);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return 0;
}

double NFCKernelModule::GetRecordDouble(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetRecordDouble(strRecordName, nRow, nCol);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return 0.0;
}

double NFCKernelModule::GetRecordDouble(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetRecordDouble(strRecordName, nRow, strColTag);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return 0.0;
}

const std::string& NFCKernelModule::GetRecordString(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetRecordString(strRecordName, nRow, nCol);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return NULL_STR;
}

const std::string& NFCKernelModule::GetRecordString(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetRecordString(strRecordName, nRow, strColTag);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return NULL_STR;
}

const AFGUID& NFCKernelModule::GetRecordObject(const AFGUID& self, const std::string& strRecordName, const int nRow, const int nCol)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetRecordObject(strRecordName, nRow, nCol);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return NULL_GUID;
}

const AFGUID& NFCKernelModule::GetRecordObject(const AFGUID& self, const std::string& strRecordName, const int nRow, const std::string& strColTag)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetRecordObject(strRecordName, nRow, strColTag);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return NULL_GUID;
}

bool NFCKernelModule::SwitchScene(const AFGUID& self, const int nTargetSceneID, const int nTargetGroupID, const float fX, const float fY, const float fZ, const float fOrient, const AFIDataList& arg)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr == pObject)
    {
        m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
        return false;
    }

    int32_t nOldSceneID = pObject->GetPropertyInt("SceneID");
    int32_t nOldGroupID = pObject->GetPropertyInt("GroupID");

    NF_SHARE_PTR<NFCSceneInfo> pOldSceneInfo = m_pSceneModule->GetElement(nOldSceneID);
    NF_SHARE_PTR<NFCSceneInfo> pNewSceneInfo = m_pSceneModule->GetElement(nTargetSceneID);
    if(nullptr == pOldSceneInfo)
    {
        m_pLogModule->LogError(self, "no this container", nOldSceneID);
        return false;
    }

    if(nullptr == pNewSceneInfo)
    {
        m_pLogModule->LogError(self, "no this container", nTargetSceneID);
        return false;
    }

    if(nullptr == pNewSceneInfo->GetElement(nTargetGroupID))
    {
        m_pLogModule->LogError(self, "no this group", nTargetGroupID);
        return false;
    }

    pOldSceneInfo->RemoveObjectFromGroup(nOldGroupID, self, true);

    //可以在同一场景切换到不同的层
    if(nTargetSceneID != nOldSceneID)
    {
        //真的切场景
        //先退回到0层，才能修改场景ID
        pObject->SetPropertyInt("GroupID", 0);

        pObject->SetPropertyInt("SceneID", nTargetSceneID);
        //进新的场景0层
    }

    pObject->SetPropertyInt("GroupID", nTargetGroupID);

    pObject->SetPropertyDouble("X", fX);
    pObject->SetPropertyDouble("Y", fY);
    pObject->SetPropertyDouble("Z", fZ);

    pNewSceneInfo->AddObjectToGroup(nTargetGroupID, self, true);

    return true;
}

bool NFCKernelModule::CreateScene(const int nSceneID)
{
    NF_SHARE_PTR<NFCSceneInfo> pSceneInfo = m_pSceneModule->GetElement(nSceneID);
    if(nullptr != pSceneInfo)
    {
        return false;
    }

    //容器nSceneIndex
    pSceneInfo = NF_SHARE_PTR<NFCSceneInfo>(NF_NEW NFCSceneInfo(nSceneID));
    if (nullptr == pSceneInfo)
    {
        return false;
    }

    m_pSceneModule->AddElement(nSceneID, pSceneInfo);

    //默认分组0
    NF_SHARE_PTR<NFCSceneGroupInfo> pGroupInfo = NF_SHARE_PTR<NFCSceneGroupInfo>(NF_NEW NFCSceneGroupInfo(nSceneID, 0));
    if (nullptr == pGroupInfo)
    {
        return false;
    }

    pSceneInfo->AddElement(0, pGroupInfo);
    m_pLogModule->LogInfo(NULL_GUID, "Create scene success, groupId:0, scene id:", nSceneID, __FUNCTION__, __LINE__);
    return true;
}

bool NFCKernelModule::DestroyScene(const int nSceneID)
{
    return m_pSceneModule->RemoveElement(nSceneID);
}

int NFCKernelModule::GetOnLineCount()
{
    int nCount = 0;
    NF_SHARE_PTR<NFCSceneInfo> pSceneInfo = m_pSceneModule->First();
    while(nullptr != pSceneInfo)
    {
        NF_SHARE_PTR<NFCSceneGroupInfo> pGroupInfo = pSceneInfo->First();
        while(nullptr != pGroupInfo)
        {
            nCount += pGroupInfo->mxPlayerList.Count();
            pGroupInfo = pSceneInfo->Next();
        }

        pSceneInfo = m_pSceneModule->Next();
    }

    return nCount;
}

int NFCKernelModule::GetMaxOnLineCount()
{
    // test count 5000
    // and it should be define in a config file
    return 10000;
}

int NFCKernelModule::GetSceneOnLineCount(const int nSceneID)
{
    int nCount = 0;

    NF_SHARE_PTR<NFCSceneInfo> pSceneInfo = m_pSceneModule->GetElement(nSceneID);
    if (nullptr == pSceneInfo)
    {
        return nCount;
    }

    NF_SHARE_PTR<NFCSceneGroupInfo> pGroupInfo = pSceneInfo->First();
    while (nullptr != pGroupInfo)
    {
        nCount += pGroupInfo->mxPlayerList.Count();
        pGroupInfo = pSceneInfo->Next();
    }

    return nCount;
}

int NFCKernelModule::GetSceneOnLineCount(const int nSceneID, const int nGroupID)
{
    int nCount = 0;

    NF_SHARE_PTR<NFCSceneInfo> pSceneInfo = m_pSceneModule->GetElement(nSceneID);
    if (nullptr == pSceneInfo)
    {
        return nCount;
    }

    NF_SHARE_PTR<NFCSceneGroupInfo> pGroupInfo = pSceneInfo->GetElement(nGroupID);
    if (nullptr != pGroupInfo)
    {
        nCount = pGroupInfo->mxPlayerList.Count();
    }

    return nCount;
}

//int NFCKernelModule::GetSceneOnLineList( const int nSceneID, type, AFDataList& var )
int NFCKernelModule::GetSceneOnLineList(const int nSceneID, AFIDataList& var)
{
    NF_SHARE_PTR<NFCSceneInfo> pSceneInfo = m_pSceneModule->GetElement(nSceneID);
    if (nullptr == pSceneInfo)
    {
        return 0;
    }

    NF_SHARE_PTR<NFCSceneGroupInfo> pGroupInfo = pSceneInfo->First();
    while(nullptr != pGroupInfo)
    {
        AFGUID ident;

        NF_SHARE_PTR<int> pRet  = pGroupInfo->mxPlayerList.First(ident);
        while(!ident.IsNull())
        {
            var.AddObject(ident);

            ident = NULL_GUID;
            pRet = pGroupInfo->mxPlayerList.Next(ident);
        }

        pGroupInfo = pSceneInfo->Next();
    }

    return var.GetCount();
}

int NFCKernelModule::RequestGroupScene(const int nSceneID)
{
    NF_SHARE_PTR<NFCSceneInfo> pSceneInfo = m_pSceneModule->GetElement(nSceneID);
    if (nullptr == pSceneInfo)
    {
        return -1;
    }

    int nNewGroupID = pSceneInfo->NewGroupID();
    if (NULL == pSceneInfo->GetElement(nNewGroupID))
    {
        return -1;
    }
    NF_SHARE_PTR<NFCSceneGroupInfo> pGroupInfo(NF_NEW NFCSceneGroupInfo(nSceneID, nNewGroupID, pSceneInfo->GetWidth()));
    if (NULL == pGroupInfo)
    {
        return -1;
    }

    pSceneInfo->AddElement(nNewGroupID, pGroupInfo);
    return nNewGroupID;
}

bool NFCKernelModule::ReleaseGroupScene(const int nSceneID, const int nGroupID)
{
    NF_SHARE_PTR<NFCSceneInfo> pSceneInfo = m_pSceneModule->GetElement(nSceneID);
    if (nullptr == pSceneInfo)
    {
        return false;
    }
     
    if (nullptr == pSceneInfo->GetElement(nGroupID))
    {
        return false;
    }

    AFXDataList listObject;
    if (GetGroupObjectList(nSceneID, nGroupID, listObject))
    {
        for (int i = 0; i < listObject.GetCount(); ++i)
        {
            AFGUID ident = listObject.Object(i);
            DestroyObject(ident);
        }
    }

    pSceneInfo->RemoveElement(nGroupID);

    return true;
}

bool NFCKernelModule::ExitGroupScene(const int nSceneID, const int nGroupID)
{
    NF_SHARE_PTR<NFCSceneInfo> pSceneInfo = m_pSceneModule->GetElement(nSceneID);
    if(nullptr != pSceneInfo)
    {
        NF_SHARE_PTR<NFCSceneGroupInfo> pGroupInfo = pSceneInfo->GetElement(nGroupID);
        return (nullptr != pGroupInfo);
    }

    return false;
}

bool NFCKernelModule::GetGroupObjectList(const int nSceneID, const int nGroupID, AFIDataList& list)
{
    NF_SHARE_PTR<NFCSceneInfo> pSceneInfo = m_pSceneModule->GetElement(nSceneID);
    if (nullptr == pSceneInfo)
    {
        return false;
    }

    NF_SHARE_PTR<NFCSceneGroupInfo> pGroupInfo = pSceneInfo->GetElement(nGroupID);
    if (nullptr == pGroupInfo)
    {
        return false;
    }

    AFGUID ident = NULL_GUID;
    NF_SHARE_PTR<int> pRet = pGroupInfo->mxPlayerList.First(ident);
    while (!ident.IsNull())
    {
        list.AddObject(ident);

        ident = NULL_GUID;
        pRet = pGroupInfo->mxPlayerList.Next(ident);
    }

    pRet = pGroupInfo->mxOtherList.First(ident);
    while (!ident.IsNull())
    {
        list.AddObject(ident);

        ident = NULL_GUID;
        pRet = pGroupInfo->mxOtherList.Next(ident);
    }

    return true;
}

bool NFCKernelModule::LogStack()
{
#if NF_PLATFORM == NF_PLATFORM_WIN
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
#endif

#if NF_PLATFORM == NF_PLATFORM_WIN
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
#endif // NF_PLATFORM

    return true;
}

bool NFCKernelModule::LogInfo(const AFGUID& ident)
{
    //看是容器还是普通对象，容器则打印所有对象
    NF_SHARE_PTR<NFIObject> pObject = GetObject(ident);
    if (nullptr == pObject)
    {
        m_pLogModule->LogError(ident, "Cannot find this object", NULL_STR, __FUNCTION__, __LINE__);
        return false;
    }

    if(IsContainer(ident))
    {
        int nSceneID = GetPropertyInt(ident, "SceneID");

        m_pLogModule->LogInfo(ident, "//----------child object list-------- SceneID = ", nSceneID);

        AFXDataList valObjectList;
        int nCount = GetSceneOnLineList(nSceneID, valObjectList);
        for(int i  = 0; i < nCount; i++)
        {
            AFGUID targetIdent = valObjectList.Object(i);
            LogInfo(targetIdent);
        }
    }
    else
    {
        m_pLogModule->LogInfo(ident, "//---------printf object start-------- ", "");


        m_pLogModule->LogInfo(ident, "//---------printf object end-------- ", "");
    }

    return true;
}

int NFCKernelModule::OnPropertyCommonEvent(const AFGUID& self, const std::string& strPropertyName, const AFIData& oldVar, const AFIData& newVar)
{
    if(IsContainer(self))
    {
        return 0;
    }

    std::list<PROPERTY_EVENT_FUNCTOR_PTR>::iterator it = mtCommonPropertyCallBackList.begin();
    for(it; it != mtCommonPropertyCallBackList.end(); it++)
    {
        (**it)(self, strPropertyName, oldVar, newVar);
    }

    return 0;
}

NF_SHARE_PTR<NFIObject> NFCKernelModule::GetObject(const AFGUID& ident)
{
    return GetElement(ident);
}

bool NFCKernelModule::IsContainer(const AFGUID& self)
{
    NF_SHARE_PTR<NFIObject> pObject = GetObject(self);
    if(nullptr != pObject)
    {
        return (pObject->GetPropertyInt("GroupID") < 0);
    }

    m_pLogModule->LogError(self, "There is no object", NULL_STR, __FUNCTION__, __LINE__);
    return false;
}

int NFCKernelModule::GetObjectByProperty(const int nSceneID, const std::string& strPropertyName, const AFIDataList& valueArg, AFIDataList& list)
{
    AFXDataList varObjectList;
    GetSceneOnLineList(nSceneID, varObjectList);
    int nWorldCount = varObjectList.GetCount();
    for(int i = 0; i < nWorldCount; i++)
    {
        AFGUID ident = varObjectList.Object(i);
        if (!FindProperty(ident, strPropertyName))
        {
            continue;
        }

        int eType = valueArg.GetType(0);
        switch(eType)
        {
        case DT_BOOLEAN:
            {
                bool bValue = GetPropertyBool(ident, strPropertyName.c_str());
                if (valueArg.Bool(0) == bValue)
                {
                    list.AddObject(ident);
                }
            }
            break;
        case DT_INT:
            {
                int nValue = GetPropertyInt(ident, strPropertyName.c_str());
                if(valueArg.Int(0) == nValue)
                {
                    list.AddObject(ident);
                }
            }
            break;
        case DT_INT64:
            {
                int64_t nValue = GetPropertyInt64(ident, strPropertyName.c_str());
                if (valueArg.Int64(0) == nValue)
                {
                    list.AddObject(ident);
                }
            }
            break;
        case DT_FLOAT:
            {
                float fValue = GetPropertyFloat(ident, strPropertyName.c_str());
                float fCompareValue = valueArg.Float(0);
                if (fValue == fCompareValue)
                {
                    list.AddObject(ident);
                }
            }
            break;
        case DT_DOUBLE:
            {
                double dValue = GetPropertyDouble(ident, strPropertyName.c_str());
                double dCompareValue = valueArg.Double(0);
                if(dValue == dCompareValue)
                {
                    list.AddObject(ident);
                }
            }
            break;
        case DT_STRING:
            {
                std::string strValue = GetPropertyString(ident, strPropertyName.c_str());
                std::string strCompareValue = valueArg.String(0);
                if(strValue == strCompareValue)
                {
                    list.AddObject(ident);
                }
            }
            break;
        case DT_OBJECT:
            {
                AFGUID identObject = GetPropertyObject(ident, strPropertyName.c_str());
                if(valueArg.Object(0) == identObject)
                {
                    list.AddObject(ident);
                }
            }
            break;
        default:
            break;
        }
    }

    return list.GetCount();
}

bool NFCKernelModule::ExistContainer(const int nSceneID)
{
    NF_SHARE_PTR<NFCSceneInfo> pSceneInfo = m_pSceneModule->GetElement(nSceneID);
    return (nullptr != pSceneInfo);
}

bool NFCKernelModule::DestroySelf(const AFGUID& self)
{
    mtDeleteSelfList.push_back(self);
    return true;
}

int NFCKernelModule::OnRecordCommonEvent(const AFGUID& self, const RECORD_EVENT_DATA& xEventData, const AFIData& oldVar, const AFIData& newVar)
{
    if(IsContainer(self))
    {
        return 0;
    }

    std::list<RECORD_EVENT_FUNCTOR_PTR>::iterator it = mtCommonRecordCallBackList.begin();
    for(it; it != mtCommonRecordCallBackList.end(); it++)
    {
        (**it)(self, xEventData, oldVar, newVar);
    }

    return 0;
}

int NFCKernelModule::OnClassCommonEvent(const AFGUID& self, const std::string& strClassName, const CLASS_OBJECT_EVENT eClassEvent, const AFIDataList& var)
{
    if(IsContainer(self))
    {
        return 0;
    }

    std::list<CLASS_EVENT_FUNCTOR_PTR>::iterator it = mtCommonClassCallBackList.begin();
    for(it; it != mtCommonClassCallBackList.end(); it++)
    {
        (**it)(self, strClassName, eClassEvent, var);
    }

    return 0;
}

bool NFCKernelModule::RegisterCommonClassEvent(const CLASS_EVENT_FUNCTOR_PTR& cb)
{
    mtCommonClassCallBackList.push_back(cb);
    return true;
}

bool NFCKernelModule::RegisterCommonPropertyEvent(const PROPERTY_EVENT_FUNCTOR_PTR& cb)
{
    mtCommonPropertyCallBackList.push_back(cb);
    return true;
}

bool NFCKernelModule::RegisterCommonRecordEvent(const RECORD_EVENT_FUNCTOR_PTR& cb)
{
    mtCommonRecordCallBackList.push_back(cb);
    return true;
}

bool NFCKernelModule::LogSelfInfo(const AFGUID& ident)
{
    return false;
}

bool NFCKernelModule::AfterInit()
{
    NF_SHARE_PTR<NFIClass> pClass = m_pClassModule->First();
    while(nullptr != pClass)
    {
        NFIKernelModule::AddClassCallBack(pClass->GetClassName(), this, &NFCKernelModule::OnClassCommonEvent);
        pClass = m_pClassModule->Next();
    }

    return true;
}

bool NFCKernelModule::DestroyAll()
{
    NF_SHARE_PTR<NFIObject> pObject = First();
    while(nullptr != pObject)
    {
        mtDeleteSelfList.push_back(pObject->Self());

        pObject = Next();
    }

    // 为了释放object
    Execute();

    m_pSceneModule->ClearAll();

    mvRandom.clear();
    mtCommonClassCallBackList.clear();
    mtCommonPropertyCallBackList.clear();
    mtCommonRecordCallBackList.clear();

    return true;
}

bool NFCKernelModule::BeforeShut()
{
    DestroyAll();
    return true;
}

void NFCKernelModule::Random(int nStart, int nEnd, int nCount, AFIDataList& valueList)
{
    if(mnRandomPos + nCount >= mvRandom.size())
    {
        mnRandomPos = 0;
    }

    for(int i = mnRandomPos; i < mnRandomPos + nCount; i++)
    {
        float fRanValue = mvRandom.at(i);
        int nValue = (nEnd - nStart) * fRanValue + nStart;
        valueList.AddInt64((NFINT64)nValue);
    }

    mnRandomPos += nCount;
}

bool NFCKernelModule::AddEventCallBack(const AFGUID& self, const int nEventID, const EVENT_PROCESS_FUNCTOR_PTR& cb)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetEventManager()->AddEventCallBack(nEventID, cb);
    }

    return false;
}

bool NFCKernelModule::AddClassCallBack(const std::string& strClassName, const CLASS_EVENT_FUNCTOR_PTR& cb)
{
    return m_pClassModule->AddClassCallBack(strClassName, cb);
}

void NFCKernelModule::ProcessMemFree()
{
    if((nLastTime + 30) > pPluginManager->GetNowTime())
    {
        return;
    }

    nLastTime = pPluginManager->GetNowTime();

    NFCMemManger::GetSingletonPtr()->FreeMem();
}

bool NFCKernelModule::DoEvent(const AFGUID& self, const std::string& strClassName, CLASS_OBJECT_EVENT eEvent, const AFIDataList& valueList)
{
    return m_pClassModule->DoEvent(self, strClassName, eEvent, valueList);
}

bool NFCKernelModule::DoEvent(const AFGUID& self, const int nEventID, const AFIDataList& valueList)
{
    NF_SHARE_PTR<NFIObject> pObject = GetElement(self);
    if(nullptr != pObject)
    {
        return pObject->GetEventManager()->DoEvent(nEventID, valueList);
    }

    return false;
}
