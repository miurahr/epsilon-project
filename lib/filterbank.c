/*
 * $Id: filterbank.c,v 1.15 2010/03/19 22:57:29 simakov Exp $
 *
 * EPSILON - wavelet image compression library.
 * Copyright (C) 2006,2007,2010 Alexander Simakov, <xander@entropyware.info>
 *
 * This file is part of EPSILON
 *
 * EPSILON is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EPSILON is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with EPSILON.  If not, see <http://www.gnu.org/licenses/>.
 *
 * http://epsilon-project.sourceforge.net
 */

#include <common.h>
#include <filterbank.h>

/* Haar filter. */

static coeff_t haar_lowpass_analysis_coeffs[] = {
    0.7071067811865475,
    0.7071067811865475,
};

static coeff_t haar_highpass_analysis_coeffs[] = {
    0.7071067811865475,
   -0.7071067811865475,
};

static coeff_t haar_lowpass_synthesis_coeffs[] = {
    0.7071067811865475,
    0.7071067811865475,
};

static coeff_t haar_highpass_synthesis_coeffs[] = {
   -0.7071067811865475,
    0.7071067811865475,
};

static filter_t haar_lowpass_analysis = {
    2,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    haar_lowpass_analysis_coeffs,
};

static filter_t haar_highpass_analysis = {
    2,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    haar_highpass_analysis_coeffs,
};

static filter_t haar_lowpass_synthesis = {
    2,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    haar_lowpass_synthesis_coeffs,
};

static filter_t haar_highpass_synthesis = {
    2,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    haar_highpass_synthesis_coeffs,
};

static filterbank_t haar = {
   "haar",
   "Haar",
    ORTHOGONAL,
    &haar_lowpass_analysis,
    &haar_highpass_analysis,
    &haar_lowpass_synthesis,
    &haar_highpass_synthesis,
};

/* I. Daubechies, "Orthonormal Bases of Compactly Supported Wavelets,"
 * Communications on Pure and Applied Mathematics, vol. 41,
 * pp. 909-996, 1988. */

static coeff_t daub4_lowpass_analysis_coeffs[] = {
    0.4829629131445341,
    0.8365163037378077,
    0.2241438680420134,
   -0.1294095225512603,
};

static coeff_t daub4_highpass_analysis_coeffs[] = {
   -0.1294095225512603,
   -0.2241438680420134,
    0.8365163037378077,
   -0.4829629131445341,
};

static coeff_t daub4_lowpass_synthesis_coeffs[] = {
   -0.1294095225512603,
    0.2241438680420134,
    0.8365163037378077,
    0.4829629131445341,
};

static coeff_t daub4_highpass_synthesis_coeffs[] = {
   -0.4829629131445341,
    0.8365163037378077,
   -0.2241438680420134,
   -0.1294095225512603,
};

static filter_t daub4_lowpass_analysis = {
    4,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    daub4_lowpass_analysis_coeffs,
};

static filter_t daub4_highpass_analysis = {
    4,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    daub4_highpass_analysis_coeffs,
};

static filter_t daub4_lowpass_synthesis = {
    4,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    daub4_lowpass_synthesis_coeffs,
};

static filter_t daub4_highpass_synthesis = {
    4,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    daub4_highpass_synthesis_coeffs,
};

static filterbank_t daub4 = {
   "daub4",
   "Daubechies D4",
    ORTHOGONAL,
    &daub4_lowpass_analysis,
    &daub4_highpass_analysis,
    &daub4_lowpass_synthesis,
    &daub4_highpass_synthesis,
};

/* I. Daubechies, "Orthonormal Bases of Compactly Supported Wavelets,"
 * Communications on Pure and Applied Mathematics, vol. 41,
 * pp. 909-996, 1988. */

static coeff_t daub6_lowpass_analysis_coeffs[] = {
    0.3326705529500825,
    0.8068915093110924,
    0.4598775021184914,
   -0.1350110200102546,
   -0.0854412738820267,
    0.0352262918857095,
};

static coeff_t daub6_highpass_analysis_coeffs[] = {
    0.0352262918857095,
    0.0854412738820267,
   -0.1350110200102546,
   -0.4598775021184914,
    0.8068915093110924,
   -0.3326705529500825,
};

static coeff_t daub6_lowpass_synthesis_coeffs[] = {
    0.0352262918857095,
   -0.0854412738820267,
   -0.1350110200102546,
    0.4598775021184914,
    0.8068915093110924,
    0.3326705529500825,
};

static coeff_t daub6_highpass_synthesis_coeffs[] = {
   -0.3326705529500825,
    0.8068915093110924,
   -0.4598775021184914,
   -0.1350110200102546,
    0.0854412738820267,
    0.0352262918857095,
};

static filter_t daub6_lowpass_analysis = {
    6,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    daub6_lowpass_analysis_coeffs,
};

static filter_t daub6_highpass_analysis = {
    6,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    daub6_highpass_analysis_coeffs,
};

static filter_t daub6_lowpass_synthesis = {
    6,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    daub6_lowpass_synthesis_coeffs,
};

static filter_t daub6_highpass_synthesis = {
    6,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    daub6_highpass_synthesis_coeffs,
};

static filterbank_t daub6 = {
   "daub6",
   "Daubechies D6",
    ORTHOGONAL,
    &daub6_lowpass_analysis,
    &daub6_highpass_analysis,
    &daub6_lowpass_synthesis,
    &daub6_highpass_synthesis,
};

/* I. Daubechies, "Orthonormal Bases of Compactly Supported Wavelets,"
 * Communications on Pure and Applied Mathematics, vol. 41,
 * pp. 909-996, 1988. */

static coeff_t daub8_lowpass_analysis_coeffs[] = {
    0.2303778133088964,
    0.7148465705529154,
    0.6308807679398587,
   -0.0279837694168599,
   -0.1870348117190931,
    0.0308413818355607,
    0.0328830116668852,
   -0.0105974017850690,
};

static coeff_t daub8_highpass_analysis_coeffs[] = {
   -0.0105974017850690,
   -0.0328830116668852,
    0.0308413818355607,
    0.1870348117190931,
   -0.0279837694168599,
   -0.6308807679398587,
    0.7148465705529154,
   -0.2303778133088964,
};

static coeff_t daub8_lowpass_synthesis_coeffs[] = {
   -0.0105974017850690,
    0.0328830116668852,
    0.0308413818355607,
   -0.1870348117190931,
   -0.0279837694168599,
    0.6308807679398587,
    0.7148465705529154,
    0.2303778133088964,
};

static coeff_t daub8_highpass_synthesis_coeffs[] = {
   -0.2303778133088964,
    0.7148465705529154,
   -0.6308807679398587,
   -0.0279837694168599,
    0.1870348117190931,
    0.0308413818355607,
   -0.0328830116668852,
   -0.0105974017850690,
};

static filter_t daub8_lowpass_analysis = {
    8,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    daub8_lowpass_analysis_coeffs,
};

static filter_t daub8_highpass_analysis = {
    8,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    daub8_highpass_analysis_coeffs,
};

static filter_t daub8_lowpass_synthesis = {
    8,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    daub8_lowpass_synthesis_coeffs,
};

static filter_t daub8_highpass_synthesis = {
    8,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    daub8_highpass_synthesis_coeffs,
};

static filterbank_t daub8 = {
   "daub8",
   "Daubechies D8",
    ORTHOGONAL,
    &daub8_lowpass_analysis,
    &daub8_highpass_analysis,
    &daub8_lowpass_synthesis,
    &daub8_highpass_synthesis,
};

/* I. Daubechies, "Orthonormal Bases of Compactly Supported Wavelets,"
 * Communications on Pure and Applied Mathematics, vol. 41,
 * pp. 909-996, 1988. */

static coeff_t daub10_lowpass_analysis_coeffs[] = {
    0.16010239797419,
    0.60382926979719,
    0.72430852843777,
    0.13842814590132,
   -0.24229488706638,
   -0.03224486958464,
    0.07757149384005,
   -0.00624149021280,
   -0.01258075199908,
    0.00333572528547,
};

static coeff_t daub10_highpass_analysis_coeffs[] = {
    0.00333572528547,
    0.01258075199908,
   -0.00624149021280,
   -0.07757149384005,
   -0.03224486958464,
    0.24229488706638,
    0.13842814590132,
   -0.72430852843777,
    0.60382926979719,
   -0.16010239797419,
};

static coeff_t daub10_lowpass_synthesis_coeffs[] = {
    0.00333572528547,
   -0.01258075199908,
   -0.00624149021280,
    0.07757149384005,
   -0.03224486958464,
   -0.24229488706638,
    0.13842814590132,
    0.72430852843777,
    0.60382926979719,
    0.16010239797419,
};

static coeff_t daub10_highpass_synthesis_coeffs[] = {
   -0.16010239797419,
    0.60382926979719,
   -0.72430852843777,
    0.13842814590132,
    0.24229488706638,
   -0.03224486958464,
   -0.07757149384005,
   -0.00624149021280,
    0.01258075199908,
    0.00333572528547,
};

static filter_t daub10_lowpass_analysis = {
    10,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    daub10_lowpass_analysis_coeffs,
};

static filter_t daub10_highpass_analysis = {
    10,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    daub10_highpass_analysis_coeffs,
};

static filter_t daub10_lowpass_synthesis = {
    10,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    daub10_lowpass_synthesis_coeffs,
};

static filter_t daub10_highpass_synthesis = {
    10,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    daub10_highpass_synthesis_coeffs,
};

static filterbank_t daub10 = {
   "daub10",
   "Daubechies D10",
    ORTHOGONAL,
    &daub10_lowpass_analysis,
    &daub10_highpass_analysis,
    &daub10_lowpass_synthesis,
    &daub10_highpass_synthesis,
};

/* I. Daubechies, "Orthonormal Bases of Compactly Supported Wavelets,"
 * Communications on Pure and Applied Mathematics, vol. 41,
 * pp. 909-996, 1988. */

