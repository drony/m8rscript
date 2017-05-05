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

#include "Atom.h"
#include "Program.h"

using namespace m8r;

AtomTable::AtomTable()
{
}

Atom AtomTable::internalAtom(SharedAtom a) const
{
    auto it = _sharedAtomMap.find(static_cast<int32_t>(a));
    if (it == _sharedAtomMap.end()) {
        Atom atom = atomizeString(sharedAtom(a));
        _sharedAtomMap.emplace(static_cast<int32_t>(a), atom);
        return atom;
    }
    return it->value;
}

Atom AtomTable::atomizeString(const char* romstr) const
{
    int32_t index = findSharedAtom(romstr);
    if (index >= 0) {
        return Atom(static_cast<Atom::Raw>(index));
    }
    
    size_t len = ROMstrlen(romstr);
    if (len > MaxAtomSize || len == 0) {
        return Atom();
    }

    char* s = new char[len + 1];
    ROMCopyString(s, romstr);
    
    int32_t offset = sizeof(_sharedAtoms);
    
    index = findAtom(s);
    if (index >= 0) {
        delete [ ] s;
        return Atom(static_cast<Atom::Raw>(index + offset));
    }
    
    Atom a(static_cast<Atom::Raw>(_table.size() + offset));
    for (size_t i = 0; i < len; ++i) {
        _table.push_back(s[i]);
    }
    _table.push_back('\xff');
    delete [ ] s;
    return a;
}

int32_t AtomTable::findAtom(const char* s) const
{
    size_t len = strlen(s);

    if (_table.size() == 0) {
        return -1;
    }

    const char* start = reinterpret_cast<const char*>(&(_table[0]));
    const char* p = start;
    while(p && *p != '\0') {
        p++;
        p = strstr(p, s);
        if (p && (p == start || p[-1] == '\0')) {
        // The next char either needs to be '\xff' (meaning the end of this word) or the end of the string
        if (p[len] == '\xff') {
                return static_cast<int32_t>(p - start);
            }
        }
    }
    
    return -1;
}

int32_t AtomTable::findSharedAtom(const char* s) const
{
    size_t len = ROMstrlen(s);

    const char* p = _sharedAtoms;
    while(p && *p != '\0') {
        p++;
        p = ROMstrstr(p, s);
        if (p && (p == _sharedAtoms || p[-1] == '\xff')) {
            // The next char either needs to be '\xff' (meaning the end of this word) or the end of the string
            if (p[len] == '\xff') {
                return static_cast<int32_t>(p - _sharedAtoms);
            }
        }
    }
    
    return -1;
}
