# SIFT (hov) variant of Branson: notes

This branch (`sift-hov`) holds all the work on a SIFT (`hov` in code) variant of Branson. It is **parked**. Branson turned out not to fit SIFT, so `main` keeps only the ref variant, used for preliminary data. Everything below describes the state of this branch.

## Why it is parked: Branson's index is not a long-lived array

Branson's indirect accesses are `cells[photon.cell]` and `tallies[photon.cell]`. The index is **mutable photon state**: `set_cell()` changes it on every boundary crossing (`transport_photon.h`, ELEMENT case; `transport_photon_hov.h`). Branson has no long-lived index array, so the hov port **builds one** before every sweep:

```cpp
// Phase A, outside the ROI: write the index array from the photons' current cells
for (size_t i = 0; i < n; ++i) g_hov_data.phtn_cells_idx[i] = photons[active[i]].get_cell() - off;
roi_begin_();
// Phase B, the ROI: phtn_cells_idx is only read, but photons move: phtn.set_cell(next)
roi_end_();
```

Within a sweep the index array is read-only, so `ldind` semantics hold, but it is rewritten every sweep. Consequences:
1. **Not SIFT's target pattern.** SIFT targets `data[index[i]]` with a long-lived, read-mostly index array, such as UME's connectivity maps or HPCG's column indices.
2. **The index lines are always written by the core just before the `ldind`s,** so they sit dirty in the Indirect Unit. The index-caching invariant (index lines only at or below the Gather Unit) then forces a post-access eviction of every index line, every sweep.
3. **Unfair ROI.** The index build is outside the ROI, so hov's extra work is not measured, while ref needs no such step.

---

## Branson background

Branson is a LANL mini-app for **Implicit Monte Carlo (IMC)** thermal radiation transport: photons carry energy through a 3-D mesh of cells, and each cell has a temperature and material properties.

**Timestep loop** (`src/particle_pass_driver.h`). Each step:
1. `mesh.calculate_photon_energy(...)` recomputes three per-cell properties from the current temperature (`mesh.h`):
   - `op_a`, the absorption opacity;
   - `op_s`, the scattering opacity;
   - `f`, the Fleck factor, an IMC weighting term.
2. It creates new photons (`make_photons`) and appends the **census photons**: photons still alive when the previous step ended.
3. `particle_pass_transport(...)` moves every photon until it ends. Photons deposit energy into per-cell **tallies**: `abs_E` (absorbed) and `track_E` (path-length weighted).
4. `mesh.update_temperature(abs_E, track_E, ...)` turns the tallies into the next step's temperatures.

**A photon's life is a sequence of events.** Each event:
- reads its cell's `op_a`, `op_s`, `f` and geometry;
- computes the distance to the next scatter, cell boundary and census;
- moves the photon and adds to the tallies;
- records the outcome in the photon's **descriptor** (`constants.h`):

```cpp
enum event_type : unsigned char { EXIT, PASS, CENSUS, SCATTER, KILLED, BOUND };
//                                 end   end   end     continue  end    continue
bool is_active() const { return d != CENSUS && d != EXIT && d != KILLED && d != PASS; }
```

`PASS` means the photon crossed into a cell owned by **another MPI rank**. In **particle-passing** mode the mesh is split across ranks and photons are sent between them; received photons are transported again (`phtn_recv_list`). In **replicated** mode every rank has the whole mesh. With one rank, Branson forces replicated mode (`input.h`).

**Two variants.**

| | ref (`transport_photon.h`) | hov (`transport_photon_hov.h`, `hov_setup.{h,cc}`) |
|---|---|---|
| Cell data | `std::vector<Cell>`: one struct per cell (array of structs) | one array per field in the HOV memory pool: `f_soa`, `op_a_soa[g]`, `node_*_soa`, ... (struct of arrays) |
| Indirect read | `cells[idx].get_op_a(g)` | `hov_gather_f64_u32(&res, &g_hov_data.pair_op_a[g], i)` (an `ldind`) |
| Index | the photon's own cell field | `g_hov_data.phtn_cells_idx[i]`, filled before each sweep |
| Tallies | `vector<Cell_Tally>` | `tally_abs_E_soa`, `tally_track_E_soa`, via `hov_gather`/`hov_scatter` |

A **pair** (`hov_pair_t`) ties one data array to the shared index array `phtn_cells_idx`. Pairs are created once in `main.cc` with capacity `batch_size + particle_message_size`, which is 15,000 for the multi inputs.

---

## Changes on this branch

### 1. ref transport restructured into per-event sweeps, with a ROI per sweep (`transport_photon.h`)

hov processes photons in **sweeps**: each pass advances every active photon by one event, and the ROI wraps each sweep. ref processed one photon at a time to its end and had no ROI markers, and the checkpoint harness stops at the first `roi_end`. So the two variants measured different work.

The old loop body is now `transport_photon_event()` (one event; it returns whether the photon is still active). `transport_photon()` loops on it, so its behaviour is unchanged. `cpu_transport_photons` now sweeps, with the markers where hov has them:

