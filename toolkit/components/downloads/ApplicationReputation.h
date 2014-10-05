/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ApplicationReputation_h__
#define ApplicationReputation_h__

#include "nsIApplicationReputation.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsISupports.h"

#include "nsCOMPtr.h"
#include "nsString.h"

class nsIRequest;
class nsIUrlClassifierDBService;
class nsIScriptSecurityManager;
class PendingLookup;

class ApplicationReputationService MOZ_FINAL :
  public nsIApplicationReputationService {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPLICATIONREPUTATIONSERVICE

public:
  static ApplicationReputationService* GetSingleton();

private:
  /**
   * Global singleton object for holding this factory service.
   */
  static ApplicationReputationService* gApplicationReputationService;
  /**
   * Keeps track of services used to query the local database of URLs.
   */
  nsCOMPtr<nsIUrlClassifierDBService> mDBService;
  nsCOMPtr<nsIScriptSecurityManager> mSecurityManager;
  /**
   * This is a singleton, so disallow construction.
   */
  ApplicationReputationService();
  ~ApplicationReputationService();
  /**
   * Wrapper function for QueryReputation that makes it easier to ensure the
   * callback is called.
   */
  nsresult QueryReputationInternal(nsIApplicationReputationQuery* aQuery,
                                   nsIApplicationReputationCallback* aCallback);
};
#endif /* ApplicationReputation_h__ */
