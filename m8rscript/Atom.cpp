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

Atom AtomTable::atomizeString(const char* romstr) const
{
    size_t len = ROMstrlen(romstr);
    if (len > MaxAtomSize || len == 0) {
        return Atom();
    }

    SharedAtom sharedAtom = findSharedAtom(romstr, len);
    if (sharedAtom != SharedAtom::NoSharedAtom) {
        return ATOM(sharedAtom);
    }
    
    char* s = new char[len + 1];
    ROMCopyString(s, romstr);

    int32_t index = findAtom(s, len);
    if (index >= 0) {
        delete [ ] s;
        return Atom(static_cast<Atom::Raw>(index + NumSharedAtoms));
    }
    
    Atom a(static_cast<Atom::value_type>(_table.size() + NumSharedAtoms));
    for (size_t i = 0; i < len; ++i) {
        _table.push_back(s[i]);
    }
    _table.push_back('\xff');
    delete [ ] s;
    return a;
}

static int32_t atomLength(int8_t* p)
{
    for (int8_t* end = p; ; ++end) {
        if (*end == '\xff' || *end == '\0') {
            return static_cast<int32_t>(end - p);
        }
    }
}

m8r::String AtomTable::stringFromAtom(const Atom atom) const
{
    if (!atom) {
        return String();
    }
    
    if (atom.raw() < NumSharedAtoms) {
        const char* p = _sharedAtoms;
        while (1) {
            p = ROMstrstr(p, "\x01");
            if (!p) {
                break;
            }
            p++;
            if (readRomByte(reinterpret_cast<const uint8_t*>(p++)) == atom.raw()) {
                const char* end = ROMstrstr(p, "\x01");
                return end ? String(p, static_cast<int32_t>(end - p)) : String(p);
            }
        }
    }
    
    uint16_t index = atom.raw() - NumSharedAtoms;
    int32_t length = atomLength(&(_table[index]));
    assert(length > 0 && length <= MaxAtomSize);
    return m8r::String(reinterpret_cast<const char*>(&(_table[index])), length);
}

int32_t AtomTable::findAtom(const char* s, size_t length) const
{
    if (_table.size() == 0) {
        return -1;
    }

    const char* start = reinterpret_cast<const char*>(&(_table[0]));
    const char* p = start;
    while(p && *p != '\0') {
        p = strstr(p, s);
        if (!p) {
            break;
        }
        if (p == start || p[-1] == '\xff') {
            // The next char either needs to be '\xff' (meaning the end of this word) or the end of the string
            if (p[length] == '\xff' || p[length] == '\0') {
                return static_cast<int32_t>(p - start);
            }
        }
        p++;
    }
    
    return -1;
}

SharedAtom AtomTable::findSharedAtom(const char* s, size_t length) const
{
    // Each SharedAtom is precedded by 2 bytes, 0x01 and the id which is the enumerator cast into a byte.
    assert(_sharedAtoms[0] == '\x01' && _sharedAtoms[1] != '\0');
    const char* p = _sharedAtoms + 2;
    
    while(p && *p != '\0') {
        p = ROMstrstr(p, s);
        if (!p) {
            break;
        }
        if (readRomByte(reinterpret_cast<const uint8_t*>(p) - 2) == '\x01') {
            // The next char either needs to be 0x01 (meaning the end of this word) or the end of the string
            char c = readRomByte(reinterpret_cast<const uint8_t*>(p) + length);
            if (c == '\x01' || c == '\0') {
                return static_cast<SharedAtom>(readRomByte(reinterpret_cast<const uint8_t*>(p) - 1));
            }
        }
        p++;
    }
    
    return SharedAtom::NoSharedAtom;
}