static coeff_t daub12_lowpass_analysis_coeffs[] = {
    0.111540743350,
    0.494623890398,
    0.751133908021,
    0.315250351709,
   -0.226264693965,
   -0.129766867567,
    0.097501605587,
    0.027522865530,
   -0.031582039317,
    0.000553842201,
    0.004777257511,
   -0.001077301085,
};

static coeff_t daub12_highpass_analysis_coeffs[] = {
   -0.001077301085,
   -0.004777257511,
    0.000553842201,
    0.031582039317,
    0.027522865530,
   -0.097501605587,
   -0.129766867567,
    0.226264693965,
    0.315250351709,
   -0.751133908021,
    0.494623890398,
   -0.111540743350,
};

static coeff_t daub12_lowpass_synthesis_coeffs[] = {
   -0.001077301085,
    0.004777257511,
    0.000553842201,
   -0.031582039317,
    0.027522865530,
    0.097501605587,
   -0.129766867567,
   -0.226264693965,
    0.315250351709,
    0.751133908021,
    0.494623890398,
    0.111540743350,
};

static coeff_t daub12_highpass_synthesis_coeffs[] = {
   -0.111540743350,
    0.494623890398,
   -0.751133908021,
    0.315250351709,
    0.226264693965,
   -0.129766867567,
   -0.097501605587,
    0.027522865530,
    0.031582039317,
    0.000553842201,
   -0.004777257511,
   -0.001077301085,
};

static filter_t daub12_lowpass_analysis = {
    12,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    daub12_lowpass_analysis_coeffs,
};

static filter_t daub12_highpass_analysis = {
    12,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    daub12_highpass_analysis_coeffs,
};

static filter_t daub12_lowpass_synthesis = {
    12,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    daub12_lowpass_synthesis_coeffs,
};

static filter_t daub12_highpass_synthesis = {
    12,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    daub12_highpass_synthesis_coeffs,
};

static filterbank_t daub12 = {
   "daub12",
   "Daubechies D12",
    ORTHOGONAL,
    &daub12_lowpass_analysis,
    &daub12_highpass_analysis,
    &daub12_lowpass_synthesis,
    &daub12_highpass_synthesis,
};

/* I. Daubechies, "Orthonormal Bases of Compactly Supported Wavelets,"
 * Communications on Pure and Applied Mathematics, vol. 41,
 * pp. 909-996, 1988. */

static coeff_t daub14_lowpass_analysis_coeffs[] = {
    0.077852054085,
    0.396539319482,
    0.729132090846,
    0.469782287405,
   -0.143906003929,
   -0.224036184994,
    0.071309219267,
    0.080612609151,
   -0.038029936935,
   -0.016574541631,
    0.012550998556,
    0.000429577973,
   -0.001801640704,
    0.000353713800,
};

static coeff_t daub14_highpass_analysis_coeffs[] = {
    0.000353713800,
    0.001801640704,
    0.000429577973,
   -0.012550998556,
   -0.016574541631,
    0.038029936935,
    0.080612609151,
   -0.071309219267,
   -0.224036184994,
    0.143906003929,
    0.469782287405,
   -0.729132090846,
    0.396539319482,
   -0.077852054085,
};

static coeff_t daub14_lowpass_synthesis_coeffs[] = {
    0.000353713800,
   -0.001801640704,
    0.000429577973,
    0.012550998556,
   -0.016574541631,
   -0.038029936935,
    0.080612609151,
    0.071309219267,
   -0.224036184994,
   -0.143906003929,
    0.469782287405,
    0.729132090846,
    0.396539319482,
    0.077852054085,
};

static coeff_t daub14_highpass_synthesis_coeffs[] = {
   -0.077852054085,
    0.396539319482,
   -0.729132090846,
    0.469782287405,
    0.143906003929,
   -0.224036184994,
   -0.071309219267,
    0.080612609151,
    0.038029936935,
   -0.016574541631,
   -0.012550998556,
    0.000429577973,
    0.001801640704,
    0.000353713800,
};

static filter_t daub14_lowpass_analysis = {
    14,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    daub14_lowpass_analysis_coeffs,
};

static filter_t daub14_highpass_analysis = {
    14,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    daub14_highpass_analysis_coeffs,
};

static filter_t daub14_lowpass_synthesis = {
    14,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    daub14_lowpass_synthesis_coeffs,
};

static filter_t daub14_highpass_synthesis = {
    14,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    daub14_highpass_synthesis_coeffs,
};

static filterbank_t daub14 = {
   "daub14",
   "Daubechies D14",
    ORTHOGONAL,
    &daub14_lowpass_analysis,
    &daub14_highpass_analysis,
    &daub14_lowpass_synthesis,
    &daub14_highpass_synthesis,
};

/* I. Daubechies, "Orthonormal Bases of Compactly Supported Wavelets,"
 * Communications on Pure and Applied Mathematics, vol. 41,
 * pp. 909-996, 1988. */

static coeff_t daub16_lowpass_analysis_coeffs[] = {
    0.054415842243,
    0.312871590914,
    0.675630736297,
    0.585354683654,
   -0.015829105256,
   -0.284015542962,
    0.000472484574,
    0.128747426620,
   -0.017369301002,
   -0.044088253931,
    0.013981027917,
    0.008746094047,
   -0.004870352993,
   -0.000391740373,
    0.000675449406,
   -0.000117476784,
};

static coeff_t daub16_highpass_analysis_coeffs[] = {
   -0.000117476784,
   -0.000675449406,
   -0.000391740373,
    0.004870352993,
    0.008746094047,
   -0.013981027917,
   -0.044088253931,
    0.017369301002,
    0.128747426620,
   -0.000472484574,
   -0.284015542962,
    0.015829105256,
    0.585354683654,
   -0.675630736297,
    0.312871590914,
   -0.054415842243,
};

static coeff_t daub16_lowpass_synthesis_coeffs[] = {
   -0.000117476784,
    0.000675449406,
   -0.000391740373,
   -0.004870352993,
    0.008746094047,
    0.013981027917,
   -0.044088253931,
   -0.017369301002,
    0.128747426620,
    0.000472484574,
   -0.284015542962,
   -0.015829105256,
    0.585354683654,
    0.675630736297,
    0.312871590914,
    0.054415842243,
};

static coeff_t daub16_highpass_synthesis_coeffs[] = {
   -0.054415842243,
    0.312871590914,
   -0.675630736297,
    0.585354683654,
    0.015829105256,
   -0.284015542962,
   -0.000472484574,
    0.128747426620,
    0.017369301002,
   -0.044088253931,
   -0.013981027917,
    0.008746094047,
    0.004870352993,
   -0.000391740373,
   -0.000675449406,
   -0.000117476784,
};

static filter_t daub16_lowpass_analysis = {
    16,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    daub16_lowpass_analysis_coeffs,
};

static filter_t daub16_highpass_analysis = {
    16,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    daub16_highpass_analysis_coeffs,
};

static filter_t daub16_lowpass_synthesis = {
    16,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    daub16_lowpass_synthesis_coeffs,
};

static filter_t daub16_highpass_synthesis = {
    16,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    daub16_highpass_synthesis_coeffs,
};

static filterbank_t daub16 = {
   "daub16",
   "Daubechies D16",
    ORTHOGONAL,
    &daub16_lowpass_analysis,
    &daub16_highpass_analysis,
    &daub16_lowpass_synthesis,
    &daub16_highpass_synthesis,
};

/* I. Daubechies, "Orthonormal Bases of Compactly Supported Wavelets,"
 * Communications on Pure and Applied Mathematics, vol. 41,
 * pp. 909-996, 1988. */

static coeff_t daub18_lowpass_analysis_coeffs[] = {
    0.038077947364,
    0.243834674613,
    0.604823123690,
    0.657288078051,
    0.133197385825,
   -0.293273783279,
   -0.096840783223,
    0.148540749338,
    0.030725681479,
   -0.067632829061,
    0.000250947115,
    0.022361662124,
   -0.004723204758,
   -0.004281503682,
    0.001847646883,
    0.000230385764,
   -0.000251963189,
    0.000039347320,
};

static coeff_t daub18_highpass_analysis_coeffs[] = {
    0.000039347320,
    0.000251963189,
    0.000230385764,
   -0.001847646883,
   -0.004281503682,
    0.004723204758,
    0.022361662124,
   -0.000250947115,
   -0.067632829061,
   -0.030725681479,
    0.148540749338,
    0.096840783223,
   -0.293273783279,
   -0.133197385825,
    0.657288078051,
   -0.604823123690,
    0.243834674613,
   -0.038077947364,
};

static coeff_t daub18_lowpass_synthesis_coeffs[] = {
    0.000039347320,
   -0.000251963189,
    0.000230385764,
    0.001847646883,
   -0.004281503682,
   -0.004723204758,
    0.022361662124,
    0.000250947115,
   -0.067632829061,
    0.030725681479,
    0.148540749338,
   -0.096840783223,
   -0.293273783279,
    0.133197385825,
    0.657288078051,
    0.604823123690,
    0.243834674613,
    0.038077947364,
};

static coeff_t daub18_highpass_synthesis_coeffs[] = {
   -0.038077947364,
    0.243834674613,
   -0.604823123690,
    0.657288078051,
   -0.133197385825,
   -0.293273783279,
    0.096840783223,
    0.148540749338,
   -0.030725681479,
   -0.067632829061,
   -0.000250947115,
    0.022361662124,
    0.004723204758,
   -0.004281503682,
   -0.001847646883,
    0.000230385764,
    0.000251963189,
    0.000039347320,
};

static filter_t daub18_lowpass_analysis = {
    18,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    daub18_lowpass_analysis_coeffs,
};

static filter_t daub18_highpass_analysis = {
    18,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    daub18_highpass_analysis_coeffs,
};

static filter_t daub18_lowpass_synthesis = {
    18,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    daub18_lowpass_synthesis_coeffs,
};

static filter_t daub18_highpass_synthesis = {
    18,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    daub18_highpass_synthesis_coeffs,
};

