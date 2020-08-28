/*
 * CUDADNAInteraction.cu
 *
 *  Created on: 22/feb/2013
 *      Author: lorenzo
 */

#include "CUDADNACTInteraction.h"
#include "CUDA_DNA.cuh"
#include "CUDA_ACT.cuh"
#include "../Lists/CUDASimpleVerletList.h"
#include "../Lists/CUDANoList.h"
#include "../../Interactions/DNA2Interaction.h"
#include "../../Interactions/DNACTInteraction.h"

template<typename number, typename number4>
CUDADNACTInteraction<number, number4>::CUDADNACTInteraction() {
    _d_spring_pottype = NULL;
    _d_spring_potential = NULL;
    _d_spring_eqdist = NULL;
    _d_ang_params = NULL;
    _h_spring_pottype = NULL;
    _h_spring_potential = NULL;
    _h_spring_eqdist = NULL;
    _h_ang_params = NULL;

    _spring_param_size_number = 0;
    _spring_param_size_char = 0;
    _ang_param_size = 0;
}

template<typename number, typename number4>
CUDADNACTInteraction<number, number4>::~CUDADNACTInteraction() {
    //Delete All pointers required for spring potential parameters
    if(_d_spring_pottype != NULL) CUDA_SAFE_CALL( cudaFree(_d_spring_pottype) );
    if(_d_spring_potential != NULL) CUDA_SAFE_CALL( cudaFree(_d_spring_potential) );
    if(_d_spring_eqdist != NULL) CUDA_SAFE_CALL( cudaFree(_d_spring_eqdist) );
    if(_d_ang_params != NULL) CUDA_SAFE_CALL( cudaFree(_d_ang_params) );
    if(_h_spring_pottype != NULL) delete[] _h_spring_pottype;
    if(_h_spring_potential != NULL) delete[] _h_spring_potential;
    if(_h_spring_eqdist != NULL) delete[] _h_spring_eqdist;
    if(_h_ang_params != NULL) delete[] _h_ang_params;
}

template<typename number, typename number4>
void CUDADNACTInteraction<number, number4>::get_settings(input_file &inp) {
    _use_debye_huckel = false;
    _use_oxDNA2_coaxial_stacking = false;
    _use_oxDNA2_FENE = false;
    std::string inter_type;
    if (!getInputString(&inp, "parfile", this->_parameterfile, 0) == KEY_FOUND){
        throw oxDNAException("Key 'PARFILE' not found. Necessary for Protein sims.");
    }
    if (!getInputString(&inp, "topology", this->_topology_filename, 0) == KEY_FOUND){
        throw oxDNAException("Key 'topology_file' not found.");
    }

    _use_debye_huckel = true;
    _use_oxDNA2_coaxial_stacking = true;
    _use_oxDNA2_FENE = true;

    //Aren't these constructed in the implicit DNACT Constructor Call?
//    // copy-pasted from the DNA2Interaction constructor
//    this->_int_map[this->DEBYE_HUCKEL] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)) &DNACTInteraction<number>::_debye_huckel;
//    // I assume these are needed. I think the interaction map is used for when the observables want to print energy
//    //this->_int_map[this->BACKBONE] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)) &DNAInteraction<number>::_backbone;
//    this->_int_map[this->COAXIAL_STACKING] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)) &DNACTInteraction<number>::_coaxial_stacking;
//    //Protein Methods Function Pointers
//    this->_int_map[this->SPRING] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)) &DNACTInteraction<number>::_protein_spring;
//    this->_int_map[this->PRO_EXC_VOL] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)) &DNACTInteraction<number>::_protein_exc_volume;
//    this->_int_map[this->PRO_ANG_POT] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)) &DNACTInteraction<number>::_protein_ang_pot;
//    //Protein-DNA Function Pointers
//    this->_int_map[this->PRO_DNA_EXC_VOL] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)) &DNACTInteraction<number>::_protein_dna_exc_volume;

// we don't need the F4_... terms as the macros are used in the CUDA_DNA.cuh file; this doesn't apply for the F2_K term
    this->F2_K[1] = CXST_K_OXDNA2;
    _debye_huckel_half_charged_ends = true;
    this->_grooving = true;
    // end copy from DNA2Interaction

    // copied from DNA2Interaction::get_settings() (CPU), the least bad way of doing things
    getInputNumber(&inp, "salt_concentration", &_salt_concentration, 1);
    getInputBool(&inp, "dh_half_charged_ends", &_debye_huckel_half_charged_ends, 0);

    // lambda-factor (the dh length at T = 300K, I = 1.0)
    _debye_huckel_lambdafactor = 0.3616455f;
    getInputFloat(&inp, "dh_lambda", &_debye_huckel_lambdafactor, 0);

    // the prefactor to the Debye-Huckel term
    _debye_huckel_prefactor = 0.0543f;
    getInputFloat(&inp, "dh_strength", &_debye_huckel_prefactor, 0);
    // End copy from DNA2Interaction

//    this->DNACTInteraction<number>::init();
    this->DNACTInteraction<number>::get_settings(inp);

}

