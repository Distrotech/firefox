/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/ModuleUtils.h"
#include "nsAppStartup.h"
#include "nsUserInfo.h"
#include "nsToolkitCompsCID.h"
#include "nsFindService.h"
#if defined(USE_MOZ_UPDATER)
#include "nsUpdateDriver.h"
#endif

#if defined(XP_WIN) && !defined(MOZ_DISABLE_PARENTAL_CONTROLS)
#include "nsParentalControlsServiceWin.h"
#endif

#include "nsAlertsService.h"

#include "nsDownloadManager.h"
#include "DownloadPlatform.h"
#include "nsDownloadProxy.h"
#include "nsCharsetMenu.h"
#include "rdf.h"

#include "nsTypeAheadFind.h"

#ifdef MOZ_URL_CLASSIFIER
#include "ApplicationReputation.h"
#include "nsUrlClassifierDBService.h"
#include "nsUrlClassifierStreamUpdater.h"
#include "nsUrlClassifierUtils.h"
#include "nsUrlClassifierPrefixSet.h"
#endif

#include "nsBrowserStatusFilter.h"
#include "mozilla/FinalizationWitnessService.h"

using namespace mozilla;

/////////////////////////////////////////////////////////////////////////////

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsAppStartup, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUserInfo)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFindService)

#if defined(XP_WIN) && !defined(MOZ_DISABLE_PARENTAL_CONTROLS)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsParentalControlsServiceWin)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsAlertsService)

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsDownloadManager,
                                         nsDownloadManager::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(DownloadPlatform)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDownloadProxy)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsTypeAheadFind)

#ifdef MOZ_URL_CLASSIFIER
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(ApplicationReputationService,
                                         ApplicationReputationService::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUrlClassifierPrefixSet)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUrlClassifierStreamUpdater)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsUrlClassifierUtils, Init)

static nsresult
nsUrlClassifierDBServiceConstructor(nsISupports *aOuter, REFNSIID aIID,
                                    void **aResult)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_NO_AGGREGATION(aOuter);

    nsUrlClassifierDBService *inst = nsUrlClassifierDBService::GetInstance(&rv);
    if (nullptr == inst) {
        return rv;
    }
    /* NS_ADDREF(inst); */
    rv = inst->QueryInterface(aIID, aResult);
    NS_RELEASE(inst);

    return rv;
}
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsBrowserStatusFilter)
#if defined(USE_MOZ_UPDATER)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUpdateProcessor)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(FinalizationWitnessService)

NS_DEFINE_NAMED_CID(NS_TOOLKIT_APPSTARTUP_CID);
NS_DEFINE_NAMED_CID(NS_USERINFO_CID);
NS_DEFINE_NAMED_CID(NS_ALERTSSERVICE_CID);
#if defined(XP_WIN) && !defined(MOZ_DISABLE_PARENTAL_CONTROLS)
NS_DEFINE_NAMED_CID(NS_PARENTALCONTROLSSERVICE_CID);
#endif
NS_DEFINE_NAMED_CID(NS_DOWNLOADMANAGER_CID);
NS_DEFINE_NAMED_CID(NS_DOWNLOADPLATFORM_CID);
NS_DEFINE_NAMED_CID(NS_DOWNLOAD_CID);
NS_DEFINE_NAMED_CID(NS_FIND_SERVICE_CID);
NS_DEFINE_NAMED_CID(NS_TYPEAHEADFIND_CID);
#ifdef MOZ_URL_CLASSIFIER
NS_DEFINE_NAMED_CID(NS_APPLICATION_REPUTATION_SERVICE_CID);
NS_DEFINE_NAMED_CID(NS_URLCLASSIFIERPREFIXSET_CID);
NS_DEFINE_NAMED_CID(NS_URLCLASSIFIERDBSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_URLCLASSIFIERSTREAMUPDATER_CID);
NS_DEFINE_NAMED_CID(NS_URLCLASSIFIERUTILS_CID);
#endif
NS_DEFINE_NAMED_CID(NS_BROWSERSTATUSFILTER_CID);
NS_DEFINE_NAMED_CID(NS_CHARSETMENU_CID);
#if defined(USE_MOZ_UPDATER)
NS_DEFINE_NAMED_CID(NS_UPDATEPROCESSOR_CID);
#endif
NS_DEFINE_NAMED_CID(FINALIZATIONWITNESSSERVICE_CID);

