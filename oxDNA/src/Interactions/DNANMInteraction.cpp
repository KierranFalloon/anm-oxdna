/*
 * DNANMInteraction.cpp
 *
 *  Created on: Apr 17, 2019
 *      Author: jonah
 *  Inherits from the DNA2Interaction Class
 *  Uses DNA2 Model and ACProtein Model
 *  ACProtein Functions are implemented here as to avoid a multitude of multiple inheritance
 *  problems of which I am unsure are truly solvable
 */


#include "DNANMInteraction.h"
#include <sstream>
#include <fstream>

#include "../Particles/DNANucleotide.h"
#include "../Particles/ACParticle.h"

template<typename number>
DNANMInteraction<number>::DNANMInteraction() : DNA2Interaction<number>() { // @suppress("Class members should be properly initialized")
	//typedef std::map<int, number (child::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)> interaction_map;

	//Protein Methods Function Pointers
	this->_int_map[SPRING] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)) &DNANMInteraction<number>::_protein_spring;
	this->_int_map[PRO_EXC_VOL] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)) &DNANMInteraction<number>::_protein_exc_volume;

	//Protein-DNA Function Pointers
	this->_int_map[PRO_DNA_EXC_VOL] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)) &DNANMInteraction<number>::_protein_dna_exc_volume;

	//DNA Methods Function Pointers
	this->_int_map[this->BACKBONE] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)) &DNANMInteraction<number>::_dna_backbone;
	this->_int_map[this->COAXIAL_STACKING] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces))  &DNANMInteraction<number>::_dna_coaxial_stacking;
	this->_int_map[this->CROSS_STACKING] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces))  &DNANMInteraction<number>::_dna_cross_stacking;
	this->_int_map[this->BONDED_EXCLUDED_VOLUME] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces))  &DNANMInteraction<number>::_dna_bonded_excluded_volume;
	this->_int_map[this->STACKING] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces))  &DNANMInteraction<number>::_dna_stacking;
	this->_int_map[this->HYDROGEN_BONDING] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces))  &DNANMInteraction<number>::_dna_hydrogen_bonding;
	this->_int_map[this->NONBONDED_EXCLUDED_VOLUME] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces))  &DNANMInteraction<number>::_dna_nonbonded_excluded_volume;
	this->_int_map[this->DEBYE_HUCKEL] = (number (DNAInteraction<number>::*)(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces))  &DNANMInteraction<number>::_dna_debye_huckel;
}

template<typename number>
void DNANMInteraction<number>::get_settings(input_file &inp){
	this->DNA2Interaction<number>::get_settings(inp);
	getInputString(&inp, "PARFILE", _parameterfile, 0);
	//Addition of Reading Parameter File
    char s[5] = "none";
    if(strcmp(_parameterfile, s) != 0) {
        int key1, key2;
        char potswitch;
        double potential, dist;
        string carbons;
        fstream parameters;
        parameters.open(_parameterfile, ios::in);
        getline (parameters,carbons);
        if (parameters.is_open())
        {
            while (parameters >> key1 >> key2 >> dist >> potswitch >> potential)
            {
                pair <int, int> lkeys (key1, key2);
                pair <char, double> pot (potswitch, potential);
                _rknot[lkeys] = dist;
                _potential[lkeys] = pot;
            }
        }
        else
        {
            throw oxDNAException("ParameterFile Could Not Be Opened");
        }
        parameters.close();
    } else {
        OX_LOG(Logger::LOG_INFO, "Parfile: NONE, No protein parameters were filled");
    }
}

template<typename number>
void DNANMInteraction<number>::check_input_sanity(BaseParticle<number> **particles, int N){
	//this->DNA2Interaction<number>::check_input_sanity(particles,N);
	//Need to make own function that checks the input sanity
}