template<typename number, typename number4>
void CUDADNACTInteraction<number, number4>::cuda_init(number box_side, int N) {
    CUDABaseInteraction<number, number4>::cuda_init(box_side, N);

//    Addition of Reading Parameter File -> Moved from get_settings due to needing to fill variables that are filled in the CPU version of DNACTInteraction::read_topology
    fstream top;
    int tmp1, tmp2;
    top.open(this->_topology_filename, ios::in);
    if (top.is_open()){
        top >> tmp1 >> tmp2 >> this->ndna >> this->npro >> this->ndnas;
        top >> this->_firststrand;
        top.close();
    } else {
        throw oxDNAException("Could not open Topology File");
    }


    if(this->_firststrand < 0) offset = 0;
    else if(this->_firststrand > 0) offset = this->ndna;
    else throw oxDNAException("No Strand should have an ID of 0");


//    //Initalizing Host and Device Arrays for Spring Parameters
    _spring_param_size_number = sizeof(number) * (this->npro*this->npro);
    _spring_param_size_char = sizeof(char) * (this->npro*this->npro);
    _ang_param_size = sizeof(number) * (this->npro*4);
    _h_spring_pottype = new char[this->npro*this->npro]();
    CUDA_SAFE_CALL( cudaMalloc(&_d_spring_pottype, _spring_param_size_char));
    _h_spring_potential = new number[this->npro*this->npro]();
    CUDA_SAFE_CALL(cudaMalloc(&_d_spring_potential, _spring_param_size_number));
    _h_spring_eqdist = new number[this->npro*this->npro]();
    CUDA_SAFE_CALL(cudaMalloc(&_d_spring_eqdist, _spring_param_size_number));
    //Initializing Host and Device Arrays for Angular Parameters
    _h_ang_params = new number[this->npro*4]();
    CUDA_SAFE_CALL( cudaMalloc(&_d_ang_params, _ang_param_size));

    char potswitch = 'x';
    number potential = 0.f, dist = 0.f;
    for(int i = 0; i< (this->npro*this->npro); i++){
        _h_spring_eqdist[i] = dist;
        _h_spring_potential[i] = potential;
        _h_spring_pottype[i] = potswitch;
        if(i < (this->npro *4 )) _h_ang_params[i] = dist;
    }

    //Checkers as Lambdas
    auto valid_angles = [](double a, double b, double c, double d)
    {
        double anglemin = min({a, b, c, d});
        double anglemax = max({a, b, c, d});
        if (anglemin < -1.0 || anglemax > 1.0){
            throw oxDNAException("Cos of Angle in Parameter File not in Valid bounds");
        }
    };

    auto valid_spring_params = [](int N, int x, int y, double d, char s, double k){
        if(x < 0 || x > N) throw oxDNAException("Invalid Particle ID %d in Parameter File", x);
        if(y < 0 || y > N) throw oxDNAException("Invalid Particle ID %d in Parameter File", y);
        if(d < 0) throw oxDNAException("Invalid Eq Distance %d in Parameter File", d);
        if(s != 's') throw oxDNAException("Potential Type %c Not Supported", s);
        if(k < 0) throw oxDNAException("Spring Constant %f Not Supported", k);
    };

    //Reading Parameter File
    int key1, key2 = 0;
    number a0, b0, c0, d0;
    string carbons;
    fstream parameters;
    parameters.open(this->_parameterfile, ios::in);
    getline (parameters,carbons);
    if (parameters.is_open())
    {
        while (parameters.good())
        {
            parameters >> key1 >> key2 >> dist >> potswitch >> potential;

            if(offset != 0) {
                key1 -= offset;
                key2 -= offset;
            }

            valid_spring_params(N, key1, key2, dist, potswitch, potential);
            if (key2 - key1 == 1){
                //Angular Parameters
                parameters >> a0 >> b0 >> c0 >> d0;
                valid_angles(a0, b0, c0, d0);
                _h_ang_params[key1*4] = a0;
                _h_ang_params[key1*4+1] = b0;
                _h_ang_params[key1*4+2] = c0;
                _h_ang_params[key1*4+3] = d0;

                _h_spring_potential[key1*this->npro + key2] = potential;
                _h_spring_eqdist[key1*this->npro + key2] = dist;
                _h_spring_pottype[key1*this->npro + key2] = potswitch;
                //added so CUDA will play nice
                _h_spring_potential[key2*this->npro + key1] = potential;
                _h_spring_eqdist[key2*this->npro + key1] = dist;
                _h_spring_pottype[key2*this->npro + key1] = potswitch;

            } else {
                _h_spring_potential[key1*this->npro + key2] = potential;
                _h_spring_eqdist[key1*this->npro + key2] = dist;
                _h_spring_pottype[key1*this->npro + key2] = potswitch;
                //added so CUDA will play nice
                _h_spring_potential[key2*this->npro + key1] = potential;
                _h_spring_eqdist[key2*this->npro + key1] = dist;
                _h_spring_pottype[key2*this->npro + key1] = potswitch;
            }
        }
        parameters.close();
    } else {
        throw oxDNAException("ParameterFile Could Not Be Opened");
    }
    //Some System Checks
    // WHAT ELSE SHOULD I BE DOUBLE CHECKING?

    // Copied from CUDADNAINTERACTION
    DNAInteraction<number>::init();

    float f_copy = this->_hb_multiplier;
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_hb_multi, &f_copy, sizeof(float)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_N, &N, sizeof(int)) );

    number tmp[50];
    for(int i = 0; i < 2; i++) for(int j = 0; j < 5; j++) for(int k = 0; k < 5; k++) tmp[i*25 + j*5 + k] = this->F1_EPS[i][j][k];

    COPY_ARRAY_TO_CONSTANT(MD_F1_EPS, tmp, 50);

    for(int i = 0; i < 2; i++) for(int j = 0; j < 5; j++) for(int k = 0; k < 5; k++) tmp[i*25 + j*5 + k] = this->F1_SHIFT[i][j][k];

    COPY_ARRAY_TO_CONSTANT(MD_F1_SHIFT, tmp, 50);

    COPY_ARRAY_TO_CONSTANT(MD_F1_A, this->F1_A, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F1_RC, this->F1_RC, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F1_R0, this->F1_R0, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F1_BLOW, this->F1_BLOW, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F1_BHIGH, this->F1_BHIGH, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F1_RLOW, this->F1_RLOW, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F1_RHIGH, this->F1_RHIGH, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F1_RCLOW, this->F1_RCLOW, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F1_RCHIGH, this->F1_RCHIGH, 2);

    COPY_ARRAY_TO_CONSTANT(MD_F2_K, this->F2_K, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F2_RC, this->F2_RC, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F2_R0, this->F2_R0, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F2_BLOW, this->F2_BLOW, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F2_BHIGH, this->F2_BHIGH, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F2_RLOW, this->F2_RLOW, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F2_RHIGH, this->F2_RHIGH, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F2_RCLOW, this->F2_RCLOW, 2);
    COPY_ARRAY_TO_CONSTANT(MD_F2_RCHIGH, this->F2_RCHIGH, 2);

    COPY_ARRAY_TO_CONSTANT(MD_F5_PHI_A, this->F5_PHI_A, 4);
    COPY_ARRAY_TO_CONSTANT(MD_F5_PHI_B, this->F5_PHI_B, 4);
    COPY_ARRAY_TO_CONSTANT(MD_F5_PHI_XC, this->F5_PHI_XC, 4);
    COPY_ARRAY_TO_CONSTANT(MD_F5_PHI_XS, this->F5_PHI_XS, 4);


    if(this->_use_edge) CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_n_forces, &this->_n_forces, sizeof(int)) );
    if (_use_debye_huckel) {
        // copied from DNA2Interaction::init() (CPU), the least bad way of doing things
        // We wish to normalise with respect to T=300K, I=1M. 300K=0.1 s.u. so divide this->_T by 0.1
        number lambda = _debye_huckel_lambdafactor * sqrt(this->_T / 0.1f) / sqrt(_salt_concentration);
        // RHIGH gives the distance at which the smoothing begins
        _debye_huckel_RHIGH = 3.0 * lambda;
        _minus_kappa = -1.0 / lambda;

        // these are just for convenience for the smoothing parameter computation
        number x = _debye_huckel_RHIGH;
        number q = _debye_huckel_prefactor;
        number l = lambda;

        // compute the some smoothing parameters
        _debye_huckel_B = -(exp(-x / l) * q * q * (x + l) * (x + l)) / (-4. * x * x * x * l * l * q);
        _debye_huckel_RC = x * (q * x + 3. * q * l) / (q * (x + l));

        number debyecut;
        if (this->_grooving) {
            debyecut = 2.0f * sqrt((POS_MM_BACK1) * (POS_MM_BACK1) + (POS_MM_BACK2) * (POS_MM_BACK2)) + _debye_huckel_RC;
        } else {
            debyecut = 2.0f * sqrt(SQR(POS_BACK)) + _debye_huckel_RC;
        }

        // the cutoff radius for the potential should be the larger of rcut and debyecut
        if (debyecut > this->_rcut){
            this->_rcut = debyecut;
            this->_sqr_rcut = debyecut*debyecut;
        }
        //End copy from DNA2Interaction

        CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_dh_RC, &_debye_huckel_RC, sizeof(float)) );
        CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_dh_RHIGH, &_debye_huckel_RHIGH, sizeof(float)) );
        CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_dh_prefactor, &_debye_huckel_prefactor, sizeof(float)) );
        CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_dh_B, &_debye_huckel_B, sizeof(float)) );
        CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_dh_minus_kappa, &_minus_kappa, sizeof(float)) );
        CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_dh_half_charged_ends, &_debye_huckel_half_charged_ends, sizeof(bool)) );
    }
    //Constants for DNA/Protein Interactions
    //NEW VERSION #QuarticExcludedVolume
    //Backbone-Protein Excluded Volume Parameters
    _pro_backbone_sigma = 0.57f;
    _pro_backbone_rstar= 0.569f;
    _pro_backbone_b = 178699253.5f;
    _pro_backbone_rcut = 0.572934f;
    //Base-Protein Excluded Volume Parameters
    _pro_base_sigma = 0.36f;
    _pro_base_rstar= 0.359f;
    _pro_base_b = 296866090.0f;
    _pro_base_rcut = 0.362897f;
    //Protein-Protein Excluded Volume Parameters
    _pro_sigma = 0.35f;
    _pro_rstar = 0.349f;
    _pro_b = 306484596.0f;
    _pro_rcut = 0.352894;

    _kbend = this->_k_bend;
    _ktor = this->_k_tor;

    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_sigma, &_pro_sigma, sizeof(float)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_rstar, &_pro_rstar, sizeof(float)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_rc, &_pro_rcut, sizeof(float)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_b, &_pro_b, sizeof(float)) );

    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_backbone_sigma, &_pro_backbone_sigma, sizeof(float)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_backbone_rstar, &_pro_backbone_rstar, sizeof(float)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_backbone_rc, &_pro_backbone_rcut, sizeof(float)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_backbone_b, &_pro_backbone_b, sizeof(float)) );

    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_base_sigma, &_pro_base_sigma, sizeof(float)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_base_rstar, &_pro_base_rstar, sizeof(float)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_base_rc, &_pro_base_rcut, sizeof(float)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(MD_pro_base_b, &_pro_base_b, sizeof(float)) );

    //Parameters for DNACT book keeping
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(_ndna, &this->ndna, sizeof(int)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(_npro, &this->npro, sizeof(int)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(_offset, &this->offset, sizeof(int)) );

    //kb and kt Parameters
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(_kb, &_kbend, sizeof(float)) );
    CUDA_SAFE_CALL( cudaMemcpyToSymbol(_kt, &_ktor, sizeof(float)) );

    //Parameters for ANM
    CUDA_SAFE_CALL( cudaMemcpy(_d_spring_pottype, _h_spring_pottype, _spring_param_size_char, cudaMemcpyHostToDevice));
    CUDA_SAFE_CALL( cudaMemcpy(_d_spring_potential, _h_spring_potential, _spring_param_size_number, cudaMemcpyHostToDevice));
    CUDA_SAFE_CALL(cudaMemcpy(_d_spring_eqdist, _h_spring_eqdist, _spring_param_size_number, cudaMemcpyHostToDevice));
    CUDA_SAFE_CALL(cudaMemcpy(_d_ang_params, _h_ang_params, _ang_param_size, cudaMemcpyHostToDevice));
}