static const Module::CIDEntry kToolkitCIDs[] = {
  { &kNS_TOOLKIT_APPSTARTUP_CID, false, nullptr, nsAppStartupConstructor },
  { &kNS_USERINFO_CID, false, nullptr, nsUserInfoConstructor },
  { &kNS_ALERTSSERVICE_CID, false, nullptr, nsAlertsServiceConstructor },
#if defined(XP_WIN) && !defined(MOZ_DISABLE_PARENTAL_CONTROLS)
  { &kNS_PARENTALCONTROLSSERVICE_CID, false, nullptr, nsParentalControlsServiceWinConstructor },
#endif
  { &kNS_DOWNLOADMANAGER_CID, false, nullptr, nsDownloadManagerConstructor },
  { &kNS_DOWNLOADPLATFORM_CID, false, nullptr, DownloadPlatformConstructor },
  { &kNS_DOWNLOAD_CID, false, nullptr, nsDownloadProxyConstructor },
  { &kNS_FIND_SERVICE_CID, false, nullptr, nsFindServiceConstructor },
  { &kNS_TYPEAHEADFIND_CID, false, nullptr, nsTypeAheadFindConstructor },
#ifdef MOZ_URL_CLASSIFIER
  { &kNS_APPLICATION_REPUTATION_SERVICE_CID, false, nullptr, ApplicationReputationServiceConstructor },
  { &kNS_URLCLASSIFIERPREFIXSET_CID, false, nullptr, nsUrlClassifierPrefixSetConstructor },
  { &kNS_URLCLASSIFIERDBSERVICE_CID, false, nullptr, nsUrlClassifierDBServiceConstructor },
  { &kNS_URLCLASSIFIERSTREAMUPDATER_CID, false, nullptr, nsUrlClassifierStreamUpdaterConstructor },
  { &kNS_URLCLASSIFIERUTILS_CID, false, nullptr, nsUrlClassifierUtilsConstructor },
#endif
  { &kNS_BROWSERSTATUSFILTER_CID, false, nullptr, nsBrowserStatusFilterConstructor },
  { &kNS_CHARSETMENU_CID, false, nullptr, NS_NewCharsetMenu },
#if defined(USE_MOZ_UPDATER)
  { &kNS_UPDATEPROCESSOR_CID, false, nullptr, nsUpdateProcessorConstructor },
#endif
  { &kFINALIZATIONWITNESSSERVICE_CID, false, nullptr, FinalizationWitnessServiceConstructor },
  { nullptr }
};

static const Module::ContractIDEntry kToolkitContracts[] = {
  { NS_APPSTARTUP_CONTRACTID, &kNS_TOOLKIT_APPSTARTUP_CID },
  { NS_USERINFO_CONTRACTID, &kNS_USERINFO_CID },
  { NS_ALERTSERVICE_CONTRACTID, &kNS_ALERTSSERVICE_CID },
#if defined(XP_WIN) && !defined(MOZ_DISABLE_PARENTAL_CONTROLS)
  { NS_PARENTALCONTROLSSERVICE_CONTRACTID, &kNS_PARENTALCONTROLSSERVICE_CID },
#endif
  { NS_DOWNLOADMANAGER_CONTRACTID, &kNS_DOWNLOADMANAGER_CID },
  { NS_DOWNLOADPLATFORM_CONTRACTID, &kNS_DOWNLOADPLATFORM_CID },
  { NS_TRANSFER_CONTRACTID, &kNS_DOWNLOAD_CID },
  { NS_FIND_SERVICE_CONTRACTID, &kNS_FIND_SERVICE_CID },
  { NS_TYPEAHEADFIND_CONTRACTID, &kNS_TYPEAHEADFIND_CID },
#ifdef MOZ_URL_CLASSIFIER
  { NS_APPLICATION_REPUTATION_SERVICE_CONTRACTID, &kNS_APPLICATION_REPUTATION_SERVICE_CID },
  { NS_URLCLASSIFIERPREFIXSET_CONTRACTID, &kNS_URLCLASSIFIERPREFIXSET_CID },
  { NS_URLCLASSIFIERDBSERVICE_CONTRACTID, &kNS_URLCLASSIFIERDBSERVICE_CID },
  { NS_URICLASSIFIERSERVICE_CONTRACTID, &kNS_URLCLASSIFIERDBSERVICE_CID },
  { NS_URLCLASSIFIERSTREAMUPDATER_CONTRACTID, &kNS_URLCLASSIFIERSTREAMUPDATER_CID },
  { NS_URLCLASSIFIERUTILS_CONTRACTID, &kNS_URLCLASSIFIERUTILS_CID },
#endif
  { NS_BROWSERSTATUSFILTER_CONTRACTID, &kNS_BROWSERSTATUSFILTER_CID },
  { NS_RDF_DATASOURCE_CONTRACTID_PREFIX NS_CHARSETMENU_PID, &kNS_CHARSETMENU_CID },
#if defined(USE_MOZ_UPDATER)
  { NS_UPDATEPROCESSOR_CONTRACTID, &kNS_UPDATEPROCESSOR_CID },
#endif
  { FINALIZATIONWITNESSSERVICE_CONTRACTID, &kFINALIZATIONWITNESSSERVICE_CID },
  { nullptr }
};

static const Module kToolkitModule = {
  Module::kVersion,
  kToolkitCIDs,
  kToolkitContracts
};

NSMODULE_DEFN(nsToolkitCompsModule) = &kToolkitModule;
