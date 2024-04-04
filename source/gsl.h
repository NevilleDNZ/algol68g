/*!
\file gsl.h
\brief contains gsl and calculus related definitions
**/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2005 J. Marcel van der Veer <algol68g@xs4all.nl>.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef A68G_GSL_H
#define A68G_GSL_H

extern GENIE_PROCEDURE genie_exp_complex;
extern GENIE_PROCEDURE genie_cos_complex;
extern GENIE_PROCEDURE genie_cosh_complex;
extern GENIE_PROCEDURE genie_arccos_complex;
extern GENIE_PROCEDURE genie_arccosh_complex;
extern GENIE_PROCEDURE genie_arccosh_real;
extern GENIE_PROCEDURE genie_arcsin_complex;
extern GENIE_PROCEDURE genie_arcsinh_complex;
extern GENIE_PROCEDURE genie_arcsinh_real;
extern GENIE_PROCEDURE genie_arctan_complex;
extern GENIE_PROCEDURE genie_arctanh_complex;
extern GENIE_PROCEDURE genie_arctanh_real;
extern GENIE_PROCEDURE genie_inverf_real;
extern GENIE_PROCEDURE genie_inverfc_real;
extern GENIE_PROCEDURE genie_ln_complex;
extern GENIE_PROCEDURE genie_sin_complex;
extern GENIE_PROCEDURE genie_sinh_complex;
extern GENIE_PROCEDURE genie_sqrt_complex;
extern GENIE_PROCEDURE genie_tan_complex;
extern GENIE_PROCEDURE genie_tanh_complex;

