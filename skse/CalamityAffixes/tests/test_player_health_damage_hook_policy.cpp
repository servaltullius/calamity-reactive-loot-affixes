#include "CalamityAffixes/PlayerHealthDamageHookPolicy.h"

#include <type_traits>

using CalamityAffixes::detail::ResolvePlayerHealthDamageHookEnabled;

static_assert(ResolvePlayerHealthDamageHookEnabled());

// The policy must stay configuration-independent. Pinning the signature is what
// catches a regression that reintroduces a parameter and starts honouring the
// retired `allowPlayerHealthDamageHook` runtime key -- the value assertion above
// cannot see that on its own, since a parameterised version returning true would
// still satisfy it.
static_assert(std::is_same_v<decltype(ResolvePlayerHealthDamageHookEnabled()), bool>);
static_assert(std::is_invocable_v<decltype(ResolvePlayerHealthDamageHookEnabled)>);
static_assert(!std::is_invocable_v<decltype(ResolvePlayerHealthDamageHookEnabled), bool>);
