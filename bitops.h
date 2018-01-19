/* TODO: Add bitfields datatype
 * BitFields<unit64_t,
 *        field1, Bits<uint64_t>::Layout<>,
 *        field2, Bits<uint64_t>::Layout<>,
 *        field3, Bits<uint64_t>::Layout<>> bitfields;
 *
 * uint64_t bitfields::GetField(field1, uint64_t &data);
 * bitfields::SetField(field2, uint64_t &data, uint64_t &val);
 * uint64_t bitfields::GetMask(field1);
 * bool bitfields::IsAllBitsRst(field1, uint64_t& data);
 * bool bitfields::IsAllBitsSet(field1, uint64_t& data);
 */

namespace bitops
{

template<typename Int>
class Bits
{
    // Arch-dependent, for ARM it may be up to 64, for Intel up to 31
    static constexpr int MaxShift = 31;

    // due to 6.5.7 Bitwise shift operators:
    //              3. The integer promotions are performed on each of the operands.
    //                 The type of the result is that of the promoted left operand.
    //                 If the value of the right operand is negative or is greater
    //                 than or equal to the width of the promoted left operand,
    //                 the behavior is undefined.
    static constexpr int MaxShiftForInt = sizeof(int) * 8 <= MaxShift ? sizeof(int) * 8 - 1 : MaxShift;

    template<typename Slice>
    struct SliceOp;

    template<typename B, typename ... Bs>
    class Unpacker : public Unpacker<Bs ...>
    {
        using Op = SliceOp<B>;
    protected:
        static constexpr int next_offset = Op::Len + Unpacker<Bs ...>::next_offset;
        constexpr Unpacker(const Int v,
                           const Int r) : Unpacker<Bs ...>(v,
                                                           r
                                                           | Op::Unpack(ShiftRight(v,
                                                                                   Unpacker<Bs ...>::next_offset)))
        {
        }

    public:
        constexpr Unpacker(const Int v) : Unpacker<Bs ...>(v,
                                                           Op::Unpack(ShiftRight(v,
                                                                                 Unpacker<Bs ...>::next_offset)))
        {
        }

        constexpr operator const Int()
        {
            return Unpacker<Bs ...>::operator const Int();
        }

    };

    template<typename B>
    class Unpacker<B>
    {
        using Op = SliceOp<B>;
    private:
        const Int val;
    protected:
        static constexpr int next_offset = Op::Len;
        constexpr Unpacker(const Int v,
                           const Int r) : val(r | Op::Unpack(v))
        {
        }

    public:
        constexpr Unpacker(const Int v) : val(Op::Unpack(v))
        {
        }

        constexpr operator const Int()
        {
            return val;
        }

    };

    template<typename B, typename ... Bs>
    class Packer : public Packer<Bs ...>
    {
        using Op = SliceOp<B>;
    protected:
        constexpr Packer(const Int v,
                         const Int r) : Packer<Bs ...>(v, ShiftLeft(r, Op::Len) | Op::Pack(v))
        {
        }

    public:
        constexpr Packer(const Int v) : Packer<Bs ...>(v, Op::Pack(v))
        {
        }

        constexpr operator const Int()
        {
            return Packer<Bs ...>:: operator const Int();
        }

    };

    template<typename B>
    class Packer<B>
    {
        using Op = SliceOp<B>;
    private:
        const Int val;
    protected:
        constexpr Packer(const Int v,
                         const Int r) : val(ShiftLeft(r, Op::Len) | Op::Pack(v))
        {
        }

    public:
        constexpr Packer(const Int v) : val(Op::Pack(v))
        {
        }

        constexpr operator const Int()
        {
            return val;
        }

    };

    template<typename B, typename ... Bs>
    class Masker : public Masker<Bs ...>
    {
        using Op = SliceOp<B>;
    protected:
        constexpr Masker(const Int r) : Masker<Bs ...>(r | Op::Mask())
        {
        }

    public:
        constexpr Masker() : Masker<Bs ...>(Op::Mask())
        {
        }

        constexpr operator const Int()
        {
            return Masker<Bs ...>::operator const Int();
        }

    };

    template<typename B>
    class Masker<B>
    {
        using Op = SliceOp<B>;
    private:
        const Int val;
    protected:
        constexpr Masker(const Int r) : val(r | Op::Mask())
        {
        }

    public:
        constexpr Masker() : val(Op::Mask())
        {
        }

        constexpr operator const Int()
        {
            return val;
        }

    };

