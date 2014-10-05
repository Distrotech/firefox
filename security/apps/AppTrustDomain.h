/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_psm_AppsTrustDomain_h
#define mozilla_psm_AppsTrustDomain_h

#include "insanity/pkixtypes.h"
#include "nsDebug.h"
#include "nsIX509CertDB.h"

namespace mozilla { namespace psm {

class AppTrustDomain MOZ_FINAL : public insanity::pkix::TrustDomain
{
public:
  AppTrustDomain(void* pinArg);

  SECStatus SetTrustedRoot(AppTrustedRoot trustedRoot);

  SECStatus GetCertTrust(insanity::pkix::EndEntityOrCA endEntityOrCA,
                         SECOidTag policy,
                         const CERTCertificate* candidateCert,
                 /*out*/ TrustLevel* trustLevel) MOZ_OVERRIDE;
  SECStatus FindPotentialIssuers(const SECItem* encodedIssuerName,
                                 PRTime time,
                         /*out*/ insanity::pkix::ScopedCERTCertList& results)
                                 MOZ_OVERRIDE;
  SECStatus VerifySignedData(const CERTSignedData* signedData,
                             const CERTCertificate* cert) MOZ_OVERRIDE;
  SECStatus CheckRevocation(insanity::pkix::EndEntityOrCA endEntityOrCA,
                            const CERTCertificate* cert,
                            /*const*/ CERTCertificate* issuerCertToDup,
                            PRTime time,
                            /*optional*/ const SECItem* stapledOCSPresponse);
private:
  void* mPinArg; // non-owning!
  insanity::pkix::ScopedCERTCertificate mTrustedRoot;
};

} } // namespace mozilla::psm

#endif // mozilla_psm_AppsTrustDomain_h
