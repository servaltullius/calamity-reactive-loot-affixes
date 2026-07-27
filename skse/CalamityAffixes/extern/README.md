# extern/

## CommonLibSSE-NG

Vendored, not a submodule: ~1800 upstream files are committed directly into this
repository. That decision predates this document and is not being reversed here
(a submodule would break the offline/WSL cross-compile lane that several
contributors rely on), but on its own it made the dependency reproducible only
by accident — nothing recorded which upstream commit the tree came from, and a
deliberate local fix was indistinguishable from upstream code.

Three files answer that now:

| File | Purpose |
| --- | --- |
| `CommonLibSSE-NG.pin.json` | Upstream URL, pinned commit/tag, the list of local patches with the reason for each, and content hashes |
| `patches/*.patch` | The local changes, as patches that apply cleanly to the pinned commit |
| `tools/verify_commonlib_pin.py` | Verifies the vendored tree still matches the pin |

Current pin: **v3.5.4** (`b6e0c134b08a2108300c8d033c3b81860ce62354`) from
<https://github.com/CharmedBaryon/CommonLibSSE-NG>.

### Local patches

Two of the three are upstream bugs that simply do not compile, and are marked
`upstreamable: true` in the pin file:

- `include/RE/B/BSTPointerAndFlags.h` — the move assignment writes
  `a_rhs.storage.address`, missing the leading underscore.
- `include/RE/G/GHashSetBase.h` — `Assign()` calls `a_src.Begin()/End()`, which
  do not exist on the const path.

The third is ours to keep:

- `CMakeLists.txt` — adds `/EHsc`. The cross-compile lane builds with `clang-cl`
  and CommonLibSSE headers use exceptions; without it the plugin's own `/EHsc`
  does not reach the interface target.

### Verifying

```bash
python3 tools/verify_commonlib_pin.py            # offline: hash check, no network
python3 tools/verify_commonlib_pin.py --fetch    # online: rebuild from upstream + patches and diff
```

The offline check hashes every tracked file under `CommonLibSSE-NG/` and
compares against the pin, so any undeclared edit — including one that silently
reverts a local patch — fails immediately. Run `--fetch` when bumping the pin;
it proves the vendored tree is exactly `upstream@commit` plus the recorded
patches.

### Changing the vendored tree

1. Edit the file under `CommonLibSSE-NG/`.
2. Regenerate its patch:
   ```bash
   cd skse/CalamityAffixes/extern/CommonLibSSE-NG
   git diff -- <path> > ../patches/<path-with-dashes>.patch
   ```
3. Add or update the entry in `CommonLibSSE-NG.pin.json`, including a `reason`
   and whether it is `upstreamable`.
4. `python3 tools/verify_commonlib_pin.py --update` to refresh the hashes, then
   commit the vendored file, the patch, and the pin together.

### Bumping the pin

```bash
cd skse/CalamityAffixes/extern/CommonLibSSE-NG
git fetch origin --tags
git checkout <new-commit>
for p in ../patches/*.patch; do git apply "$p"; done   # resolve any rejects here
cd - && python3 tools/verify_commonlib_pin.py --update
python3 tools/verify_commonlib_pin.py --fetch          # confirm reproducibility
```

Then update `commit`/`tag` in the pin file and rebuild the plugin lane.

> **Careful:** `CommonLibSSE-NG/` contains its own `.git` (gitignored). Running
> `git checkout -- <file>` *inside* that directory restores the **upstream**
> content and silently drops the local patch. To undo an edit, restore from the
> outer repository instead:
> ```bash
> git checkout -- skse/CalamityAffixes/extern/CommonLibSSE-NG/<path>
> ```
> The offline verifier catches this if it happens.
