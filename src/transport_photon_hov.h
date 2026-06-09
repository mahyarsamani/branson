#ifndef TRANSPORT_PHOTON_HOV_H
#define TRANSPORT_PHOTON_HOV_H

#include <vector>
#include <algorithm>
#include <cmath>
#include "cell.h"
#include "cell_tally.h"
#include "photon.h"
#include "hov_setup.h"
#include "sampling_functions.h"

#ifdef HOV
extern "C" {
#include "annotate.h"
}

static inline double sgn_local(double v) {
    return v < 0.0 ? 0 : 1;
}

void cpu_transport_photons_hov(const uint32_t rank_cell_offset,
                               std::vector<Photon> &photons,
                               const std::vector<Cell> &cells,
                               std::vector<Cell_Tally> &cell_tallies,
                               int n_omp_threads) {
    using Constants::bc_type;
    using Constants::c;

    std::vector<uint32_t> active_photons;
    active_photons.reserve(photons.size());

    // Initial population
    for (size_t i = 0; i < photons.size(); ++i) {
        if (photons[i].is_active()) {
            active_photons.push_back((uint32_t)i);
        }
    }

    while (!active_photons.empty()) {
        // Phase A: Setup index array for HOV
        for (size_t i = 0; i < active_photons.size(); ++i) {
            uint32_t pid = active_photons[i];
            g_hov_data.phtn_cells_idx[i] = photons[pid].get_cell() - rank_cell_offset;
        }

        // Phase B: Physics step (ROI)
#if defined(ANNOTATE)
        roi_begin_();
#ifdef SYNC_ON_ROI
        annotate_synchronize_(1);
#endif // SYNC_ON_ROI
#endif // ANNOTATE

        for (size_t i = 0; i < active_photons.size(); ++i) {
            uint32_t pid = active_photons[i];
            Photon& phtn = photons[pid];
            auto& rng = phtn.get_rng();
            uint32_t group = phtn.get_group();

            hov_result_f64_u32_t res_f, res_op_s, res_op_a;
            hov_gather_f64_u32(&res_f, &g_hov_data.pair_f, i);
            hov_gather_f64_u32(&res_op_s, &g_hov_data.pair_op_s[group], i);
            hov_gather_f64_u32(&res_op_a, &g_hov_data.pair_op_a[group], i);

            double f = res_f.data_val;
            double sigma_s = res_op_s.data_val;
            double sigma_a = res_op_a.data_val;

            hov_result_f64_u32_t res_n0, res_n1, res_n2, res_n3, res_n4, res_n5;
            hov_gather_f64_u32(&res_n0, &g_hov_data.pair_node_0, i);
            hov_gather_f64_u32(&res_n1, &g_hov_data.pair_node_1, i);
            hov_gather_f64_u32(&res_n2, &g_hov_data.pair_node_2, i);
            hov_gather_f64_u32(&res_n3, &g_hov_data.pair_node_3, i);
            hov_gather_f64_u32(&res_n4, &g_hov_data.pair_node_4, i);
            hov_gather_f64_u32(&res_n5, &g_hov_data.pair_node_5, i);

            double nodes[6] = {res_n0.data_val, res_n1.data_val, res_n2.data_val,
                               res_n3.data_val, res_n4.data_val, res_n5.data_val};

            double total_sigma_s = (1.0 - f) * sigma_a + sigma_s;
            double dist_to_scatter = (total_sigma_s > 0.0) ?
                -log(rng.generate_random_number()) / total_sigma_s : 1.0e100;

            double min_dist = 1.0e16;
            uint32_t surface_cross = 0;
            auto pos_arr = phtn.get_position();
            const double* pos = pos_arr.data();
            auto angle_arr = phtn.get_angle();
            const double* angle = angle_arr.data();
            for (uint32_t d = 0; d < 3; d++) {
                uint32_t index = 2 * d + sgn_local(angle[d]);
                double dist = (nodes[index] - pos[d]) / angle[d];
                if (dist < min_dist) {
                    min_dist = dist;
                    surface_cross = index;
                }
            }
            double dist_to_boundary = min_dist;
            double dist_to_census = phtn.get_distance_remaining();

            double dist_to_event = std::min(dist_to_scatter, std::min(dist_to_boundary, dist_to_census));
            double absorbed_E = phtn.get_E() * (1.0 - exp(-sigma_a * f * dist_to_event));

            hov_result_f64_u32_t res_abs_E, res_track_E;
            hov_gather_f64_u32(&res_abs_E, &g_hov_data.pair_abs_E, i);
            hov_gather_f64_u32(&res_track_E, &g_hov_data.pair_track_E, i);

            hov_scatter_f64_u32(res_abs_E.data_val + absorbed_E, &g_hov_data.pair_abs_E, i);
            hov_scatter_f64_u32(res_track_E.data_val + (absorbed_E / (sigma_a * f)), &g_hov_data.pair_track_E, i);

            phtn.set_E(phtn.get_E() - absorbed_E);
            phtn.move(dist_to_event);

            if (phtn.below_cutoff(Constants::cutoff_fraction)) {
                hov_gather_f64_u32(&res_abs_E, &g_hov_data.pair_abs_E, i);
                hov_scatter_f64_u32(res_abs_E.data_val + phtn.get_E(), &g_hov_data.pair_abs_E, i);
                phtn.set_descriptor(Constants::KILLED);
            } else {
                if (dist_to_event == dist_to_scatter) {
                    phtn.set_angle(get_uniform_angle(rng));
                    if (rng.generate_random_number() > (sigma_s / ((1.0 - f) * sigma_a + sigma_s))) {
                        uint32_t cell_idx = g_hov_data.phtn_cells_idx[i];
                        const Cell& cell_data = cells[cell_idx];
                        phtn.set_group(sample_emission_group(rng, cell_data));
                    }
                    phtn.set_descriptor(Constants::SCATTER);
                } else if (dist_to_event == dist_to_boundary) {
                    hov_pair_t* pair_bc;
                    hov_pair_t* pair_e_next;
                    if (surface_cross == 0) { pair_bc = &g_hov_data.pair_bc_0; pair_e_next = &g_hov_data.pair_e_next_0; }
                    else if (surface_cross == 1) { pair_bc = &g_hov_data.pair_bc_1; pair_e_next = &g_hov_data.pair_e_next_1; }
                    else if (surface_cross == 2) { pair_bc = &g_hov_data.pair_bc_2; pair_e_next = &g_hov_data.pair_e_next_2; }
                    else if (surface_cross == 3) { pair_bc = &g_hov_data.pair_bc_3; pair_e_next = &g_hov_data.pair_e_next_3; }
                    else if (surface_cross == 4) { pair_bc = &g_hov_data.pair_bc_4; pair_e_next = &g_hov_data.pair_e_next_4; }
                    else { pair_bc = &g_hov_data.pair_bc_5; pair_e_next = &g_hov_data.pair_e_next_5; }

                    hov_result_u32_u32_t res_bc, res_e_next;
                    hov_gather_u32_u32(&res_bc, pair_bc, i);
                    hov_gather_u32_u32(&res_e_next, pair_e_next, i);

                    uint32_t boundary_event = res_bc.data_val;
                    if (boundary_event == Constants::ELEMENT) {
                        phtn.set_cell(res_e_next.data_val);
                        phtn.set_descriptor(Constants::BOUND);
                    } else if (boundary_event == Constants::PROCESSOR) {
                        phtn.set_cell(res_e_next.data_val);
                        phtn.set_descriptor(Constants::PASS);
                    } else if (boundary_event == Constants::VACUUM || boundary_event == Constants::SOURCE) {
                        phtn.set_descriptor(Constants::EXIT);
                    } else {
                        phtn.reflect(surface_cross);
                        phtn.set_descriptor(Constants::BOUND);
                    }
                } else if (dist_to_event == dist_to_census) {
                    phtn.set_descriptor(Constants::CENSUS);
                }
            }
        } // End Phase B (ROI)

#if defined(ANNOTATE)
        roi_end_();
#ifdef SYNC_ON_ROI
        annotate_synchronize_(2);
#endif // SYNC_ON_ROI
#endif // ANNOTATE

        // Phase C: State updates and compaction
        std::vector<uint32_t> next_active;
        next_active.reserve(active_photons.size());
        for (size_t i = 0; i < active_photons.size(); ++i) {
            uint32_t pid = active_photons[i];
            if (photons[pid].is_active()) {
                next_active.push_back(pid);
            }
        }
        active_photons = std::move(next_active);
    }
}

#endif // HOV

#endif // TRANSPORT_PHOTON_HOV_H
