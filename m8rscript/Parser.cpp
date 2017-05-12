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

#include "Parser.h"

#include "ParseEngine.h"
#include "ExecutionUnit.h"
#include <limits>

using namespace m8r;

uint32_t Parser::_nextLabelId = 1;

Parser::Parser(SystemInterface* system)
    : _parseStack(this)
    , _program(new Program(system))
{
}

void Parser::parse(m8r::Stream* istream, bool debug)
{
    _debug = debug;
    _scanner.setStream(istream);
    ParseEngine p(this);
    _functions.emplace_back(_program, false);
    while(1) {
        if (!p.statement()) {
            Scanner::TokenType type;
            if (_scanner.getToken(type) != Token::EndOfFile) {
                expectedError(Token::EndOfFile);
            }
            break;
        }
    }
    functionEnd();
}

void Parser::printError(const char* format, ...)
{
    ++_nerrors;

    va_list args;
    va_start(args, format);
    Error::vprintError(_program->system(), Error::Code::ParseError, _scanner.lineno(), format, args);
    va_end(args);
    
    char s[80];
    va_start(args, format);
    ROMvsnprintf(s, 79, format, args);
    _syntaxErrors.emplace_back(s, _scanner.lineno(), 1, 1);
    va_end(args);
}

void Parser::unknownError(Token token)
{
    uint8_t c = static_cast<uint8_t>(token);
    printError(ROMSTR("unknown token (%s)"), Value::toString(c).c_str());
}

void Parser::expectedError(Token token)
{
    char c = static_cast<char>(token);
    if (c >= 0x20 && c <= 0x7f) {
        printError(ROMSTR("syntax error: expected '%c'"), c);
    } else {
        switch(token) {
            case Token::DuplicateDefault: printError(ROMSTR("multiple default cases not allowed")); break;
            case Token::Expr: printError(ROMSTR("expression")); break;
            case Token::PropertyAssignment: printError(ROMSTR("property assignment")); break;
            case Token::Statement: printError(ROMSTR("statement expected")); break;
            case Token::Identifier: printError(ROMSTR("identifier")); break;
            case Token::MissingVarDecl: printError(ROMSTR("missing var declaration")); break;
            case Token::OneVarDeclAllowed: printError(ROMSTR("only one var declaration allowed here")); break;
            case Token::ConstantValueRequired: printError(ROMSTR("constant value required")); break;
            case Token::EndOfFile: printError(ROMSTR("unable to continue parsing")); break;
            default: printError(ROMSTR("*** UNKNOWN TOKEN ***")); break;
        }
    }
}

Label Parser::label()
{
    Label label;
    if (!_nerrors) {
        label.label = static_cast<int32_t>(_deferred ? _deferredCode.size() : currentFunction()->code()->size());
        label.uniqueID = _nextLabelId++;
    }
    return label;
}

void Parser::addMatchedJump(Op op, Label& label)
{
    if (_nerrors) return;
    
    assert(op == Op::JMP || op == Op::JT || op == Op::JF);

    uint32_t reg = 0;
    if (op != Op::JMP) {
        reg = _parseStack.bake();
        _parseStack.pop();
    }
    // Emit opcode with a dummy address
    label.matchedAddr = static_cast<int32_t>(_deferred ? _deferredCode.size() : currentFunction()->code()->size());
    emitCodeRSN(op, reg, 0);
}

void Parser::doMatchJump(int32_t matchAddr, int32_t jumpAddr)
{
    if (_nerrors) return;
    
    if (jumpAddr < -32767 || jumpAddr > 32767) {
        printError(ROMSTR("JUMP ADDRESS TOO BIG TO EXIT LOOP. CODE WILL NOT WORK!\n"));
        return;
    }
    
    Instruction inst = _deferred ? _deferredCode.at(matchAddr) : currentFunction()->code()->at(matchAddr);
    Op op = static_cast<Op>(inst.op());
    assert(op == Op::JMP || op == Op::JF || op == Op::JT);
    uint32_t reg = inst.ra();
    if (_deferred) {
        _deferredCode.at(matchAddr) = Instruction(op, reg, jumpAddr);
    } else {
        currentFunction()->code()->at(matchAddr) = Instruction(op, reg, jumpAddr);
    }
}