    template<typename Slice>
    struct SliceOp
    {
        static constexpr int From = Slice::From;
        static constexpr int To   = Slice::To;
        static constexpr int Len  = To > From ? To - From + 1 : From - To + 1;

        static constexpr Int Unpack(const Int v)
        {
            constexpr Int mask = MakeMask(Len);
            if (To >= From)
            {
                return ShiftLeft((v & mask), From);
            }
            else
            {
                const Int rev = Reverse(v, Len);
                return ShiftLeft(rev & mask, To);
            }
        }

        static constexpr Int Pack(const Int v)
        {
            constexpr Int mask = MakeMask(Len);
            if (To >= From)
            {
                return ShiftRight(v, From) & mask;
            }
            else
            {
                return Reverse(ShiftRight(v, To) & mask, Len);
            }
        }

        static constexpr Int Mask()
        {
            return To >= From ? ShiftLeft(MakeMask(Len), From)
                   : ShiftLeft(MakeMask(Len), To);
        }

    };

public:
    // Shifts are moved in separate functions to make them safe
    static constexpr Int ShiftLeft(const Int val,
                                   int       n)
    {
        Int v = val;
        while (n >= MaxShiftForInt)
        {
            v <<= MaxShiftForInt;
            n  -= MaxShiftForInt;
        }
        v <<= n;
        return v;
    }

    static constexpr Int ShiftRight(const Int val,
                                    int       n)
    {
        Int v = val;
        while (n >= MaxShiftForInt)
        {
            v >>= MaxShiftForInt;
            n  -= MaxShiftForInt;
        }
        v >>= n;
        return v;
    }

    static constexpr Int MakeMask(const int l)
    {
        return static_cast<const Int>(ShiftLeft(static_cast<const Int>(1), l) - static_cast<const Int>(1));
    }

    static constexpr Int Reverse(const Int v,
                                 const int len)
    {
        switch (len)
        {
        case 1: return v;
        case 2: return ShiftLeft((v & 0x1), 1) | ShiftRight((v & 0x2), 1);
        case 3: return ShiftLeft((v & 0x1), 2) | ShiftRight((v & 0x4), 2) | (v & 0x2);
        case 4: return ShiftLeft((v & 0x1), 3) | ShiftRight((v & 0x8), 3)
                   | ShiftLeft((v & 0x2), 1) | ShiftRight((v & 0x4), 1);
        }
        const int new_len = len >> 1;
        const Int mask    = MakeMask(new_len);
        const int shift   = (len & 0x1) == 0 ? new_len : new_len + 1;
        const Int result  = ShiftLeft(Reverse(ShiftRight(v, shift) & mask, new_len), shift) | Reverse(v & mask,
                                                                                                      new_len);
        if ((len & 0x1) != 0)
        {
            return result | (v & ShiftLeft(0x1, new_len));
        }
        return result;
    }

    template<const int High, const int Low>
    struct Slice
    {
        static constexpr int From = Low;
        static constexpr int To   = High;
    };

    template<typename ... Slices>
    struct Layout
    {
        using Unpack = Unpacker<Slices ...>;
        using Pack   = Packer<Slices ...>;
        using Mask   = Masker<Slices ...>;
    };
};

} // end of namespace


/*
Usage:

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

using namespace bitops;

using MyBits = Bits<uint64_t>;

template<const int From, const int To>
using Slice = MyBits::Slice<From, To>;

// note, when slice is reversed (low-high vs high-low),
// then bits pack is reversed too
using Converter = MyBits::Layout<Slice<23, 16>, Slice<7, 3> >;

// Unpack takes packed bits and unpacks them:
// layout = 15..13; 5..6; 3..2 (Slice<15,13>, Slice<5,6>, Slice<3,2>)
//  ...6543210 -> ...654xxxxxx23x10xx
//                            ^^
//                reversed----/

using Unpack = Converter::Unpack;

// Pack is reverse of Unpack
using Pack = Converter::Pack;

using Mask = Converter::Mask;

int
main(int    argc,
     char** argv)
{
    constexpr uint64_t bf = static_cast<uint64_t>(Unpack(0xFAAF));
    constexpr uint64_t v  = static_cast<uint64_t>(Pack(bf));
    constexpr uint64_t m  = static_cast<uint64_t>(Mask());
    printf("%08X\n", bf);
    printf("%08X\n", v);
    printf("%08X\n", m);
    return 0;
}

*/
