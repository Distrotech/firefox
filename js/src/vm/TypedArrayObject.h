/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef vm_TypedArrayObject_h
#define vm_TypedArrayObject_h

#include "jsobj.h"

#include "builtin/TypeRepresentation.h"
#include "gc/Barrier.h"
#include "js/Class.h"

typedef struct JSProperty JSProperty;

namespace js {

typedef Vector<ArrayBufferObject *, 0, SystemAllocPolicy> ArrayBufferVector;

// The inheritance hierarchy for the various classes relating to typed arrays
// is as follows.
//
// - JSObject
//   - ArrayBufferObject
//   - ArrayBufferViewObject
//     - DataViewObject
//     - TypedArrayObject
//       - TypedArrayObjectTemplate
//         - Int8ArrayObject
//         - Uint8ArrayObject
//         - ...
//
// Note that |TypedArrayObjectTemplate| is just an implementation detail that
// makes implementing its various subclasses easier.

class ArrayBufferViewObject;

/*
 * ArrayBufferObject
 *
 * This class holds the underlying raw buffer that the various
 * ArrayBufferViewObject subclasses (DataViewObject and the TypedArrays)
 * access. It can be created explicitly and passed to an ArrayBufferViewObject
 * subclass, or can be created implicitly by constructing a TypedArrayObject
 * with a size.
 */
class ArrayBufferObject : public JSObject
{
    static bool byteLengthGetterImpl(JSContext *cx, CallArgs args);
    static bool fun_slice_impl(JSContext *cx, CallArgs args);

  public:
    static const Class class_;

    static const Class protoClass;
    static const JSFunctionSpec jsfuncs[];
    static const JSFunctionSpec jsstaticfuncs[];

    static bool byteLengthGetter(JSContext *cx, unsigned argc, Value *vp);

    static bool fun_slice(JSContext *cx, unsigned argc, Value *vp);

    static bool fun_isView(JSContext *cx, unsigned argc, Value *vp);

    static bool class_constructor(JSContext *cx, unsigned argc, Value *vp);

    static JSObject *create(JSContext *cx, uint32_t nbytes, bool clear = true);

    static JSObject *createSlice(JSContext *cx, Handle<ArrayBufferObject*> arrayBuffer,
                                 uint32_t begin, uint32_t end);

    static bool createDataViewForThisImpl(JSContext *cx, CallArgs args);
    static bool createDataViewForThis(JSContext *cx, unsigned argc, Value *vp);

    template<typename T>
    static bool createTypedArrayFromBufferImpl(JSContext *cx, CallArgs args);

    template<typename T>
    static bool createTypedArrayFromBuffer(JSContext *cx, unsigned argc, Value *vp);

    static void obj_trace(JSTracer *trc, JSObject *obj);

