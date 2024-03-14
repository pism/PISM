#!/bin/bash

# This PISM runs have been conducted in the CalvingMIP experiments phase 1 (EXP1-5) by torsten.albrecht@pik-potsdam.de
# https://github.com/JRowanJordan/CalvingMIP/wiki/Experiments---Phase-1

# mkdir results

# EXP1 5km
pismr -i circular_input.nc -bootstrap -Mx 321 -My 321 -Mz 3 -Mbz 1 -Lz 1600 -Lbz 0 -bootstrapping.defaults.geothermal_flux 0.0 -ssa_method fd -stress_balance ssa -ssa_dirichlet_bc -stress_balance.ssa.Glen_exponent 3.0 -stress_balance.ssa.compute_surface_gradient_inward no -stress_balance.ssa.flow_law isothermal_glen -ssafd_ksp_rtol 1e-7 -ocean.sub_shelf_heat_flux_into_ice 0.0 -geometry.update.use_basal_melt_rate no -energy.enabled no -energy.temperature_based -flow_law.isothermal_Glen.ice_softness 9.309208381069816e-26 -basal_resistance.pseudo_plastic.enabled -basal_resistance.pseudo_plastic.q 3.333333e-01 -basal_resistance.pseudo_plastic.u_threshold 3.1556926e+07 -basal_yield_stress.constant.value 3160081.114726032 -basal_yield_stress.model constant -hydrology null -calving calvingmip_calving -calvingmip_experiment 1 -max_dt 1.0 -calve_along_flow_direction -front_retreat_wrap_around -front_retreat_cfl -cfbc -part_grid -extra_times 10 -extra_vars ismip6,mask,velbase_mag,calvingmip_calving_rate,ice_area_specific_volume -output.ISMIP6 -extra_file results/extra_exp1.nc -no_extra_force_output_times -ts_file results/ts_exp1.nc -ts_times 100000:yearly:110000 -constants.ice.density 917.0 -constants.sea_water.density 1028.0 -constants.standard_gravity 9.81 -o_order zyx -output.sizes.medium mask -options_left -verbose 3 -ys 100000 -ye 110000 -o results/result_exp1.nc

# EXP2 5km
pismr -i result_exp1.nc -ssa_method fd -stress_balance ssa -ssa_dirichlet_bc -stress_balance.ssa.Glen_exponent 3.0 -stress_balance.ssa.compute_surface_gradient_inward no -stress_balance.ssa.flow_law isothermal_glen -ssafd_ksp_rtol 1e-7 -ocean.sub_shelf_heat_flux_into_ice 0.0 -geometry.update.use_basal_melt_rate no -energy.enabled no -energy.temperature_based -flow_law.isothermal_Glen.ice_softness 9.309208381069816e-26 -basal_resistance.pseudo_plastic.enabled -basal_resistance.pseudo_plastic.q 3.333333e-01 -basal_resistance.pseudo_plastic.u_threshold 3.1556926e+07 -basal_yield_stress.constant.value 3160081.114726032 -basal_yield_stress.model constant -hydrology null -calving calvingmip_calving -calvingmip_experiment 2 -max_dt 1.0 -calve_along_flow_direction -front_retreat_wrap_around -front_retreat_cfl -cfbc -part_grid -extra_times 1 -extra_vars ismip6,mask,velbase_mag,calvingmip_calving_rate,ice_area_specific_volume -output.ISMIP6 -extra_file results/extra_exp2.nc -no_extra_force_output_times -ts_file results/ts_exp2.nc -ts_times 110000:yearly:111000 -constants.ice.density 917.0 -constants.sea_water.density 1028.0 -constants.standard_gravity 9.81 -o_order zyx -output.sizes.medium mask -options_left -verbose 3 -ys 110000 -ye 111000 -o results/result_exp2.nc