static filterbank_t daub18 = {
   "daub18",
   "Daubechies D18",
    ORTHOGONAL,
    &daub18_lowpass_analysis,
    &daub18_highpass_analysis,
    &daub18_lowpass_synthesis,
    &daub18_highpass_synthesis,
};

/* I. Daubechies, "Orthonormal Bases of Compactly Supported Wavelets,"
 * Communications on Pure and Applied Mathematics, vol. 41,
 * pp. 909-996, 1988. */

static coeff_t daub20_lowpass_analysis_coeffs[] = {
    0.026670057901,
    0.188176800078,
    0.527201188932,
    0.688459039454,
    0.281172343661,
   -0.249846424327,
   -0.195946274377,
    0.127369340336,
    0.093057364604,
   -0.071394147166,
   -0.029457536822,
    0.033212674059,
    0.003606553567,
   -0.010733175483,
    0.001395351747,
    0.001992405295,
   -0.000685856695,
   -0.000116466855,
    0.000093588670,
   -0.000013264203,
};

static coeff_t daub20_highpass_analysis_coeffs[] = {
   -0.000013264203,
   -0.000093588670,
   -0.000116466855,
    0.000685856695,
    0.001992405295,
   -0.001395351747,
   -0.010733175483,
   -0.003606553567,
    0.033212674059,
    0.029457536822,
   -0.071394147166,
   -0.093057364604,
    0.127369340336,
    0.195946274377,
   -0.249846424327,
   -0.281172343661,
    0.688459039454,
   -0.527201188932,
    0.188176800078,
   -0.026670057901,
};

static coeff_t daub20_lowpass_synthesis_coeffs[] = {
   -0.000013264203,
    0.000093588670,
   -0.000116466855,
   -0.000685856695,
    0.001992405295,
    0.001395351747,
   -0.010733175483,
    0.003606553567,
    0.033212674059,
   -0.029457536822,
   -0.071394147166,
    0.093057364604,
    0.127369340336,
   -0.195946274377,
   -0.249846424327,
    0.281172343661,
    0.688459039454,
    0.527201188932,
    0.188176800078,
    0.026670057901,
};

static coeff_t daub20_highpass_synthesis_coeffs[] = {
   -0.026670057901,
    0.188176800078,
   -0.527201188932,
    0.688459039454,
   -0.281172343661,
   -0.249846424327,
    0.195946274377,
    0.127369340336,
   -0.093057364604,
   -0.071394147166,
    0.029457536822,
    0.033212674059,
   -0.003606553567,
   -0.010733175483,
   -0.001395351747,
    0.001992405295,
    0.000685856695,
   -0.000116466855,
   -0.000093588670,
   -0.000013264203,
};

static filter_t daub20_lowpass_analysis = {
    20,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    daub20_lowpass_analysis_coeffs,
};

static filter_t daub20_highpass_analysis = {
    20,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    daub20_highpass_analysis_coeffs,
};

static filter_t daub20_lowpass_synthesis = {
    20,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    daub20_lowpass_synthesis_coeffs,
};

static filter_t daub20_highpass_synthesis = {
    20,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    daub20_highpass_synthesis_coeffs,
};

static filterbank_t daub20 = {
   "daub20",
   "Daubechies D20",
    ORTHOGONAL,
    &daub20_lowpass_analysis,
    &daub20_highpass_analysis,
    &daub20_lowpass_synthesis,
    &daub20_highpass_synthesis,
};

/* The Beylkin filter places roots for the frequency response function
 * close to the Nyquist frequency on the real axis. */

static coeff_t beylkin_lowpass_analysis_coeffs[] = {
    0.099305765374,
    0.424215360813,
    0.699825214057,
    0.449718251149,
   -0.110927598348,
   -0.264497231446,
    0.026900308804,
    0.155538731877,
   -0.017520746267,
   -0.088543630623,
    0.019679866044,
    0.042916387274,
   -0.017460408696,
   -0.014365807969,
    0.010040411845,
    0.001484234782,
   -0.002736031626,
    0.000640485329,
};

static coeff_t beylkin_highpass_analysis_coeffs[] = {
    0.000640485329,
    0.002736031626,
    0.001484234782,
   -0.010040411845,
   -0.014365807969,
    0.017460408696,
    0.042916387274,
   -0.019679866044,
   -0.088543630623,
    0.017520746267,
    0.155538731877,
   -0.026900308804,
   -0.264497231446,
    0.110927598348,
    0.449718251149,
   -0.699825214057,
    0.424215360813,
   -0.099305765374,
};

static coeff_t beylkin_lowpass_synthesis_coeffs[] = {
    0.000640485329,
   -0.002736031626,
    0.001484234782,
    0.010040411845,
   -0.014365807969,
   -0.017460408696,
    0.042916387274,
    0.019679866044,
   -0.088543630623,
   -0.017520746267,
    0.155538731877,
    0.026900308804,
   -0.264497231446,
   -0.110927598348,
    0.449718251149,
    0.699825214057,
    0.424215360813,
    0.099305765374,
};

static coeff_t beylkin_highpass_synthesis_coeffs[] = {
   -0.099305765374,
    0.424215360813,
   -0.699825214057,
    0.449718251149,
    0.110927598348,
   -0.264497231446,
   -0.026900308804,
    0.155538731877,
    0.017520746267,
   -0.088543630623,
   -0.019679866044,
    0.042916387274,
    0.017460408696,
   -0.014365807969,
   -0.010040411845,
    0.001484234782,
    0.002736031626,
    0.000640485329,
};

static filter_t beylkin_lowpass_analysis = {
    18,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    beylkin_lowpass_analysis_coeffs,
};

static filter_t beylkin_highpass_analysis = {
    18,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    beylkin_highpass_analysis_coeffs,
};

static filter_t beylkin_lowpass_synthesis = {
    18,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    beylkin_lowpass_synthesis_coeffs,
};

static filter_t beylkin_highpass_synthesis = {
    18,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    beylkin_highpass_synthesis_coeffs,
};

static filterbank_t beylkin = {
   "beylkin",
   "Beylkin",
    ORTHOGONAL,
    &beylkin_lowpass_analysis,
    &beylkin_highpass_analysis,
    &beylkin_lowpass_synthesis,
    &beylkin_highpass_synthesis,
};

/* The Vaidyanathan filter gives an exact reconstruction, but does not
 * satisfy any moment condition.  The filter has been optimized for
 * speech coding. */

static coeff_t vaidyanathan_lowpass_analysis_coeffs[] = {
   -0.000062906118,
    0.000343631905,
   -0.000453956620,
   -0.000944897136,
    0.002843834547,
    0.000708137504,
   -0.008839103409,
    0.003153847056,
    0.019687215010,
   -0.014853448005,
   -0.035470398607,
    0.038742619293,
    0.055892523691,
   -0.077709750902,
   -0.083928884366,
    0.131971661417,
    0.135084227129,
   -0.194450471766,
   -0.263494802488,
    0.201612161775,
    0.635601059872,
    0.572797793211,
    0.250184129505,
    0.045799334111,
};

static coeff_t vaidyanathan_highpass_analysis_coeffs[] = {
    0.045799334111,
   -0.250184129505,
    0.572797793211,
   -0.635601059872,
    0.201612161775,
    0.263494802488,
   -0.194450471766,
   -0.135084227129,
    0.131971661417,
    0.083928884366,
   -0.077709750902,
   -0.055892523691,
    0.038742619293,
    0.035470398607,
   -0.014853448005,
   -0.019687215010,
    0.003153847056,
    0.008839103409,
    0.000708137504,
   -0.002843834547,
   -0.000944897136,
    0.000453956620,
    0.000343631905,
    0.000062906118,
};

static coeff_t vaidyanathan_lowpass_synthesis_coeffs[] = {
    0.045799334111,
    0.250184129505,
    0.572797793211,
    0.635601059872,
    0.201612161775,
   -0.263494802488,
   -0.194450471766,
    0.135084227129,
    0.131971661417,
   -0.083928884366,
   -0.077709750902,
    0.055892523691,
    0.038742619293,
   -0.035470398607,
   -0.014853448005,
    0.019687215010,
    0.003153847056,
   -0.008839103409,
    0.000708137504,
    0.002843834547,
   -0.000944897136,
   -0.000453956620,
    0.000343631905,
   -0.000062906118,
};

static coeff_t vaidyanathan_highpass_synthesis_coeffs[] = {
    0.000062906118,
    0.000343631905,
    0.000453956620,
   -0.000944897136,
   -0.002843834547,
    0.000708137504,
    0.008839103409,
    0.003153847056,
   -0.019687215010,
   -0.014853448005,
    0.035470398607,
    0.038742619293,
   -0.055892523691,
   -0.077709750902,
    0.083928884366,
    0.131971661417,
   -0.135084227129,
   -0.194450471766,
    0.263494802488,
    0.201612161775,
   -0.635601059872,
    0.572797793211,
   -0.250184129505,
    0.045799334111,
};

static filter_t vaidyanathan_lowpass_analysis = {
    24,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    vaidyanathan_lowpass_analysis_coeffs,
};

static filter_t vaidyanathan_highpass_analysis = {
    24,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    vaidyanathan_highpass_analysis_coeffs,
};

static filter_t vaidyanathan_lowpass_synthesis = {
    24,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    vaidyanathan_lowpass_synthesis_coeffs,
};

static filter_t vaidyanathan_highpass_synthesis = {
    24,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    vaidyanathan_highpass_synthesis_coeffs,
};

static filterbank_t vaidyanathan = {
   "vaidyanathan",
   "Vaidyanathan",
    ORTHOGONAL,
    &vaidyanathan_lowpass_analysis,
    &vaidyanathan_highpass_analysis,
    &vaidyanathan_lowpass_synthesis,
    &vaidyanathan_highpass_synthesis,
};

/* Coeflet C6 filter. */

static coeff_t coiflet6_lowpass_analysis_coeffs[] = {
    0.038580777748,
   -0.126969125396,
   -0.077161555496,
    0.607491641386,
    0.745687558934,
    0.226584265197,
};