    static bool obj_lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                  MutableHandleObject objp, MutableHandleShape propp);
    static bool obj_lookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                   MutableHandleObject objp, MutableHandleShape propp);
    static bool obj_lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                  MutableHandleObject objp, MutableHandleShape propp);
    static bool obj_lookupSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                  MutableHandleObject objp, MutableHandleShape propp);

    static bool obj_defineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static bool obj_defineProperty(JSContext *cx, HandleObject obj,
                                   HandlePropertyName name, HandleValue v,
                                   PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static bool obj_defineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue v,
                                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static bool obj_defineSpecial(JSContext *cx, HandleObject obj,
                                  HandleSpecialId sid, HandleValue v,
                                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static bool obj_getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                               HandleId id, MutableHandleValue vp);

    static bool obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                                HandlePropertyName name, MutableHandleValue vp);

    static bool obj_getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                               uint32_t index, MutableHandleValue vp);

    static bool obj_getSpecial(JSContext *cx, HandleObject obj, HandleObject receiver,
                               HandleSpecialId sid, MutableHandleValue vp);

    static bool obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                               MutableHandleValue vp, bool strict);
    static bool obj_setProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                MutableHandleValue vp, bool strict);
    static bool obj_setElement(JSContext *cx, HandleObject obj, uint32_t index,
                               MutableHandleValue vp, bool strict);
    static bool obj_setSpecial(JSContext *cx, HandleObject obj,
                               HandleSpecialId sid, MutableHandleValue vp, bool strict);

    static bool obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                         HandleId id, unsigned *attrsp);
    static bool obj_setGenericAttributes(JSContext *cx, HandleObject obj,
                                         HandleId id, unsigned *attrsp);

    static bool obj_deleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                   bool *succeeded);
    static bool obj_deleteElement(JSContext *cx, HandleObject obj, uint32_t index,
                                  bool *succeeded);
    static bool obj_deleteSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                  bool *succeeded);

    static bool obj_enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
                              MutableHandleValue statep, MutableHandleId idp);

    static void sweep(JSCompartment *rt);

    static void resetArrayBufferList(JSCompartment *rt);
    static bool saveArrayBufferList(JSCompartment *c, ArrayBufferVector &vector);
    static void restoreArrayBufferLists(ArrayBufferVector &vector);

    bool hasStealableContents() const {
        // Inline elements strictly adhere to the corresponding buffer.
        if (!hasDynamicElements())
            return false;

        // asm.js buffer contents are transferred by copying, just like inline
        // elements.
        if (isAsmJSArrayBuffer())
            return false;

        // Neutered contents aren't transferrable because we want a neutered
        // array's contents to be backed by zeroed memory equal in length to
        // the original buffer contents.  Transferring these contents would
        // allocate new ones based on the current byteLength, which is 0 for a
        // neutered array -- not the original byteLength.
        return !isNeutered();
    }

    static bool stealContents(JSContext *cx, Handle<ArrayBufferObject*> buffer, void **contents,
                              uint8_t **data);

    static void updateElementsHeader(js::ObjectElements *header, uint32_t bytes) {
        header->initializedLength = bytes;

        // NB: one or both of these fields is clobbered by GetViewList to store
        // the 'views' link. Set them to 0 to effectively initialize 'views'
        // to nullptr.
        header->length = 0;
        header->capacity = 0;
    }

    static void initElementsHeader(js::ObjectElements *header, uint32_t bytes) {
        header->flags = 0;
        updateElementsHeader(header, bytes);
    }

    static uint32_t headerInitializedLength(const js::ObjectElements *header) {
        return header->initializedLength;
    }

    void addView(ArrayBufferViewObject *view);

    bool allocateSlots(JSContext *cx, uint32_t size, bool clear);

    void changeContents(JSContext *cx, ObjectElements *newHeader);

    /*
     * Copy the data into freshly-allocated memory. Used when un-inlining or
     * when converting an ArrayBuffer to an AsmJS (MMU-assisted) ArrayBuffer.
     */
    bool copyData(JSContext *maybecx);

    /*
     * Ensure data is not stored inline in the object. Used when handing back a
     * GC-safe pointer.
     */
    bool ensureNonInline(JSContext *maybecx);

    uint32_t byteLength() const {
        return getElementsHeader()->initializedLength;
    }

    /*
     * Neuter all views of an ArrayBuffer.
     */
    static bool neuterViews(JSContext *cx, Handle<ArrayBufferObject*> buffer);

    inline uint8_t * dataPointer() const {
        return (uint8_t *) elements;
    }

     /*
     * Discard the ArrayBuffer contents, and use |newHeader| for the buffer's
     * new contents.  (These new contents are zeroed, of identical size in
     * memory as the current contents, but appear to be neutered and of zero
     * length.  This is purely precautionary against stale indexes that were
     * in-bounds with respect to the initial length but would not be after
     * neutering.  This precaution will be removed once we're sure such stale
     * indexing no longer happens.)  For asm.js buffers, at least, should
     * be called after neuterViews().
     */
    void neuter(ObjectElements *newHeader, JSContext *cx);

    /*
     * Check if the arrayBuffer contains any data. This will return false for
     * ArrayBuffer.prototype and neutered ArrayBuffers.
     */
    bool hasData() const {
        return getClass() == &class_;
    }

    bool isAsmJSArrayBuffer() const {
        return getElementsHeader()->isAsmJSArrayBuffer();
    }
    bool isNeutered() const {
        return getElementsHeader()->isNeuteredBuffer();
    }
    static bool prepareForAsmJS(JSContext *cx, Handle<ArrayBufferObject*> buffer);
    static bool neuterAsmJSArrayBuffer(JSContext *cx, ArrayBufferObject &buffer);
    static void releaseAsmJSArrayBuffer(FreeOp *fop, JSObject *obj);
};

/*
 * ArrayBufferViewObject
 *
 * Common definitions shared by all ArrayBufferViews.
 */

class ArrayBufferViewObject : public JSObject
{
  protected:
    /* Offset of view in underlying ArrayBufferObject */
    static const size_t BYTEOFFSET_SLOT  = 0;

