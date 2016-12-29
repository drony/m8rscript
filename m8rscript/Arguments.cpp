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

#include "Arguments.h"

#include "ExecutionUnit.h"
#include "Program.h"

using namespace m8r;

Arguments::Arguments(Program* program)
{
    program->addObject(this, false);
    _lengthAtom = program->atomizeString("length");
}

const Value Arguments::element(ExecutionUnit* eu, const Value& elt) const
{
    int32_t index = elt.toIntValue(eu);
    return (index >= 0 && index < eu->argumentCount()) ? eu->argument(index) : Value();
}
bool Arguments::setElement(ExecutionUnit* eu, const Value& elt, const Value& value)
{
    int32_t index = elt.toIntValue(eu);
    if (index < 0 || index >= eu->argumentCount()) {
        return false;
    }
    eu->argument(index) = value;
    return true;
}

const Value Arguments::property(ExecutionUnit* eu, const Atom& name) const
{
    return (name == _lengthAtom) ? Value(static_cast<int32_t>(eu->argumentCount())) : Value();
}

bool Arguments::setProperty(ExecutionUnit* eu, const Atom& name, const Value& value)
{
    if (name == _lengthAtom) {
        Error::printError(Error::Code::RuntimeError, -1, "length property of arguments is read-only");
    }
    return false;
}

Atom Arguments::propertyName(ExecutionUnit*, uint32_t index) const
{
    switch(static_cast<Property>(index)) {
        case Property::Length: return _lengthAtom;
        default: return Atom();
    }
}

size_t Arguments::propertyCount(ExecutionUnit*) const
{
    return PropertyCount;
}