static coeff_t coiflet6_highpass_analysis_coeffs[] = {
    0.226584265197,
   -0.745687558934,
    0.607491641386,
    0.077161555496,
   -0.126969125396,
   -0.038580777748,
};

static coeff_t coiflet6_lowpass_synthesis_coeffs[] = {
    0.226584265197,
    0.745687558934,
    0.607491641386,
   -0.077161555496,
   -0.126969125396,
    0.038580777748,
};

static coeff_t coiflet6_highpass_synthesis_coeffs[] = {
   -0.038580777748,
   -0.126969125396,
    0.077161555496,
    0.607491641386,
   -0.745687558934,
    0.226584265197,
};

static filter_t coiflet6_lowpass_analysis = {
    6,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    coiflet6_lowpass_analysis_coeffs,
};

static filter_t coiflet6_highpass_analysis = {
    6,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    coiflet6_highpass_analysis_coeffs,
};

static filter_t coiflet6_lowpass_synthesis = {
    6,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    coiflet6_lowpass_synthesis_coeffs,
};

static filter_t coiflet6_highpass_synthesis = {
    6,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    coiflet6_highpass_synthesis_coeffs,
};

static filterbank_t coiflet6 = {
   "coiflet6",
   "Coeflet C6",
    ORTHOGONAL,
    &coiflet6_lowpass_analysis,
    &coiflet6_highpass_analysis,
    &coiflet6_lowpass_synthesis,
    &coiflet6_highpass_synthesis,
};

/* Coeflet C12 filter. */

static coeff_t coiflet12_lowpass_analysis_coeffs[] = {
    0.016387336463,
   -0.041464936782,
   -0.067372554722,
    0.386110066823,
    0.812723635450,
    0.417005184424,
   -0.076488599078,
   -0.059434418646,
    0.023680171947,
    0.005611434819,
   -0.001823208871,
   -0.000720549445,
};

static coeff_t coiflet12_highpass_analysis_coeffs[] = {
   -0.000720549445,
    0.001823208871,
    0.005611434819,
   -0.023680171947,
   -0.059434418646,
    0.076488599078,
    0.417005184424,
   -0.812723635450,
    0.386110066823,
    0.067372554722,
   -0.041464936782,
   -0.016387336463,
};

static coeff_t coiflet12_lowpass_synthesis_coeffs[] = {
   -0.000720549445,
   -0.001823208871,
    0.005611434819,
    0.023680171947,
   -0.059434418646,
   -0.076488599078,
    0.417005184424,
    0.812723635450,
    0.386110066823,
   -0.067372554722,
   -0.041464936782,
    0.016387336463,
};

static coeff_t coiflet12_highpass_synthesis_coeffs[] = {
   -0.016387336463,
   -0.041464936782,
    0.067372554722,
    0.386110066823,
   -0.812723635450,
    0.417005184424,
    0.076488599078,
   -0.059434418646,
   -0.023680171947,
    0.005611434819,
    0.001823208871,
   -0.000720549445,
};

static filter_t coiflet12_lowpass_analysis = {
    12,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    coiflet12_lowpass_analysis_coeffs,
};

static filter_t coiflet12_highpass_analysis = {
    12,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    coiflet12_highpass_analysis_coeffs,
};

static filter_t coiflet12_lowpass_synthesis = {
    12,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    coiflet12_lowpass_synthesis_coeffs,
};

static filter_t coiflet12_highpass_synthesis = {
    12,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    coiflet12_highpass_synthesis_coeffs,
};

static filterbank_t coiflet12 = {
   "coiflet12",
   "Coeflet C12",
    ORTHOGONAL,
    &coiflet12_lowpass_analysis,
    &coiflet12_highpass_analysis,
    &coiflet12_lowpass_synthesis,
    &coiflet12_highpass_synthesis,
};

/* Coeflet C18 filter. */

static coeff_t coiflet18_lowpass_analysis_coeffs[] = {
   -0.003793512864,
    0.007782596426,
    0.023452696142,
   -0.065771911281,
   -0.061123390003,
    0.405176902410,
    0.793777222626,
    0.428483476378,
   -0.071799821619,
   -0.082301927106,
    0.034555027573,
    0.015880544864,
   -0.009007976137,
   -0.002574517688,
    0.001117518771,
    0.000466216960,
   -0.000070983303,
   -0.000034599773,
};

static coeff_t coiflet18_highpass_analysis_coeffs[] = {
   -0.000034599773,
    0.000070983303,
    0.000466216960,
   -0.001117518771,
   -0.002574517688,
    0.009007976137,
    0.015880544864,
   -0.034555027573,
   -0.082301927106,
    0.071799821619,
    0.428483476378,
   -0.793777222626,
    0.405176902410,
    0.061123390003,
   -0.065771911281,
   -0.023452696142,
    0.007782596426,
    0.003793512864,
};

static coeff_t coiflet18_lowpass_synthesis_coeffs[] = {
   -0.000034599773,
   -0.000070983303,
    0.000466216960,
    0.001117518771,
   -0.002574517688,
   -0.009007976137,
    0.015880544864,
    0.034555027573,
   -0.082301927106,
   -0.071799821619,
    0.428483476378,
    0.793777222626,
    0.405176902410,
   -0.061123390003,
   -0.065771911281,
    0.023452696142,
    0.007782596426,
   -0.003793512864,
};

static coeff_t coiflet18_highpass_synthesis_coeffs[] = {
    0.003793512864,
    0.007782596426,
   -0.023452696142,
   -0.065771911281,
    0.061123390003,
    0.405176902410,
   -0.793777222626,
    0.428483476378,
    0.071799821619,
   -0.082301927106,
   -0.034555027573,
    0.015880544864,
    0.009007976137,
   -0.002574517688,
   -0.001117518771,
    0.000466216960,
    0.000070983303,
   -0.000034599773,
};

static filter_t coiflet18_lowpass_analysis = {
    18,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    coiflet18_lowpass_analysis_coeffs,
};

static filter_t coiflet18_highpass_analysis = {
    18,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    coiflet18_highpass_analysis_coeffs,
};

static filter_t coiflet18_lowpass_synthesis = {
    18,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    coiflet18_lowpass_synthesis_coeffs,
};

static filter_t coiflet18_highpass_synthesis = {
    18,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    coiflet18_highpass_synthesis_coeffs,
};

static filterbank_t coiflet18 = {
   "coiflet18",
   "Coeflet C18",
    ORTHOGONAL,
    &coiflet18_lowpass_analysis,
    &coiflet18_highpass_analysis,
    &coiflet18_lowpass_synthesis,
    &coiflet18_highpass_synthesis,
};

/* Coeflet C24 filter. */

static coeff_t coiflet24_lowpass_analysis_coeffs[] = {
    0.000892313668,
   -0.001629492013,
   -0.007346166328,
    0.016068943964,
    0.026682300156,
   -0.081266699680,
   -0.056077313316,
    0.415308407030,
    0.782238930920,
    0.434386056491,
   -0.066627474263,
   -0.096220442034,
    0.039334427123,
    0.025082261845,
   -0.015211731527,
   -0.005658286686,
    0.003751436157,
    0.001266561929,
   -0.000589020757,
   -0.000259974552,
    0.000062339034,
    0.000031229876,
   -0.000003259680,
   -0.000001784985,
};

static coeff_t coiflet24_highpass_analysis_coeffs[] = {
   -0.000001784985,
    0.000003259680,
    0.000031229876,
   -0.000062339034,
   -0.000259974552,
    0.000589020757,
    0.001266561929,
   -0.003751436157,
   -0.005658286686,
    0.015211731527,
    0.025082261845,
   -0.039334427123,
   -0.096220442034,
    0.066627474263,
    0.434386056491,
   -0.782238930920,
    0.415308407030,
    0.056077313316,
   -0.081266699680,
   -0.026682300156,
    0.016068943964,
    0.007346166328,
   -0.001629492013,
   -0.000892313668,
};

static coeff_t coiflet24_lowpass_synthesis_coeffs[] = {
   -0.000001784985,
   -0.000003259680,
    0.000031229876,
    0.000062339034,
   -0.000259974552,
   -0.000589020757,
    0.001266561929,
    0.003751436157,
   -0.005658286686,
   -0.015211731527,
    0.025082261845,
    0.039334427123,
   -0.096220442034,
   -0.066627474263,
    0.434386056491,
    0.782238930920,
    0.415308407030,
   -0.056077313316,
   -0.081266699680,
    0.026682300156,
    0.016068943964,
   -0.007346166328,
   -0.001629492013,
    0.000892313668,
};

static coeff_t coiflet24_highpass_synthesis_coeffs[] = {
   -0.000892313668,
   -0.001629492013,
    0.007346166328,
    0.016068943964,
   -0.026682300156,
   -0.081266699680,
    0.056077313316,
    0.415308407030,
   -0.782238930920,
    0.434386056491,
    0.066627474263,
   -0.096220442034,
   -0.039334427123,
    0.025082261845,
    0.015211731527,
   -0.005658286686,
   -0.003751436157,
    0.001266561929,
    0.000589020757,
   -0.000259974552,
   -0.000062339034,
    0.000031229876,
    0.000003259680,
   -0.000001784985,
};

static filter_t coiflet24_lowpass_analysis = {
    24,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    coiflet24_lowpass_analysis_coeffs,
};

static filter_t coiflet24_highpass_analysis = {
    24,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    coiflet24_highpass_analysis_coeffs,
};

static filter_t coiflet24_lowpass_synthesis = {
    24,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    coiflet24_lowpass_synthesis_coeffs,
};

static filter_t coiflet24_highpass_synthesis = {
    24,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    coiflet24_highpass_synthesis_coeffs,
};

static filterbank_t coiflet24 = {
   "coiflet24",
   "Coeflet C24",
    ORTHOGONAL,
    &coiflet24_lowpass_analysis,
    &coiflet24_highpass_analysis,
    &coiflet24_lowpass_synthesis,
    &coiflet24_highpass_synthesis,
};