    /* Byte length of view */
    static const size_t BYTELENGTH_SLOT  = 1;

    /* Underlying ArrayBufferObject */
    static const size_t BUFFER_SLOT      = 2;

    /* ArrayBufferObjects point to a linked list of views, chained through this slot */
    static const size_t NEXT_VIEW_SLOT   = 3;

    /*
     * When ArrayBufferObjects are traced during GC, they construct a linked
     * list of ArrayBufferObjects with more than one view, chained through this
     * slot of the first view of each ArrayBufferObject.
     */
    static const size_t NEXT_BUFFER_SLOT = 4;

    static const size_t NUM_SLOTS        = 5;

  public:
    JSObject *bufferObject() const {
        return &getFixedSlot(BUFFER_SLOT).toObject();
    }

    ArrayBufferObject *bufferLink() {
        return static_cast<ArrayBufferObject*>(getFixedSlot(NEXT_BUFFER_SLOT).toPrivate());
    }

    inline void setBufferLink(ArrayBufferObject *buffer);

    ArrayBufferViewObject *nextView() const {
        return static_cast<ArrayBufferViewObject*>(getFixedSlot(NEXT_VIEW_SLOT).toPrivate());
    }

    inline void setNextView(ArrayBufferViewObject *view);

    void prependToViews(ArrayBufferViewObject *viewsHead);

    void neuter(JSContext *cx);

    static void trace(JSTracer *trc, JSObject *obj);
};

/*
 * TypedArrayObject
 *
 * The non-templated base class for the specific typed implementations.
 * This class holds all the member variables that are used by
 * the subclasses.
 */

class TypedArrayObject : public ArrayBufferViewObject
{
  protected:
    // Typed array properties stored in slots, beyond those shared by all
    // ArrayBufferViews.
    static const size_t LENGTH_SLOT    = ArrayBufferViewObject::NUM_SLOTS;
    static const size_t TYPE_SLOT      = ArrayBufferViewObject::NUM_SLOTS + 1;
    static const size_t RESERVED_SLOTS = ArrayBufferViewObject::NUM_SLOTS + 2;
    static const size_t DATA_SLOT      = 7; // private slot, based on alloc kind

  public:
    static const Class classes[ScalarTypeRepresentation::TYPE_MAX];
    static const Class protoClasses[ScalarTypeRepresentation::TYPE_MAX];

    static bool obj_lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                  MutableHandleObject objp, MutableHandleShape propp);
    static bool obj_lookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                   MutableHandleObject objp, MutableHandleShape propp);
    static bool obj_lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                  MutableHandleObject objp, MutableHandleShape propp);
    static bool obj_lookupSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                  MutableHandleObject objp, MutableHandleShape propp);

    static bool obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                         HandleId id, unsigned *attrsp);
    static bool obj_setGenericAttributes(JSContext *cx, HandleObject obj,
                                         HandleId id, unsigned *attrsp);

    static Value bufferValue(TypedArrayObject *tarr) {
        return tarr->getFixedSlot(BUFFER_SLOT);
    }
    static Value byteOffsetValue(TypedArrayObject *tarr) {
        return tarr->getFixedSlot(BYTEOFFSET_SLOT);
    }
    static Value byteLengthValue(TypedArrayObject *tarr) {
        return tarr->getFixedSlot(BYTELENGTH_SLOT);
    }
    static Value lengthValue(TypedArrayObject *tarr) {
        return tarr->getFixedSlot(LENGTH_SLOT);
    }

    ArrayBufferObject *buffer() const {
        return &bufferValue(const_cast<TypedArrayObject*>(this)).toObject().as<ArrayBufferObject>();
    }
    uint32_t byteOffset() const {
        return byteOffsetValue(const_cast<TypedArrayObject*>(this)).toInt32();
    }
    uint32_t byteLength() const {
        return byteLengthValue(const_cast<TypedArrayObject*>(this)).toInt32();
    }
    uint32_t length() const {
        return lengthValue(const_cast<TypedArrayObject*>(this)).toInt32();
    }

    uint32_t type() const {
        return getFixedSlot(TYPE_SLOT).toInt32();
    }
    void *viewData() const {
        return static_cast<void*>(getPrivate(DATA_SLOT));
    }

    inline bool isArrayIndex(jsid id, uint32_t *ip = nullptr);
    void copyTypedArrayElement(uint32_t index, MutableHandleValue vp);

    void neuter(JSContext *cx);

    static uint32_t slotWidth(int atype) {
        switch (atype) {
          case ScalarTypeRepresentation::TYPE_INT8:
          case ScalarTypeRepresentation::TYPE_UINT8:
          case ScalarTypeRepresentation::TYPE_UINT8_CLAMPED:
            return 1;
          case ScalarTypeRepresentation::TYPE_INT16:
          case ScalarTypeRepresentation::TYPE_UINT16:
            return 2;
          case ScalarTypeRepresentation::TYPE_INT32:
          case ScalarTypeRepresentation::TYPE_UINT32:
          case ScalarTypeRepresentation::TYPE_FLOAT32:
            return 4;
          case ScalarTypeRepresentation::TYPE_FLOAT64:
            return 8;
          default:
            MOZ_ASSUME_UNREACHABLE("invalid typed array type");
        }
    }

    int slotWidth() {
        return slotWidth(type());
    }

    /*
     * Byte length above which created typed arrays and data views will have
     * singleton types regardless of the context in which they are created.
     */
    static const uint32_t SINGLETON_TYPE_BYTE_LENGTH = 1024 * 1024 * 10;

    static int lengthOffset();
    static int dataOffset();
};

