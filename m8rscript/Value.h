/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
    
    - Redistributions in binary form must reproduce the above copyright 
    notice, this list of conditions and the following disclaimer in the 
    documentation and/or other materials provided with the distribution.
    
    - Neither the name of the <ORGANIZATION> nor the names of its 
    contributors may be used to endorse or promote products derived from 
    this software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#pragma once

#include "Atom.h"
#include "Containers.h"
#include "Error.h"
#include "Float.h"

namespace m8r {

class Object;
//class NativeObject;
class Value;
class ExecutionUnit;
class Program;
class Stream;

class CallReturnValue {
public:
    static constexpr uint32_t MaxReturnCount = 999;
    static constexpr uint32_t FunctionStartValue = 1000;
    static constexpr uint32_t ErrorValue = 1001;
    static constexpr uint32_t FinishedValue = 1002;
    static constexpr uint32_t TerminatedValue = 1003;
    static constexpr uint32_t WaitForEventValue = 1004;
    static constexpr uint32_t ContinueValue = 1004;
    static constexpr uint32_t MaxMsDelay = 6000000;
    
    enum class Type { ReturnCount = 0, MsDelay = 1, FunctionStart, Error, Finished, Terminated, WaitForEvent, Continue };
    
    CallReturnValue(Type type = Type::ReturnCount, uint32_t value = 0)
    {
        switch(type) {
            case Type::ReturnCount: assert(value <= MaxReturnCount); _value = value; break;
            case Type::MsDelay: assert(value > 0 && value <= MaxMsDelay); _value = -value; break;
            case Type::FunctionStart: _value = FunctionStartValue; break;
            case Type::Error: _value = ErrorValue; break;
            case Type::Finished: _value = FinishedValue; break;
            case Type::Terminated: _value = TerminatedValue; break;
            case Type::WaitForEvent: _value = WaitForEventValue; break;
            case Type::Continue: _value = ContinueValue; break;
        }
    }
    
    bool isFunctionStart() const { return _value == FunctionStartValue; }
    bool isError() const { return _value == ErrorValue; }
    bool isFinished() const { return _value == FinishedValue; }
    bool isTerminated() const { return _value == TerminatedValue; }
    bool isWaitForEvent() const { return _value == WaitForEventValue; }
    bool isContinue() const { return _value == ContinueValue; }
    bool isReturnCount() const { return _value >= 0 && _value <= MaxReturnCount; }
    bool isMsDelay() const { return _value < 0 && _value >= -MaxMsDelay; }
    uint32_t msDelay() const { assert(isMsDelay()); return -_value; }
    uint32_t returnCount() const { assert(isReturnCount()); return _value; }

private:
    int32_t _value = 0;
};
        
class Value {
public:
#ifdef __APPLE__
    typedef __uint128_t RawValueType;
    typedef uint64_t RawIntType;
    static_assert(sizeof(RawValueType) == 16, "RawValue must be 128 bits");
#else
    typedef uint64_t RawValueType;
    typedef uint32_t RawIntType;
    static_assert(sizeof(RawValueType) == 8, "RawValue must be 64 bits");
#endif
    static_assert(sizeof(RawIntType) == sizeof(void*), "RawIntValue must be the same size as a pointer");

    static Value fromRaw(RawValueType r) { Value v; v._value._raw = r; return v; }
    static RawValueType toRaw(Value v) { return v._value._raw; }

    typedef m8r::Map<Atom, Value> Map;
    
    enum class Type : uint8_t {
        None = 0,
        Object, Float, Integer, String, StringLiteral, Id, Null, NativeObject,
        PreviousContextA, PreviousContextB,
    };

    Value() { }
    
    Value(ObjectId objectId) : _value(objectId) { }
    Value(Type type) : _value(type) { }
    Value(Float value) : _value(value) { }
    Value(int32_t value) : _value(value) { }
    Value(Atom value) : _value(value) { }
    Value(StringId stringId) : _value(stringId) { }
    Value(StringLiteral stringId) : _value(stringId) { }
    Value(uint32_t prevPC, ObjectId prevObj) : _value(prevPC, prevObj) { }
    Value(uint32_t prevFrame, ObjectId prevThis, uint8_t prevParamCount, bool ctor) : _value(prevFrame, prevThis, prevParamCount, ctor) { }
    //Value(NativeObject* obj) : _value(obj) { }
    
    operator bool() const { return type() != Type::None; }
    bool operator==(const Value& other) { return _value == other._value; }
    bool operator!=(const Value& other) { return _value != other._value; }

