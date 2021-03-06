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

#include "EspTaskManager.h"

#include "Esp.h"

using namespace m8r;

EspTaskManager::EspTaskManager()
{
    system_os_task(executionTask, ExecutionTaskPrio, _executionTaskQueue, ExecutionTaskQueueLen);
    os_timer_disarm(&_executionTimer);
    os_timer_setfn(&_executionTimer, (os_timer_func_t*) &executionTimerTick, this);
}

EspTaskManager::~EspTaskManager()
{
}

TaskManager* TaskManager::shared()
{
    if (!_sharedTaskManager) {
        _sharedTaskManager = new EspTaskManager();
    }
    return _sharedTaskManager;
}

void EspTaskManager::postEvent()
{
    system_os_post(ExecutionTaskPrio, 0, reinterpret_cast<uint32_t>(this));
}

void EspTaskManager::executionTask(os_event_t *event)
{
    EspTaskManager* taskManager = reinterpret_cast<EspTaskManager*>(event->par);
    if (taskManager->empty()) {
        return;
    }

    int32_t now = taskManager->msNow();
    int32_t timeToNextEvent = taskManager->nextTimeToFire() - now;
    if (timeToNextEvent <= 5) {
        taskManager->fireEvent();
    } else {
        os_timer_arm(&taskManager->_executionTimer, timeToNextEvent, false);
    }
}

void EspTaskManager::executionTimerTick(void* data)
{
    EspTaskManager* taskManager = reinterpret_cast<EspTaskManager*>(data);
    taskManager->postEvent();
}
