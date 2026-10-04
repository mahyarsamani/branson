#ifndef HOV_SETUP_H
#define HOV_SETUP_H

#include <vector>
#include "cell.h"
#include "cell_tally.h"

#ifdef HOV

#include "hov_allocator.h"

template <class T>
using hov_vector = std::vector<T, HovAllocator<T>>;

struct BransonHOVData {
    hov_vector<double> f_soa;
    hov_vector<double> node_0_soa;
    hov_vector<double> node_1_soa;
    hov_vector<double> node_2_soa;
    hov_vector<double> node_3_soa;
    hov_vector<double> node_4_soa;
    hov_vector<double> node_5_soa;

    hov_vector<uint32_t> bc_0_soa;
    hov_vector<uint32_t> bc_1_soa;
    hov_vector<uint32_t> bc_2_soa;
    hov_vector<uint32_t> bc_3_soa;
    hov_vector<uint32_t> bc_4_soa;
    hov_vector<uint32_t> bc_5_soa;

    hov_vector<uint32_t> e_next_0_soa;
    hov_vector<uint32_t> e_next_1_soa;
    hov_vector<uint32_t> e_next_2_soa;
    hov_vector<uint32_t> e_next_3_soa;
    hov_vector<uint32_t> e_next_4_soa;
    hov_vector<uint32_t> e_next_5_soa;

    hov_vector<double> op_a_soa[BRANSON_N_GROUPS];
    hov_vector<double> op_s_soa[BRANSON_N_GROUPS];

    hov_vector<double> tally_abs_E_soa;
    hov_vector<double> tally_track_E_soa;

    hov_vector<uint32_t> phtn_cells_idx;

    hov_pair_t pair_f;
    hov_pair_t pair_node_0;
    hov_pair_t pair_node_1;
    hov_pair_t pair_node_2;
    hov_pair_t pair_node_3;
    hov_pair_t pair_node_4;
    hov_pair_t pair_node_5;

    hov_pair_t pair_bc_0;
    hov_pair_t pair_bc_1;
    hov_pair_t pair_bc_2;
    hov_pair_t pair_bc_3;
    hov_pair_t pair_bc_4;
    hov_pair_t pair_bc_5;

    hov_pair_t pair_e_next_0;
    hov_pair_t pair_e_next_1;
    hov_pair_t pair_e_next_2;
    hov_pair_t pair_e_next_3;
    hov_pair_t pair_e_next_4;
    hov_pair_t pair_e_next_5;

    hov_pair_t pair_op_a[BRANSON_N_GROUPS];
    hov_pair_t pair_op_s[BRANSON_N_GROUPS];

    hov_pair_t pair_abs_E;
    hov_pair_t pair_track_E;
};

extern BransonHOVData g_hov_data;

void unpack_cells_soa(const std::vector<Cell>& cells, size_t max_photons);
void setup_tally_pairs(size_t n_cells, size_t max_photons);
void collect_tally_soa(std::vector<double>& rank_abs_E, std::vector<double>& rank_track_E);
// MYSTUFF
void refresh_cells_soa(const std::vector<Cell>& cells);
void reset_tally_soa();
// FFUTSYM

#endif // HOV

#endif // HOV_SETUP_H