#EXP3 5km
pismr -i thule_input.nc -bootstrap -Mx 401 -My 401 -Mz 3 -Mbz 1 -Lz 2500 -Lbz 0 -bootstrapping.defaults.geothermal_flux 0.0 -ssa_method fd -stress_balance ssa -ssa_dirichlet_bc -stress_balance.ssa.Glen_exponent 3.0 -stress_balance.ssa.compute_surface_gradient_inward no -stress_balance.ssa.flow_law isothermal_glen -ssafd_ksp_rtol 1e-7 -ocean.sub_shelf_heat_flux_into_ice 0.0 -geometry.update.use_basal_melt_rate no -energy.enabled no -energy.temperature_based -flow_law.isothermal_Glen.ice_softness 9.309208381069816e-26 -basal_resistance.pseudo_plastic.enabled -basal_resistance.pseudo_plastic.q 3.333333e-01 -basal_resistance.pseudo_plastic.u_threshold 3.1556926e+07 -basal_yield_stress.constant.value 3160081.114726032 -basal_yield_stress.model constant -hydrology null -calving calvingmip_calving -calvingmip_experiment 3 -max_dt 1.0 -calve_along_flow_direction -front_retreat_wrap_around -front_retreat_cfl -cfbc -part_grid -extra_times 10 -extra_vars ismip6,mask,velbase_mag,calvingmip_calving_rate,ice_area_specific_volume -output.ISMIP6 -extra_file results/extra_exp3.nc -no_extra_force_output_times -ts_file results/ts_exp3.nc -ts_times 100000:yearly:110000 -constants.ice.density 917.0 -constants.sea_water.density 1028.0 -constants.standard_gravity 9.81 -o_order zyx -output.sizes.medium mask -options_left -verbose 3 -ys 100000 -ye 110000 -o results/result_exp3.nc

#EXP4 5km
pismr -i result_exp3.nc -ssa_method fd -stress_balance ssa -ssa_dirichlet_bc -stress_balance.ssa.Glen_exponent 3.0 -stress_balance.ssa.compute_surface_gradient_inward no -stress_balance.ssa.flow_law isothermal_glen -ssafd_ksp_rtol 1e-7 -ocean.sub_shelf_heat_flux_into_ice 0.0 -geometry.update.use_basal_melt_rate no -energy.enabled no -energy.temperature_based -flow_law.isothermal_Glen.ice_softness 9.309208381069816e-26 -basal_resistance.pseudo_plastic.enabled -basal_resistance.pseudo_plastic.q 3.333333e-01 -basal_resistance.pseudo_plastic.u_threshold 3.1556926e+07 -basal_yield_stress.constant.value 3160081.114726032 -basal_yield_stress.model constant -hydrology null -calving calvingmip_calving -calvingmip_experiment 4 -max_dt 1.0 -calve_along_flow_direction -front_retreat_wrap_around -front_retreat_cfl -cfbc -part_grid -extra_times 1 -extra_vars ismip6,mask,velbase_mag,calvingmip_calving_rate,ice_area_specific_volume -output.ISMIP6 -extra_file results/extra_exp4.nc -no_extra_force_output_times -ts_file results/ts_exp4.nc -ts_times 110000:yearly:111000 -constants.ice.density 917.0 -constants.sea_water.density 1028.0 -constants.standard_gravity 9.81 -o_order zyx -output.sizes.medium mask -options_left -verbose 3 -ys 110000 -ye 111000 -o results/result_exp4.nc

#EXP5 5km
pismr -i result_exp3.nc -ssa_method fd -stress_balance ssa -ssa_dirichlet_bc -stress_balance.ssa.Glen_exponent 3.0 -stress_balance.ssa.compute_surface_gradient_inward no -stress_balance.ssa.flow_law isothermal_glen -ssafd_ksp_rtol 1e-7 -ocean.sub_shelf_heat_flux_into_ice 0.0 -geometry.update.use_basal_melt_rate no -energy.enabled no -energy.temperature_based -flow_law.isothermal_Glen.ice_softness 9.309208381069816e-26 -basal_resistance.pseudo_plastic.enabled -basal_resistance.pseudo_plastic.q 3.333333e-01 -basal_resistance.pseudo_plastic.u_threshold 3.1556926e+07 -basal_yield_stress.constant.value 3160081.114726032 -basal_yield_stress.model constant -hydrology null -calving calvingmip_calving -calvingmip_experiment 5 -exp5_calving_threshold 275.0 -max_dt 1.0 -calve_along_flow_direction -front_retreat_wrap_around -front_retreat_cfl -cfbc -part_grid -extra_times 10 -extra_vars ismip6,mask,velbase_mag,calvingmip_calving_rate,ice_area_specific_volume -output.ISMIP6 -extra_file results/extra_exp5.nc -no_extra_force_output_times -ts_file results/ts_exp5.nc -ts_times 110000:yearly:120000 -constants.ice.density 917.0 -constants.sea_water.density 1028.0 -constants.standard_gravity 9.81 -o_order zyx -output.sizes.medium mask -options_left -verbose 3 -ys 110000 -ye 120000 -o results/result_exp5.nc