    ~Value() { }
    
    bool serialize(Stream*, Error&) const
    {
        // FIXME: Implement
        return false;
    }

    bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&)
    {
        // FIXME: Implement
        return false;
    }

    Type type() const { return _value.type(); }
    
    //
    // asXXX() functions are lightweight and simply cast the Value to that type. If not the correct type it returns 0 or null
    // toXXX() functions are heavyweight and attempt to convert the Value type to a primitive of the requested type
    
    ObjectId asObjectIdValue() const { return (type() == Type::Object || type() == Type::PreviousContextA || type() == Type::PreviousContextB) ? objectIdFromValue() : ObjectId(); }
    StringId asStringIdValue() const { return (type() == Type::String) ? stringIdFromValue() : StringId(); }
    StringLiteral asStringLiteralValue() const { return (type() == Type::StringLiteral) ? stringLiteralFromValue() : StringLiteral(); }
    int32_t asIntValue() const { return (type() == Type::Integer) ? intFromValue() : 0; }
    uint32_t asPreviousPCValue() const { return (type() == Type::PreviousContextA) ? intFromValue() : 0; }
    uint32_t asPreviousFrameValue() const { return (type() == Type::PreviousContextB) ? intFromValue() : 0; }
    uint32_t asPreviousParamCountValue() const { return (type() == Type::PreviousContextB) ? paramCountFromValue() : 0; }
    bool asCtorValue() const { return (type() == Type::PreviousContextB) ? ctorFromValue() : 0; }
    Float asFloatValue() const { return (type() == Type::Float) ? floatFromValue() : Float(); }
    Atom asIdValue() const { return (type() == Type::Id) ? atomFromValue() : Atom(); }
    //NativeObject* asNativeObject() const { return (type() == Type::NativeObject) ? nativeObjectFromValue() : nullptr; }
    
    m8r::String toStringValue(ExecutionUnit*) const;
    bool toBoolValue(ExecutionUnit* eu) const { return toIntValue(eu) != 0; }
    Float toFloatValue(ExecutionUnit* eu) const
    {
        if (type() == Type::Float) {
            return floatFromValue();
        }
        return _toFloatValue(eu);
    }

    int32_t toIntValue(ExecutionUnit* eu) const
    {
        if (type() == Type::Integer) {
            return intFromValue();
        }
        return static_cast<int32_t>(toFloatValue(eu));
    }
    
    Atom toIdValue(ExecutionUnit* eu) const
    {
        if (type() == Type::Id) {
            return atomFromValue();
        }
        return _toIdValue(eu);
    }
    
    bool isInteger() const { return type() == Type::Integer; }
    bool isFloat() const { return type() == Type::Float; }
    bool isNumber() const { return isInteger() || isFloat(); }
    bool isNone() const { return type() == Type::None; }
    bool isObjectId() const { return type() == Type::Object || type() == Type::PreviousContextA || type() == Type::PreviousContextB; }
    
    static m8r::String toString(Float value);
    static m8r::String toString(int32_t value);
    static bool toFloat(Float&, const char*, bool allowWhitespace = true);
    static bool toInt(int32_t&, const char*, bool allowWhitespace = true);
    static bool toUInt(uint32_t&, const char*, bool allowWhitespace = true);
    
    void gcMark(ExecutionUnit* eu) 
    {
        if (asObjectIdValue() || asStringIdValue()) {
            _gcMark(eu);
        }
    }
    
    CallReturnValue call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor);
    
    bool needsGC() const { return type() == Type::Object || type() == Type::PreviousContextA || type() == Type::PreviousContextB || type() == Type::String; }
    
