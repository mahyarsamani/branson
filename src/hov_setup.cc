#include "hov_setup.h"
// MYSTUFF
#include <algorithm>
// FFUTSYM

#ifdef HOV

BransonHOVData g_hov_data;

void unpack_cells_soa(const std::vector<Cell>& cells, size_t max_photons) {
    size_t n_cells = cells.size();

    g_hov_data.f_soa.resize(n_cells);
    g_hov_data.node_0_soa.resize(n_cells);
    g_hov_data.node_1_soa.resize(n_cells);
    g_hov_data.node_2_soa.resize(n_cells);
    g_hov_data.node_3_soa.resize(n_cells);
    g_hov_data.node_4_soa.resize(n_cells);
    g_hov_data.node_5_soa.resize(n_cells);
    
    g_hov_data.bc_0_soa.resize(n_cells);
    g_hov_data.bc_1_soa.resize(n_cells);
    g_hov_data.bc_2_soa.resize(n_cells);
    g_hov_data.bc_3_soa.resize(n_cells);
    g_hov_data.bc_4_soa.resize(n_cells);
    g_hov_data.bc_5_soa.resize(n_cells);

    g_hov_data.e_next_0_soa.resize(n_cells);
    g_hov_data.e_next_1_soa.resize(n_cells);
    g_hov_data.e_next_2_soa.resize(n_cells);
    g_hov_data.e_next_3_soa.resize(n_cells);
    g_hov_data.e_next_4_soa.resize(n_cells);
    g_hov_data.e_next_5_soa.resize(n_cells);

    for (int g = 0; g < BRANSON_N_GROUPS; ++g) {
        g_hov_data.op_a_soa[g].resize(n_cells);
        g_hov_data.op_s_soa[g].resize(n_cells);
    }

    g_hov_data.phtn_cells_idx.resize(max_photons, 0);

    for (size_t i = 0; i < n_cells; ++i) {
        g_hov_data.f_soa[i] = cells[i].get_f();
        
        const double* nodes = cells[i].get_node_array();
        g_hov_data.node_0_soa[i] = nodes[0];
        g_hov_data.node_1_soa[i] = nodes[1];
        g_hov_data.node_2_soa[i] = nodes[2];
        g_hov_data.node_3_soa[i] = nodes[3];
        g_hov_data.node_4_soa[i] = nodes[4];
        g_hov_data.node_5_soa[i] = nodes[5];

        g_hov_data.bc_0_soa[i] = (uint32_t)cells[i].get_bc(0);
        g_hov_data.bc_1_soa[i] = (uint32_t)cells[i].get_bc(1);
        g_hov_data.bc_2_soa[i] = (uint32_t)cells[i].get_bc(2);
        g_hov_data.bc_3_soa[i] = (uint32_t)cells[i].get_bc(3);
        g_hov_data.bc_4_soa[i] = (uint32_t)cells[i].get_bc(4);
        g_hov_data.bc_5_soa[i] = (uint32_t)cells[i].get_bc(5);

        g_hov_data.e_next_0_soa[i] = cells[i].get_next_cell(0);
        g_hov_data.e_next_1_soa[i] = cells[i].get_next_cell(1);
        g_hov_data.e_next_2_soa[i] = cells[i].get_next_cell(2);
        g_hov_data.e_next_3_soa[i] = cells[i].get_next_cell(3);
        g_hov_data.e_next_4_soa[i] = cells[i].get_next_cell(4);
        g_hov_data.e_next_5_soa[i] = cells[i].get_next_cell(5);

        for (int g = 0; g < BRANSON_N_GROUPS; ++g) {
            g_hov_data.op_a_soa[g][i] = cells[i].get_op_a(g);
            g_hov_data.op_s_soa[g][i] = cells[i].get_op_s(g);
        }
    }

    void* idx_base = (void*)g_hov_data.phtn_cells_idx.data();

    g_hov_data.pair_f = hov_create_pair(g_hov_data.f_soa.data(), idx_base, sizeof(double), sizeof(uint32_t), max_photons);
    
    g_hov_data.pair_node_0 = hov_create_pair(g_hov_data.node_0_soa.data(), idx_base, sizeof(double), sizeof(uint32_t), max_photons);
    g_hov_data.pair_node_1 = hov_create_pair(g_hov_data.node_1_soa.data(), idx_base, sizeof(double), sizeof(uint32_t), max_photons);
    g_hov_data.pair_node_2 = hov_create_pair(g_hov_data.node_2_soa.data(), idx_base, sizeof(double), sizeof(uint32_t), max_photons);
    g_hov_data.pair_node_3 = hov_create_pair(g_hov_data.node_3_soa.data(), idx_base, sizeof(double), sizeof(uint32_t), max_photons);
    g_hov_data.pair_node_4 = hov_create_pair(g_hov_data.node_4_soa.data(), idx_base, sizeof(double), sizeof(uint32_t), max_photons);
    g_hov_data.pair_node_5 = hov_create_pair(g_hov_data.node_5_soa.data(), idx_base, sizeof(double), sizeof(uint32_t), max_photons);

    g_hov_data.pair_bc_0 = hov_create_pair(g_hov_data.bc_0_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);
    g_hov_data.pair_bc_1 = hov_create_pair(g_hov_data.bc_1_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);
    g_hov_data.pair_bc_2 = hov_create_pair(g_hov_data.bc_2_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);
    g_hov_data.pair_bc_3 = hov_create_pair(g_hov_data.bc_3_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);
    g_hov_data.pair_bc_4 = hov_create_pair(g_hov_data.bc_4_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);
    g_hov_data.pair_bc_5 = hov_create_pair(g_hov_data.bc_5_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);

    g_hov_data.pair_e_next_0 = hov_create_pair(g_hov_data.e_next_0_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);
    g_hov_data.pair_e_next_1 = hov_create_pair(g_hov_data.e_next_1_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);
    g_hov_data.pair_e_next_2 = hov_create_pair(g_hov_data.e_next_2_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);
    g_hov_data.pair_e_next_3 = hov_create_pair(g_hov_data.e_next_3_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);
    g_hov_data.pair_e_next_4 = hov_create_pair(g_hov_data.e_next_4_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);
    g_hov_data.pair_e_next_5 = hov_create_pair(g_hov_data.e_next_5_soa.data(), idx_base, sizeof(uint32_t), sizeof(uint32_t), max_photons);

    for (int g = 0; g < BRANSON_N_GROUPS; ++g) {
        g_hov_data.pair_op_a[g] = hov_create_pair(g_hov_data.op_a_soa[g].data(), idx_base, sizeof(double), sizeof(uint32_t), max_photons);
        g_hov_data.pair_op_s[g] = hov_create_pair(g_hov_data.op_s_soa[g].data(), idx_base, sizeof(double), sizeof(uint32_t), max_photons);
    }
}

