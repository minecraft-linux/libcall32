#pragma once

#include <asmjit/asmjit.h>
#include <algorithm>

namespace call32 {

class TrampolineGen {

private:
    asmjit::x86::Assembler &cw;
    asmjit::x86::Mem sourceStack;
    size_t sourceStackOff = 0;
    int resultReg = 0;
    size_t resultStackOff = 0;
    asmjit::x86::Gp::Id resultRegisters[6] = {
            asmjit::x86::Gp::kIdDi,
            asmjit::x86::Gp::kIdSi,
            asmjit::x86::Gp::kIdDx,
            asmjit::x86::Gp::kIdCx,
            asmjit::x86::Gp::kIdR8,
            asmjit::x86::Gp::kIdR9
    };
    bool resultUseRegister() const {
        return resultReg < 6;
    }

    asmjit::x86::Gp::Id const &resultNextRegister() const {
        return resultRegisters[resultReg];
    }

public:
    TrampolineGen(asmjit::x86::Assembler &cw, asmjit::x86::Mem src) : cw(cw), sourceStack(src) {
    }

    size_t getStackSize() const { return resultStackOff; }

private:
    template <typename RClass>
    void genArgSimple();

public:
    template <typename T>
    void genArg();

    template <typename... Args>
    void genArgs();

    void genCall(void *to) {
        cw.call((uint64_t) to);
    }

    /*

    template <typename Ret, typename = std::enable_if_t<std::is_pointer<Ret>::value>>
    void genCheckRet() {
        using namespace asmjit;
        cw.cmp(x86::rax, 0xFFFFFFFF);
        cw.jg((uint64_t) sf.reportBadPointerReturn);
    }

    template <typename Ret>
    void genCheckRet() {
    }

    */

};

namespace detail {

    template<typename ...Args>
    struct ArgGen;

    template<typename ArgHead, typename ...Args>
    struct ArgGen<ArgHead, Args...> {
        static void gen(TrampolineGen &c) {
            c.genArg<ArgHead>();
            ArgGen<Args...>::gen(c);
        }
    };

    template<>
    struct ArgGen<> {
        static void gen(TrampolineGen &c) {
        }
    };

}

template <typename RClass>
inline void TrampolineGen::genArgSimple() {
    using namespace asmjit;
    auto src = sourceStack;
    src.addOffset(sourceStackOff);
    sourceStackOff += std::max(RClass::kThisSize, 4u);
    if (resultUseRegister()) {
        cw.mov(x86::Gp(RClass::kSignature, resultNextRegister()), src);
        resultReg += 1;
    } else {
        auto ax = x86::Gp(RClass::kSignature, x86::Gp::kIdAx);
        cw.mov(ax, src);
        cw.mov(x86::ptr(x86::rsp, resultStackOff), ax);
        resultStackOff += std::max(RClass::kThisSize, 4u);
    }
}

template<>
inline void TrampolineGen::genArg<short>() { genArgSimple<asmjit::x86::Gpw>(); }
template<>
inline void TrampolineGen::genArg<unsigned short>() { genArgSimple<asmjit::x86::Gpw>(); }
template<>
inline void TrampolineGen::genArg<int>() { genArgSimple<asmjit::x86::Gpd>(); }
template<>
inline void TrampolineGen::genArg<unsigned int>() { genArgSimple<asmjit::x86::Gpd>(); }


template <typename... Args>
void TrampolineGen::genArgs() {
    detail::ArgGen<Args...>::gen(*this);
}


template <typename Ret, typename ...Args>
void *createTrampolineFor(asmjit::JitRuntime &rt, Ret (*what)(Args...)) {
    using namespace asmjit;

    CodeHolder code64, code32;
    code64.init(rt.codeInfo());

    x86::Assembler azero;
    x86::Assembler a (&code64);
    TrampolineGen genSize (azero, x86::ptr(x86::esp));
    genSize.genArgs<Args...>();
    auto stackSize = genSize.getStackSize();
    TrampolineGen gen (a, x86::ptr(x86::esp, 12 + stackSize));
    if (stackSize > 0)
        a.sub(x86::esp, stackSize);
    gen.genArgs<Args...>();
    gen.genCall((void *) what);
    if (stackSize > 0)
        a.add(x86::esp, stackSize);
    uint8_t retf[1] = {0xCB};
    a.embed(retf, 1);

    void (*t64)();
    auto err = rt.add(&t64, &code64);
    if (err)
        throw std::runtime_error("trampoline64 add failed");
    if (((size_t) (void *) t64) & ~0xffffffff)
        throw std::runtime_error("trampoline64 unreachable");

    CodeInfo ci32(ArchInfo::kIdX86);
    code32.init(ci32);
    x86::Assembler a32 (&code32);
    uint8_t callf[7] = {0x9A, 0, 0, 0, 0, 0x33, 0};
    *((uint32_t *) &callf[1]) = (uint32_t) (size_t) t64;
    a32.embed(callf, 7);
    a32.ret();

    void (*t32)();
    err = rt.add(&t32, &code32);
    if (err)
        throw std::runtime_error("trampoline32 add failed");
    if (((size_t) (void *) t32) & ~0xffffffff)
        throw std::runtime_error("trampoline32 unreachable");

    return (void *) t32;
}

}