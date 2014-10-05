/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozPersonalDictionary.h"
#include "nsIUnicharInputStream.h"
#include "nsReadableUtils.h"
#include "nsIFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsICharsetConverterManager.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIWeakReference.h"
#include "nsCRT.h"
#include "nsNetUtil.h"
#include "nsStringEnumerator.h"
#include "nsUnicharInputStream.h"

#define MOZ_PERSONAL_DICT_NAME "persdict.dat"

const int kMaxWordLen=256;

/**
 * This is the most braindead implementation of a personal dictionary possible.
 * There is not much complexity needed, though.  It could be made much faster,
 *  and probably should, but I don't see much need for more in terms of interface.
 *
 * Allowing personal words to be associated with only certain dictionaries maybe.
 *
 * TODO:
 * Implement the suggestion record.
 */


NS_IMPL_CYCLE_COLLECTING_ADDREF(mozPersonalDictionary)
NS_IMPL_CYCLE_COLLECTING_RELEASE(mozPersonalDictionary)

NS_INTERFACE_MAP_BEGIN(mozPersonalDictionary)
  NS_INTERFACE_MAP_ENTRY(mozIPersonalDictionary)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, mozIPersonalDictionary)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(mozPersonalDictionary)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_1(mozPersonalDictionary, mEncoder)

mozPersonalDictionary::mozPersonalDictionary()
 : mDirty(false)
{
}

mozPersonalDictionary::~mozPersonalDictionary()
{
}

nsresult mozPersonalDictionary::Init()
{
  nsCOMPtr<nsIObserverService> svc =
    do_GetService("@mozilla.org/observer-service;1");

  NS_ENSURE_STATE(svc);
  // we want to reload the dictionary if the profile switches
  nsresult rv = svc->AddObserver(this, "profile-do-change", true);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = svc->AddObserver(this, "profile-before-change", true);
  NS_ENSURE_SUCCESS(rv, rv);

  Load();
  
  return NS_OK;
}

/* void Load (); */
NS_IMETHODIMP mozPersonalDictionary::Load()
{
  //FIXME Deinst  -- get dictionary name from prefs;
  nsresult res;
  nsCOMPtr<nsIFile> theFile;
  bool dictExists;

  res = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(theFile));
  if(NS_FAILED(res)) return res;
  if(!theFile)return NS_ERROR_FAILURE;
  res = theFile->Append(NS_LITERAL_STRING(MOZ_PERSONAL_DICT_NAME));
  if(NS_FAILED(res)) return res;
  res = theFile->Exists(&dictExists);
  if(NS_FAILED(res)) return res;

  if (!dictExists) {
    // Nothing is really wrong...
    return NS_OK;
  }
  
  nsCOMPtr<nsIInputStream> inStream;
  NS_NewLocalFileInputStream(getter_AddRefs(inStream), theFile);

  nsCOMPtr<nsIUnicharInputStream> convStream;
  res = nsSimpleUnicharStreamFactory::GetInstance()->
    CreateInstanceFromUTF8Stream(inStream, getter_AddRefs(convStream));
  if(NS_FAILED(res)) return res;
  
  // we're rereading to get rid of the old data  -- we shouldn't have any, but...
  mDictionaryTable.Clear();

  char16_t c;
  uint32_t nRead;
  bool done = false;
  do{  // read each line of text into the string array.
    if( (NS_OK != convStream->Read(&c, 1, &nRead)) || (nRead != 1)) break;
    while(!done && ((c == '\n') || (c == '\r'))){
      if( (NS_OK != convStream->Read(&c, 1, &nRead)) || (nRead != 1)) done = true;
    }
    if (!done){ 
      nsAutoString word;
      while((c != '\n') && (c != '\r') && !done){
        word.Append(c);
        if( (NS_OK != convStream->Read(&c, 1, &nRead)) || (nRead != 1)) done = true;
      }
      mDictionaryTable.PutEntry(word.get());
    }
  } while(!done);
  mDirty = false;
  
  return res;
}

// A little helper function to add the key to the list.
// This is not threadsafe, and only safe if the consumer does not 
// modify the list.
static PLDHashOperator
AddHostToStringArray(nsUnicharPtrHashKey *aEntry, void *aArg)
{
  static_cast<nsTArray<nsString>*>(aArg)->AppendElement(nsDependentString(aEntry->GetKey()));
  return PL_DHASH_NEXT;
}

