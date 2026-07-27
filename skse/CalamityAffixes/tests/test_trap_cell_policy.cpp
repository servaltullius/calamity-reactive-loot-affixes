#include "CalamityAffixes/TrapCellPolicy.h"

using CalamityAffixes::detail::IsTrapCellUsable;

static_assert(!IsTrapCellUsable(false, false),
	"Trap cells must exist before runtime effects can use them");
static_assert(!IsTrapCellUsable(false, true),
	"An attached state without a cell cannot make a trap usable");
static_assert(!IsTrapCellUsable(true, false),
	"Detached cells must not retain active traps or receive runtime effects");
static_assert(IsTrapCellUsable(true, true),
	"Attached cells should keep their traps active");