private:
    Float _toFloatValue(ExecutionUnit*) const;
    Atom _toIdValue(ExecutionUnit*) const;
    void _gcMark(ExecutionUnit*);

    inline Float floatFromValue() const { return Float(static_cast<Float::value_type>(_value._raw & ~1)); }
    inline int32_t intFromValue() const { return static_cast<int32_t>(_value.raw32.get()); }
    inline Atom atomFromValue() const { return Atom(static_cast<Atom::value_type>(_value.raw32.get())); }
    inline ObjectId objectIdFromValue() const { return ObjectId(static_cast<ObjectId::value_type>(_value.raw16.get())); }
    inline StringId stringIdFromValue() const { return StringId(static_cast<StringId::value_type>(_value.raw32.get())); }
    inline StringLiteral stringLiteralFromValue() const { return StringLiteral(static_cast<StringLiteral::value_type>(_value.raw32.get())); }
    inline uint8_t paramCountFromValue() const { return _value.raw8.get(); }
    inline bool ctorFromValue() const { return _value.rawBool.get(); }
    //inline NativeObject* nativeObjectFromValue() const { return reinterpret_cast<NativeObject*>(_value.rawPtr.get()); }
    
    // The motivation for this RawValue structure is to keep a value in 64 bits on Esp. We need to store a pointer as well as a
    // type field. That works fine for Esp since pointers are 32 bits. But it doesn't work for Mac which has 64 bit pointers, so
    // there would be no room for the type. So we make the structure 128 bits for Mac. Fortunately Mac supports the __uint128_t
    // type to make it easy to store this as a raw value.
    //
    // The problematic type on Esp is Float. We need to keep it and a type field in 64 bits. Float is a fixed point value so we
    // can do fast integer math on Esp. We don't need to use all 64 bits for Float, but fixed point limits its range, so we want
    // as many bits as possible. So we use a single bit in the LSB. If this is on, we know this is a Float. For other types we
    // use the lowest 5 bits and make all the enum values even. For floats, we take the raw value, clear the LSB and cast it to
    // Float. For the others we cast the lower 5 bits to Type and use that. 
    struct RawValue {
        RawValue() { rawType.set(Type::None); }
        RawValue(Type type) { rawType.set(type); }
        RawValue(Float f) { _raw = f.raw(); rawType.set(Type::Float); }
        RawValue(int32_t i) { raw32.set(i); rawType.set(Type::Integer); }
        RawValue(Atom atom) { raw32.set(atom.raw()); rawType.set(Type::Id); }
        RawValue(StringId id) { raw32.set(id.raw()); rawType.set(Type::String); }
        RawValue(StringLiteral id) { raw32.set(id.raw()); rawType.set(Type::StringLiteral); }
        RawValue(ObjectId id) { raw16.set(id.raw()); rawType.set(Type::Object); }
        //RawValue(NativeObject* obj) { rawPtr.set(id.raw()); rawType.set(Type::Object); }
        
        RawValue(uint32_t prevPC, ObjectId prevObj)
        {
            raw32.set(prevPC);
            raw16.set(prevObj.raw());
            rawType.set(Type::PreviousContextA);
        }
        
        RawValue(uint32_t prevFrame, ObjectId prevThis, uint8_t prevParamCount, bool ctor)
        {
            raw32.set(prevFrame);
            raw16.set(prevThis.raw());
            raw8.set(prevParamCount);
            rawBool.set(ctor);
            rawType.set(Type::PreviousContextB);
        }
        
        Type type() const { return rawType.get(); }
        
        bool operator==(const RawValue& other) { return _raw == other._raw; }
        bool operator!=(const RawValue& other) { return !(*this == other); }

        // 64 bit value can hold one 32 bit value, one 16 bit value, one 8 bit value,a 1 bit flag, and a 5 bit type.
        // On Mac, the 32 bit value is replaced with a 64 bit value so it can hold a pointer. This is typedefed to
        // a RawIntType.
        template <typename type, uint8_t BitCount, uint8_t Shift>
        struct RawComponent {
            static constexpr RawValueType Mask = ((static_cast<RawValueType>(1) << BitCount) - 1) << Shift;
            
            type get(RawValueType raw) const { return static_cast<type>((raw & Mask) >> Shift); }
            
            void set(RawValueType& raw, type v)
            {
                raw = (_raw & ~Mask | ((static_cast<RawValueType>(v) << Shift) & Mask);
            }
        };

        RawComponent<Type, 5, 0> rawType;
        //RawComponent<NativeObject*, sizeof(NativeObject*) * 8, 32> rawPtr;
        RawComponent<uint32_t, 32, 32> raw32;
        RawComponent<uint16_t, 16, 16> raw16;
        RawComponent<uint8_t, 8, 8> raw8;
        RawComponent<bool, 1, 7> rawBool;

        RawValueType _raw = 0;
    };
        
    RawValue _value;
    
    // In order to fit everything, we have some requirements
    static_assert(sizeof(_value) >= sizeof(Float), "Value must be large enough to hold a Float");
};

template<>
inline Type RawComponent<Type, 5, 0>::get() const { return ((_raw & 1) == 1) ? Type::Float : static_cast<Type>(_raw & TypeMask); }

template<>
inline void RawComponent<Type, 5, 0>::set(Type type)
{
    if (type == Type::Float) {
        _raw |= 1;
    } else {
        _raw = (_raw & ~TypeMask) | static_cast<RawValueType>(type);
    }
}

}
