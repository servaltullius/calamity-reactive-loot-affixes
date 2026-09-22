#pragma once

namespace CalamityAffixes::detail
{
	// Proc recursion prevention must not bypass friendly-fire protection. A
	// rejected hit never reaches the engine, even as a zero-damage callback.
	template <class Protected, class Forward, class Process>
	void DispatchHealthDamageBoundary(
		bool& a_inHook, bool a_inProc, Protected&& a_protected,
		Forward&& a_forward, Process&& a_process)
	{
		if (a_protected()) {
			return;
		}
		if (a_inHook || a_inProc) {
			a_forward();
			return;
		}
		struct Scope
		{
			bool& active;
			explicit Scope(bool& a_active) : active(a_active) { active = true; }
			~Scope() { active = false; }
		} scope(a_inHook);
		a_process();
	}
}
