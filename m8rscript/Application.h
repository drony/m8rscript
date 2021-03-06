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

#include "ExecutionUnit.h"
#include "GPIOInterface.h"
#include "SystemInterface.h"
#include "TaskManager.h"
#include <functional>

namespace m8r {

static const char* MainFileName = "main";

class Program;
class Error;
class ExecutionUnit;
class FS;
class Shell;
class TCP;

class Application {
public:
    Application(FS*, SystemInterface*, uint16_t port);
    ~Application();
    
    FS* fileSystem() { return _fs; }
    SystemInterface* system() const { return _system; }
    
    bool load(Error&, bool debug, const char* name = nullptr);
    const ErrorList* syntaxErrors() const { return _syntaxErrors.empty() ? nullptr : &_syntaxErrors; }
    void run(std::function<void()>);
    void clear()
    {
        stop();
        if (!_program) {
            return;
        }
        _program = nullptr;
    }
    void pause();
    void stop();
    
    Program* program() const { return _program; }
    
    enum class NameValidationType { Ok, BadLength, InvalidChar };
    static NameValidationType validateFileName(const char* name);
    static NameValidationType validateBonjourName(const char* name);

private:
    class MyRunTask : public Task {
    public:
        MyRunTask() : _eu() { }
        
        void run(Program* program, std::function<void()> function)
        {
            stop();
            _function = function;
            _running = true;
            _eu.startExecution(program);
            runOnce();
        }
        
        void pause() { }
        
        bool stop()
        {
            if (_running) {
                _eu.requestTermination();
                return true;
            }
            return false;
        }

    private:
        virtual bool execute() override;
        
        ExecutionUnit _eu;
        std::function<void()> _function;
        bool _running = false;
    };

    class MyHeartbeatTask : public m8r::Task {
    public:
        MyHeartbeatTask(SystemInterface* system)
            : _system(system)
        {
            _system->gpio().enableHeartbeat();
            execute();
        }
        
    private:
        static constexpr uint32_t HeartrateMs = 3000;
        static constexpr uint32_t DownbeatMs = 50;
        
        virtual bool execute() override
        {
            _system->gpio().heartbeat(!_upbeat);
            _upbeat = !_upbeat;
            runOnce(_upbeat ? (HeartrateMs - DownbeatMs) : DownbeatMs);
            return true;
        }
        
        SystemInterface* _system;

        // Heartbeat is a short flash of the LED. The state of the LED is inverted from what it was
        // when the downbeat started 
        bool _upbeat = false; // When true, heartbeat is occuring
    };

    TCPDelegate* _shellSocket;
    FS* _fs;
    SystemInterface* _system;
    
    Program* _program = nullptr;
    MyRunTask _runTask;
    MyHeartbeatTask _heartbeatTask;
    
    ErrorList _syntaxErrors;
};
    
}
