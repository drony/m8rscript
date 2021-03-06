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

#include "Iterator.h"

#include "ExecutionUnit.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

Iterator::Iterator(Program* program)
    : ObjectFactory(program, ATOM(program, Iterator))
    , _constructor(constructor)
    , _done(done)
    , _next(next)
    , _getValue(getValue)
    , _setValue(setValue)
{
    addProperty(ATOM(program, constructor), &_constructor);
    addProperty(ATOM(program, done), &_done);
    addProperty(ATOM(program, next), &_next);
    addProperty(ATOM(program, getValue), &_getValue);
    addProperty(ATOM(program, setValue), &_setValue);
}

static bool done(ExecutionUnit* eu, Value thisValue, Object*& obj, int32_t& index)
{
    obj = thisValue.property(eu, ATOM(eu, __object)).asObject();
    index = thisValue.property(eu, ATOM(eu, __index)).asIntValue();
    if (!obj) {
        return true;
    }
    int32_t size = obj->property(eu, ATOM(eu, length)).asIntValue();
    return index >= size;
}

CallReturnValue Iterator::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    Object* obj = eu->stack().top(1 - nparams).asObject();
    if (!obj) {
        return CallReturnValue(CallReturnValue::Error::InvalidArgumentValue);
    }
    
    thisValue.setProperty(eu, ATOM(eu, __object), Value(obj), Value::SetPropertyType::AlwaysAdd);
    thisValue.setProperty(eu, ATOM(eu, __index), Value(0), Value::SetPropertyType::AlwaysAdd);
    
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Iterator::done(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Object* obj;
    int32_t index;
    eu->stack().push(Value(::done(eu, thisValue, obj, index)));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue Iterator::next(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Object* obj;
    int32_t index;
    if (!::done(eu, thisValue, obj, index)) {
        ++index;
        if (!thisValue.setProperty(eu, ATOM(eu, __index), Value(index), Value::SetPropertyType::NeverAdd)) {
            return CallReturnValue(CallReturnValue::Error::InternalError);
        }
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Iterator::getValue(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Object* obj;
    int32_t index;
    if (!::done(eu, thisValue, obj, index)) {
        eu->stack().push(obj->element(eu, Value(index)));
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue Iterator::setValue(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    Object* obj;
    int32_t index;
    if (!::done(eu, thisValue, obj, index)) {
        obj->setElement(eu, Value(index), eu->stack().top(1 - nparams), false);
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
