/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MediaShutdownManager.h"
#include "nsContentUtils.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/ClearOnShutdown.h"
#include "MediaDecoder.h"

namespace mozilla {

StateMachineThread::StateMachineThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_CTOR(StateMachineThread);
}

StateMachineThread::~StateMachineThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_DTOR(StateMachineThread);
}

void
StateMachineThread::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mThread);
  if (mThread) {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &StateMachineThread::ShutdownThread);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }
}

void
StateMachineThread::ShutdownThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mThread);
  mThread->Shutdown();
  mThread = nullptr;
  MediaShutdownManager::Instance().Unregister(this);
}

nsresult
StateMachineThread::Init()
{
  MOZ_ASSERT(NS_IsMainThread());
  nsresult rv = NS_NewNamedThread("Media State", getter_AddRefs(mThread));
  NS_ENSURE_SUCCESS(rv, rv);
  MediaShutdownManager::Instance().Register(this);
  return NS_OK;
}

nsIThread*
StateMachineThread::GetThread()
{
  MOZ_ASSERT(mThread);
  return mThread;
}

void
StateMachineThread::SpinUntilShutdownComplete()
{
  MOZ_ASSERT(NS_IsMainThread());
  while (mThread) {
    bool processed = false;
    nsresult rv = NS_GetCurrentThread()->ProcessNextEvent(true, &processed);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to spin main thread while awaiting media shutdown");
      break;
    }
  }
}

NS_IMPL_ISUPPORTS1(MediaShutdownManager, nsIObserver)

MediaShutdownManager::MediaShutdownManager()
  : mIsObservingShutdown(false),
    mIsDoingXPCOMShutDown(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_CTOR(MediaShutdownManager);
}

MediaShutdownManager::~MediaShutdownManager()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_DTOR(MediaShutdownManager);
}

// Note that we don't use ClearOnShutdown() on this StaticRefPtr, as that
// may interfere with our shutdown listener.
StaticRefPtr<MediaShutdownManager> MediaShutdownManager::sInstance;

MediaShutdownManager&
MediaShutdownManager::Instance()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!sInstance) {
    sInstance = new MediaShutdownManager();
  }
  return *sInstance;
}

void
MediaShutdownManager::EnsureCorrectShutdownObserverState()
{
  MOZ_ASSERT(!mIsDoingXPCOMShutDown);
  bool needShutdownObserver = (mDecoders.Count() > 0) ||
                              (mStateMachineThreads.Count() > 0);
  if (needShutdownObserver != mIsObservingShutdown) {
    mIsObservingShutdown = needShutdownObserver;
    if (mIsObservingShutdown) {
      nsContentUtils::RegisterShutdownObserver(this);
    } else {
      nsContentUtils::UnregisterShutdownObserver(this);
      // Clear our singleton reference. This will probably delete
      // this instance, so don't deref |this| clearing sInstance.
      sInstance = nullptr;
    }
  }
}

void
MediaShutdownManager::Register(MediaDecoder* aDecoder)
{
  MOZ_ASSERT(NS_IsMainThread());
  // Don't call Register() after you've Unregistered() all the decoders,
  // that's not going to work.
  MOZ_ASSERT(!mDecoders.Contains(aDecoder));
  mDecoders.PutEntry(aDecoder);
  MOZ_ASSERT(mDecoders.Contains(aDecoder));
  MOZ_ASSERT(mDecoders.Count() > 0);
  EnsureCorrectShutdownObserverState();
}

void
MediaShutdownManager::Unregister(MediaDecoder* aDecoder)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mDecoders.Contains(aDecoder));
  if (!mIsDoingXPCOMShutDown) {
    mDecoders.RemoveEntry(aDecoder);
    EnsureCorrectShutdownObserverState();
  }
}

NS_IMETHODIMP
MediaShutdownManager::Observe(nsISupports *aSubjet,
                              const char *aTopic,
                              const char16_t *someData)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    Shutdown();
  }
  return NS_OK;
}

void
MediaShutdownManager::Register(StateMachineThread* aThread)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mStateMachineThreads.Contains(aThread));
  mStateMachineThreads.PutEntry(aThread);
  MOZ_ASSERT(mStateMachineThreads.Contains(aThread));
  MOZ_ASSERT(mStateMachineThreads.Count() > 0);
  EnsureCorrectShutdownObserverState();
}

void
MediaShutdownManager::Unregister(StateMachineThread* aThread)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mStateMachineThreads.Contains(aThread));
  if (!mIsDoingXPCOMShutDown) {
    mStateMachineThreads.RemoveEntry(aThread);
    EnsureCorrectShutdownObserverState();
  }
}

static PLDHashOperator
ShutdownMediaDecoder(nsRefPtrHashKey<MediaDecoder>* aEntry, void*)
{
  aEntry->GetKey()->Shutdown();
  return PL_DHASH_REMOVE;
}

static PLDHashOperator
JoinStateMachineThreads(nsRefPtrHashKey<StateMachineThread>* aEntry, void*)
{
  // Hold a strong reference to the StateMachineThread, so that if it
  // is Unregistered() and the hashtable's owning reference is cleared,
  // it won't be destroyed while we're spinning here.
  RefPtr<StateMachineThread> thread = aEntry->GetKey();
  thread->SpinUntilShutdownComplete();
  return PL_DHASH_REMOVE;
}

void
MediaShutdownManager::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sInstance);

  // Mark that we're shutting down, so that Unregister(*) calls don't remove
  // hashtable entries. If Unregsiter(*) was to remove from the hash table,
  // the iterations over the hashtables below would be disrupted.
  mIsDoingXPCOMShutDown = true;

  // Iterate over the decoders and shut them down, and remove them from the
  // hashtable.
  mDecoders.EnumerateEntries(ShutdownMediaDecoder, nullptr);
 
  // Iterate over the StateMachineThreads and wait for them to have finished
  // shutting down, and remove them from the hashtable. Once all the decoders
  // have shutdown the active state machine thread will naturally shutdown
  // asynchronously. We may also have another state machine thread active,
  // if construction and shutdown of the state machine thread has interleaved.
  mStateMachineThreads.EnumerateEntries(JoinStateMachineThreads, nullptr);
 
  // Remove the MediaShutdownManager instance from the shutdown observer
  // list.
  nsContentUtils::UnregisterShutdownObserver(this);

  // Clear the singleton instance. The only remaining reference should be the
  // reference that the observer service used to call us with. The
  // MediaShutdownManager will be deleted once the observer service cleans
  // up after it finishes its notifications.
  sInstance = nullptr;
}

} // namespace mozilla
