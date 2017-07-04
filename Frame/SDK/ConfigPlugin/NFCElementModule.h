// -------------------------------------------------------------------------
//    @FileName			:    NFCElementModule.h
//    @Author           :    Ark Game Tech
//    @Date             :    2012-12-15
//    @Module           :    NFCElementModule
//
// -------------------------------------------------------------------------

#ifndef NFC_ELEMENT_MODULE_H
#define NFC_ELEMENT_MODULE_H

#include <map>
#include <string>
#include <iostream>
#include "RapidXML/rapidxml.hpp"
#include "RapidXML/rapidxml_iterators.hpp"
#include "RapidXML/rapidxml_print.hpp"
#include "RapidXML/rapidxml_utils.hpp"
#include "SDK/Core/NFMap.h"
#include "SDK/Core/NFList.h"
#include "SDK/Core/AFCDataList.h"
#include "SDK/Core/NFCRecord.h"
#include "SDK/Core/NFCPropertyManager.h"
#include "SDK/Core/NFCRecordManager.h"
//#include "SDK/Core/NFCComponentManager.h"
#include "SDK/Interface/NFIElementModule.h"
#include "SDK/Interface/NFIClassModule.h"

class NFCClass;

/**
 * @class   ElementConfigInfo
 *
 * @brief   Information about the element configuration.
 *
 * @author  flyicegood
 * @date    2016/11/22
 */

class ElementConfigInfo
{
public:
    ElementConfigInfo()
    {
        m_pPropertyManager = NF_SHARE_PTR<NFIPropertyManager>(NF_NEW NFCPropertyManager(NULL_GUID));
        m_pRecordManager = NF_SHARE_PTR<NFIRecordManager>(NF_NEW NFCRecordManager(NULL_GUID));
        //m_pComponentManager = NF_SHARE_PTR<NFIComponentManager>(NF_NEW NFCComponentManager(NULL_GUID));
    }

    virtual ~ElementConfigInfo()
    {
    }

    NF_SHARE_PTR<NFIPropertyManager> GetPropertyManager()
    {
        return m_pPropertyManager;
    }

    NF_SHARE_PTR<NFIRecordManager> GetRecordManager()
    {
        return m_pRecordManager;
    }

    //NF_SHARE_PTR<NFIComponentManager> GetComponentManager()
    //{
    //    return m_pComponentManager;
    //}
protected:

    //std::string mstrConfigID;

    NF_SHARE_PTR<NFIPropertyManager> m_pPropertyManager;
    NF_SHARE_PTR<NFIRecordManager> m_pRecordManager;
    //NF_SHARE_PTR<NFIComponentManager> m_pComponentManager;
};

/**
 * @class   NFCElementModule
 *
 * @brief   A nfc element module.
 *
 * @author  flyicegood
 * @date    2016/11/22
 */

class NFCElementModule
    : public NFIElementModule,
      NFMapEx<std::string, ElementConfigInfo>
{
public:
    NFCElementModule(NFIPluginManager* p);
    virtual ~NFCElementModule();

    virtual bool Init();
    virtual bool Shut();

    virtual bool AfterInit();
    virtual bool BeforeShut();
    virtual bool Execute();

    virtual bool Load();
    virtual bool Save();
    virtual bool Clear();

    virtual bool ExistElement(const std::string& strConfigName);
    virtual bool ExistElement(const std::string& strClassName, const std::string& strConfigName);

    virtual NF_SHARE_PTR<NFIPropertyManager> GetPropertyManager(const std::string& strConfigName);
    virtual NF_SHARE_PTR<NFIRecordManager> GetRecordManager(const std::string& strConfigName);
    //virtual NF_SHARE_PTR<NFIComponentManager> GetComponentManager(const std::string& strConfigName);

    virtual NFINT64 GetPropertyInt(const std::string& strConfigName, const std::string& strPropertyName);
    virtual double GetPropertyFloat(const std::string& strConfigName, const std::string& strPropertyName);
    virtual const std::string& GetPropertyString(const std::string& strConfigName, const std::string& strPropertyName);

protected:
    virtual NF_SHARE_PTR<NFIProperty> GetProperty(const std::string& strConfigName, const std::string& strPropertyName);

    virtual bool Load(rapidxml::xml_node<>* attrNode, NF_SHARE_PTR<NFIClass> pLogicClass);

    virtual bool LegalNumber(const char* str);
protected:
    NFIClassModule* m_pClassModule;
    bool mbLoaded;
};

#endif