void Parser::jumpToLabel(Op op, Label& label)
{
    if (_nerrors) return;
    
    assert(op == Op::JMP || op == Op::JF || op == Op::JT);
    int32_t jumpAddr = label.label - static_cast<int32_t>(_deferred ? _deferredCode.size() : currentFunction()->code()->size());
    
    uint32_t r = 0;
    if (op != Op::JMP) {
        r = _parseStack.bake();
        _parseStack.pop();
    }

    emitCodeRSN(op, (op == Op::JMP) ? 0 : r, jumpAddr);
}

void Parser::emitCodeRRR(Op op, uint32_t ra, uint32_t rb, uint32_t rc)
{
    emitLineNumber();
    addCode(Instruction(op, ra, rb, rc));
}

void Parser::emitCodeRUN(Op op, uint32_t rn, uint32_t n)
{
    emitLineNumber();
    addCode(Instruction(op, rn, n));
}

void Parser::emitCodeRSN(Op op, uint32_t rn, int32_t n)
{
    // Tbis Op is used for jumps, so we need to put the line number after
    addCode(Instruction(op, rn, n));
    emitLineNumber();
}

void Parser::emitCodeCall(Op op, uint32_t rcall, uint32_t rthis, uint32_t nparams)
{
    emitLineNumber();
    addCode(Instruction(op, rcall, rthis, nparams, true));
}

void Parser::addCode(Instruction inst)
{
    if (_deferred) {
        assert(_deferredCodeBlocks.size() > 0);
        _deferredCode.push_back(inst);
    } else {
        currentFunction()->code()->push_back(inst);
    }
}

void Parser::pushK(StringLiteral::Raw s)
{
    if (_nerrors) return;
    
    ConstantId id = currentFunction()->addConstant(Value(StringLiteral(s)));
    _parseStack.pushConstant(id.raw());
}

void Parser::pushK(const char* s)
{
    if (_nerrors) return;
    
    ConstantId id = currentFunction()->addConstant(Value(_program->addStringLiteral(s)));
    _parseStack.pushConstant(id.raw());
}

void Parser::pushK(uint32_t value)
{
    if (_nerrors) return;
    
    ConstantId id = currentFunction()->addConstant(Value(value));
    _parseStack.pushConstant(id.raw());
}

void Parser::pushK(Float value)
{
    if (_nerrors) return;
    
    ConstantId id = currentFunction()->addConstant(Value(value));
    _parseStack.pushConstant(id.raw());
}

void Parser::pushK(bool value)
{
    if (_nerrors) return;
    
    // FIXME: Support booleans as a first class type
    ConstantId id = currentFunction()->addConstant(Value(value ? 1 : 0));
    _parseStack.pushConstant(id.raw());
}

void Parser::pushK()
{
    if (_nerrors) return;
    
    ConstantId id = currentFunction()->addConstant(Value::NullValue());
    _parseStack.pushConstant(id.raw());
}

void Parser::pushK(Function* function)
{
    if (_nerrors) return;
    
    assert(function);
    ConstantId id = currentFunction()->addConstant(Value(function));
    _parseStack.pushConstant(id.raw());
}

void Parser::pushK(MaterObject* obj)
{
    if (_nerrors) return;
    
    assert(obj);
    ConstantId id = currentFunction()->addConstant(Value(obj));
    _parseStack.pushConstant(id.raw());
}

void Parser::addNamedFunction(Function* func, const Atom& name)
{
    if (_nerrors) return;
    
    currentFunction()->addConstant(Value(func));
    func->setName(name);
}

void Parser::pushThis()
{
    if (_nerrors) return;
    _parseStack.push(ParseStack::Type::This, 0);
}

void Parser::pushTmp()
{
    if (_nerrors) return;
    _parseStack.pushRegister();
}

void Parser::emitId(const char* s, IdType type)
{
    if (_nerrors) return;
    
    emitId(_program->atomizeString(s), type);
}

