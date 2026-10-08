The GCN warning sample declarations are retained as sibling-engine source.
Their media initializers are local inputs, excluded from the public repository
and source exports. The current `audio_debug.cpp` caller is disabled by its
original `#if 0`; these samples are not a PC reconstruction prerequisite.

For future GCN work, supply the corresponding original Area51 headers locally:

```
python -m hobbit.core.assets --sibling audio_notfound --source /local/NotFound_gcn.hpp
python -m hobbit.core.assets --sibling audio_notloaded --source /local/NotLoaded_gcn.hpp
```

The extractor checks the full source and decoded sample hashes recorded in
`config/retail/local-assets.json`, then writes includes under ignored
`build/gen/retail-assets/`. Missing inputs are not replaced with fabricated data.
The Area51 revision `431f72b9` is a historical provenance hint; the full source
hashes identify the actual inputs. See `docs/imports/entropy-audio-io.json` for
the earlier source import and `docs/08-sibling-entropy-source.md` for policy.
