// -------------------------------------------------------------------------
//    @FileName         :    NFCClassModule.h
//    @Author           :    Ark Game Tech
//    @Date             :    2012-12-15
//    @Module           :    NFCClassModule
//
// -------------------------------------------------------------------------

#ifndef NFC_LOGICCLASS_MODULE_H
#define NFC_LOGICCLASS_MODULE_H

#include <string>
#include <map>
#include <iostream>
#include "NFCElementModule.h"
#include "RapidXML/rapidxml.hpp"
#include "SDK/Core/NFMap.h"
#include "SDK/Core/NFList.h"
#include "SDK/Core/AFCDataList.h"
#include "SDK/Core/NFCRecord.h"
#include "SDK/Core/NFList.h"
#include "SDK/Core/NFCPropertyManager.h"
#include "SDK/Core/NFCRecordManager.h"
#include "SDK/Interface/NFIClassModule.h"
#include "SDK/Interface/NFIElementModule.h"
#include "SDK/Interface/NFIPluginManager.h"

/**
 * @class   NFCClass
 *
 * @brief   A nfc class.
 *
 * @author  flyicegood
 * @date    2016/11/22
 */

class NFCClass : public NFIClass
{
public:

    NFCClass(const std::string& strClassName)
    {
        m_pParentClass = NULL;
        mstrClassName = strClassName;

        m_pPropertyManager = NF_SHARE_PTR<NFIPropertyManager>(NF_NEW NFCPropertyManager(NULL_GUID));
        m_pRecordManager = NF_SHARE_PTR<NFIRecordManager>(NF_NEW NFCRecordManager(NULL_GUID));
        //m_pComponentManager = NF_SHARE_PTR<NFIComponentManager>(NF_NEW NFCComponentManager(NULL_GUID));
    }

    virtual ~NFCClass()
    {
        ClearAll();
    }

    virtual NF_SHARE_PTR<NFIPropertyManager> GetPropertyManager()
    {
        return m_pPropertyManager;
    }

    virtual NF_SHARE_PTR<NFIRecordManager> GetRecordManager()
    {
        return m_pRecordManager;
    }

    //virtual NF_SHARE_PTR<NFIComponentManager> GetComponentManager()
    //{
    //    return m_pComponentManager;
    //}

    virtual bool AddClassCallBack(const CLASS_EVENT_FUNCTOR_PTR& cb)
    {
        return mxClassEventInfo.Add(cb);
    }

    virtual bool DoEvent(const AFGUID& objectID, const CLASS_OBJECT_EVENT eClassEvent, const AFIDataList& valueList)
    {
        CLASS_EVENT_FUNCTOR_PTR cb;
        bool bRet = mxClassEventInfo.First(cb);
        while(bRet)
        {
            (*cb)(objectID, mstrClassName, eClassEvent,  valueList);

            bRet = mxClassEventInfo.Next(cb);
        }

        return true;
    }

    void SetParent(NF_SHARE_PTR<NFIClass> pClass)
    {
        m_pParentClass = pClass;
    }

    NF_SHARE_PTR<NFIClass> GetParent()
    {
        return m_pParentClass;
    }

    void SetTypeName(const char* strType)
    {
        mstrType = strType;
    }

    const std::string& GetTypeName()
    {
        return mstrType;
    }

    const std::string& GetClassName()
    {
        return mstrClassName;
    }

    const bool AddConfigName(std::string& strConfigName)
    {
        mlConfigList.Add(strConfigName);

        return true;
    }

    NFList<std::string>& GetConfigNameList()
    {
        return mlConfigList;
    }

    void SetInstancePath(const std::string& strPath)
    {
        mstrClassInstancePath = strPath;
    }

    const std::string& GetInstancePath()
    {
        return mstrClassInstancePath;
    }

private:
    NF_SHARE_PTR<NFIPropertyManager> m_pPropertyManager;
    NF_SHARE_PTR<NFIRecordManager> m_pRecordManager;
    //NF_SHARE_PTR<NFIComponentManager> m_pComponentManager;

    NF_SHARE_PTR<NFIClass> m_pParentClass;
    std::string mstrType;
    std::string mstrClassName;
    std::string mstrClassInstancePath;

    NFList<std::string> mlConfigList;

    NFList<CLASS_EVENT_FUNCTOR_PTR> mxClassEventInfo;
};

class NFCClassModule
    : public NFIClassModule
{
public:
    NFCClassModule(NFIPluginManager* p);
    virtual ~NFCClassModule();

    virtual bool Init();
    virtual bool Shut();

    virtual bool Load();
    virtual bool Save();
    virtual bool Clear();

    virtual bool AddClassCallBack(const std::string& strClassName, const CLASS_EVENT_FUNCTOR_PTR& cb);
    virtual bool DoEvent(const AFGUID& objectID, const std::string& strClassName, const CLASS_OBJECT_EVENT eClassEvent, const AFIDataList& valueList);

    virtual NF_SHARE_PTR<NFIPropertyManager> GetClassPropertyManager(const std::string& strClassName);
    virtual NF_SHARE_PTR<NFIRecordManager> GetClassRecordManager(const std::string& strClassName);
    //virtual NF_SHARE_PTR<NFIComponentManager> GetClassComponentManager(const std::string& strClassName);

    virtual bool AddClass(const std::string& strClassName, const std::string& strParentName);

protected:
    virtual int ComputerType(const char* pstrTypeName, AFIData& var);
    virtual bool AddPropertys(rapidxml::xml_node<>* pPropertyRootNode, NF_SHARE_PTR<NFIClass> pClass);
    virtual bool AddRecords(rapidxml::xml_node<>* pRecordRootNode, NF_SHARE_PTR<NFIClass> pClass);
    virtual bool AddComponents(rapidxml::xml_node<>* pRecordRootNode, NF_SHARE_PTR<NFIClass> pClass);
    virtual bool AddClassInclude(const char* pstrClassFilePath, NF_SHARE_PTR<NFIClass> pClass);
    virtual bool AddClass(const char* pstrClassFilePath, NF_SHARE_PTR<NFIClass> pClass);

    //加载此节点的类
    virtual bool Load(rapidxml::xml_node<>* attrNode, NF_SHARE_PTR<NFIClass> pParentClass);

protected:
    NFIElementModule* m_pElementModule;

    std::string msConfigFileName;
};

#endif
