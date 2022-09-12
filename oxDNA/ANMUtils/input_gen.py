import sys
import os
from os import listdir

def oxdna_input_file(interaction_type, sim_type, top_file, dat_file, trap):
    
    assert sim_type == "MD" or sim_type == "MC", "Only MD and MC are available for oxDNA2"
    
    prog_params = str("""##############################
####  PROGRAM PARAMETERS  ####
###############################
backend = CPU
backend_precision = double
debug = 1
#seed = 10\n""").format(interaction_type)
    
    if sim_type == "MD":

        sim_params = str("""############################## 
####    SIM PARAMETERS    #### 
############################## 
steps = 10000
newtonian_steps = 103
diff_coeff = 2.50
#pt = 0.1
thermostat = john
T = 20C  
dt = 0.003
verlet_skin = 0.05\n""").format(sim_type)
        
    else:   
        sim_params = str("""############################## 
####    SIM PARAMETERS    #### 
############################## 
sim_type = {}
ensemble = NVT
steps = 10000
check_energy_every = 1e2
check_energy_threshold = 1.e-4

delta_translation = 0.10
delta_rotation = 0.2 
T = 23C
verlet_skin = 0.20\n""").format(sim_type)

    output = str("""\n##############################
####    INPUT / OUTPUT    ####
##############################
topology = {}
conf_file = {}
trajectory_file = trajectory.dat
no_stdout_energy = 0
restart_step_counter = 1
energy_file = energy.dat
print_conf_interval = 1e3
print_energy_every = 1e3
time_scale = linear
external_forces = {}
external_forces_file = {}\n""").format(top_file, dat_file, ext_forces_bool, trap)
        
    return prog_params, sim_params, output
    

def anm_input_file(interaction_type, sim_type, par_file, top_file, dat_file, trap): # type = AC or ACT, file = name of top, dat and par files

    prog_params = str("""##############################
####  PROGRAM PARAMETERS  ####
###############################
backend = CPU
backend_precision = double
debug = 1
interaction_type = {}
salt_concentration = 1.
max_io = 4\n""").format(interaction_type)
    
    sim_params = str("""############################## 
####    SIM PARAMETERS    #### 
############################## 
### Universal Params ### 
T = 300K 
verlet_skin = 0.20 
sim_type = {} 
steps = 10000\n""").format(sim_type)
    
    if interaction_type == "DNANM" or interaction_type == "DNACT":
        ext_forces_string = str("external_forces_file")
    else:
        ext_forces_string = str("external_forces")
        
    output = str("""\n##############################
####    INPUT / OUTPUT    ####
##############################
parfile = {}
topology = {}
conf_file = {}
trajectory_file = trajectory.dat
log_file = log.dat
no_stdout_energy = 0
restart_step_counter = 1
energy_file = energy.dat
print_conf_interval = 1000
print_energy_every = 1000
time_scale = linear
external_forces = {}
{} = {}\n""").format(par_file, top_file, dat_file, ext_forces_bool, ext_forces_string, trap)
    
    
    if sim_type == "MD":
        
        if interaction_type == "DNANM" or interaction_type == "DNACT":
            
            MD_params = str("""\n### MD PARAMS ###
thermostat = john
dt = 0.002
maxclust = 63
diff_coeff = 2.5
newtonian_steps = 103
refresh_vel = 1\n""")

            MC_params = str(""" """)
        
        else:
            MD_params = str("""\n### MD PARAMS ### 
dt = 0.002
newtonian_steps=51
diff_coeff=1
thermostat=john
refresh_vel=1\n""")

            MC_params = str(""" """)
    
    elif sim_type == "MC":
        MD_params = str(""" """)
        
        if interaction_type == "DNANM" or interaction_type == "DNACT":
            
            MC_params = str("""\n### MC PARAMS ###
delta_translation = 0.01
delta_rotation = 0.25
ensemble = NVT
thermostat = john
maxclust = 63
diff_coeff = 2.5
newtonian_steps = 103\n""")
        
        else:
            MC_params = str("""\n### MC PARAMS ###
delta_translation = 0.01
delta_rotation = 0.25
check_energy_every = 10
check_energy_threshold = 1.e-4
ensemble = NVT\n""")
    
    if interaction_type == "ACT" or interaction_type == "DNACT":
        
        interactions = str("""\n############################## 
####     INTERACTIONS     #### 
############################## 
bending_k = 50.0 
torsion_k = 50.0\n""")
        
    else:
        
        interactions = str("""\n##############################
####     INTERACTIONS     ####
##############################\n""")
        
    return prog_params, sim_params, MD_params, MC_params, interactions, output

