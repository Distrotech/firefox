/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PeriodicWave_h_
#define PeriodicWave_h_

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "EnableWebAudioCheck.h"
#include "AudioContext.h"
#include "AudioNodeEngine.h"
#include "nsAutoPtr.h"

namespace mozilla {

namespace dom {

class PeriodicWave MOZ_FINAL : public nsWrapperCache,
                               public EnableWebAudioCheck
{
public:
  PeriodicWave(AudioContext* aContext,
               const float* aRealData,
               const float* aImagData,
               const uint32_t aLength,
               ErrorResult& aRv);

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(PeriodicWave)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(PeriodicWave)

  AudioContext* GetParentObject() const
  {
    return mContext;
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  uint32_t DataLength() const
  {
    return mLength;
  }

  ThreadSharedFloatArrayBufferList* GetThreadSharedBuffer() const
  {
    return mCoefficients;
  }

private:
  nsRefPtr<AudioContext> mContext;
  nsRefPtr<ThreadSharedFloatArrayBufferList> mCoefficients;
  uint32_t mLength;
};

}
}

#endif
