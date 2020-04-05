#pragma once

#include <asmjit/asmjit.h>
#include <algorithm>
#include <stdexcept>
#include <dlfcn.h>

namespace call32 {

namespace detail {
    template <typename FromClass, int FromAlignment, typename ToClass, int ToAlignment>
    struct SimpleArgGen;
}

class TrampolineGen {

private:
    template <typename F, int FA, typename T, int TA>
    friend struct detail::SimpleArgGen;

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


    template <typename... Args>
    void genArgs();

    void genCall(void *to) {
        cw.call((uint64_t) to);
    }

    template <typename Ret, std::enable_if_t<std::is_pointer<Ret>::value, int> = 0>
    void genCheckRet(void *to) {
        using namespace asmjit;
        cw.push(x86::rax);
        cw.shr(x86::rax, 32);
        cw.pop(x86::rax);
        auto finelbl = cw.newLabel();
        cw.jz(finelbl);
        cw.mov(x86::rdi, (uint64_t) to);
        cw.call((uint64_t) (void *) +[](void *realfunc) {
            printf("bad pointer return\n");
            abort();
        });
        cw.bind(finelbl);
    }

    template <typename Ret, std::enable_if_t<!std::is_pointer<Ret>::value, int> = 0>
    void genCheckRet(void *to) {
    }

};

namespace detail {

    template<typename Arg, class Enable = void>
    struct ArgGen;

    template<typename ...Args>
    struct ArgsGen;

    template<typename ArgHead, typename ...Args>
    struct ArgsGen<ArgHead, Args...> {
        static void gen(TrampolineGen &c) {
            ArgGen<ArgHead>::gen(c);
            ArgsGen<Args...>::gen(c);
        }
    };

    template<>
    struct ArgsGen<> {
        static void gen(TrampolineGen &c) {
        }
    };

    template <typename FromClass, int FromAlignment = 0, typename ToClass = FromClass, int ToAlignment = FromAlignment>
    struct SimpleArgGen {
        static void gen(TrampolineGen &g) {
            using namespace asmjit;
            auto src = g.sourceStack;
            if (FromAlignment != 0)
                g.sourceStackOff = (g.sourceStackOff + FromAlignment - 1) / FromAlignment * FromAlignment;
            src.addOffset(g.sourceStackOff);
            g.sourceStackOff += std::max(FromClass::kThisSize, 4u);
            if (g.resultUseRegister()) {
                auto dst = x86::Gp(ToClass::kSignature, g.resultNextRegister());
                if (ToClass::kThisSize > FromClass::kThisSize) {
                    g.cw.xor_(dst, dst);
                    dst = x86::Gp(FromClass::kSignature, g.resultNextRegister());
                }
                g.cw.mov(dst, src);
                g.resultReg += 1;
            } else {
                if (ToAlignment != 0)
                    g.resultStackOff = (g.resultStackOff + ToAlignment - 1) / ToAlignment * ToAlignment;
                auto axf = x86::Gp(FromClass::kSignature, x86::Gp::kIdAx);
                auto axt = x86::Gp(ToClass::kSignature, x86::Gp::kIdAx);
                if (axt.size() > axf.size())
                    g.cw.xor_(axt, axt);
                g.cw.mov(axf, src);
                g.cw.mov(x86::ptr(x86::rsp, g.resultStackOff), axt);
                g.resultStackOff += std::max(ToClass::kThisSize, 4u);
            }
        }
    };

    template <> struct ArgGen<char> : SimpleArgGen<asmjit::x86::GpbLo, 1> {};
    template <> struct ArgGen<unsigned char> : SimpleArgGen<asmjit::x86::GpbLo, 1> {};
    template <> struct ArgGen<short> : SimpleArgGen<asmjit::x86::Gpw, 2> {};
    template <> struct ArgGen<unsigned short> : SimpleArgGen<asmjit::x86::Gpw, 2> {};
    template <> struct ArgGen<int> : SimpleArgGen<asmjit::x86::Gpd, 4> {};
    template <> struct ArgGen<unsigned int> : SimpleArgGen<asmjit::x86::Gpd, 4> {};
    template <> struct ArgGen<long> : SimpleArgGen<asmjit::x86::Gpd, 4, asmjit::x86::Gpq, 8> {};
    template <> struct ArgGen<unsigned long> : SimpleArgGen<asmjit::x86::Gpd, 4, asmjit::x86::Gpq, 8> {};
    template <> struct ArgGen<long long> : SimpleArgGen<asmjit::x86::Gpq, 8> {};
    template <> struct ArgGen<unsigned long long> : SimpleArgGen<asmjit::x86::Gpq, 8> {};
    template <> struct ArgGen<float> : SimpleArgGen<asmjit::x86::Gpd, 4> {};
    template <> struct ArgGen<double> : SimpleArgGen<asmjit::x86::Gpq, 8> {}; //todo: broken?


    template <typename T>
    struct ArgGen<T, typename std::enable_if_t<std::is_pointer<T>::value>> :
            SimpleArgGen<asmjit::x86::Gpd, 4, asmjit::x86::Gpq, 8> {};
    template <typename T>
    struct ArgGen<T, typename std::enable_if_t<std::is_enum<T>::value>> :
            SimpleArgGen<asmjit::x86::Gpd, 4, asmjit::x86::Gpd, 4> {};

}


template <typename... Args>
void TrampolineGen::genArgs() {
    detail::ArgsGen<Args...>::gen(*this);
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
    auto totalStackSizeUsed = stackSize + /* esi&edx save */ 8 + /* long jump */ 8 + /* final jump */ 4;
    if ((totalStackSizeUsed % 16) != 0) // align up stack
        stackSize += 16 - (totalStackSizeUsed % 16);
    TrampolineGen gen (a, x86::ptr(x86::esp, 20 + stackSize));
    if (stackSize > 0)
        a.sub(x86::esp, stackSize);
    gen.genArgs<Args...>();
    gen.genCall((void *) what);
    gen.genCheckRet<Ret>((void *) what);
    if (stackSize > 0)
        a.add(x86::esp, stackSize);
    uint8_t retf[1] = {0xCB};
    a.embed(retf, 1);

    void (*t64)();
    auto err = rt.add(&t64, &code64);
    if (err)
        throw std::runtime_error("trampoline64 add failed");
    if (((size_t) (void *) t64) & ~0xffffffffLU)
        throw std::runtime_error("trampoline64 unreachable");

    CodeInfo ci32(ArchInfo::kIdX86);
    code32.init(ci32);
    x86::Assembler a32 (&code32);
    a32.push(x86::edi);
    a32.push(x86::esi);
    uint8_t callf[7] = {0x9A, 0, 0, 0, 0, 0x33, 0};
    *((uint32_t *) &callf[1]) = (uint32_t) (size_t) t64;
    a32.embed(callf, 7);
    a32.pop(x86::esi);
    a32.pop(x86::edi);
    a32.ret();

    void (*t32)();
    err = rt.add(&t32, &code32);
    if (err)
        throw std::runtime_error("trampoline32 add failed");
    if (((size_t) (void *) t32) & ~0xffffffffLU)
        throw std::runtime_error("trampoline32 unreachable");

    return (void *) t32;
}

}