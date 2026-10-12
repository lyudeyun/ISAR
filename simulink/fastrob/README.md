These files are copied from the AutoPatch repository, commit `1edcc84`, unchanged except as noted below:

| File | Source in AutoPatch | Last commit |
| --- | --- | --- |
| `fastRobOpen.m` | `util/sim/fastRobOpen.m` | `d9d1abf` |
| `fastRob.m` | `util/sim/fastRob.m` | `62cad06` |
| `fastRobClose.m` | `util/sim/fastRobClose.m` | `62cad06` |
| `assignStructToBase.m` | `util/par/assignStructToBase.m` | `5f89105` |
| `initParpool.m` | `util/par/initParpool.m` | `47920a8` |

They simulate a Simulink model in fast restart and compute STL robustness with Breach.
Candidate weights are written by `isarSetParams.m` in the parent folder instead of AutoPatch's `fastRobSetWeights.m`, since ISAR changes every weight and bias.

`fastRobOpen.m` and `fastRob.m` also carry the backward-compatible `extraSigs` addition of AutoRepair_Alpha's versions (`util/sim/`, commit `a317187fb`): `simConf.extraSigs` (optional) names signals recorded besides those of the specification, and `[rob, tout, X] = fastRob(h, sysInputs)` returns the trajectory (columns of `X` follow `h.sigNames`, empty if the simulation failed). Without `extraSigs`, the sessions and the robustness are as before. `evaluate_testset.py` uses it to compute OPERA's metrics.