void Parser::emitId(const Atom& atom, IdType type)
{
    if (_nerrors) return;
    
    if (type == IdType::MightBeLocal || type == IdType::MustBeLocal) {
        // See if it's a local function
        for (uint32_t i = 0; i < currentFunction()->constantCount(); ++i) {
            Object* func = currentFunction()->constant(ConstantId(i)).asObject();
            if (func) {
                if (func->name() == atom) {
                    _parseStack.pushConstant(i);
                    return;
                }
            }
        }
        
        // Find the id in the function chain
        bool local = true;
        uint16_t frame = 0;
        for (int32_t i = static_cast<int32_t>(_functions.size()) - 1; i >= 0; --i) {
            Function* function = _functions[i]._function;
            int32_t index = function->localIndex(atom);
            
            if (index >= 0) {
                if (local) {
                    _parseStack.push(ParseStack::Type::Local, static_cast<uint32_t>(index));
                    return;
                }
                
                _parseStack.push(ParseStack::Type::UpValue, static_cast<uint32_t>(currentFunction()->addUpValue(index, frame, atom)));
                return;
            }
            local = false;
            frame++;
            
            if (type == IdType::MustBeLocal) {
                String s = "nonexistent variable '";
                s += _program->stringFromAtom(atom);
                s += "'";
                printError(s.c_str());
                return;
            }
        }
    }
    
    if (atom == SharedAtom::value) {
        _parseStack.push((type == IdType::NotLocal) ? ParseStack::Type::Constant : ParseStack::Type::RefK, 0);
        _parseStack.setIsValue(true);
        return;
    }
    
    ConstantId id = currentFunction()->addConstant(Value(atom));
    _parseStack.push((type == IdType::NotLocal) ? ParseStack::Type::Constant : ParseStack::Type::RefK, id.raw());
}

void Parser::emitMove()
{
    if (_nerrors) return;
    
    // RefK needs to do a STOREFK TOS-1, TOS and then leave TOS-1 (the source) on the stack
    // All others need to do a save and leave TOS (the dest) on the stack
    uint32_t srcReg = _parseStack.bake();
    _parseStack.swap();
    ParseStack::Type dstType = _parseStack.topType();
    
    switch(dstType) {
        case ParseStack::Type::This:
            printError(ROMSTR("Assignment to 'this' not allowed\n"));
            break;
        case ParseStack::Type::Unknown:
        case ParseStack::Type::Constant:
            assert(0);
            break;
        case ParseStack::Type::PropRef:
        case ParseStack::Type::EltRef: {
            if (_parseStack.topIsValue()) {
                // Currently TOS is a PropRef and TOS-1 is the source. We need to convert the
                // PropRef into a simple register, then swap it back to its original position,
                // then push a "setValue" atom, the swap again. Then we will have:
                // TOS=>src, ATOM(setValue), dst. Now we need to generate the equivalent of:
                //
                //      dst.setValue(src
                //
                _parseStack.propRefToReg();
                _parseStack.swap();
                emitId(SharedAtom::setValue, IdType::NotLocal);
                _parseStack.swap();
                emitPush();
                uint32_t objReg = emitDeref(DerefType::Prop);
                emitCallRet(Op::CALL, objReg, 1);
                discardResult();
                return;
            } else {
                emitCodeRRR((dstType == ParseStack::Type::PropRef) ? Op::STOPROP : Op::STOELT, _parseStack.topReg(), _parseStack.topDerefReg(), srcReg);
            }
            break;
        case ParseStack::Type::Local:
        case ParseStack::Type::Register:
            emitCodeRRR(Op::MOVE, _parseStack.topReg(), srcReg);
            break;
        case ParseStack::Type::RefK:
            emitCodeRRR(Op::STOREFK, 0, _parseStack.topReg(), srcReg);
            _parseStack.pop();
            assert(_parseStack.topReg() == srcReg);
            return;
        case ParseStack::Type::UpValue:
            emitCodeRRR(Op::STOREUP, _parseStack.topReg(), srcReg);
            break;
        }
    }
    
    _parseStack.swap();
    _parseStack.pop();
}

uint32_t Parser::emitDeref(DerefType type)
{
    if (_nerrors) return 0;
    
    bool isValue = _parseStack.topIsValue();
    uint32_t derefReg = _parseStack.bake();
    _parseStack.swap();
    uint32_t objectReg = _parseStack.bake();
    _parseStack.swap();
    _parseStack.pop();
    _parseStack.replaceTop((type == DerefType::Prop) ? ParseStack::Type::PropRef : ParseStack::Type::EltRef, objectReg, derefReg);
    _parseStack.setIsValue(isValue);
    return objectReg;
}

