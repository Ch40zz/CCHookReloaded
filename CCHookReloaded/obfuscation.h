//-------------------------------------------------------------//
// "Malware related compile-time hacks with C++11" by LeFF     //
// You can use this code however you like, I just don't really //
// give a shit, but if you feel some respect for me, please    //
// don't cut off this comment when copy-pasting... ;-)         //
//-------------------------------------------------------------//

// Modified to have a larger XOR key and also support wchar_t.
// Higher resistance to removal due to compiler optimizations.

#include <type_traits>

namespace obf
{
#ifdef __clang__
	template <unsigned char C>
	struct noopt { [[clang::optnone]] unsigned char operator()() const { return C; } };

	#define NODATA(v) (obf::noopt<(v)>{}())
#else
	#define NODATA(v)
#endif


	template <unsigned long V>
	struct EnsureCompileTime
	{
		enum : unsigned long
		{
			Value = V
		};
	};

	constexpr auto Seed = ((__TIME__[7] - '0') * 1 + (__TIME__[6] - '0') * 10  + \
							(__TIME__[4] - '0') * 60 + (__TIME__[3] - '0') * 600 + \
							(__TIME__[1] - '0') * 3600 + (__TIME__[0] - '0') * 36000 + \
							(__DATE__[0] << 0)  + (__DATE__[1] << 3)  + (__DATE__[2] << 6)  + (__DATE__[4] << 9)  + \
							(__DATE__[5] << 12) + (__DATE__[7] << 15) + (__DATE__[8] << 18) + (__DATE__[9] << 21) + (__DATE__[10] << 24));

	constexpr unsigned long LinearCongruentGenerator(int Rounds)
	{
		return static_cast<unsigned long>(
				1013904223ull + 1664525ull * ((Rounds > 0) ? LinearCongruentGenerator(Rounds - 1) : Seed)
			);
	}

	template <int Rounds>
	constexpr unsigned long Random()
	{
		return EnsureCompileTime<LinearCongruentGenerator(Rounds)>::Value;
	}

	template <int Rounds, class T>
	constexpr T RandBetween(T Min, T Max)
	{
		return ((Min) + (Random<Rounds>() % ((Max) - (Min) + 1)));
	}


	template <int... Pack> struct IndexList {};

	template <typename IndexList, int Right> struct Append;
	template <int... Left, int Right> struct Append<IndexList<Left...>, Right>
	{
		typedef IndexList<Left..., Right> Result;
	};

	template <int N> struct ConstructIndexList
	{
		typedef typename Append<typename ConstructIndexList<N - 1>::Result, N - 1>::Result Result;
	};

	template <> struct ConstructIndexList<0>
	{
		typedef IndexList<> Result;
	};


	const BYTE XORKEY_1 = static_cast<BYTE>(RandBetween<12>(0, 0xFF));
	const BYTE XORKEY_2 = static_cast<BYTE>(RandBetween<07>(0, 0xFF));
	const BYTE XORKEY_3 = static_cast<BYTE>(RandBetween<23>(0, 0xFF));

	template<class Char>
	__forceinline constexpr Char CryptCharacter(const Char Character, int Index)
	{
		return Character ^ (XORKEY_1 + ((Index + 69) << 3)) ^ (XORKEY_2 - (Index + 12)) ^ (-XORKEY_3);
	}

	template <class Char, typename IndexList>
	class CXorString;

	template <class Char, int... Index>
	class CXorString<Char, IndexList<Index...> >
	{
		volatile Char Value[sizeof...(Index)];

	public:
		__forceinline constexpr CXorString(const Char *const String) : Value{ CryptCharacter(String[Index], Index)... } {}

		__forceinline Char *decrypt()
		{
			for (int t = 0; t < sizeof...(Index); t++)
				Value[t] = CryptCharacter(Value[t], t);

			Value[sizeof...(Index)-1] = (Char)(0);

			return (Char*)Value;
		}

		__forceinline Char *get()
		{
			return (Char*)Value;
		}

		__forceinline size_t len() const
		{
			return sizeof...(Index)-1;
		}

		__forceinline ~CXorString()
		{
			for (int t = 0; t < sizeof...(Index); t++)
				Value[t] = 0;
		}
	};
}

#define XorS(X, String) obf::CXorString<typename std::decay<decltype(String[0])>::type, \
						obf::ConstructIndexList<std::size(String)>::Result> X(String)

#define XorString(String) (obf::CXorString<typename std::decay<decltype(String[0])>::type, \
						obf::ConstructIndexList<std::size(String)>::Result>(String).decrypt())