inline bool
IsTypedArrayClass(const Class *clasp)
{
    return &TypedArrayObject::classes[0] <= clasp &&
           clasp < &TypedArrayObject::classes[ScalarTypeRepresentation::TYPE_MAX];
}

inline bool
IsTypedArrayProtoClass(const Class *clasp)
{
    return &TypedArrayObject::protoClasses[0] <= clasp &&
           clasp < &TypedArrayObject::protoClasses[ScalarTypeRepresentation::TYPE_MAX];
}

bool
IsTypedArrayConstructor(HandleValue v, uint32_t type);

bool
IsTypedArrayBuffer(HandleValue v);

static inline unsigned
TypedArrayShift(ArrayBufferView::ViewType viewType)
{
    switch (viewType) {
      case ArrayBufferView::TYPE_INT8:
      case ArrayBufferView::TYPE_UINT8:
      case ArrayBufferView::TYPE_UINT8_CLAMPED:
        return 0;
      case ArrayBufferView::TYPE_INT16:
      case ArrayBufferView::TYPE_UINT16:
        return 1;
      case ArrayBufferView::TYPE_INT32:
      case ArrayBufferView::TYPE_UINT32:
      case ArrayBufferView::TYPE_FLOAT32:
        return 2;
      case ArrayBufferView::TYPE_FLOAT64:
        return 3;
      default:;
    }
    MOZ_ASSUME_UNREACHABLE("Unexpected array type");
}

class DataViewObject : public ArrayBufferViewObject
{
    static const size_t RESERVED_SLOTS = ArrayBufferViewObject::NUM_SLOTS;
    static const size_t DATA_SLOT      = 7; // private slot, based on alloc kind

  private:
    static const Class protoClass;

    static bool is(HandleValue v) {
        return v.isObject() && v.toObject().hasClass(&class_);
    }

    template<Value ValueGetter(DataViewObject *view)>
    static bool
    getterImpl(JSContext *cx, CallArgs args);

    template<Value ValueGetter(DataViewObject *view)>
    static bool
    getter(JSContext *cx, unsigned argc, Value *vp);

    template<Value ValueGetter(DataViewObject *view)>
    static bool
    defineGetter(JSContext *cx, PropertyName *name, HandleObject proto);

  public:
    static const Class class_;

    static Value byteOffsetValue(DataViewObject *view) {
        Value v = view->getReservedSlot(BYTEOFFSET_SLOT);
        JS_ASSERT(v.toInt32() >= 0);
        return v;
    }

    static Value byteLengthValue(DataViewObject *view) {
        Value v = view->getReservedSlot(BYTELENGTH_SLOT);
        JS_ASSERT(v.toInt32() >= 0);
        return v;
    }

    uint32_t byteOffset() const {
        return byteOffsetValue(const_cast<DataViewObject*>(this)).toInt32();
    }

    uint32_t byteLength() const {
        return byteLengthValue(const_cast<DataViewObject*>(this)).toInt32();
    }

    bool hasBuffer() const {
        return getReservedSlot(BUFFER_SLOT).isObject();
    }

    ArrayBufferObject &arrayBuffer() const {
        return getReservedSlot(BUFFER_SLOT).toObject().as<ArrayBufferObject>();
    }