template<typename number>
void DNANMInteraction<number>::allocate_particles(BaseParticle<number> **particles, int N) {
	if (ndna==0 || ndnas==0) {
        OX_LOG(Logger::LOG_INFO, "No DNA Particles Specified, Continuing with just Protein Particles");
        for (int i = 0; i < npro; i++) particles[i] = new ACParticle<number>();
    } else if (npro == 0) {
        OX_LOG(Logger::LOG_INFO, "No Protein Particles Specified, Continuing with just DNA Particles");
        for (int i = 0; i < ndna; i++) particles[i] = new DNANucleotide<number>(this->_grooving);
	} else {
	    if (_firststrand > 0){
            for (int i = 0; i < ndna; i++) particles[i] = new DNANucleotide<number>(this->_grooving);
            for (int i = ndna; i < N; i++) particles[i] = new ACParticle<number>();
	    } else {
            for (int i = 0; i < npro; i++) particles[i] = new ACParticle<number>();
            for (int i = npro; i < N; i++) particles[i] = new DNANucleotide<number>(this->_grooving);
	    }
	}
}

template<typename number>
void DNANMInteraction<number>::read_topology(int N, int *N_strands, BaseParticle<number> **particles) {
    *N_strands = N;
    int my_N, my_N_strands;

    char line[5120];
    std::ifstream topology;
    topology.open(this->_topology_filename, ios::in);

    if (!topology.good())
        throw oxDNAException("Can't read topology file '%s'. Aborting",
                             this->_topology_filename);

    topology.getline(line, 5120);
    std::stringstream head(line);

    head >> my_N >> my_N_strands >> ndna >> npro >>ndnas;
    if (head.fail()) throw oxDNAException("Problem with header make sure the format is correct for DNANM Interaction");

    if(N < 0 || my_N_strands < 0 || my_N_strands > my_N || ndna > my_N || ndna < 0 || npro > my_N || npro < 0 || ndnas < 0 || ndnas > my_N) {
        throw oxDNAException("Problem with header make sure the format is correct for DNANM Interaction");
    }


    int strand, i = 0;
    while (topology.good()) {
        topology.getline(line, 5120);
        if (strlen(line) == 0 || line[0] == '#')
            continue;
        if (i == N)
            throw oxDNAException(
                    "Too many particles found in the topology file (should be %d), aborting",
                    N);

        std::stringstream ss(line);
        ss >> strand;
        if (i == 0) {
            _firststrand = strand; //Must be set prior to allocation of particles
            allocate_particles(particles, N);
            for (int j = 0; j < N; j++) {
                particles[j]->index = j;
                particles[j]->type = 0;
                particles[j]->strand_id = 0;
            }
        }

        // Amino Acid
        if (strand < 0) {
            char aminoacid[256];
            int nside, cside;
            ss >> aminoacid >> nside >> cside;

            int x;
            std::set<int> myneighs;
            if (nside >= 0) myneighs.insert(nside);
            if (cside >= 0) myneighs.insert(cside);
            while (ss.good()) {
                ss >> x;
                if (x < 0 || x >= N) {
                    throw oxDNAException("Line %d of the topology file has an invalid syntax, neighbor has invalid id",
                                         i + 2);
                }
                myneighs.insert(x);
            }

            ACParticle<number> *p = dynamic_cast< ACParticle<number> * > (particles[i]);

            if (strlen(aminoacid) == 1) {
                p->btype = Utils::decode_aa(aminoacid[0]);
            }

            p->strand_id = abs(strand) + ndnas - 1;
            p->index = i;

            for (std::set<int>::iterator k = myneighs.begin(); k != myneighs.end(); ++k) {
                if (p->index < *k) {
                    p->add_bonded_neighbor(dynamic_cast<ACParticle<number> *> (particles[*k]));
                }
            }

            i++;
        }
        if (strand > 0) {
            char base[256];
            int tmpn3, tmpn5;
            ss >> base >> tmpn3 >> tmpn5;

            DNANucleotide<number> *p = dynamic_cast<DNANucleotide<number> *> (particles[i]);

            if (tmpn3 < 0) p->n3 = P_VIRTUAL;
            else p->n3 = particles[tmpn3];
            if (tmpn5 < 0) p->n5 = P_VIRTUAL;
            else p->n5 = particles[tmpn5];


            p->strand_id = strand - 1;

            // the base can be either a char or an integer
            if (strlen(base) == 1) {
                p->type = Utils::decode_base(base[0]);
                p->btype = Utils::decode_base(base[0]);

            } else {
                if (atoi(base) > 0) p->type = atoi(base) % 4;
                else p->type = 3 - ((3 - atoi(base)) % 4);
                p->btype = atoi(base);
            }

            if (p->type == P_INVALID)
                throw oxDNAException("Particle #%d in strand #%d contains a non valid base '%c'. Aborting", i, strand,
                                     base);

            p->index = i;
//            printf("DNA %d %d %d \n", p->index, (p->n3)->index, (p->n5)->index); // (*p).n3->index, (*p).n5->index);
            i++;

            // here we fill the affected vector
            if (p->n3 != P_VIRTUAL) p->affected.push_back(ParticlePair<number>(p->n3, p));
            if (p->n5 != P_VIRTUAL) p->affected.push_back(ParticlePair<number>(p, p->n5));


            //Debug
//            typedef typename std::vector<ParticlePair<number> >::iterator iter;
//            iter it;
//            for (it = p->affected.begin(); it != p->affected.end(); ++it) {
//                printf("Pair %d %d \n", (*it).first->index, (*it).second->index);
//            }

        }
        if (strand == 0) throw oxDNAException("No strand 0 should be present please check topology file");

    }


    if (i < my_N)
        throw oxDNAException(
                "Not enough particles found in the topology file (should be %d). Aborting",
                my_N);

    topology.close();

    if (my_N != N)
        throw oxDNAException(
                "Number of lines in the configuration file and number of particles in the topology files don't match. Aborting");

    *N_strands = my_N_strands;

}