if __name__ == "__main__":
    
    cwd = os.getcwd()
    interaction_type = str(sys.argv[1])
    sim_type = str(sys.argv[2])
    
    assert interaction_type == "oxDNA2" or interaction_type == "AC" or interaction_type == "ACT" or interaction_type == "DNANM" or interaction_type == "DNACT", "Please input valid type - AC for ANM, ACT for ANMT or DNANM / DNACT for hybrid"
    assert sim_type == "MC" or sim_type == "MD", "Please input a valid sim type"
    
    if interaction_type == "oxDNA2":
        
        top_file = [f for f in listdir(os.getcwd()) if f.endswith(".top")]
        if top_file == []:
            raise Exception("No suitable .top files found in {}".format(os.getcwd()))
        
        dat_file = [f for f in listdir(os.getcwd()) if f.endswith(".dat") 
                    and f != "log.dat" and f != "trajectory.dat" and f != "energy.dat" and f != "last_conf.dat" and f != ".dat"] # Standard .dats
        
        if dat_file == []:
            raise Exception("No suitable .dat files found in {}".format(os.getcwd()))
        
        if len(dat_file) > 1:
            raise Exception("More than one .dat file found:\n{}".format(dat_file))
        
        trap = [f for f in listdir(os.getcwd()) if f == "trap.txt"]

        with open('input_{}'.format(interaction_type), 'w') as writer:
        
            if trap == []:
                    print(("No trap.txt file found - assuming 0 external forces.".format(os.getcwd())))
                    ext_forces_bool = 0
                    writer.write(''.join(str(x) for x in oxdna_input_file(interaction_type, 
                                                                    sim_type, 
                                                                    *top_file, 
                                                                    *dat_file,
                                                                    0)))
                
            else:
                ext_forces_bool = 1
                writer.write(''.join(str(x) for x in oxdna_input_file(interaction_type, 
                                                    sim_type, 
                                                    *top_file, 
                                                    *dat_file,
                                                    *trap)))
        
        print("Wrote file input_{} to {}".format(interaction_type, cwd))
    
    else:
        par_file = [f for f in listdir(os.getcwd()) if f.endswith(".par")]
        
        top_file = [f for f in listdir(os.getcwd()) if f.endswith(".top")]
        if top_file == []:
            raise Exception("No suitable .top files found in {}".format(os.getcwd()))
        
        dat_file = [f for f in listdir(os.getcwd()) if f.endswith(".dat") 
                    and f != "log.dat" and f != "trajectory.dat" and f != "energy.dat" and f != "last_conf.dat" and f != ".dat"] # Standard .dats
        
        if dat_file == []:
            raise Exception("No suitable .dat files found in {}".format(os.getcwd()))
        
        if len(dat_file) > 1:
            raise Exception("More than one .dat file found:\n{}".format(dat_file))
            
        trap = [f for f in listdir(os.getcwd()) if f == "trap.txt"]

        with open('input_{}'.format(interaction_type), 'w') as writer:

            if par_file == [] and trap == []:
                print("No .par file found - assuming DNA")
                print(("No trap.txt file found - assuming 0 external forces."))
                ext_forces_bool = 0
                writer.write(''.join(str(x) for x in anm_input_file(interaction_type, 
                                                                sim_type, 
                                                                "none", 
                                                                *top_file,
                                                                *dat_file,
                                                                0)))
            elif par_file == []:
                print("No .par file found - assuming DNA")
                ext_forces_bool = 1
                writer.write(''.join(str(x) for x in anm_input_file(interaction_type, 
                                                                sim_type, 
                                                                "none", 
                                                                *top_file, 
                                                                *dat_file,
                                                                *trap)))
            elif trap == []:
                print(("No trap.txt file found - assuming 0 external forces.".format(os.getcwd())))
                ext_forces_bool = 0
                writer.write(''.join(str(x) for x in anm_input_file(interaction_type, 
                                                                sim_type, 
                                                                *par_file, 
                                                                *top_file, 
                                                                *dat_file,
                                                                0)))
            
            else:
                ext_forces_bool = 1
                writer.write(''.join(str(x) for x in anm_input_file(interaction_type, 
                                                    sim_type, 
                                                    *par_file, 
                                                    *top_file, 
                                                    *dat_file,
                                                    *trap)))
            
        print("Wrote file input_{} to {}".format(interaction_type, cwd))