/* Coiflet C30 filter. */

static coeff_t coiflet30_lowpass_analysis_coeffs[] = {
   -0.000212080863,
    0.000358589677,
    0.002178236305,
   -0.004159358782,
   -0.010131117538,
    0.023408156762,
    0.028168029062,
   -0.091920010549,
   -0.052043163216,
    0.421566206729,
    0.774289603740,
    0.437991626228,
   -0.062035963906,
   -0.105574208706,
    0.041289208741,
    0.032683574283,
   -0.019761779012,
   -0.009164231153,
    0.006764185419,
    0.002433373209,
   -0.001662863769,
   -0.000638131296,
    0.000302259520,
    0.000140541149,
   -0.000041340484,
   -0.000021315014,
    0.000003734597,
    0.000002063806,
   -0.000000167408,
   -0.000000095158,
};

static coeff_t coiflet30_highpass_analysis_coeffs[] = {
   -0.000000095158,
    0.000000167408,
    0.000002063806,
   -0.000003734597,
   -0.000021315014,
    0.000041340484,
    0.000140541149,
   -0.000302259520,
   -0.000638131296,
    0.001662863769,
    0.002433373209,
   -0.006764185419,
   -0.009164231153,
    0.019761779012,
    0.032683574283,
   -0.041289208741,
   -0.105574208706,
    0.062035963906,
    0.437991626228,
   -0.774289603740,
    0.421566206729,
    0.052043163216,
   -0.091920010549,
   -0.028168029062,
    0.023408156762,
    0.010131117538,
   -0.004159358782,
   -0.002178236305,
    0.000358589677,
    0.000212080863,
};

static coeff_t coiflet30_lowpass_synthesis_coeffs[] = {
   -0.000000095158,
   -0.000000167408,
    0.000002063806,
    0.000003734597,
   -0.000021315014,
   -0.000041340484,
    0.000140541149,
    0.000302259520,
   -0.000638131296,
   -0.001662863769,
    0.002433373209,
    0.006764185419,
   -0.009164231153,
   -0.019761779012,
    0.032683574283,
    0.041289208741,
   -0.105574208706,
   -0.062035963906,
    0.437991626228,
    0.774289603740,
    0.421566206729,
   -0.052043163216,
   -0.091920010549,
    0.028168029062,
    0.023408156762,
   -0.010131117538,
   -0.004159358782,
    0.002178236305,
    0.000358589677,
   -0.000212080863,
};

static coeff_t coiflet30_highpass_synthesis_coeffs[] = {
    0.000212080863,
    0.000358589677,
   -0.002178236305,
   -0.004159358782,
    0.010131117538,
    0.023408156762,
   -0.028168029062,
   -0.091920010549,
    0.052043163216,
    0.421566206729,
   -0.774289603740,
    0.437991626228,
    0.062035963906,
   -0.105574208706,
   -0.041289208741,
    0.032683574283,
    0.019761779012,
   -0.009164231153,
   -0.006764185419,
    0.002433373209,
    0.001662863769,
   -0.000638131296,
   -0.000302259520,
    0.000140541149,
    0.000041340484,
   -0.000021315014,
   -0.000003734597,
    0.000002063806,
    0.000000167408,
   -0.000000095158,
};

static filter_t coiflet30_lowpass_analysis = {
    30,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    coiflet30_lowpass_analysis_coeffs,
};

static filter_t coiflet30_highpass_analysis = {
    30,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    coiflet30_highpass_analysis_coeffs,
};

static filter_t coiflet30_lowpass_synthesis = {
    30,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    coiflet30_lowpass_synthesis_coeffs,
};

static filter_t coiflet30_highpass_synthesis = {
    30,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    coiflet30_highpass_synthesis_coeffs,
};

static filterbank_t coiflet30 = {
   "coiflet30",
   "Coiflet C30",
    ORTHOGONAL,
    &coiflet30_lowpass_analysis,
    &coiflet30_highpass_analysis,
    &coiflet30_lowpass_synthesis,
    &coiflet30_highpass_synthesis,
};

/* Symmlets are wavelets within a minimum size support for a given
 * number of vanishing moments, but they are as symmetrical as possible,
 * as opposed to the Daubechies filters which are highly asymmetrical. */

static coeff_t symmlet8_lowpass_analysis_coeffs[] = {
   -0.075765714789357,
   -0.029635527645960,
    0.497618667632563,
    0.803738751805386,
    0.297857795605605,
   -0.099219543576956,
   -0.012603967262264,
    0.032223100604078,
};

static coeff_t symmlet8_highpass_analysis_coeffs[] = {
    0.032223100604078,
    0.012603967262264,
   -0.099219543576956,
   -0.297857795605605,
    0.803738751805386,
   -0.497618667632563,
   -0.029635527645960,
    0.075765714789357,
};

static coeff_t symmlet8_lowpass_synthesis_coeffs[] = {
    0.032223100604078,
   -0.012603967262264,
   -0.099219543576956,
    0.297857795605605,
    0.803738751805386,
    0.497618667632563,
   -0.029635527645960,
   -0.075765714789357,
};

static coeff_t symmlet8_highpass_synthesis_coeffs[] = {
    0.075765714789357,
   -0.029635527645960,
   -0.497618667632563,
    0.803738751805386,
   -0.297857795605605,
   -0.099219543576956,
    0.012603967262264,
    0.032223100604078,
};

static filter_t symmlet8_lowpass_analysis = {
    8,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    symmlet8_lowpass_analysis_coeffs,
};

static filter_t symmlet8_highpass_analysis = {
    8,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    symmlet8_highpass_analysis_coeffs,
};

static filter_t symmlet8_lowpass_synthesis = {
    8,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    symmlet8_lowpass_synthesis_coeffs,
};

static filter_t symmlet8_highpass_synthesis = {
    8,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    symmlet8_highpass_synthesis_coeffs,
};

static filterbank_t symmlet8 = {
   "symmlet8",
   "Symmlet S8",
    ORTHOGONAL,
    &symmlet8_lowpass_analysis,
    &symmlet8_highpass_analysis,
    &symmlet8_lowpass_synthesis,
    &symmlet8_highpass_synthesis,
};

/* Symmlets are wavelets within a minimum size support for a given
 * number of vanishing moments, but they are as symmetrical as possible,
 * as opposed to the Daubechies filters which are highly asymmetrical. */

static coeff_t symmlet10_lowpass_analysis_coeffs[] = {
    0.027333068345163,
    0.029519490926072,
   -0.039134249302581,
    0.199397533976983,
    0.723407690403764,
    0.633978963456911,
    0.016602105764423,
   -0.175328089908097,
   -0.021101834024929,
    0.019538882735386,
};

static coeff_t symmlet10_highpass_analysis_coeffs[] = {
    0.019538882735386,
    0.021101834024929,
   -0.175328089908097,
   -0.016602105764423,
    0.633978963456911,
   -0.723407690403764,
    0.199397533976983,
    0.039134249302581,
    0.029519490926072,
   -0.027333068345163,
};

static coeff_t symmlet10_lowpass_synthesis_coeffs[] = {
    0.019538882735386,
   -0.021101834024929,
   -0.175328089908097,
    0.016602105764423,
    0.633978963456911,
    0.723407690403764,
    0.199397533976983,
   -0.039134249302581,
    0.029519490926072,
    0.027333068345163,
};

static coeff_t symmlet10_highpass_synthesis_coeffs[] = {
   -0.027333068345163,
    0.029519490926072,
    0.039134249302581,
    0.199397533976983,
   -0.723407690403764,
    0.633978963456911,
   -0.016602105764423,
   -0.175328089908097,
    0.021101834024929,
    0.019538882735386,
};

static filter_t symmlet10_lowpass_analysis = {
    10,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    symmlet10_lowpass_analysis_coeffs,
};

static filter_t symmlet10_highpass_analysis = {
    10,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    symmlet10_highpass_analysis_coeffs,
};

static filter_t symmlet10_lowpass_synthesis = {
    10,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    symmlet10_lowpass_synthesis_coeffs,
};

static filter_t symmlet10_highpass_synthesis = {
    10,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    symmlet10_highpass_synthesis_coeffs,
};

static filterbank_t symmlet10 = {
   "symmlet10",
   "Symmlet S10",
    ORTHOGONAL,
    &symmlet10_lowpass_analysis,
    &symmlet10_highpass_analysis,
    &symmlet10_lowpass_synthesis,
    &symmlet10_highpass_synthesis,
};

/* Symmlets are wavelets within a minimum size support for a given
 * number of vanishing moments, but they are as symmetrical as possible,
 * as opposed to the Daubechies filters which are highly asymmetrical. */

static coeff_t symmlet12_lowpass_analysis_coeffs[] = {
    0.015404109327339,
    0.003490712084331,
   -0.117990111148417,
   -0.048311742586001,
    0.491055941927666,
    0.787641141028836,
    0.337929421728258,
   -0.072637522786604,
   -0.021060292512697,
    0.044724901770751,
    0.001767711864398,
   -0.007800708324765,
};

static coeff_t symmlet12_highpass_analysis_coeffs[] = {
   -0.007800708324765,
   -0.001767711864398,
    0.044724901770751,
    0.021060292512697,
   -0.072637522786604,
   -0.337929421728258,
    0.787641141028836,
   -0.491055941927666,
   -0.048311742586001,
    0.117990111148417,
    0.003490712084331,
   -0.015404109327339,
};

static coeff_t symmlet12_lowpass_synthesis_coeffs[] = {
   -0.007800708324765,
    0.001767711864398,
    0.044724901770751,
   -0.021060292512697,
   -0.072637522786604,
    0.337929421728258,
    0.787641141028836,
    0.491055941927666,
   -0.048311742586001,
   -0.117990111148417,
    0.003490712084331,
    0.015404109327339,
};

