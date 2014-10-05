/* vim:set ts=2 sw=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsUDPSocket_h__
#define nsUDPSocket_h__

#include "nsIUDPSocket.h"
#include "mozilla/Mutex.h"
#include "nsIOutputStream.h"
#include "nsAutoPtr.h"

//-----------------------------------------------------------------------------

class nsUDPSocket : public nsASocketHandler
                  , public nsIUDPSocket
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIUDPSOCKET

  // nsASocketHandler methods:
  virtual void OnSocketReady(PRFileDesc* fd, int16_t outFlags);
  virtual void OnSocketDetached(PRFileDesc* fd);
  virtual void IsLocal(bool* aIsLocal);

  uint64_t ByteCountSent() { return mByteWriteCount; }
  uint64_t ByteCountReceived() { return mByteReadCount; }

  void AddOutputBytes(uint64_t aBytes);

  nsUDPSocket();

  // This must be public to support older compilers (xlC_r on AIX)
  virtual ~nsUDPSocket();

private:
  void OnMsgClose();
  void OnMsgAttach();

  // try attaching our socket (mFD) to the STS's poll list.
  nsresult TryAttach();

  // lock protects access to mListener;
  // so mListener is not cleared while being used/locked.
  mozilla::Mutex                       mLock;
  PRFileDesc                           *mFD;
  mozilla::net::NetAddr                mAddr;
  nsCOMPtr<nsIUDPSocketListener>       mListener;
  nsCOMPtr<nsIEventTarget>             mListenerTarget;
  bool                                 mAttached;
  nsRefPtr<nsSocketTransportService>   mSts;

  uint64_t   mByteReadCount;
  uint64_t   mByteWriteCount;
};

//-----------------------------------------------------------------------------

class nsUDPMessage : public nsIUDPMessage
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIUDPMESSAGE

  nsUDPMessage(PRNetAddr* aAddr,
               nsIOutputStream* aOutputStream,
               const nsACString& aData);

private:
  virtual ~nsUDPMessage();

  PRNetAddr mAddr;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsCString mData;
};


//-----------------------------------------------------------------------------

class nsUDPOutputStream : public nsIOutputStream
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM

  nsUDPOutputStream(nsUDPSocket* aSocket,
                    PRFileDesc* aFD,
                    PRNetAddr& aPrClientAddr);
  virtual ~nsUDPOutputStream();

private:
  nsRefPtr<nsUDPSocket>       mSocket;
  PRFileDesc                  *mFD;
  PRNetAddr                   mPrClientAddr;
  bool                        mIsClosed;
};

#endif // nsUDPSocket_h__