void Parser::emitDup()
{
    if (_nerrors) return;
    
    switch(_parseStack.topType()) {
        case ParseStack::Type::PropRef:
        case ParseStack::Type::EltRef:
        case ParseStack::Type::RefK:
            _parseStack.dup();
            _parseStack.bake();
            break;
        case ParseStack::Type::Local:
            _parseStack.push(ParseStack::Type::Local, _parseStack.topReg());
            break;
        case ParseStack::Type::UpValue:
            _parseStack.push(ParseStack::Type::UpValue, _parseStack.topReg());
            break;
        default:
            assert(0);
            break;
    }
}

void Parser::emitAppendElt()
{
    if (_nerrors) return;
    
    // tos-1 object to append to
    // tos value to store
    // leave object on tos
    uint32_t srcReg = _parseStack.bake();
    _parseStack.swap();
    uint32_t objectReg = _parseStack.bake();
    _parseStack.swap();
    _parseStack.pop();
    
    emitCodeRRR(Op::APPENDELT, objectReg, srcReg);
}

void Parser::emitAppendProp()
{
    if (_nerrors) return;
    
    // tos-2 object to append to
    // tos-1 property on that object
    // tos value to store
    // leave object on tos
    uint32_t srcReg = _parseStack.bake();
    _parseStack.swap();
    uint32_t propReg = _parseStack.bake();
    _parseStack.pop();
    _parseStack.pop();
    assert(!_parseStack.needsBaking());
    uint32_t objectReg = _parseStack.topReg();
    
    emitCodeRRR(Op::APPENDPROP, objectReg, propReg, srcReg);
}

void Parser::emitBinOp(Op op)
{
    if (_nerrors) return;
    
    if (op == Op::MOVE) {
        emitMove();
        return;
    }
    
    uint32_t rightReg = _parseStack.bake();
    _parseStack.swap();
    uint32_t leftReg = _parseStack.bake();
    _parseStack.pop();
    _parseStack.pop();
    uint32_t dst = _parseStack.pushRegister();

    emitCodeRRR(op, dst, leftReg, rightReg);
}

void Parser::emitCaseTest()
{
    if (_nerrors) return;
    
    // This is like emitBinOp(Op::EQ), but does not pop the left operand
    uint32_t rightReg = _parseStack.bake();
    _parseStack.swap();
    uint32_t leftReg = _parseStack.bake();
    _parseStack.swap();
    _parseStack.pop();
    uint32_t dst = _parseStack.pushRegister();

    emitCodeRRR(Op::EQ, dst, leftReg, rightReg);
}

void Parser::emitUnOp(Op op)
{
    if (_nerrors) return;
    
    if (op == Op::PREDEC || op == Op::PREINC || op == Op::POSTDEC || op == Op::POSTINC) {
        uint32_t dst = _parseStack.pushRegister();
        _parseStack.swap();
        emitDup();
        uint32_t srcReg = _parseStack.bake();
        emitCodeRRR(op, dst, srcReg);
        emitMove();
        _parseStack.pop();
        return;
    }
    
    uint32_t srcReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t dst = _parseStack.pushRegister();
    emitCodeRRR(op, dst, srcReg);
}

void Parser::emitLoadLit(bool array)
{
    if (_nerrors) return;
    
    uint32_t dst = _parseStack.pushRegister();
    emitCodeRRR(array ? Op::LOADLITA : Op::LOADLITO, dst);
}

void Parser::emitPush()
{
    if (_nerrors) return;
    
    uint32_t src = _parseStack.bake();
    _parseStack.pop();
    
    // Value pushed is a register or constant, so it has to go into rb
    emitCodeRUN(Op::PUSH, src, 0);
}

void Parser::emitPop()
{
    if (_nerrors) return;
    
    uint32_t dst = _parseStack.pushRegister();
    emitCodeRRR(Op::POP, dst);
}

void Parser::emitEnd()
{
    if (_nerrors) return;
    
    // If we have no errors we expect an empty stack, otherwise, there might be cruft left over
    if (_nerrors) {
        _parseStack.clear();
    }
    emitCodeRRR(Op::END);
}

