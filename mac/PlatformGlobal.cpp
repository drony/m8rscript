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

#include "PlatformGlobal.h"

#include "ExecutionUnit.h"
#include "SystemInterface.h"

#include <ctime>
#include <thread>
#include <chrono>

using namespace m8r;

uint64_t PlatformGlobal::currentTime() const
{
    return static_cast<uint64_t>(std::clock() * 1000000 / CLOCKS_PER_SEC) - _startTime;
}

int32_t PlatformGlobal::callProperty(uint32_t index, Program* program, ExecutionUnit* eu, uint32_t nparams)
{
    int32_t result = Global::callProperty(index, program, eu, nparams);
    if (result >= 0) {
        return result;
    }
    
    switch(static_cast<Property>(index)) {
        case Property::GPIO_pinMode: {
            uint32_t pin = eu->stack().top(-1).toUIntValue();
            uint32_t mode = eu->stack().top().toUIntValue();
            _pinio = (_pinio & ~(1 << pin)) | ((mode == PLATFORM_GPIO_OUTPUT) ? (1 << pin) : 0);
            _system->updateGPIOState(_pinio, _pinstate);
            return 0;
        }
        case Property::GPIO_digitalWrite: {
            uint8_t pin = eu->stack().top(-1).toUIntValue();
            uint8_t state = eu->stack().top().toUIntValue();
            _pinstate = (_pinstate & ~(1 << pin)) | ((state == PLATFORM_GPIO_HIGH) ? (1 << pin) : 0);
            _system->updateGPIOState(_pinio, _pinstate);
            return 0;
        }
        case Property::System_delay: {
            uint32_t ms = eu->stack().top().toUIntValue();
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            return 0;
        }

        default: return -1;
    }
}