template<typename number, typename number4>
void CUDADNACTInteraction<number, number4>::compute_forces(CUDABaseList<number, number4> *lists, number4 *d_poss, GPU_quat<number> *d_orientations, number4 *d_forces, number4 *d_torques, LR_bonds *d_bonds, CUDABox<number, number4> *d_box) {
	CUDASimpleVerletList<number, number4> *_v_lists = dynamic_cast<CUDASimpleVerletList<number, number4> *>(lists);
	if(_v_lists != NULL) {
        if (_v_lists->use_edge()) {
            dnact_forces_edge_nonbonded<number, number4>
                    << < (_v_lists->_N_edges - 1) / (this->_launch_cfg.threads_per_block) + 1,
                    this->_launch_cfg.threads_per_block >> >
                    (d_poss, d_orientations, this->_d_edge_forces, this->_d_edge_torques, _v_lists->_d_edge_list, _v_lists->_N_edges, d_bonds, this->_grooving, _use_debye_huckel, _use_oxDNA2_coaxial_stacking, d_box);

            this->_sum_edge_forces_torques(d_forces, d_torques);

            // potential for removal here
            cudaThreadSynchronize();
            CUT_CHECK_ERROR("forces_second_step error -- after non-bonded");

            dnact_forces_edge_bonded<number, number4>
                    << < this->_launch_cfg.blocks, this->_launch_cfg.threads_per_block >> >
                   (d_poss, d_orientations, d_forces, d_torques, d_bonds, this->_grooving, _use_oxDNA2_FENE, this->_use_mbf, this->_mbf_xmax, this->_mbf_finf, d_box, _d_spring_eqdist, _d_spring_potential, _d_ang_params);

        } else throw oxDNAException("Edge Approach is only implemented for DNACT Interaction using CUDA approach. Please add use_edge = 1 to your input file.");

	} else throw oxDNAException("Must Use with Lists to run simulation");
}

template class CUDADNACTInteraction<float, float4>;
template class CUDADNACTInteraction<double, LR_double4>;