template<typename number>
number DNANMInteraction<number>::pair_interaction(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces){
    if (p->btype >= 0 && q->btype >=0){
        if(p->is_bonded(q)) return this->pair_interaction_bonded(p, q, r, update_forces);
        else return this->pair_interaction_nonbonded(p, q, r, update_forces);
    }
    if ((p->btype >= 0 && q->btype < 0) || (p->btype < 0 && q->btype >= 0)) return this->pair_interaction_nonbonded(p, q, r, update_forces);

    if (p->btype <0 && q->btype <0){
        ACParticle<number> *cp = dynamic_cast< ACParticle<number> * > (p);
        if ((*cp).ACParticle<number>::is_bonded(q)) return this->pair_interaction_bonded(p, q, r, update_forces);
        else return this->pair_interaction_nonbonded(p, q, r, update_forces);
    }
    return 0.f;
}

template<typename number>
number DNANMInteraction<number>::pair_interaction_bonded(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {

    LR_vector<number> computed_r(0, 0, 0);
    if(r == NULL) {
        if (q != P_VIRTUAL && p != P_VIRTUAL) {
            computed_r = this->_box->min_image(p->pos, q->pos);
            r = &computed_r;
        }
    }

    if (p->btype >= 0 && q->btype >=0){
        if(!this->_check_bonded_neighbour(&p, &q, r)) return (number) 0;
        number energy = _dna_backbone(p,q,r,update_forces);
        energy += _dna_bonded_excluded_volume(p,q,r,update_forces);
        energy += _dna_stacking(p,q,r,update_forces);
        return energy;
    }

    if ((p->btype >= 0 && q->btype <0) ||(p->btype <0 && q->btype >=0)) return 0.f;

    if ((p->btype <0 && q->btype <0)){
        ACParticle<number> *cp = dynamic_cast< ACParticle<number> * > (p);
        if ((*cp).ACParticle<number>::is_bonded(q)){
            number energy = _protein_spring(p,q,r,update_forces);
            energy += _protein_exc_volume(p,q,r,update_forces);
            return energy;
        } else{
            return 0.f;
        }
    }

    return 0.f;
}

template<typename number>
number DNANMInteraction<number>::pair_interaction_nonbonded(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {

    LR_vector<number> computed_r(0, 0, 0);
    if (r == NULL) {
        computed_r = this->_box->min_image(p->pos, q->pos);
        r = &computed_r;
    }

    if (p->btype >= 0 && q->btype >= 0) { //DNA-DNA Interaction
        if (r->norm() >= this->_sqr_rcut) return (number) 0.f;
        number energy = _dna_nonbonded_excluded_volume(p, q, r, update_forces);
        energy += _dna_hydrogen_bonding(p, q, r, update_forces);
        energy += _dna_cross_stacking(p, q, r, update_forces);
        energy += _dna_coaxial_stacking(p, q, r, update_forces);
        energy += _dna_debye_huckel(p, q, r, update_forces);
        return energy;
    }

    if ((p->btype >= 0 && q->btype < 0) || (p->btype < 0 && q->btype >= 0)) {
//        if((p->index == 703 && q->index == 482) || (p->index == 482 && q->index == 703)){
//            printf("r.x %.5f r.y %.5f r.z %.5f\n", r->x, r->y, r->z);
//            printf("p.x %.5f p.y %.5f p.z %.5f\n", p->pos.x, p->pos.y, p->pos.z);
//            printf("q.x %.5f q.y %.5f q.z %.5f\n", q->pos.x, q->pos.y, q->pos.z);
//        }
        number energy = _protein_dna_exc_volume(p, q, r, update_forces);
        return energy;
    }

    if (p->btype < 0 && q->btype < 0) {
        number energy = _protein_exc_volume(p, q, r, update_forces);
        return energy;
    }

    return 0.f;
}

template<typename number>
number DNANMInteraction<number>::_protein_dna_exc_volume(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces)
{
	 //printf("Calling crodwer DNA on %d %d \n",p->index,q->index);
	 BaseParticle<number> *protein;
	 BaseParticle<number> *nuc;

	 LR_vector<number> force(0, 0, 0);
     LR_vector<number> rcenter = *r;

	 if(p->btype >= 0 && q->btype < 0)
     {
		 //rcenter = -rcenter;
		 protein = q;
		 nuc = p;
	 }
	 else if (p->btype < 0 && q->btype >= 0 )
	 {
		 rcenter = -rcenter;
		 protein = p;
		 nuc = q;
	 }
	 else
	 {
		 return 0.f;
	 }

	 LR_vector<number> r_to_back = rcenter  - nuc->int_centers[DNANucleotide<number>::BACK];
	 LR_vector<number> r_to_base = rcenter  - nuc->int_centers[DNANucleotide<number>::BASE];

	 LR_vector<number> torquenuc(0,0,0);

	 number energy = this->_protein_dna_repulsive_lj(r_to_back, force, update_forces, _pro_backbone_sigma, _pro_backbone_b, _pro_backbone_rstar,_pro_backbone_rcut,_pro_backbone_stiffness);
	 //printf("back-pro %d %d %f\n",p->index,q->index,energy);
	 if (update_forces) {
		    torquenuc  -= nuc->int_centers[DNANucleotide<number>::BACK].cross(force);
	 		nuc->force -= force;
		 	protein->force += force;
	 }


	 energy += this->_protein_dna_repulsive_lj(r_to_base, force, update_forces, _pro_base_sigma, _pro_base_b, _pro_base_rstar, _pro_base_rcut, _pro_base_stiffness);
	 if(update_forces) {

		    torquenuc  -= nuc->int_centers[DNANucleotide<number>::BASE].cross(force);
		    nuc->torque += nuc->orientationT * torquenuc;

	 		nuc->force -= force;
	 		protein->force += force;
	 }


	 return energy;
}

template<typename number>
number DNANMInteraction<number>::_protein_dna_repulsive_lj(const LR_vector<number> &r, LR_vector<number> &force, bool update_forces, number &sigma, number &b, number &rstar, number &rcut, number &stiffness) {
	// this is a bit faster than calling r.norm()
	number rnorm = SQR(r.x) + SQR(r.y) + SQR(r.z);
	number energy = (number) 0;

	if(rnorm < SQR(rcut)) {
		if(rnorm > SQR(rstar)) {
			number rmod = sqrt(rnorm);
			number rrc = rmod - rcut;
			energy = stiffness * b * SQR(SQR(rrc));
			if(update_forces) force = -r * (4 * stiffness * b * CUB(rrc) / rmod);
		}
		else {
			number tmp = SQR(sigma) / rnorm;
			number lj_part = tmp * tmp * tmp;
			energy = 4 * stiffness * (SQR(lj_part) - lj_part);
			if(update_forces) force = -r * (24 * stiffness * (lj_part - 2*SQR(lj_part)) / rnorm);
		}
	}

	if(update_forces && energy == (number) 0) force.x = force.y = force.z = (number) 0;

	return energy;
}

template<typename number>
void DNANMInteraction<number>::init() {
	this->DNA2Interaction<number>::init();
    ndna=0, npro=0, ndnas =0;
	//OLD VERSIONS
    //Backbone-Protein Excluded Volume Parameters
//    _pro_backbone_sigma = 0.4085f;
//    _pro_backbone_rstar= 0.3585f;
//    _pro_backbone_b = 5883.8f;
//    _pro_backbone_rcut = 0.400561f;
//    _pro_backbone_stiffness = 1.0f;
//    //Base-Protein Excluded Volume Parameters
//    _pro_base_sigma = 0.2235f;
//    _pro_base_rstar= 0.1735f;
//    _pro_base_b = 101416.f;
//    _pro_base_rcut = 0.198864f;
//    _pro_base_stiffness = 1.0f;
//    //Protein-Protein Excluded Volume Parameters
//    _pro_sigma = 0.117f;
//    _pro_rstar= 0.087f;
//    _pro_b = 671492.f;
//    _pro_rcut = 0.100161f;
    //NEW_VERSION(Quartic LJ version)
//    _pro_backbone_sigma = 0.68f;
//    _pro_backbone_rstar= 0.679f;
//    _pro_backbone_b = 147802936.f;
//    _pro_backbone_rcut = 0.682945f;
//    _pro_backbone_stiffness = 1.0f;
//    //Base-Protein Excluded Volume Parameters
//    _pro_base_sigma = 0.47f;
//    _pro_base_rstar= 0.45f;
//    _pro_base_b = 157081.f;
//    _pro_base_rcut = 0.506028f;
//    _pro_base_stiffness = 1.0f;
//    //Protein-Protein Excluded Volume Parameters
//    _pro_sigma = 0.55f;
//    _pro_rstar= 0.47f;
//    _pro_b = 80892.1f;
//    _pro_rcut = 0.588787f;
    //that ain't it chief
    //let's try this
    _pro_backbone_sigma = 0.57f;
    _pro_backbone_rstar= 0.569f;
    _pro_backbone_b = 178699253.5f;
    _pro_backbone_rcut = 0.572934f;
    _pro_backbone_stiffness = 1.0f;
    //Base-Protein Excluded Volume Parameters
    _pro_base_sigma = 0.36f;
    _pro_base_rstar= 0.359f;
    _pro_base_b = 296866090.f;
    _pro_base_rcut = 0.362897f;
    _pro_base_stiffness = 1.0f;
    //Protein-Protein Excluded Volume Parameters
    _pro_sigma = 0.35f;
    _pro_rstar = 0.349f;
    _pro_b = 306484596.421f;
    _pro_rcut = 0.352894;

}

//Functions from ACInteraction.h
//Stolen due to inheritance issues

template<typename number>
number DNANMInteraction<number>::_protein_repulsive_lj(const LR_vector<number> &r, LR_vector<number> &force, bool update_forces) {
	// this is a bit faster than calling r.norm()
	//changed to a quartic form
	number rnorm = SQR(r.x) + SQR(r.y) + SQR(r.z);
	number energy = (number) 0;
	if(rnorm < SQR(_pro_rcut)) {
		if(rnorm > SQR(_pro_rstar)) {
			number rmod = sqrt(rnorm);
			number rrc = rmod - _pro_rcut;
			energy = EXCL_EPS * _pro_b * SQR(SQR(rrc));
			if(update_forces) force = -r * (4 * EXCL_EPS * _pro_b * CUB(rrc)/ rmod);
		}
		else {
			number tmp = SQR(_pro_sigma) / rnorm;
			number lj_part = tmp * tmp * tmp;
			energy = 4 * EXCL_EPS * (SQR(lj_part) - lj_part);
			if(update_forces) force = -r* (24 * EXCL_EPS * (lj_part - 2*SQR(lj_part))/rnorm);
		}
	}

	if(update_forces && energy == (number) 0) force.x = force.y = force.z = (number) 0;

	return energy;
}

template<typename number>
number DNANMInteraction<number>::_protein_exc_volume(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
	if (p->index != q->index && (p->btype < 0 && q-> btype < 0 )  ){
		LR_vector<number> force(0,0,0);

		number energy =  DNANMInteraction<number>::_protein_repulsive_lj(*r, force, update_forces);

		if(update_forces)
		{
			p->force -= force;
			q->force += force;
		}

		return energy;
	} else {
		return (number) 0.f;
	}
}

template<typename number>
number DNANMInteraction<number>::_protein_spring(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
    if(p->btype >= 0 || q->btype >= 0)    //this function is only for proteins
    {
        return 0.f;
    }
	number eqdist;
	char interactiontype;
	pair <int, int> keys (std::min(p->index, q->index), std::max(p->index, q->index));
    eqdist = _rknot[keys];
    interactiontype = _potential[keys].first;
    if (eqdist != 0.0) { //only returns number if eqdist is in .par file
        switch (interactiontype) {
            case 's': {
                //Harmonic Spring Potential
                //if ((eqdist < 0.f) || (eqdist > 3.f))  //ensures r0 is nonnegative
//                {
//                    if (keys.first + 1 != keys.second) {
//                        throw oxDNAException("No rknot or invalid rknot value for particle %d and %d rknot was %f",
//                                             q->index, p->index, eqdist);
//                    }
//                }
                number _k = _potential[keys].second; //stiffness of the spring
//                if ((_k == 0) || (_k < 0)) {
//                    throw oxDNAException(
//                            "No Spring Constant or invalid Spring Constant for particle %d and %d spring constant was %f",
//                            p->index, q->index, _k);
//                }
                number rnorm = r->norm();
                number rinsta = sqrt(rnorm);
                number energy = 0.5 * _k * SQR((rinsta - eqdist));

                if (update_forces) {
                    LR_vector<number> force(*r);
                    force *= (-1.0f * _k) * (rinsta - eqdist) / rinsta;
                    p->force -= force;
                    q->force += force;
//                    printf("p %d q %d d=%f ro=%f df.x=%.8f df.y=%.8f  df.z=%.8f p.x=%.8f p.y=%.8f p.z=%.8f q.x=%.8f "
//                           "q.y=%.8f q.z=%.8f\n", p->index, q->index, rinsta, eqdist, force.x, force.y, force.z, p->force.x,
//                           p->force.y, p->force.z, q->force.x, q->force.y, q->force.z);
                           //"%f, prefactor = %f, force = %f,%f,%f, ener=%f \n",p->index,q->index,rinsta,eqdist, rinsta-eqdist, (-1.0f * _k ) * (rinsta-eqdist)/rinsta, force.x,force.y,force.z,energy);
//                    printf("@@@: %f %f \n",rinsta,(-1.0f * _k ) * (rinsta-eqdist)/rinsta);
                }
                return energy;
            }
                break;
            case 'i': {
                //Every Possible Pair of Particles Needs to be Calculated
                number _k = _potential[keys].second; //stiffness of the spring
                if ((_k == 0) || (_k < 0)) {
                    throw oxDNAException(
                            "No Spring Constant or invalid Spring Constant for particle %d and %d spring constant was %f",
                            p->index, q->index, _k);
                }
                number rnorm = r->norm();
                number rinsta = sqrt(rnorm);
                number energy = 0.5 * _k * SQR(rinsta - eqdist) * (1 / eqdist);

                if (update_forces) {
                    LR_vector<number> force(*r);
                    force *= (-1.0f * _k) * ((rinsta - eqdist) / rinsta) * (1 / eqdist);
                    p->force -= force;
                    q->force += force;
                    //printf("@@@: particle %d and %d rinsta=%f , eqdist=%f, r-r0 = %f, prefactor = %f, force = %f,%f,%f, ener=%f \n",p->index,q->index,rinsta,eqdist, rinsta-eqdist, (-1.0f * _k ) * (rinsta-eqdist)/rinsta, force.x,force.y,force.z,energy);
                    //printf("@@@: %f %f \n",rinsta,(-1.0f * _k ) * (rinsta-eqdist)/rinsta);
                }
                return energy;
            }
                break;
            case 'e': {
                //Every Possible Pair of Particles Needs to be Calculated
                number _k = _potential[keys].second; //stiffness of the spring
                if ((_k == 0) || (_k < 0)) {
                    throw oxDNAException(
                            "No Spring Constant or invalid Spring Constant for particle %d and %d spring constant was %f",
                            p->index, q->index, _k);
                }
                number rnorm = r->norm();
                number rinsta = sqrt(rnorm);
                number energy = 0.5 * _k * SQR(rinsta - eqdist) * (1 / (eqdist * eqdist));

                if (update_forces) {
                    LR_vector<number> force(*r);
                    force *= (-1.0f * _k) * ((rinsta - eqdist) / rinsta) * (1 / (eqdist * eqdist));
                    p->force -= force;
                    q->force += force;
                    //printf("@@@: particle %d and %d rinsta=%f , eqdist=%f, r-r0 = %f, prefactor = %f, force = %f,%f,%f, ener=%f \n",p->index,q->index,rinsta,eqdist, rinsta-eqdist, (-1.0f * _k ) * (rinsta-eqdist)/rinsta, force.x,force.y,force.z,energy);
                    //printf("@@@: %f %f \n",rinsta,(-1.0f * _k ) * (rinsta-eqdist)/rinsta);
                }
                return energy;
            }
                break;
            default: {
                throw oxDNAException(
                        "Interaction type specified in .par file is Invalid, particles %d and %d, switch %c", p->index,
                        q->index, interactiontype);
            }
        }
    } else {
        return (number) 0.f; //returns 0 if no rknot value in parameter value aka they aren't bonded
    }
	//} else {
		//return (number) 0.f; //returns 0 if particle pair consists of particle and itself
	//}

}

template<typename number>
number DNANMInteraction<number>::_dna_backbone(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces){
    if(p->btype >= 0 && q->btype >=0){
        return DNA2Interaction<number>::_backbone(p,q,r,update_forces);
    } else return 0.f;
}

template<typename number>
number DNANMInteraction<number>::_dna_bonded_excluded_volume(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
    if(p->btype >= 0 && q->btype >=0){
        return DNAInteraction<number>::_bonded_excluded_volume(p,q,r,update_forces);
    } else return 0.f;
}

template<typename number>
number DNANMInteraction<number>::_dna_nonbonded_excluded_volume(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
    if(p->btype >= 0 && q->btype >=0){
        return DNA2Interaction<number>::_nonbonded_excluded_volume(p,q,r,update_forces);
    } else return 0.f;
}

template<typename number>
number DNANMInteraction<number>::_dna_stacking(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
    if(p->btype >= 0 && q->btype >=0){
        return DNA2Interaction<number>::_stacking(p,q,r,update_forces);
    } else return 0.f;
}

template<typename number>
number DNANMInteraction<number>::_dna_coaxial_stacking(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
    if(p->btype >= 0 && q->btype >=0){
        return DNA2Interaction<number>::_coaxial_stacking(p,q,r,update_forces);
    } else return 0.f;
}

template<typename number>
number DNANMInteraction<number>::_dna_hydrogen_bonding(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
    if(p->btype >= 0 && q->btype >=0){
        return DNA2Interaction<number>::_hydrogen_bonding(p,q,r,update_forces);
    } else return 0.f;
}

template<typename number>
number DNANMInteraction<number>::_dna_cross_stacking(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
    if(p->btype >= 0 && q->btype >=0){
        return DNA2Interaction<number>::_cross_stacking(p,q,r,update_forces);
    } else return 0.f;
}

template<typename number>
number DNANMInteraction<number>::_dna_debye_huckel(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
    if(p->btype >= 0 && q->btype >=0){
        return DNA2Interaction<number>::_debye_huckel(p,q,r,update_forces);
    } else return 0.f;
}


template<typename number>
DNANMInteraction<number>::~DNANMInteraction() {
}

template class DNANMInteraction<float>;
template class DNANMInteraction<double>;