/* void Save (); */
NS_IMETHODIMP mozPersonalDictionary::Save()
{
  nsCOMPtr<nsIFile> theFile;
  nsresult res;

  if(!mDirty) return NS_OK;

  //FIXME Deinst  -- get dictionary name from prefs;
  res = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(theFile));
  if(NS_FAILED(res)) return res;
  if(!theFile)return NS_ERROR_FAILURE;
  res = theFile->Append(NS_LITERAL_STRING(MOZ_PERSONAL_DICT_NAME));
  if(NS_FAILED(res)) return res;

  nsCOMPtr<nsIOutputStream> outStream;
  NS_NewSafeLocalFileOutputStream(getter_AddRefs(outStream), theFile, PR_CREATE_FILE | PR_WRONLY | PR_TRUNCATE ,0664);

  // get a buffered output stream 4096 bytes big, to optimize writes
  nsCOMPtr<nsIOutputStream> bufferedOutputStream;
  res = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOutputStream), outStream, 4096);
  if (NS_FAILED(res)) return res;

  nsTArray<nsString> array(mDictionaryTable.Count());
  mDictionaryTable.EnumerateEntries(AddHostToStringArray, &array);

  uint32_t bytesWritten;
  nsAutoCString utf8Key;
  for (uint32_t i = 0; i < array.Length(); ++i ) {
    CopyUTF16toUTF8(array[i], utf8Key);

    bufferedOutputStream->Write(utf8Key.get(), utf8Key.Length(), &bytesWritten);
    bufferedOutputStream->Write("\n", 1, &bytesWritten);
  }
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(bufferedOutputStream);
  NS_ASSERTION(safeStream, "expected a safe output stream!");
  if (safeStream) {
    res = safeStream->Finish();
    if (NS_FAILED(res)) {
      NS_WARNING("failed to save personal dictionary file! possible data loss");
    }
  }
  return res;
}

/* readonly attribute nsIStringEnumerator GetWordList() */
NS_IMETHODIMP mozPersonalDictionary::GetWordList(nsIStringEnumerator **aWords)
{
  NS_ENSURE_ARG_POINTER(aWords);
  *aWords = nullptr;

  nsTArray<nsString> *array = new nsTArray<nsString>(mDictionaryTable.Count());
  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  mDictionaryTable.EnumerateEntries(AddHostToStringArray, array);

  array->Sort();

  return NS_NewAdoptingStringEnumerator(aWords, array);
}

/* boolean Check (in wstring word, in wstring language); */
NS_IMETHODIMP mozPersonalDictionary::Check(const char16_t *aWord, const char16_t *aLanguage, bool *aResult)
{
  NS_ENSURE_ARG_POINTER(aWord);
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = (mDictionaryTable.GetEntry(aWord) || mIgnoreTable.GetEntry(aWord));
  return NS_OK;
}

/* void AddWord (in wstring word); */
NS_IMETHODIMP mozPersonalDictionary::AddWord(const char16_t *aWord, const char16_t *aLang)
{
  mDictionaryTable.PutEntry(aWord);
  mDirty = true;
  return NS_OK;
}

/* void RemoveWord (in wstring word); */
NS_IMETHODIMP mozPersonalDictionary::RemoveWord(const char16_t *aWord, const char16_t *aLang)
{
  mDictionaryTable.RemoveEntry(aWord);
  mDirty = true;
  return NS_OK;
}

/* void IgnoreWord (in wstring word); */
NS_IMETHODIMP mozPersonalDictionary::IgnoreWord(const char16_t *aWord)
{
  // avoid adding duplicate words to the ignore list
  if (aWord && !mIgnoreTable.GetEntry(aWord)) 
    mIgnoreTable.PutEntry(aWord);
  return NS_OK;
}

/* void EndSession (); */
NS_IMETHODIMP mozPersonalDictionary::EndSession()
{
  Save(); // save any custom words at the end of a spell check session
  mIgnoreTable.Clear();
  return NS_OK;
}

/* void AddCorrection (in wstring word, in wstring correction); */
NS_IMETHODIMP mozPersonalDictionary::AddCorrection(const char16_t *word, const char16_t *correction, const char16_t *lang)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void RemoveCorrection (in wstring word, in wstring correction); */
NS_IMETHODIMP mozPersonalDictionary::RemoveCorrection(const char16_t *word, const char16_t *correction, const char16_t *lang)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetCorrection (in wstring word, [array, size_is (count)] out wstring words, out uint32_t count); */
NS_IMETHODIMP mozPersonalDictionary::GetCorrection(const char16_t *word, char16_t ***words, uint32_t *count)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void observe (in nsISupports aSubject, in string aTopic, in wstring aData); */
NS_IMETHODIMP mozPersonalDictionary::Observe(nsISupports *aSubject, const char *aTopic, const char16_t *aData)
{
  if (!nsCRT::strcmp(aTopic, "profile-do-change")) {
    Load();  // load automatically clears out the existing dictionary table
  } else if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    Save();
  }

  return NS_OK;
}