void setup_tally_pairs(size_t n_cells, size_t max_photons) {
    g_hov_data.tally_abs_E_soa.assign(n_cells, 0.0);
    g_hov_data.tally_track_E_soa.assign(n_cells, 0.0);

    void* idx_base = (void*)g_hov_data.phtn_cells_idx.data();
    g_hov_data.pair_abs_E = hov_create_pair(g_hov_data.tally_abs_E_soa.data(), idx_base, sizeof(double), sizeof(uint32_t), max_photons);
    g_hov_data.pair_track_E = hov_create_pair(g_hov_data.tally_track_E_soa.data(), idx_base, sizeof(double), sizeof(uint32_t), max_photons);
}

void collect_tally_soa(std::vector<double>& rank_abs_E, std::vector<double>& rank_track_E) {
    for (size_t i = 0; i < rank_abs_E.size(); ++i) {
        rank_abs_E[i] += g_hov_data.tally_abs_E_soa[i];
        rank_track_E[i] += g_hov_data.tally_track_E_soa[i];
    }
}

// MYSTUFF
// Copy the per-step cell properties (set by Mesh::calculate_photon_energy)
// into the SoA arrays. Writes in place, without resizing, so the data_base
// pointers held by pair_f, pair_op_a and pair_op_s stay valid.
void refresh_cells_soa(const std::vector<Cell>& cells) {
    for (size_t i = 0; i < cells.size(); ++i) {
        g_hov_data.f_soa[i] = cells[i].get_f();
        for (int g = 0; g < BRANSON_N_GROUPS; ++g) {
            g_hov_data.op_a_soa[g][i] = cells[i].get_op_a(g);
            g_hov_data.op_s_soa[g][i] = cells[i].get_op_s(g);
        }
    }
}

// Zero the SoA tallies in place, the counterpart of the fresh cell_tallies
// that particle_pass_transport builds on every call.
void reset_tally_soa() {
    std::fill(g_hov_data.tally_abs_E_soa.begin(), g_hov_data.tally_abs_E_soa.end(), 0.0);
    std::fill(g_hov_data.tally_track_E_soa.begin(), g_hov_data.tally_track_E_soa.end(), 0.0);
}
// FFUTSYM

#endif // HOV