```cpp
void cpu_transport_photons(...) {
  std::vector<uint32_t> active_photons(photons.size());
  std::iota(active_photons.begin(), active_photons.end(), 0);  // every photon passed in
  while (!active_photons.empty()) {
    roi_begin_(); annotate_synchronize_(1);                    // under ANNOTATE / SYNC_ON_ROI
    for (size_t i = 0; i < active_photons.size(); ++i)         // OpenMP and serial variants
      still_active[i] = transport_photon_event(..., photons[active_photons[i]], ...);
    roi_end_();   annotate_synchronize_(2);
    /* compaction, outside the ROI as in hov */
  }
  /* OpenMP: merge per-thread tallies, as before */
}
```

Physics is unchanged. Each photon has its own RNG and cell data is read-only during transport, so each photon follows the same path; only the order of the tally additions changes.

### 2. Build fixes
- **`src/CMakeLists.txt`:** `WORKDIR` is set unconditionally, so `-DHOV=ON` without `ANNOTATE_TOOL` finds `hov/`.
- **`transport_photon_hov.h`:** `annotate.h` is included only under `ANNOTATE`.

### 3. hov correctness fixes

**3a. Photons skipped at the start of a sweep (`transport_photon_hov.h`).** The kernel used to filter the initial set with `is_active()`. Descriptors are never reset between steps, so two groups were never transported:
- **census photons**, left with `CENSUS` from the previous step;
- **received photons**, which arrive with `PASS` because the descriptor bytes are part of the MPI message (`mpi_types.h`). The post-transport switch then sends them on again, using `adjacent_procs[send_rank]` for their own rank, so they bounce between ranks or the run hangs.

Fix: transport every photon passed in, as ref does (`std::iota`).

**3b. Uninitialized descriptor (`photon.h`).** `Photon() {}` left `descriptors` undefined. It is now `Photon() : descriptors{{SCATTER, 0, 0, 0}} {}`. This is safe:
- every read of the descriptor happens after transport writes it;
- the MPI layout is unchanged;
- `Photon` is still trivially copyable for the GPU path.

**3c. Tallies accumulated across timesteps (`hov_setup.cc`, `particle_pass_transport.h`).** The SoA tallies were zeroed once at startup but added into the rank totals every step, so step *n* reported steps 1..*n*. A new `reset_tally_soa()` zeroes them in place, keeping the pairs' pointers valid. It is called where ref creates its fresh `cell_tallies`.

**3d. Cell properties all zeros (`hov_setup.cc`, `particle_pass_driver.h`).** `unpack_cells_soa` ran once in `main.cc`, before `calculate_photon_energy` ever set `op_a`, `op_s` and `f`. So the SoA copies were zero for the whole run: no scattering, no absorption, and `absorbed_E / (sigma_a * f)` = 0/0 = NaN. A new `refresh_cells_soa(cells)` rewrites them in place after `calculate_photon_energy` each step. Geometry is still copied once.

**3e. Index-array overflow (`transport_photon_hov.h`).** `phtn_cells_idx` and every pair hold 15,000 entries, but one transport call receives all source and census photons (`hohlraum_multi` is ~31M per rank). The index fill wrote past the array, and the alias addresses ran past the reserved alias region. Fix: each sweep is processed in chunks of at most `phtn_cells_idx.size()`. Each chunk fills the index (outside the ROI) and runs one ROI; compaction runs once per sweep. The array can't simply be sized up, because census photons accumulate from step to step and resizing would recreate pairs and alias regions mid-run.

**3f. Replicated mode runs the scalar kernel (`replicated_transport.h`).** Comment only, no behaviour change. Coverage with the sweep inputs:

| Input | `dd_transport_type` | 1 process | 8 processes |
|---|---|---|---|
| hohlraum_single | REPLICATED | scalar | scalar |
| hohlraum_single_shrunk | REPLICATED | scalar | scalar |
| hohlraum_multi | PARTICLE_PASS | forced replicated: scalar | hov kernel |
| hohlraum_multi_shrunk | PARTICLE_PASS | forced replicated: scalar | hov kernel |

**3g. Face selection at an angle of exactly 0 (`transport_photon_hov.h`).** `sgn_local` now returns 1 only for a strictly positive component, matching ref's `sgn` in `cell.h`.

---

## Status and challenges

- **Status:** none of this has been built on aarch64; it has only passed syntax checks with stubbed `config.h`/`mpi.h`. Correctness against ref (E1) has not been run.
- **Fit:** see the top of this file. This is the reason the work is parked.
- **Coverage:** only `hohlraum_multi*` at 8 ranks reach the hov kernel.
- **Measurement:** with the chunking in 3e, a sweep larger than 15,000 photons becomes several ROIs, and the harness measures the first one only. One ROI per sweep would need capacity for ~31M photons per rank on `hohlraum_multi`.
- **Open upstream question:** in particle-passing mode, `rank_abs_E`/`rank_track_E` are created once and added into every step (`+=` in `particle_pass_transport.h`), so temperatures see cumulative energy. Replicated mode assigns (`=`). Both variants keep upstream's behaviour.
- **What reviving it would take:**
  1. build and test this branch;
  2. move the index build (Phase A) inside the ROI;
  3. add a hov path for replicated mode, or restrict the inputs;
  4. decide between chunking and a larger capacity;
  5. run E1: ref vs hov on `hohlraum_multi_shrunk` at 8 ranks, comparing per-step conservation output and temperatures.
