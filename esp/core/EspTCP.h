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

#include "TCP.h"

#include "Containers.h"

#include <lwip/init.h>
#include <lwip/tcp.h>

namespace m8r {

class EspTCP : public TCP {
public:
    virtual ~EspTCP();
    
    virtual void send(int16_t connectionId, char c) override { send(connectionId, &c, 1); }
    virtual void send(int16_t connectionId, const char* data, uint16_t length = 0) override
    {
        if (connectionId < 0 || connectionId >= MaxConnections || !_clients[connectionId].inUse()) {
            return;
        }
        _clients[connectionId].send(data, length);
    }
    
    virtual void disconnect(int16_t connectionId) override
    {
        if (connectionId < 0 || connectionId >= MaxConnections || !_clients[connectionId].inUse()) {
            return;
        }
        _clients[connectionId].disconnect();
        _delegate->TCPevent(this, TCPDelegate::Event::SentData, connectionId);
    }
    
    EspTCP(TCPDelegate*, uint16_t, IPAddr);

private:
    static err_t _clientConnected(void* arg, tcp_pcb *pcb, err_t err) { return reinterpret_cast<EspTCP*>(arg)->clientConnected(pcb, err); }
    err_t clientConnected(tcp_pcb* pcb, err_t err);


    class Client {
    public:
        Client() : _pcb(nullptr) { }
        Client(tcp_pcb* pcb, EspTCP* tcp) : _pcb(pcb)
        {
            tcp_arg(pcb, tcp);
            tcp_recv(pcb, _recv);
            tcp_sent(pcb, _sent);
        }
        
        bool inUse() const { return _pcb; }

        void send(const char* data, uint16_t length = 0);
        void sent(uint16_t len);
        void disconnect();
        
        bool matches(tcp_pcb* pcb) { return pcb == _pcb; }
        
    private:
        tcp_pcb* _pcb;
        bool _sending = false;
        String _buffer;
    };

    static err_t _accept(void *arg, tcp_pcb* pcb, int8_t err) { return reinterpret_cast<EspTCP*>(arg)->accept(pcb, err); }
    static err_t _recv(void *arg, tcp_pcb* pcb, pbuf* buf, int8_t err) { return reinterpret_cast<EspTCP*>(arg)->recv(pcb, buf, err); }
    static err_t _sent(void *arg, tcp_pcb* pcb, u16_t len) { return reinterpret_cast<EspTCP*>(arg)->sent(pcb, len); }

    err_t accept(tcp_pcb*, int8_t err);
    err_t recv(tcp_pcb*, pbuf*, int8_t err);
    err_t sent(tcp_pcb*, u16_t len);

    int16_t findConnection(tcp_pcb*);
    
    tcp_pcb* _listenpcb;
        
    Client _clients[MaxConnections];
};

}