void Parser::emitCallRet(Op value, int32_t thisReg, uint32_t nparams)
{
    if (_nerrors) return;
    
    assert(nparams < 256);
    assert(value == Op::CALL || value == Op::NEW || value == Op::RET);
    
    uint32_t calleeReg = 0;
    if (thisReg < 0) {
        // This uses a dummy value for this
        thisReg = MaxRegister + 1;
    }

    if (value == Op::CALL) {
        // If tos is a PropRef or EltRef, emit CALLPROP with the object and property
        if (_parseStack.topType() == ParseStack::Type::PropRef || _parseStack.topType() == ParseStack::Type::EltRef) {
            emitCodeCall(Op::CALLPROP, _parseStack.topReg(), _parseStack.topDerefReg(), nparams);
            _parseStack.pop();
            emitPop();
            return;
        }
    }
            
    
    if (value == Op::CALL || value == Op::NEW) {
        calleeReg = _parseStack.bake();
        _parseStack.pop();
    } else {
        // If there is a return value, push it onto the runtime stack
        for (uint32_t i = 0; i < nparams; ++i) {
            emitPush();
        }
    }
        
    emitCodeCall(value, calleeReg, thisReg, nparams);
    
    if (value == Op::CALL || value == Op::NEW) {
        // On return there will be a value on the runtime stack. Pop it into a register
        emitPop();
    }
}

int32_t Parser::emitDeferred()
{
    if (_nerrors) return 0;
    
    assert(!_deferred);
    assert(_deferredCodeBlocks.size() > 0);
    int32_t start = static_cast<int32_t>(currentFunction()->code()->size());
    
    for (size_t i = _deferredCodeBlocks.back(); i < _deferredCode.size(); ++i) {
        currentFunction()->code()->push_back(_deferredCode[i]);
    }
    _deferredCode.resize(_deferredCodeBlocks.back());
    _deferredCodeBlocks.pop_back();
    return start;
}

void Parser::functionAddParam(const Atom& atom)
{
    if (_nerrors) return;
    
    if (currentFunction()->addLocal(atom) < 0) {
        m8r::String s = "param '";
        s += _program->stringFromAtom(atom);
        s += "' already exists";
        printError(s.c_str());
    }
}

void Parser::functionStart(bool ctor)
{
    if (_nerrors) return;
    
    _functions.emplace_back(new Function(currentFunction()), ctor);
}

void Parser::functionParamsEnd()
{
    if (_nerrors) return;
    
    currentFunction()->markParamEnd();
}

Function* Parser::functionEnd()
{
    if (_nerrors) return nullptr;
    
    assert(_functions.size());
    
    // If this is a ctor, we need to return this, just in case
    if (_functions.back()._ctor) {
        pushThis();
        emitCallRet(m8r::Op::RET, -1, 1);
    }
    
    emitEnd();
    Function* function = currentFunction();
    uint8_t tempRegisterCount = 256 - _functions.back()._minReg;
    _functions.pop_back();
    
    reconcileRegisters(function);
    function->setTempRegisterCount(tempRegisterCount);
    
    return function;
}

static inline uint32_t regFromTempReg(uint32_t reg, uint32_t numLocals)
{
    return (reg > numLocals && reg <= MaxRegister) ? (MaxRegister - reg + numLocals) : reg;
}

void Parser::reconcileRegisters(Function* function)
{
    assert(function->code());
    Code& code = *function->code();
    uint32_t numLocals = static_cast<uint32_t>(function->localSize());
    
    for (int i = 0; i < code.size(); ++i) {
        Instruction inst = code[i];
        Op op = static_cast<Op>(inst.op());
        
        if (op == Op::LINENO) {
            continue;
        }
        if (op == Op::RET || op == Op::CALL || op == Op::NEW || op == Op::CALLPROP) {
            code[i] = Instruction(op, regFromTempReg(inst.rcall(), numLocals), regFromTempReg(inst.rthis(), numLocals), inst.nparams(), true);
        } else if (op == Op::JMP || op == Op::JT || op == Op::JF) {
            uint32_t rn = regFromTempReg(inst.rn(), numLocals);
            code[i] = Instruction(op, rn, inst.sn());
        } else if (op == Op::PUSH) {
            uint32_t rn = regFromTempReg(inst.rn(), numLocals);
            code[i] = Instruction(op, rn, inst.sn());
        } else if (op == Op::LOADUP) {
            uint32_t ra = regFromTempReg(inst.ra(), numLocals);
            code[i] = Instruction(op, ra, inst.rb(), 0);
        } else if (op == Op::STOREUP) {
            uint32_t rb = regFromTempReg(inst.rb(), numLocals);
            code[i] = Instruction(op, inst.ra(), rb, 0);
        } else {
            uint32_t ra = regFromTempReg(inst.ra(), numLocals);
            code[i] = Instruction(op, ra, regFromTempReg(inst.rb(), numLocals), regFromTempReg(inst.rc(), numLocals));
        }
    }
}

