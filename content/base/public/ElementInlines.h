/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 8; -*- */
/* vim: set sw=2 sts=2 ts=8 et tw=80 : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_ElementInlines_h
#define mozilla_dom_ElementInlines_h

#include "mozilla/dom/Element.h"
#include "nsIDocument.h"

namespace mozilla {
namespace dom {

inline void
Element::MozRequestPointerLock()
{
  OwnerDoc()->RequestPointerLock(this);
}

inline void
Element::RegisterFreezableElement()
{
  OwnerDoc()->RegisterFreezableElement(this);
}

inline void
Element::UnregisterFreezableElement()
{
  OwnerDoc()->UnregisterFreezableElement(this);
}

}
}

#endif // mozilla_dom_ElementInlines_h