static coeff_t symmlet12_highpass_synthesis_coeffs[] = {
   -0.015404109327339,
    0.003490712084331,
    0.117990111148417,
   -0.048311742586001,
   -0.491055941927666,
    0.787641141028836,
   -0.337929421728258,
   -0.072637522786604,
    0.021060292512697,
    0.044724901770751,
   -0.001767711864398,
   -0.007800708324765,
};

static filter_t symmlet12_lowpass_analysis = {
    12,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    symmlet12_lowpass_analysis_coeffs,
};

static filter_t symmlet12_highpass_analysis = {
    12,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    symmlet12_highpass_analysis_coeffs,
};

static filter_t symmlet12_lowpass_synthesis = {
    12,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    symmlet12_lowpass_synthesis_coeffs,
};

static filter_t symmlet12_highpass_synthesis = {
    12,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    symmlet12_highpass_synthesis_coeffs,
};

static filterbank_t symmlet12 = {
   "symmlet12",
   "Symmlet S12",
    ORTHOGONAL,
    &symmlet12_lowpass_analysis,
    &symmlet12_highpass_analysis,
    &symmlet12_lowpass_synthesis,
    &symmlet12_highpass_synthesis,
};

/* Symmlets are wavelets within a minimum size support for a given
 * number of vanishing moments, but they are as symmetrical as possible,
 * as opposed to the Daubechies filters which are highly asymmetrical. */

static coeff_t symmlet14_lowpass_analysis_coeffs[] = {
    0.002681814568116,
   -0.001047384888965,
   -0.012636303403152,
    0.030515513165906,
    0.067892693501598,
   -0.049552834937041,
    0.017441255087110,
    0.536101917090782,
    0.767764317004585,
    0.288629631750988,
   -0.140047240442706,
   -0.107808237703619,
    0.004010244871703,
    0.010268176708497,
};

static coeff_t symmlet14_highpass_analysis_coeffs[] = {
    0.010268176708497,
   -0.004010244871703,
   -0.107808237703619,
    0.140047240442706,
    0.288629631750988,
   -0.767764317004585,
    0.536101917090782,
   -0.017441255087110,
   -0.049552834937041,
   -0.067892693501598,
    0.030515513165906,
    0.012636303403152,
   -0.001047384888965,
   -0.002681814568116,
};

static coeff_t symmlet14_lowpass_synthesis_coeffs[] = {
    0.010268176708497,
    0.004010244871703,
   -0.107808237703619,
   -0.140047240442706,
    0.288629631750988,
    0.767764317004585,
    0.536101917090782,
    0.017441255087110,
   -0.049552834937041,
    0.067892693501598,
    0.030515513165906,
   -0.012636303403152,
   -0.001047384888965,
    0.002681814568116,
};

static coeff_t symmlet14_highpass_synthesis_coeffs[] = {
   -0.002681814568116,
   -0.001047384888965,
    0.012636303403152,
    0.030515513165906,
   -0.067892693501598,
   -0.049552834937041,
   -0.017441255087110,
    0.536101917090782,
   -0.767764317004585,
    0.288629631750988,
    0.140047240442706,
   -0.107808237703619,
   -0.004010244871703,
    0.010268176708497,
};

static filter_t symmlet14_lowpass_analysis = {
    14,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    symmlet14_lowpass_analysis_coeffs,
};

static filter_t symmlet14_highpass_analysis = {
    14,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    symmlet14_highpass_analysis_coeffs,
};

static filter_t symmlet14_lowpass_synthesis = {
    14,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    symmlet14_lowpass_synthesis_coeffs,
};

static filter_t symmlet14_highpass_synthesis = {
    14,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    symmlet14_highpass_synthesis_coeffs,
};

static filterbank_t symmlet14 = {
   "symmlet14",
   "Symmlet S14",
    ORTHOGONAL,
    &symmlet14_lowpass_analysis,
    &symmlet14_highpass_analysis,
    &symmlet14_lowpass_synthesis,
    &symmlet14_highpass_synthesis,
};

/* Symmlets are wavelets within a minimum size support for a given
 * number of vanishing moments, but they are as symmetrical as possible,
 * as opposed to the Daubechies filters which are highly asymmetrical. */

static coeff_t symmlet16_lowpass_analysis_coeffs[] = {
    0.00188995033291,
   -0.00030292051455,
   -0.01495225833679,
    0.00380875201406,
    0.04913717967348,
   -0.02721902991682,
   -0.05194583810788,
    0.36444189483599,
    0.77718575169981,
    0.48135965125924,
   -0.06127335906791,
   -0.14329423835107,
    0.00760748732529,
    0.03169508781035,
   -0.00054213233164,
   -0.00338241595136,
};

static coeff_t symmlet16_highpass_analysis_coeffs[] = {
   -0.00338241595136,
    0.00054213233164,
    0.03169508781035,
   -0.00760748732529,
   -0.14329423835107,
    0.06127335906791,
    0.48135965125924,
   -0.77718575169981,
    0.36444189483599,
    0.05194583810788,
   -0.02721902991682,
   -0.04913717967348,
    0.00380875201406,
    0.01495225833679,
   -0.00030292051455,
   -0.00188995033291,
};

static coeff_t symmlet16_lowpass_synthesis_coeffs[] = {
   -0.00338241595136,
   -0.00054213233164,
    0.03169508781035,
    0.00760748732529,
   -0.14329423835107,
   -0.06127335906791,
    0.48135965125924,
    0.77718575169981,
    0.36444189483599,
   -0.05194583810788,
   -0.02721902991682,
    0.04913717967348,
    0.00380875201406,
   -0.01495225833679,
   -0.00030292051455,
    0.00188995033291,
};

static coeff_t symmlet16_highpass_synthesis_coeffs[] = {
   -0.00188995033291,
   -0.00030292051455,
    0.01495225833679,
    0.00380875201406,
   -0.04913717967348,
   -0.02721902991682,
    0.05194583810788,
    0.36444189483599,
   -0.77718575169981,
    0.48135965125924,
    0.06127335906791,
   -0.14329423835107,
   -0.00760748732529,
    0.03169508781035,
    0.00054213233164,
   -0.00338241595136,
};

static filter_t symmlet16_lowpass_analysis = {
    16,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    symmlet16_lowpass_analysis_coeffs,
};

static filter_t symmlet16_highpass_analysis = {
    16,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    symmlet16_highpass_analysis_coeffs,
};

static filter_t symmlet16_lowpass_synthesis = {
    16,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    symmlet16_lowpass_synthesis_coeffs,
};

static filter_t symmlet16_highpass_synthesis = {
    16,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    symmlet16_highpass_synthesis_coeffs,
};

static filterbank_t symmlet16 = {
   "symmlet16",
   "Symmlet S16",
    ORTHOGONAL,
    &symmlet16_lowpass_analysis,
    &symmlet16_highpass_analysis,
    &symmlet16_lowpass_synthesis,
    &symmlet16_highpass_synthesis,
};

/* Symmlets are wavelets within a minimum size support for a given
 * number of vanishing moments, but they are as symmetrical as possible,
 * as opposed to the Daubechies filters which are highly asymmetrical. */

static coeff_t symmlet18_lowpass_analysis_coeffs[] = {
    0.001069490032652,
   -0.000473154498587,
   -0.010264064027672,
    0.008859267493501,
    0.062077789302687,
   -0.018233770779803,
   -0.191550831296252,
    0.035272488035891,
    0.617338449140593,
    0.717897082763343,
    0.238760914607125,
   -0.054568958430509,
    0.000583462746330,
    0.030224878857952,
   -0.011528210207971,
   -0.013271967781517,
    0.000619780889054,
    0.001400915525570,
};

static coeff_t symmlet18_highpass_analysis_coeffs[] = {
    0.001400915525570,
   -0.000619780889054,
   -0.013271967781517,
    0.011528210207971,
    0.030224878857952,
   -0.000583462746330,
   -0.054568958430509,
   -0.238760914607125,
    0.717897082763343,
   -0.617338449140593,
    0.035272488035891,
    0.191550831296252,
   -0.018233770779803,
   -0.062077789302687,
    0.008859267493501,
    0.010264064027672,
   -0.000473154498587,
   -0.001069490032652,
};

static coeff_t symmlet18_lowpass_synthesis_coeffs[] = {
    0.001400915525570,
    0.000619780889054,
   -0.013271967781517,
   -0.011528210207971,
    0.030224878857952,
    0.000583462746330,
   -0.054568958430509,
    0.238760914607125,
    0.717897082763343,
    0.617338449140593,
    0.035272488035891,
   -0.191550831296252,
   -0.018233770779803,
    0.062077789302687,
    0.008859267493501,
   -0.010264064027672,
   -0.000473154498587,
    0.001069490032652,
};

static coeff_t symmlet18_highpass_synthesis_coeffs[] = {
   -0.001069490032652,
   -0.000473154498587,
    0.010264064027672,
    0.008859267493501,
   -0.062077789302687,
   -0.018233770779803,
    0.191550831296252,
    0.035272488035891,
   -0.617338449140593,
    0.717897082763343,
   -0.238760914607125,
   -0.054568958430509,
   -0.000583462746330,
    0.030224878857952,
    0.011528210207971,
   -0.013271967781517,
   -0.000619780889054,
    0.001400915525570,
};

static filter_t symmlet18_lowpass_analysis = {
    18,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    symmlet18_lowpass_analysis_coeffs,
};

static filter_t symmlet18_highpass_analysis = {
    18,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    symmlet18_highpass_analysis_coeffs,
};

static filter_t symmlet18_lowpass_synthesis = {
    18,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    symmlet18_lowpass_synthesis_coeffs,
};

static filter_t symmlet18_highpass_synthesis = {
    18,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    symmlet18_highpass_synthesis_coeffs,
};

static filterbank_t symmlet18 = {
   "symmlet18",
   "Symmlet S18",
    ORTHOGONAL,
    &symmlet18_lowpass_analysis,
    &symmlet18_highpass_analysis,
    &symmlet18_lowpass_synthesis,
    &symmlet18_highpass_synthesis,
};

