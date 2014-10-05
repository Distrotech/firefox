
/*
 * Copyright 2010 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef GrUserConfig_DEFINED
#define GrUserConfig_DEFINED

#if defined(GR_USER_CONFIG_FILE)
    #error "default user config pulled in but GR_USER_CONFIG_FILE is defined."
#endif

#if 0
    #undef GR_RELEASE
    #undef GR_DEBUG
    #define GR_RELEASE  0
    #define GR_DEBUG    1
#endif

/**
 * When drawing rects this causes Ganesh to use a vertex buffer containing
 * a unit square that is positioned by a matrix. Enable on systems where
 * emitting per-rect-draw verts is more expensive than constant/matrix
 * updates. Defaults to 0.
 */
//#define GR_STATIC_RECT_VB 1


/**
 * This gives a threshold in bytes of when to lock a GrGeometryBuffer vs using
 * updateData. (Note the depending on the underlying 3D API the update functions
 * may always be implemented using a lock)
 */
//#define GR_GEOM_BUFFER_LOCK_THRESHOLD (1<<15)

/**
 * This gives a threshold in megabytes for the maximum size of the texture cache
 * in vram. The value is only a default and can be overridden at runtime.
 */
//#define GR_DEFAULT_TEXTURE_CACHE_MB_LIMIT 96

/*
 * This allows us to set a callback to be called before each GL call to ensure
 * that our context is set correctly
 */
#define GR_GL_PER_GL_FUNC_CALLBACK  1

#endif