    static Value bufferValue(DataViewObject *view) {
        return view->hasBuffer() ? ObjectValue(view->arrayBuffer()) : UndefinedValue();
    }

    void *dataPointer() const {
        return getPrivate();
    }

    static bool class_constructor(JSContext *cx, unsigned argc, Value *vp);
    static bool constructWithProto(JSContext *cx, unsigned argc, Value *vp);
    static bool construct(JSContext *cx, JSObject *bufobj, const CallArgs &args,
                          HandleObject proto);

    static inline DataViewObject *
    create(JSContext *cx, uint32_t byteOffset, uint32_t byteLength,
           Handle<ArrayBufferObject*> arrayBuffer, JSObject *proto);

    static bool getInt8Impl(JSContext *cx, CallArgs args);
    static bool fun_getInt8(JSContext *cx, unsigned argc, Value *vp);

    static bool getUint8Impl(JSContext *cx, CallArgs args);
    static bool fun_getUint8(JSContext *cx, unsigned argc, Value *vp);

    static bool getInt16Impl(JSContext *cx, CallArgs args);
    static bool fun_getInt16(JSContext *cx, unsigned argc, Value *vp);

    static bool getUint16Impl(JSContext *cx, CallArgs args);
    static bool fun_getUint16(JSContext *cx, unsigned argc, Value *vp);

    static bool getInt32Impl(JSContext *cx, CallArgs args);
    static bool fun_getInt32(JSContext *cx, unsigned argc, Value *vp);

    static bool getUint32Impl(JSContext *cx, CallArgs args);
    static bool fun_getUint32(JSContext *cx, unsigned argc, Value *vp);

    static bool getFloat32Impl(JSContext *cx, CallArgs args);
    static bool fun_getFloat32(JSContext *cx, unsigned argc, Value *vp);

    static bool getFloat64Impl(JSContext *cx, CallArgs args);
    static bool fun_getFloat64(JSContext *cx, unsigned argc, Value *vp);

    static bool setInt8Impl(JSContext *cx, CallArgs args);
    static bool fun_setInt8(JSContext *cx, unsigned argc, Value *vp);

    static bool setUint8Impl(JSContext *cx, CallArgs args);
    static bool fun_setUint8(JSContext *cx, unsigned argc, Value *vp);

    static bool setInt16Impl(JSContext *cx, CallArgs args);
    static bool fun_setInt16(JSContext *cx, unsigned argc, Value *vp);

    static bool setUint16Impl(JSContext *cx, CallArgs args);
    static bool fun_setUint16(JSContext *cx, unsigned argc, Value *vp);

    static bool setInt32Impl(JSContext *cx, CallArgs args);
    static bool fun_setInt32(JSContext *cx, unsigned argc, Value *vp);

    static bool setUint32Impl(JSContext *cx, CallArgs args);
    static bool fun_setUint32(JSContext *cx, unsigned argc, Value *vp);

    static bool setFloat32Impl(JSContext *cx, CallArgs args);
    static bool fun_setFloat32(JSContext *cx, unsigned argc, Value *vp);

    static bool setFloat64Impl(JSContext *cx, CallArgs args);
    static bool fun_setFloat64(JSContext *cx, unsigned argc, Value *vp);

    static JSObject *initClass(JSContext *cx);
    static void neuter(JSObject *view);
    static bool getDataPointer(JSContext *cx, Handle<DataViewObject*> obj,
                               CallArgs args, size_t typeSize, uint8_t **data);
    template<typename NativeType>
    static bool read(JSContext *cx, Handle<DataViewObject*> obj,
                     CallArgs &args, NativeType *val, const char *method);
    template<typename NativeType>
    static bool write(JSContext *cx, Handle<DataViewObject*> obj,
                      CallArgs &args, const char *method);

    void neuter();

  private:
    static const JSFunctionSpec jsfuncs[];
};

static inline int32_t
ClampIntForUint8Array(int32_t x)
{
    if (x < 0)
        return 0;
    if (x > 255)
        return 255;
    return x;
}

bool ToDoubleForTypedArray(JSContext *cx, JS::HandleValue vp, double *d);

} // namespace js

template <>
inline bool
JSObject::is<js::TypedArrayObject>() const
{
    return js::IsTypedArrayClass(getClass());
}

template <>
inline bool
JSObject::is<js::ArrayBufferViewObject>() const
{
    return is<js::DataViewObject>() || is<js::TypedArrayObject>();
}

#endif /* vm_TypedArrayObject_h */
