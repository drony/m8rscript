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

#include "Base64.h"
#include "GPIO.h"
#include "Iterator.h"
#include "JSON.h"
#include "Object.h"
#include "TCP.h"

namespace m8r {

class Function;

class Global : public ObjectFactory {
public:
    Global(Program*);
    virtual ~Global();

private:
    MaterObject _object;
    MaterObject _array;
    Base64 _base64;
    GPIO _gpio;
    JSON _json;
    TCPProto _tcp;
    UDPProto _udp;
    IPAddrProto _ipAddr;
    Iterator _iterator;

    static CallReturnValue currentTime(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue delay(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue print(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue printf(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue println(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue toFloat(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue toInt(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue toUInt(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue arguments(ExecutionUnit*, Value thisValue, uint32_t nparams);

    NativeFunction _currentTime;
    NativeFunction _delay;
    NativeFunction _print;
    NativeFunction _printf;
    NativeFunction _println;
    NativeFunction _toFloat;
    NativeFunction _toInt;
    NativeFunction _toUInt;
    NativeFunction _arguments;
};
    
}