uint32_t Parser::ParseStack::push(ParseStack::Type type, uint32_t reg)
{
    assert(type != Type::Register);
    _stack.push({ type, reg, 0 });
    return reg;
}

uint32_t Parser::ParseStack::pushRegister()
{
    FunctionEntry& entry = _parser->_functions.back();
    uint32_t reg = entry._nextReg--;
    if (reg < entry._minReg) {
        entry._minReg = reg;
    }
    _stack.push({ Type::Register, reg, 0 });
    return reg;
}

void Parser::ParseStack::pop()
{
    if (empty()) {
        return;
    }
    if (_stack.top()._type == Type::Register) {
        assert(_parser->_functions.back()._nextReg < 255);
        _parser->_functions.back()._nextReg++;
    }
    _stack.pop();
}

void Parser::ParseStack::swap()
{
    assert(_stack.size() >= 2);
    Entry t = _stack.top();
    _stack.top() = _stack.top(-1);
    _stack.top(-1) = t;
}

uint32_t Parser::ParseStack::bake()
{
    Entry entry = _stack.top();
    
    switch(entry._type) {
        case Type::PropRef:
        case Type::EltRef: {
            if (entry._isValue) {
                // Currently TOS is a PropRef and TOS-1 is the source. We need to convert the
                // PropRef into a simple register, then swap it back to its original position,
                // then push a "setValue" atom, the swap again. Then we will have:
                // TOS=>src, ATOM(setValue), dst. Now we need to generate the equivalent of:
                //
                //      dst.setValue(src
                //
                propRefToReg();
                _parser->emitId(SharedAtom::getValue, Parser::IdType::NotLocal);
                uint32_t objectReg = _parser->emitDeref(Parser::DerefType::Prop);
                _parser->emitCallRet(Op::CALL, objectReg, 0);
                return _stack.top()._reg;
            } else {
                pop();
                uint32_t r = pushRegister();
                _parser->emitCodeRRR((entry._type == Type::PropRef) ? Op::LOADPROP : Op::LOADELT, r, entry._reg, entry._derefReg);
                return r;
            }
        }
        case Type::RefK: {
            pop();
            uint32_t r = pushRegister();
            if (entry._isValue) {
                pushRegister();
                _parser->emitId(SharedAtom::getValue, Parser::IdType::MightBeLocal);
                _parser->emitMove();
                _parser->emitCallRet(Op::CALL, -1, 0);
                _parser->emitMove();
            } else {
                _parser->emitCodeRRR(Op::LOADREFK, r, entry._reg);
            }
            return r;
        }
        case Type::This: {
            pop();
            uint32_t r = pushRegister();
            _parser->emitCodeRRR(Op::LOADTHIS, r);
            return r;
        }
        case Type::UpValue: {
            pop();
            uint32_t r = pushRegister();
            _parser->emitCodeRRR(Op::LOADUP, r, entry._reg);
            return r;
        }
        case Type::Constant: {
            uint32_t r = entry._reg;
            Value v = _parser->currentFunction()->constant(ConstantId(r - MaxRegister - 1));
            Object* obj = v.asObject();
            Function* func = (obj && obj->isFunction()) ? reinterpret_cast<Function*>(obj) : nullptr;
            if (func) {
                pop();
                uint32_t dst = pushRegister();
                _parser->emitCodeRRR(Op::CLOSURE, dst, r);
                r = dst;
            }
            return r;
        }
        case Type::Local:
        case Type::Register:
            return entry._reg;
        case Type::Unknown:
            assert(0);
            return 0;
    }
}

void Parser::ParseStack::replaceTop(Type type, uint32_t reg, uint32_t derefReg)
{
    _stack.setTop({ type, reg, derefReg });
}

void Parser::ParseStack::propRefToReg()
{
    assert(_stack.top()._type == Type::PropRef);
    uint32_t r = _stack.top()._reg;
    Type type = (r < _parser->_functions.back()._minReg) ? Type::Local : ((r <= MaxRegister) ? Type::Register : Type::Constant);
    replaceTop(type, r, 0);
}

