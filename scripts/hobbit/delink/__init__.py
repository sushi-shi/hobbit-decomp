"""hobbit.delink - resolved model -> named per-unit target objects.

The delink half of the matching pipeline:

    Model (hobbit.model.resolve)
      -> pdb_synth      build/pdb/hobbit_named.{yaml,pdb}
      -> data_manifest  build/gen/delink_data_manifest.tsv (+ section manifest)
      -> tool.delinker  vostok-delinker over the retail EXE
      -> run            collect build/objdiff/target-new/<unit>.c.obj

Mechanisms are ported from the local Gruntz delinker and HoMM1 fixed-image adaptation;
inputs are adapted to the Model - nothing here re-reads bindings.tsv.
"""
