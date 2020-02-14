#pragma once

#include <stdint.h>
#include <type_traits>

constexpr uint32_t makeSignature(const char* str, size_t size)
{
	return size == 0 ? 0 : ((makeSignature(str + 1, size - 1) << 8) | *str);
}
constexpr uint32_t operator ""_sig(const char* str, size_t size)
{
	return makeSignature(str, size);
}

namespace _pri_
{
	template <typename LAMBDA>
	struct Finally
	{
		LAMBDA m_lambda;

		Finally(LAMBDA&& lambda) noexcept
			:m_lambda(std::forward<LAMBDA>(lambda))
		{
		}

		~Finally() noexcept
		{
			m_lambda();
		}
	};
	struct FinallyHelper
	{
		template <typename LAMBDA>
		Finally<LAMBDA> operator +(LAMBDA&& lambda) const noexcept
		{
			return Finally<LAMBDA>(std::forward<LAMBDA>(lambda));
		}
	};
	static const constexpr FinallyHelper finallyHelper;
}


#define finally auto __COUNTER__##FINALLY = _pri_::finallyHelper + [&]