#ifdef HAVE_GSL
extern GENIE_PROCEDURE genie_cgs_speed_of_light;
extern GENIE_PROCEDURE genie_cgs_gravitational_constant;
extern GENIE_PROCEDURE genie_cgs_planck_constant_h;
extern GENIE_PROCEDURE genie_cgs_planck_constant_hbar;
extern GENIE_PROCEDURE genie_cgs_vacuum_permeability;
extern GENIE_PROCEDURE genie_cgs_astronomical_unit;
extern GENIE_PROCEDURE genie_cgs_light_year;
extern GENIE_PROCEDURE genie_cgs_parsec;
extern GENIE_PROCEDURE genie_cgs_grav_accel;
extern GENIE_PROCEDURE genie_cgs_electron_volt;
extern GENIE_PROCEDURE genie_cgs_mass_electron;
extern GENIE_PROCEDURE genie_cgs_mass_muon;
extern GENIE_PROCEDURE genie_cgs_mass_proton;
extern GENIE_PROCEDURE genie_cgs_mass_neutron;
extern GENIE_PROCEDURE genie_cgs_rydberg;
extern GENIE_PROCEDURE genie_cgs_boltzmann;
extern GENIE_PROCEDURE genie_cgs_bohr_magneton;
extern GENIE_PROCEDURE genie_cgs_nuclear_magneton;
extern GENIE_PROCEDURE genie_cgs_electron_magnetic_moment;
extern GENIE_PROCEDURE genie_cgs_proton_magnetic_moment;
extern GENIE_PROCEDURE genie_cgs_molar_gas;
extern GENIE_PROCEDURE genie_cgs_standard_gas_volume;
extern GENIE_PROCEDURE genie_cgs_minute;
extern GENIE_PROCEDURE genie_cgs_hour;
extern GENIE_PROCEDURE genie_cgs_day;
extern GENIE_PROCEDURE genie_cgs_week;
extern GENIE_PROCEDURE genie_cgs_inch;
extern GENIE_PROCEDURE genie_cgs_foot;
extern GENIE_PROCEDURE genie_cgs_yard;
extern GENIE_PROCEDURE genie_cgs_mile;
extern GENIE_PROCEDURE genie_cgs_nautical_mile;
extern GENIE_PROCEDURE genie_cgs_fathom;
extern GENIE_PROCEDURE genie_cgs_mil;
extern GENIE_PROCEDURE genie_cgs_point;
extern GENIE_PROCEDURE genie_cgs_texpoint;
extern GENIE_PROCEDURE genie_cgs_micron;
extern GENIE_PROCEDURE genie_cgs_angstrom;
extern GENIE_PROCEDURE genie_cgs_hectare;
extern GENIE_PROCEDURE genie_cgs_acre;
extern GENIE_PROCEDURE genie_cgs_barn;
extern GENIE_PROCEDURE genie_cgs_liter;
extern GENIE_PROCEDURE genie_cgs_us_gallon;
extern GENIE_PROCEDURE genie_cgs_quart;
extern GENIE_PROCEDURE genie_cgs_pint;
extern GENIE_PROCEDURE genie_cgs_cup;
extern GENIE_PROCEDURE genie_cgs_fluid_ounce;
extern GENIE_PROCEDURE genie_cgs_tablespoon;
extern GENIE_PROCEDURE genie_cgs_teaspoon;
extern GENIE_PROCEDURE genie_cgs_canadian_gallon;
extern GENIE_PROCEDURE genie_cgs_uk_gallon;
extern GENIE_PROCEDURE genie_cgs_miles_per_hour;
extern GENIE_PROCEDURE genie_cgs_kilometers_per_hour;
extern GENIE_PROCEDURE genie_cgs_knot;
extern GENIE_PROCEDURE genie_cgs_pound_mass;
extern GENIE_PROCEDURE genie_cgs_ounce_mass;
extern GENIE_PROCEDURE genie_cgs_ton;
extern GENIE_PROCEDURE genie_cgs_metric_ton;
extern GENIE_PROCEDURE genie_cgs_uk_ton;
extern GENIE_PROCEDURE genie_cgs_troy_ounce;
extern GENIE_PROCEDURE genie_cgs_carat;
extern GENIE_PROCEDURE genie_cgs_unified_atomic_mass;
extern GENIE_PROCEDURE genie_cgs_gram_force;
extern GENIE_PROCEDURE genie_cgs_pound_force;
extern GENIE_PROCEDURE genie_cgs_kilopound_force;
extern GENIE_PROCEDURE genie_cgs_poundal;
extern GENIE_PROCEDURE genie_cgs_calorie;
extern GENIE_PROCEDURE genie_cgs_btu;
extern GENIE_PROCEDURE genie_cgs_therm;
extern GENIE_PROCEDURE genie_cgs_horsepower;
extern GENIE_PROCEDURE genie_cgs_bar;
extern GENIE_PROCEDURE genie_cgs_std_atmosphere;
extern GENIE_PROCEDURE genie_cgs_torr;
extern GENIE_PROCEDURE genie_cgs_meter_of_mercury;
extern GENIE_PROCEDURE genie_cgs_inch_of_mercury;
extern GENIE_PROCEDURE genie_cgs_inch_of_water;
extern GENIE_PROCEDURE genie_cgs_psi;
extern GENIE_PROCEDURE genie_cgs_poise;
extern GENIE_PROCEDURE genie_cgs_stokes;
extern GENIE_PROCEDURE genie_cgs_faraday;
extern GENIE_PROCEDURE genie_cgs_electron_charge;
extern GENIE_PROCEDURE genie_cgs_gauss;
extern GENIE_PROCEDURE genie_cgs_stilb;
extern GENIE_PROCEDURE genie_cgs_lumen;
extern GENIE_PROCEDURE genie_cgs_lux;
extern GENIE_PROCEDURE genie_cgs_phot;
extern GENIE_PROCEDURE genie_cgs_footcandle;
extern GENIE_PROCEDURE genie_cgs_lambert;
extern GENIE_PROCEDURE genie_cgs_footlambert;
extern GENIE_PROCEDURE genie_cgs_curie;
extern GENIE_PROCEDURE genie_cgs_roentgen;
extern GENIE_PROCEDURE genie_cgs_rad;
extern GENIE_PROCEDURE genie_cgs_solar_mass;
extern GENIE_PROCEDURE genie_cgs_bohr_radius;
extern GENIE_PROCEDURE genie_cgs_vacuum_permittivity;
extern GENIE_PROCEDURE genie_cgs_newton;
extern GENIE_PROCEDURE genie_cgs_dyne;
extern GENIE_PROCEDURE genie_cgs_joule;
extern GENIE_PROCEDURE genie_cgs_erg;
extern GENIE_PROCEDURE genie_mks_speed_of_light;
extern GENIE_PROCEDURE genie_mks_gravitational_constant;
extern GENIE_PROCEDURE genie_mks_planck_constant_h;
extern GENIE_PROCEDURE genie_mks_planck_constant_hbar;
extern GENIE_PROCEDURE genie_mks_vacuum_permeability;
extern GENIE_PROCEDURE genie_mks_astronomical_unit;
extern GENIE_PROCEDURE genie_mks_light_year;
extern GENIE_PROCEDURE genie_mks_parsec;
extern GENIE_PROCEDURE genie_mks_grav_accel;
extern GENIE_PROCEDURE genie_mks_electron_volt;
extern GENIE_PROCEDURE genie_mks_mass_electron;
extern GENIE_PROCEDURE genie_mks_mass_muon;
extern GENIE_PROCEDURE genie_mks_mass_proton;
extern GENIE_PROCEDURE genie_mks_mass_neutron;
extern GENIE_PROCEDURE genie_mks_rydberg;
extern GENIE_PROCEDURE genie_mks_boltzmann;
extern GENIE_PROCEDURE genie_mks_bohr_magneton;
extern GENIE_PROCEDURE genie_mks_nuclear_magneton;
extern GENIE_PROCEDURE genie_mks_electron_magnetic_moment;
extern GENIE_PROCEDURE genie_mks_proton_magnetic_moment;
extern GENIE_PROCEDURE genie_mks_molar_gas;
extern GENIE_PROCEDURE genie_mks_standard_gas_volume;
extern GENIE_PROCEDURE genie_mks_minute;
extern GENIE_PROCEDURE genie_mks_hour;
extern GENIE_PROCEDURE genie_mks_day;
extern GENIE_PROCEDURE genie_mks_week;
extern GENIE_PROCEDURE genie_mks_inch;
extern GENIE_PROCEDURE genie_mks_foot;
extern GENIE_PROCEDURE genie_mks_yard;
extern GENIE_PROCEDURE genie_mks_mile;
extern GENIE_PROCEDURE genie_mks_nautical_mile;
extern GENIE_PROCEDURE genie_mks_fathom;
extern GENIE_PROCEDURE genie_mks_mil;
extern GENIE_PROCEDURE genie_mks_point;
extern GENIE_PROCEDURE genie_mks_texpoint;
extern GENIE_PROCEDURE genie_mks_micron;
extern GENIE_PROCEDURE genie_mks_angstrom;
extern GENIE_PROCEDURE genie_mks_hectare;
extern GENIE_PROCEDURE genie_mks_acre;
extern GENIE_PROCEDURE genie_mks_barn;
extern GENIE_PROCEDURE genie_mks_liter;
extern GENIE_PROCEDURE genie_mks_us_gallon;
extern GENIE_PROCEDURE genie_mks_quart;
extern GENIE_PROCEDURE genie_mks_pint;
extern GENIE_PROCEDURE genie_mks_cup;
extern GENIE_PROCEDURE genie_mks_fluid_ounce;
extern GENIE_PROCEDURE genie_mks_tablespoon;
extern GENIE_PROCEDURE genie_mks_teaspoon;
extern GENIE_PROCEDURE genie_mks_canadian_gallon;
extern GENIE_PROCEDURE genie_mks_uk_gallon;
extern GENIE_PROCEDURE genie_mks_miles_per_hour;
extern GENIE_PROCEDURE genie_mks_kilometers_per_hour;
extern GENIE_PROCEDURE genie_mks_knot;
extern GENIE_PROCEDURE genie_mks_pound_mass;
extern GENIE_PROCEDURE genie_mks_ounce_mass;
extern GENIE_PROCEDURE genie_mks_ton;
extern GENIE_PROCEDURE genie_mks_metric_ton;
extern GENIE_PROCEDURE genie_mks_uk_ton;
extern GENIE_PROCEDURE genie_mks_troy_ounce;
extern GENIE_PROCEDURE genie_mks_carat;
extern GENIE_PROCEDURE genie_mks_unified_atomic_mass;
extern GENIE_PROCEDURE genie_mks_gram_force;
extern GENIE_PROCEDURE genie_mks_pound_force;
extern GENIE_PROCEDURE genie_mks_kilopound_force;
extern GENIE_PROCEDURE genie_mks_poundal;
extern GENIE_PROCEDURE genie_mks_calorie;
extern GENIE_PROCEDURE genie_mks_btu;
extern GENIE_PROCEDURE genie_mks_therm;
extern GENIE_PROCEDURE genie_mks_horsepower;
extern GENIE_PROCEDURE genie_mks_bar;
extern GENIE_PROCEDURE genie_mks_std_atmosphere;
extern GENIE_PROCEDURE genie_mks_torr;
extern GENIE_PROCEDURE genie_mks_meter_of_mercury;
extern GENIE_PROCEDURE genie_mks_inch_of_mercury;
extern GENIE_PROCEDURE genie_mks_inch_of_water;
extern GENIE_PROCEDURE genie_mks_psi;
extern GENIE_PROCEDURE genie_mks_poise;
extern GENIE_PROCEDURE genie_mks_stokes;
extern GENIE_PROCEDURE genie_mks_faraday;
extern GENIE_PROCEDURE genie_mks_electron_charge;
extern GENIE_PROCEDURE genie_mks_gauss;
extern GENIE_PROCEDURE genie_mks_stilb;
extern GENIE_PROCEDURE genie_mks_lumen;
extern GENIE_PROCEDURE genie_mks_lux;
extern GENIE_PROCEDURE genie_mks_phot;
extern GENIE_PROCEDURE genie_mks_footcandle;
extern GENIE_PROCEDURE genie_mks_lambert;
extern GENIE_PROCEDURE genie_mks_footlambert;
extern GENIE_PROCEDURE genie_mks_curie;
extern GENIE_PROCEDURE genie_mks_roentgen;
extern GENIE_PROCEDURE genie_mks_rad;
extern GENIE_PROCEDURE genie_mks_solar_mass;
extern GENIE_PROCEDURE genie_mks_bohr_radius;
extern GENIE_PROCEDURE genie_mks_vacuum_permittivity;
extern GENIE_PROCEDURE genie_mks_newton;
extern GENIE_PROCEDURE genie_mks_dyne;
extern GENIE_PROCEDURE genie_mks_joule;
extern GENIE_PROCEDURE genie_mks_erg;
extern GENIE_PROCEDURE genie_num_fine_structure;
extern GENIE_PROCEDURE genie_num_avogadro;
extern GENIE_PROCEDURE genie_num_yotta;
extern GENIE_PROCEDURE genie_num_zetta;
extern GENIE_PROCEDURE genie_num_exa;
extern GENIE_PROCEDURE genie_num_peta;
extern GENIE_PROCEDURE genie_num_tera;
extern GENIE_PROCEDURE genie_num_giga;
extern GENIE_PROCEDURE genie_num_mega;
extern GENIE_PROCEDURE genie_num_kilo;
extern GENIE_PROCEDURE genie_num_milli;
extern GENIE_PROCEDURE genie_num_micro;
extern GENIE_PROCEDURE genie_num_nano;
extern GENIE_PROCEDURE genie_num_pico;
extern GENIE_PROCEDURE genie_num_femto;
extern GENIE_PROCEDURE genie_num_atto;
extern GENIE_PROCEDURE genie_num_zepto;
extern GENIE_PROCEDURE genie_num_yocto;
extern GENIE_PROCEDURE genie_airy_ai_deriv_real;
extern GENIE_PROCEDURE genie_airy_ai_real;
extern GENIE_PROCEDURE genie_airy_bi_deriv_real;
extern GENIE_PROCEDURE genie_airy_bi_real;
extern GENIE_PROCEDURE genie_bessel_exp_il_real;
extern GENIE_PROCEDURE genie_bessel_exp_in_real;
extern GENIE_PROCEDURE genie_bessel_exp_inu_real;
extern GENIE_PROCEDURE genie_bessel_exp_kl_real;
extern GENIE_PROCEDURE genie_bessel_exp_kn_real;
extern GENIE_PROCEDURE genie_bessel_exp_knu_real;
extern GENIE_PROCEDURE genie_bessel_in_real;
extern GENIE_PROCEDURE genie_bessel_inu_real;
extern GENIE_PROCEDURE genie_bessel_jl_real;
extern GENIE_PROCEDURE genie_bessel_jn_real;
extern GENIE_PROCEDURE genie_bessel_jnu_real;
extern GENIE_PROCEDURE genie_bessel_kn_real;
extern GENIE_PROCEDURE genie_bessel_knu_real;
extern GENIE_PROCEDURE genie_bessel_yl_real;
extern GENIE_PROCEDURE genie_bessel_yn_real;
extern GENIE_PROCEDURE genie_bessel_ynu_real;
extern GENIE_PROCEDURE genie_beta_inc_real;
extern GENIE_PROCEDURE genie_beta_real;
extern GENIE_PROCEDURE genie_elliptic_integral_e_real;
extern GENIE_PROCEDURE genie_elliptic_integral_k_real;
extern GENIE_PROCEDURE genie_elliptic_integral_rc_real;
extern GENIE_PROCEDURE genie_elliptic_integral_rd_real;
extern GENIE_PROCEDURE genie_elliptic_integral_rf_real;
extern GENIE_PROCEDURE genie_elliptic_integral_rj_real;
extern GENIE_PROCEDURE genie_factorial_real;
extern GENIE_PROCEDURE genie_gamma_inc_real;
extern GENIE_PROCEDURE genie_gamma_real;
extern GENIE_PROCEDURE genie_lngamma_real;
#endif

extern GENIE_PROCEDURE genie_erf_real;
extern GENIE_PROCEDURE genie_erfc_real;
extern GENIE_PROCEDURE genie_cosh_real;
extern GENIE_PROCEDURE genie_sinh_real;
extern GENIE_PROCEDURE genie_tanh_real;

extern void calc_rte (NODE_T *, BOOL_T, MOID_T *, const char *);
extern double curt (double);

#endif
