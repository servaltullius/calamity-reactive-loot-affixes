#pragma once

namespace CalamityAffixes::detail
{
	[[nodiscard]] constexpr bool IsTrapCellUsable(
		bool a_hasCell,
		bool a_cellAttached) noexcept
	{
		return a_hasCell && a_cellAttached;
	}
}