/* Symmlets are wavelets within a minimum size support for a given
 * number of vanishing moments, but they are as symmetrical as possible,
 * as opposed to the Daubechies filters which are highly asymmetrical. */

static coeff_t symmlet20_lowpass_analysis_coeffs[] = {
    0.0007701598089417,
    9.56326707637102e-05,
   -0.0086412992741304,
   -0.0014653825830397,
    0.0459272392141469,
    0.0116098939105411,
   -0.1594942788241300,
   -0.0708805357960178,
    0.4716906667438780,
    0.7695100368531890,
    0.3838267611450020,
   -0.0355367402980268,
   -0.0319900568214638,
    0.0499949720686861,
    0.0057649120443445,
   -0.0203549397996833,
   -0.0008043589343686,
    0.0045931735827084,
    5.70360843270715e-05,
   -0.0004593294204519,
};

static coeff_t symmlet20_highpass_analysis_coeffs[] = {
   -0.0004593294204519,
   -5.70360843270715e-05,
    0.0045931735827084,
    0.0008043589343686,
   -0.0203549397996833,
   -0.0057649120443445,
    0.0499949720686861,
    0.0319900568214638,
   -0.0355367402980268,
   -0.3838267611450020,
    0.7695100368531890,
   -0.4716906667438780,
   -0.0708805357960178,
    0.1594942788241300,
    0.0116098939105411,
   -0.0459272392141469,
   -0.0014653825830397,
    0.0086412992741304,
    9.56326707637102e-05,
   -0.0007701598089417,
};

static coeff_t symmlet20_lowpass_synthesis_coeffs[] = {
   -0.0004593294204519,
    5.70360843270715e-05,
    0.0045931735827084,
   -0.0008043589343686,
   -0.0203549397996833,
    0.0057649120443445,
    0.0499949720686861,
   -0.0319900568214638,
   -0.0355367402980268,
    0.3838267611450020,
    0.7695100368531890,
    0.4716906667438780,
   -0.0708805357960178,
   -0.1594942788241300,
    0.0116098939105411,
    0.0459272392141469,
   -0.0014653825830397,
   -0.0086412992741304,
    9.56326707637102e-05,
    0.0007701598089417,
};

static coeff_t symmlet20_highpass_synthesis_coeffs[] = {
   -0.0007701598089417,
    9.56326707637102e-05,
    0.0086412992741304,
   -0.0014653825830397,
   -0.0459272392141469,
    0.0116098939105411,
    0.1594942788241300,
   -0.0708805357960178,
   -0.4716906667438780,
    0.7695100368531890,
   -0.3838267611450020,
   -0.0355367402980268,
    0.0319900568214638,
    0.0499949720686861,
   -0.0057649120443445,
   -0.0203549397996833,
    0.0008043589343686,
    0.0045931735827084,
   -5.70360843270715e-05,
   -0.0004593294204519,
};

static filter_t symmlet20_lowpass_analysis = {
    20,
    ANTICAUSAL,
    LOWPASS_ANALYSIS,
    symmlet20_lowpass_analysis_coeffs,
};

static filter_t symmlet20_highpass_analysis = {
    20,
    ANTICAUSAL,
    HIGHPASS_ANALYSIS,
    symmlet20_highpass_analysis_coeffs,
};

static filter_t symmlet20_lowpass_synthesis = {
    20,
    CAUSAL,
    LOWPASS_SYNTHESIS,
    symmlet20_lowpass_synthesis_coeffs,
};

static filter_t symmlet20_highpass_synthesis = {
    20,
    CAUSAL,
    HIGHPASS_SYNTHESIS,
    symmlet20_highpass_synthesis_coeffs,
};

static filterbank_t symmlet20 = {
   "symmlet20",
   "Symmlet S20",
    ORTHOGONAL,
    &symmlet20_lowpass_analysis,
    &symmlet20_highpass_analysis,
    &symmlet20_lowpass_synthesis,
    &symmlet20_highpass_synthesis,
};

/* Odegard's 9/7 filter. */

static coeff_t odegard97_lowpass_analysis_coeffs[] = {
    0.7875137715277921,
    0.3869718638726204,
   -0.0930692637035827,
   -0.0334184732793468,
    0.0528657685329605,
};

static coeff_t odegard97_highpass_analysis_coeffs[] = {
   -0.8167806349921064,
    0.4403017067249854,
    0.0548369269027794,
   -0.0867483161317116,
};

static coeff_t odegard97_lowpass_synthesis_coeffs[] = {
    0.8167806349921064,
    0.4403017067249854,
   -0.0548369269027794,
   -0.0867483161317116,
};

static coeff_t odegard97_highpass_synthesis_coeffs[] = {
   -0.7875137715277921,
    0.3869718638726204,
    0.0930692637035827,
   -0.0334184732793468,
   -0.0528657685329605,
};

static filter_t odegard97_lowpass_analysis = {
    5,
    SYMMETRIC_WHOLE,
    LOWPASS_ANALYSIS,
    odegard97_lowpass_analysis_coeffs,
};

static filter_t odegard97_highpass_analysis = {
    4,
    SYMMETRIC_WHOLE,
    HIGHPASS_ANALYSIS,
    odegard97_highpass_analysis_coeffs,
};

static filter_t odegard97_lowpass_synthesis = {
    4,
    SYMMETRIC_WHOLE,
    LOWPASS_SYNTHESIS,
    odegard97_lowpass_synthesis_coeffs,
};

static filter_t odegard97_highpass_synthesis = {
    5,
    SYMMETRIC_WHOLE,
    HIGHPASS_SYNTHESIS,
    odegard97_highpass_synthesis_coeffs,
};

static filterbank_t odegard97 = {
   "odegard97",
   "Odegard 9/7",
    BIORTHOGONAL,
    &odegard97_lowpass_analysis,
    &odegard97_highpass_analysis,
    &odegard97_lowpass_synthesis,
    &odegard97_highpass_synthesis,
};

/* 9/7 filter from M. Antonini, M. Barlaud, P. Mathieu, and
 * I. Daubechies, "Image coding using wavelet transform", IEEE
 * Transactions on Image Processing, Vol. pp. 205-220, 1992. */

static coeff_t daub97_lowpass_analysis_coeffs[] = {
    0.8526986790088938,
    0.3774028556128306,
   -0.1106244044184372,
   -0.0238494650195568,
    0.0378284555072640,
};

static coeff_t daub97_highpass_analysis_coeffs[] = {
   -0.7884856164063712,
    0.4180922732220353,
    0.0406894176092047,
   -0.0645388826287616,
};

static coeff_t daub97_lowpass_synthesis_coeffs[] = {
    0.7884856164063712,
    0.4180922732220353,
   -0.0406894176092047,
   -0.0645388826287616,
};

static coeff_t daub97_highpass_synthesis_coeffs[] = {
   -0.8526986790088938,
    0.3774028556128306,
    0.1106244044184372,
   -0.0238494650195568,
   -0.0378284555072640,
};

static filter_t daub97_lowpass_analysis = {
    5,
    SYMMETRIC_WHOLE,
    LOWPASS_ANALYSIS,
    daub97_lowpass_analysis_coeffs,
};

static filter_t daub97_highpass_analysis = {
    4,
    SYMMETRIC_WHOLE,
    HIGHPASS_ANALYSIS,
    daub97_highpass_analysis_coeffs,
};

static filter_t daub97_lowpass_synthesis = {
    4,
    SYMMETRIC_WHOLE,
    LOWPASS_SYNTHESIS,
    daub97_lowpass_synthesis_coeffs,
};

static filter_t daub97_highpass_synthesis = {
    5,
    SYMMETRIC_WHOLE,
    HIGHPASS_SYNTHESIS,
    daub97_highpass_synthesis_coeffs,
};

static filterbank_t daub97 = {
   "daub97",
   "Daubechies 9/7",
    BIORTHOGONAL,
    &daub97_lowpass_analysis,
    &daub97_highpass_analysis,
    &daub97_lowpass_synthesis,
    &daub97_highpass_synthesis,
};

static filterbank_t daub97lift = {
   "daub97lift",
   "Daubechies 9/7 (Lifting)",
    BIORTHOGONAL,
    NULL,
    NULL,
    NULL,
    NULL,
};

/* A. Cohen, I. Daubechies, J. C. Feauveau, "Biorthogonal Bases of
 * Compactly Supported Wavelets," Communications on Pure and
 * Applied Mathematics, vol. 45, no. 5, pp. 485-560, May 1992. */

static coeff_t cdf53_lowpass_analysis_coeffs[] = {
    1.06066017177982,
    0.35355339059327,
   -0.17677669529664,
};

static coeff_t cdf53_highpass_analysis_coeffs[] = {
   -0.70710678118655,
    0.35355339059327,
};

static coeff_t cdf53_lowpass_synthesis_coeffs[] = {
    0.70710678118655,
    0.35355339059327,
};

static coeff_t cdf53_highpass_synthesis_coeffs[] = {
   -1.06066017177982,
    0.35355339059327,
    0.17677669529664,
};

static filter_t cdf53_lowpass_analysis = {
    3,
    SYMMETRIC_WHOLE,
    LOWPASS_ANALYSIS,
    cdf53_lowpass_analysis_coeffs,
};

static filter_t cdf53_highpass_analysis = {
    2,
    SYMMETRIC_WHOLE,
    HIGHPASS_ANALYSIS,
    cdf53_highpass_analysis_coeffs,
};

static filter_t cdf53_lowpass_synthesis = {
    2,
    SYMMETRIC_WHOLE,
    LOWPASS_SYNTHESIS,
    cdf53_lowpass_synthesis_coeffs,
};

static filter_t cdf53_highpass_synthesis = {
    3,
    SYMMETRIC_WHOLE,
    HIGHPASS_SYNTHESIS,
    cdf53_highpass_synthesis_coeffs,
};

