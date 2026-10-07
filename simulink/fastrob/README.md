These files are copied unchanged from the AutoPatch repository, commit `1edcc84`:

| File | Source in AutoPatch | Last commit |
| --- | --- | --- |
| `fastRobOpen.m` | `util/sim/fastRobOpen.m` | `d9d1abf` |
| `fastRob.m` | `util/sim/fastRob.m` | `62cad06` |
| `fastRobClose.m` | `util/sim/fastRobClose.m` | `62cad06` |
| `assignStructToBase.m` | `util/par/assignStructToBase.m` | `5f89105` |
| `initParpool.m` | `util/par/initParpool.m` | `47920a8` |

They simulate a Simulink model in fast restart and compute STL robustness with Breach.
Candidate weights are written by `isarSetParams.m` in the parent folder instead of AutoPatch's `fastRobSetWeights.m`, since ISAR changes every weight and bias.
