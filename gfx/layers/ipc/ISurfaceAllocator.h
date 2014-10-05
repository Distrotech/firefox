/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GFX_LAYERS_ISURFACEDEALLOCATOR
#define GFX_LAYERS_ISURFACEDEALLOCATOR

#include <stddef.h>                     // for size_t
#include <stdint.h>                     // for uint32_t
#include "gfxTypes.h"
#include "mozilla/gfx/Point.h"          // for IntSize
#include "mozilla/ipc/SharedMemory.h"   // for SharedMemory, etc
#include "mozilla/RefPtr.h"
#include "nsIMemoryReporter.h"          // for nsIMemoryReporter
#include "mozilla/Atomics.h"            // for Atomic

/*
 * FIXME [bjacob] *** PURE CRAZYNESS WARNING ***
 *
 * This #define is actually needed here, because subclasses of ISurfaceAllocator,
 * namely ShadowLayerForwarder, will or will not override AllocGrallocBuffer
 * depending on whether MOZ_HAVE_SURFACEDESCRIPTORGRALLOC is defined.
 */
#ifdef MOZ_WIDGET_GONK
#define MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
#endif

class gfxSharedImageSurface;

namespace base {
class Thread;
}

namespace mozilla {
namespace ipc {
class Shmem;
}

namespace layers {

class PGrallocBufferChild;
class MaybeMagicGrallocBufferHandle;
class MemoryTextureClient;
class MemoryTextureHost;

enum BufferCapabilities {
  DEFAULT_BUFFER_CAPS = 0,
  /**
   * The allocated buffer must be efficiently mappable as a
   * gfxImageSurface.
   */
  MAP_AS_IMAGE_SURFACE = 1 << 0,
  /**
   * The allocated buffer will be used for GL rendering only
   */
  USING_GL_RENDERING_ONLY = 1 << 1
};

class SurfaceDescriptor;


mozilla::ipc::SharedMemory::SharedMemoryType OptimalShmemType();
bool IsSurfaceDescriptorValid(const SurfaceDescriptor& aSurface);
bool IsSurfaceDescriptorOwned(const SurfaceDescriptor& aDescriptor);
bool ReleaseOwnedSurfaceDescriptor(const SurfaceDescriptor& aDescriptor);
/**
 * An interface used to create and destroy surfaces that are shared with the
 * Compositor process (using shmem, or gralloc, or other platform specific memory)
 *
 * Most of the methods here correspond to methods that are implemented by IPDL
 * actors without a common polymorphic interface.
 * These methods should be only called in the ipdl implementor's thread, unless
 * specified otherwise in the implementing class.
 */
class ISurfaceAllocator : public AtomicRefCounted<ISurfaceAllocator>
{
public:
  ISurfaceAllocator() {}

  /**
   * Allocate shared memory that can be accessed by only one process at a time.
   * Ownership of this memory is passed when the memory is sent in an IPDL
   * message.
   */
  virtual bool AllocShmem(size_t aSize,
                          mozilla::ipc::SharedMemory::SharedMemoryType aType,
                          mozilla::ipc::Shmem* aShmem) = 0;

  /**
   * Allocate shared memory that can be accessed by both processes at the
   * same time. Safety is left for the user of the memory to care about.
   */
  virtual bool AllocUnsafeShmem(size_t aSize,
                                mozilla::ipc::SharedMemory::SharedMemoryType aType,
                                mozilla::ipc::Shmem* aShmem) = 0;
  /**
   * Deallocate memory allocated by either AllocShmem or AllocUnsafeShmem.
   */
  virtual void DeallocShmem(mozilla::ipc::Shmem& aShmem) = 0;

  // was AllocBuffer
  virtual bool AllocSharedImageSurface(const gfx::IntSize& aSize,
                                       gfxContentType aContent,
                                       gfxSharedImageSurface** aBuffer);
  virtual bool AllocSurfaceDescriptor(const gfx::IntSize& aSize,
                                      gfxContentType aContent,
                                      SurfaceDescriptor* aBuffer);

  // was AllocBufferWithCaps
  virtual bool AllocSurfaceDescriptorWithCaps(const gfx::IntSize& aSize,
                                              gfxContentType aContent,
                                              uint32_t aCaps,
                                              SurfaceDescriptor* aBuffer);

  virtual void DestroySharedSurface(SurfaceDescriptor* aSurface);

  // method that does the actual allocation work
  virtual PGrallocBufferChild* AllocGrallocBuffer(const gfx::IntSize& aSize,
                                                  uint32_t aFormat,
                                                  uint32_t aUsage,
                                                  MaybeMagicGrallocBufferHandle* aHandle)
  {
    return nullptr;
  }

  virtual bool IPCOpen() const { return true; }

  // Returns true if aSurface wraps a Shmem.
  static bool IsShmem(SurfaceDescriptor* aSurface);

protected:
  // this method is needed for a temporary fix, will be removed after
  // DeprecatedTextureClient/Host rework.
  virtual bool IsOnCompositorSide() const = 0;
  static bool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);
  virtual bool PlatformAllocSurfaceDescriptor(const gfx::IntSize& aSize,
                                              gfxContentType aContent,
                                              uint32_t aCaps,
                                              SurfaceDescriptor* aBuffer);


  virtual ~ISurfaceAllocator() {}

  friend class detail::RefCounted<ISurfaceAllocator, detail::AtomicRefCount>;
};

class GfxMemoryImageReporter MOZ_FINAL : public nsIMemoryReporter
{
public:
  NS_DECL_ISUPPORTS

  GfxMemoryImageReporter()
  {
#ifdef DEBUG
    // There must be only one instance of this class, due to |sAmount|
    // being static.
    static bool hasRun = false;
    MOZ_ASSERT(!hasRun);
    hasRun = true;
#endif
  }

  MOZ_DEFINE_MALLOC_SIZE_OF_ON_ALLOC(MallocSizeOfOnAlloc)
  MOZ_DEFINE_MALLOC_SIZE_OF_ON_FREE(MallocSizeOfOnFree)

  static void DidAlloc(void* aPointer)
  {
    sAmount += MallocSizeOfOnAlloc(aPointer);
  }

  static void WillFree(void* aPointer)
  {
    sAmount -= MallocSizeOfOnFree(aPointer);
  }

  NS_IMETHOD CollectReports(nsIHandleReportCallback* aHandleReport,
                            nsISupports* aData)
  {
    return MOZ_COLLECT_REPORT(
      "explicit/gfx/heap-textures", KIND_HEAP, UNITS_BYTES, sAmount,
      "Heap memory shared between threads by texture clients and hosts.");
  }

private:
  static mozilla::Atomic<int32_t> sAmount;
};

} // namespace
} // namespace

#endif