static filterbank_t cdf53 = {
   "cdf53",
   "Cohen Daubechies Feauveau 5/3",
    BIORTHOGONAL,
    &cdf53_lowpass_analysis,
    &cdf53_highpass_analysis,
    &cdf53_lowpass_synthesis,
    &cdf53_highpass_synthesis,
};

/* A. Cohen, I. Daubechies, J. C. Feauveau, "Biorthogonal Bases of
 * Compactly Supported Wavelets," Communications on Pure and
 * Applied Mathematics, vol. 45, no. 5, pp. 485-560, May 1992. */

static coeff_t cdf93_lowpass_analysis_coeffs[] = {
    0.99436891104360,
    0.41984465132952,
   -0.17677669529665,
   -0.06629126073624,
    0.03314563036812,
};

static coeff_t cdf93_highpass_analysis_coeffs[] = {
   -0.70710678118655,
    0.35355339059327,
};

static coeff_t cdf93_lowpass_synthesis_coeffs[] = {
    0.70710678118655,
    0.35355339059327,
};

static coeff_t cdf93_highpass_synthesis_coeffs[] = {
   -0.99436891104360,
    0.41984465132952,
    0.17677669529665,
   -0.06629126073624,
   -0.03314563036812,
};

static filter_t cdf93_lowpass_analysis = {
    5,
    SYMMETRIC_WHOLE,
    LOWPASS_ANALYSIS,
    cdf93_lowpass_analysis_coeffs,
};

static filter_t cdf93_highpass_analysis = {
    2,
    SYMMETRIC_WHOLE,
    HIGHPASS_ANALYSIS,
    cdf93_highpass_analysis_coeffs,
};

static filter_t cdf93_lowpass_synthesis = {
    2,
    SYMMETRIC_WHOLE,
    LOWPASS_SYNTHESIS,
    cdf93_lowpass_synthesis_coeffs,
};

static filter_t cdf93_highpass_synthesis = {
    5,
    SYMMETRIC_WHOLE,
    HIGHPASS_SYNTHESIS,
    cdf93_highpass_synthesis_coeffs,
};

static filterbank_t cdf93 = {
   "cdf93",
   "Cohen Daubechies Feauveau 9/3",
    BIORTHOGONAL,
    &cdf93_lowpass_analysis,
    &cdf93_highpass_analysis,
    &cdf93_lowpass_synthesis,
    &cdf93_highpass_synthesis,
};

/* A. Cohen, I. Daubechies, J. C. Feauveau, "Biorthogonal Bases of
 * Compactly Supported Wavelets," Communications on Pure and
 * Applied Mathematics, vol. 45, no. 5, pp. 485-560, May 1992. */

static coeff_t cdf133_lowpass_analysis_coeffs[] = {
    0.96674755240348,
    0.44746600996961,
   -0.16987135563661,
   -0.10772329869638,
    0.04695630968816,
    0.01381067932004,
   -0.00690533966002,
};

static coeff_t cdf133_highpass_analysis_coeffs[] = {
   -0.70710678118655,
    0.35355339059327,
};

static coeff_t cdf133_lowpass_synthesis_coeffs[] = {
    0.70710678118655,
    0.35355339059327,
};

static coeff_t cdf133_highpass_synthesis_coeffs[] = {
   -0.96674755240348,
    0.44746600996961,
    0.16987135563661,
   -0.10772329869638,
   -0.04695630968816,
    0.01381067932004,
    0.00690533966002,
};

static filter_t cdf133_lowpass_analysis = {
    7,
    SYMMETRIC_WHOLE,
    LOWPASS_ANALYSIS,
    cdf133_lowpass_analysis_coeffs,
};

static filter_t cdf133_highpass_analysis = {
    2,
    SYMMETRIC_WHOLE,
    HIGHPASS_ANALYSIS,
    cdf133_highpass_analysis_coeffs,
};

static filter_t cdf133_lowpass_synthesis = {
    2,
    SYMMETRIC_WHOLE,
    LOWPASS_SYNTHESIS,
    cdf133_lowpass_synthesis_coeffs,
};

static filter_t cdf133_highpass_synthesis = {
    7,
    SYMMETRIC_WHOLE,
    HIGHPASS_SYNTHESIS,
    cdf133_highpass_synthesis_coeffs,
};

static filterbank_t cdf133 = {
   "cdf133",
   "Cohen Daubechies Feauveau 13/3",
    BIORTHOGONAL,
    &cdf133_lowpass_analysis,
    &cdf133_highpass_analysis,
    &cdf133_lowpass_synthesis,
    &cdf133_highpass_synthesis,
};

/* A. Cohen, I. Daubechies, J. C. Feauveau, "Biorthogonal Bases of
 * Compactly Supported Wavelets," Communications on Pure and
 * Applied Mathematics, vol. 45, no. 5, pp. 485-560, May 1992. */

static coeff_t cdf173_lowpass_analysis_coeffs[] = {
    0.95164212189717,
    0.46257144047591,
   -0.16382918343409,
   -0.13491307360773,
    0.05299848189069,
    0.02891610982635,
   -0.01294751186254,
   -0.00302108610126,
    0.00151054305063,
};

static coeff_t cdf173_highpass_analysis_coeffs[] = {
   -0.70710678118655,
    0.35355339059327,
};

static coeff_t cdf173_lowpass_synthesis_coeffs[] = {
    0.70710678118655,
    0.35355339059327,
};

static coeff_t cdf173_highpass_synthesis_coeffs[] = {
   -0.95164212189717,
    0.46257144047591,
    0.16382918343409,
   -0.13491307360773,
   -0.05299848189069,
    0.02891610982635,
    0.01294751186254,
   -0.00302108610126,
   -0.00151054305063,
};

static filter_t cdf173_lowpass_analysis = {
    9,
    SYMMETRIC_WHOLE,
    LOWPASS_ANALYSIS,
    cdf173_lowpass_analysis_coeffs,
};

static filter_t cdf173_highpass_analysis = {
    2,
    SYMMETRIC_WHOLE,
    HIGHPASS_ANALYSIS,
    cdf173_highpass_analysis_coeffs,
};

static filter_t cdf173_lowpass_synthesis = {
    2,
    SYMMETRIC_WHOLE,
    LOWPASS_SYNTHESIS,
    cdf173_lowpass_synthesis_coeffs,
};

static filter_t cdf173_highpass_synthesis = {
    9,
    SYMMETRIC_WHOLE,
    HIGHPASS_SYNTHESIS,
    cdf173_highpass_synthesis_coeffs,
};

static filterbank_t cdf173 = {
   "cdf173",
   "Cohen Daubechies Feauveau 17/3",
    BIORTHOGONAL,
    &cdf173_lowpass_analysis,
    &cdf173_highpass_analysis,
    &cdf173_lowpass_synthesis,
    &cdf173_highpass_synthesis,
};

/* Filter from J. Villasenor, B. Belzer, J. Liao, "Wavelet Filter
 * Evaluation for Image Compression." IEEE Transactions on Image
 * Processing, Vol. 2, pp. 1053-1060, August 1995. */

static coeff_t villa1311_lowpass_analysis_coeffs[] = {
    0.7672451593927493,
    0.3832692613243884,
   -0.0688781141906103,
   -0.0334750810478015,
    0.0472817528288275,
    0.0037592103166868,
   -0.0084728277413181,
};

static coeff_t villa1311_highpass_analysis_coeffs[] = {
   -0.8328475700934288,
    0.4481085999263908,
    0.0691627101203004,
   -0.1087373652243805,
   -0.0062923156668598,
    0.0141821558912635,
};

static coeff_t villa1311_lowpass_synthesis_coeffs[] = {
    0.8328475700934288,
    0.4481085999263908,
   -0.0691627101203004,
   -0.1087373652243805,
    0.0062923156668598,
    0.0141821558912635,
};

static coeff_t villa1311_highpass_synthesis_coeffs[] = {
   -0.7672451593927493,
    0.3832692613243884,
    0.0688781141906103,
   -0.0334750810478015,
   -0.0472817528288275,
    0.0037592103166868,
    0.0084728277413181,
};

static filter_t villa1311_lowpass_analysis = {
    7,
    SYMMETRIC_WHOLE,
    LOWPASS_ANALYSIS,
    villa1311_lowpass_analysis_coeffs,
};

static filter_t villa1311_highpass_analysis = {
    6,
    SYMMETRIC_WHOLE,
    HIGHPASS_ANALYSIS,
    villa1311_highpass_analysis_coeffs,
};

static filter_t villa1311_lowpass_synthesis = {
    6,
    SYMMETRIC_WHOLE,
    LOWPASS_SYNTHESIS,
    villa1311_lowpass_synthesis_coeffs,
};

static filter_t villa1311_highpass_synthesis = {
    7,
    SYMMETRIC_WHOLE,
    HIGHPASS_SYNTHESIS,
    villa1311_highpass_synthesis_coeffs,
};

static filterbank_t villa1311 = {
   "villa1311",
   "Villasenor 13/11",
    BIORTHOGONAL,
    &villa1311_lowpass_analysis,
    &villa1311_highpass_analysis,
    &villa1311_lowpass_synthesis,
    &villa1311_highpass_synthesis,
};

/* Append filterbank reference to this array: this is the only
 * thing you need to do by hand. The rest of the work will be done
 * by the make_filterbank.pl utility: just cut-and-paste its output. */

filterbank_t *filterbanks[] = {
    &haar,
    &daub4,
    &daub6,
    &daub8,
    &daub10,
    &daub12,
    &daub14,
    &daub16,
    &daub18,
    &daub20,
    &beylkin,
    &vaidyanathan,
    &coiflet6,
    &coiflet12,
    &coiflet18,
    &coiflet24,
    &coiflet30,
    &symmlet8,
    &symmlet10,
    &symmlet12,
    &symmlet14,
    &symmlet16,
    &symmlet18,
    &symmlet20,
    &odegard97,
    &daub97,
    &daub97lift,
    &cdf53,
    &cdf93,
    &cdf133,
    &cdf173,
    &villa1311,
    NULL
};
