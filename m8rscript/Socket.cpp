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

#include "Socket.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "Program.h"

using namespace m8r;

Socket::Socket(Program* program)
{
    program->addObject(this, false);
}

const Value Socket::property(ExecutionUnit*, const Atom& prop) const
{
    // FIXME: Implement
    return Value();
}

bool Socket::setProperty(ExecutionUnit*, const Atom& prop, const Value& value, bool add)
{
    // FIXME: Implement
    return false;
}

CallReturnValue Socket::callProperty(ExecutionUnit*, Atom prop, uint32_t nparams)
{
    // FIXME: Implement
    return CallReturnValue(CallReturnValue::Type::Error);
}

void Socket::TCPevent(TCP* tcp, Event event, int16_t connectionId, const char* data, uint16_t length)
{
}

SocketProto::SocketProto(Program* program)
    : ObjectFactory(program, ROMSTR("__Socket"))
    , ___construct(__construct)
{
    addObject(program, ATOM(__construct), &___construct);
}

CallReturnValue SocketProto::__construct(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Socket* sock = new Socket(eu->program());
    eu->stack().push(Value(sock->objectId()));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}



