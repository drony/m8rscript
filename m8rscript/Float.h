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

namespace m8r {

class Float
{
public:
    static Float makeFloat(uint32_t i, uint32_t f, uint8_t dp, int32_t e)
    {
#ifdef FIXED_POINT_FLOAT
        int32_t num = i*1000;
        while (dp < 3) {
            ++dp;
            f *= 10;
        }
        while (dp > 3) {
            --dp;
            f /= 10;
        }
        while ( f > 1000) {
            f /= 10;
        }
        num += f;
        while (e > 0) {
            --e;
            num *= 10;
        }
        while (e < 0) {
            ++e;
            num /= 10;
        }
        Float f;
        f._v = num;
        return f;
#else
        float num = (float) f;
        while (dp-- > 0)
            num /= 10;
        num += (float) i;
        while (e > 0) {
            --e;
            num *= 10;
        }
        while (e < 0) {
            ++e;
            num /= 10;
        }
        Float v;
        v._v = num;
        return v;
#endif
    }
    
    Float& operator=(const Float& other) { _v = other._v; return *this; }
    Float& operator=(Float& other) { _v = other._v; return *this; }
    
    Float operator+(const Float& other) { Float r; r._v = _v + other._v; return *this; }
    Float operator-(const Float& other) { Float r; r._v = _v - other._v; return *this; }

#ifdef FIXED_POINT_FLOAT
    Float operator*(const Float& other) { Float r; r._v = _v * other._v / 1000; return *this; }
    Float operator/(const Float& other) { Float r; r._v = _v * 1000 / other._v; return *this; }
#else
    Float operator*(const Float& other) { Float r; r._v = _v * other._v; return *this; }
    Float operator/(const Float& other) { Float r; r._v = _v / other._v; return *this; }
#endif
    
private:
#ifdef FIXED_POINT_FLOAT
    int32_t _v;
#else
    float _v;
#endif
};

}
