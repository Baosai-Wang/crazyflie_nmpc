/**************************************************************************************************
*                                                                                                 *
* This file is part of BLASFEO.                                                                   *
*                                                                                                 *
* BLASFEO -- BLAS for embedded optimization.                                                      *
* Copyright (C) 2019 by Gianluca Frison.                                                          *
* Developed at IMTEK (University of Freiburg) under the supervision of Moritz Diehl.              *
* All rights reserved.                                                                            *
*                                                                                                 *
* The 2-Clause BSD License                                                                        *
*                                                                                                 *
* Redistribution and use in source and binary forms, with or without                              *
* modification, are permitted provided that the following conditions are met:                     *
*                                                                                                 *
* 1. Redistributions of source code must retain the above copyright notice, this                  *
*    list of conditions and the following disclaimer.                                             *
* 2. Redistributions in binary form must reproduce the above copyright notice,                    *
*    this list of conditions and the following disclaimer in the documentation                    *
*    and/or other materials provided with the distribution.                                       *
*                                                                                                 *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND                 *
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                   *
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                          *
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR                 *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES                  *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;                    *
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND                     *
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT                      *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS                   *
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                    *
*                                                                                                 *
* Author: Gianluca Frison, gianluca.frison (at) imtek.uni-freiburg.de                             *
*                                                                                                 *
**************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../include/blasfeo_common.h"
#include "../include/blasfeo_s_aux.h"
#include "../include/blasfeo_s_kernel.h"



/****************************
* old interface
****************************/

void ssyrk_spotrf_nt_l_lib(int m, int n, int k, float *pA, int sda, float *pB, int sdb, float *pC, int sdc, float *pD, int sdd, float *inv_diag_D)
	{

	if(m<=0 || n<=0)
		return;

	int alg = 1; // XXX

	const int bs = 4;

	int i, j, l;

	i = 0;

	for(; i<m-3; i+=4)
		{
		j = 0;
		for(; j<i && j<n-3; j+=4)
			{
			kernel_sgemm_strsm_nt_rl_inv_4x4_lib4(k, &pA[i*sda], &pB[j*sdb], j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+i*sdc], &pD[j*bs+i*sdd], &pD[j*bs+j*sdd], &inv_diag_D[j]);
			}
		if(j<n)
			{
			if(j<i) // dgemm
				{
				kernel_sgemm_strsm_nt_rl_inv_4x4_vs_lib4(k, &pA[i*sda], &pB[j*sdb], j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+i*sdc], &pD[j*bs+i*sdd], &pD[j*bs+j*sdd], &inv_diag_D[j], m-i, n-j);
				}
			else // dsyrk
				{
				if(j<n-3)
					{
					kernel_ssyrk_spotrf_nt_l_4x4_lib4(k, &pA[i*sda], &pB[j*sdb], j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+j*sdc], &pD[j*bs+j*sdd], &inv_diag_D[j]);
					}
				else
					{
					kernel_ssyrk_spotrf_nt_l_4x4_vs_lib4(k, &pA[i*sda], &pB[j*sdb], j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+j*sdc], &pD[j*bs+j*sdd], &inv_diag_D[j], m-i, n-j);
					}
				}
			}
		}
	if(m>i)
		{
		goto left_4;
		}

	// common return if i==m
	return;

	// clean up loops definitions

	left_4:
	j = 0;
	for(; j<i && j<n-3; j+=4)
		{
		kernel_sgemm_strsm_nt_rl_inv_4x4_vs_lib4(k, &pA[i*sda], &pB[j*sdb], j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+i*sdc], &pD[j*bs+i*sdd], &pD[j*bs+j*sdd], &inv_diag_D[j], m-i, n-j);
		}
	if(j<n)
		{
		if(j<i) // dgemm
			{
			kernel_sgemm_strsm_nt_rl_inv_4x4_vs_lib4(k, &pA[i*sda], &pB[j*sdb], j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+i*sdc], &pD[j*bs+i*sdd], &pD[j*bs+j*sdd], &inv_diag_D[j], m-i, n-j);
			}
		else // dsyrk
			{
			kernel_ssyrk_spotrf_nt_l_4x4_vs_lib4(k, &pA[i*sda], &pB[j*sdb], j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+j*sdc], &pD[j*bs+j*sdd], &inv_diag_D[j], m-i, n-j);
			}
		}
	return;

	}



void sgetrf_nn_nopivot_lib(int m, int n, float *pC, int sdc, float *pD, int sdd, float *inv_diag_D)
	{

	if(m<=0 || n<=0)
		return;
	
	const int bs = 4;

	int ii, jj, ie;

	// main loop
	ii = 0;
	for( ; ii<m-3; ii+=4)
		{
		jj = 0;
		// solve lower
		ie = n<ii ? n : ii; // ie is multiple of 4
		for( ; jj<ie-3; jj+=4)
			{
			kernel_strsm_nn_ru_inv_4x4_lib4(jj, &pD[ii*sdd], &pD[jj*bs], sdd, &pC[jj*bs+ii*sdc], &pD[jj*bs+ii*sdd], &pD[jj*bs+jj*sdd], &inv_diag_D[jj]);
			}
		if(jj<ie)
			{
			kernel_strsm_nn_ru_inv_4x4_vs_lib4(jj, &pD[ii*sdd], &pD[jj*bs], sdd, &pC[jj*bs+ii*sdc], &pD[jj*bs+ii*sdd], &pD[jj*bs+jj*sdd], &inv_diag_D[jj], m-ii, ie-jj);
			jj+=4;
			}
		// factorize
		if(jj<n-3)
			{
			kernel_sgetrf_nn_4x4_lib4(jj, &pD[ii*sdd], &pD[jj*bs], sdd, &pC[jj*bs+ii*sdc], &pD[jj*bs+ii*sdd], &inv_diag_D[jj]);
			jj+=4;
			}
		else if(jj<n)
			{
			kernel_sgetrf_nn_4x4_vs_lib4(jj, &pD[ii*sdd], &pD[jj*bs], sdd, &pC[jj*bs+ii*sdc], &pD[jj*bs+ii*sdd], &inv_diag_D[jj], m-ii, n-jj);
			jj+=4;
			}
		// solve upper 
		for( ; jj<n-3; jj+=4)
			{
			kernel_strsm_nn_ll_one_4x4_lib4(ii, &pD[ii*sdd], &pD[jj*bs], sdd, &pC[jj*bs+ii*sdc], &pD[jj*bs+ii*sdd], &pD[ii*bs+ii*sdd]);
			}
		if(jj<n)
			{
			kernel_strsm_nn_ll_one_4x4_vs_lib4(ii, &pD[ii*sdd], &pD[jj*bs], sdd, &pC[jj*bs+ii*sdc], &pD[jj*bs+ii*sdd], &pD[ii*bs+ii*sdd], m-ii, n-jj);
			}
		}
	if(m>ii)
		{
		goto left_4;
		}

	// common return if i==m
	return;

	left_4:
	jj = 0;
	// solve lower
	ie = n<ii ? n : ii; // ie is multiple of 4
	for( ; jj<ie; jj+=4)
		{
		kernel_strsm_nn_ru_inv_4x4_vs_lib4(jj, &pD[ii*sdd], &pD[jj*bs], sdd, &pC[jj*bs+ii*sdc], &pD[jj*bs+ii*sdd], &pD[jj*bs+jj*sdd], &inv_diag_D[jj], m-ii, ie-jj);
		}
	// factorize
	if(jj<n)
		{
		kernel_sgetrf_nn_4x4_vs_lib4(jj, &pD[ii*sdd], &pD[jj*bs], sdd, &pC[jj*bs+ii*sdc], &pD[jj*bs+ii*sdd], &inv_diag_D[jj], m-ii, n-jj);
		jj+=4;
		}
	// solve upper 
	for( ; jj<n; jj+=4)
		{
		kernel_strsm_nn_ll_one_4x4_vs_lib4(ii, &pD[ii*sdd], &pD[jj*bs], sdd, &pC[jj*bs+ii*sdc], &pD[jj*bs+ii*sdd], &pD[ii*bs+ii*sdd], m-ii, n-jj);
		}
	return;

	}



void sgetrf_nn_lib(int m, int n, float *pC, int sdc, float *pD, int sdd, float *inv_diag_D, int *ipiv)
	{

	if(m<=0)
		return;
	
	const int bs = 4;

	int ii, jj, i0, i1, j0, ll, p;

	float d1 = 1.0;
	float dm1 = -1.0;

//	// needs to perform row-excanges on the yet-to-be-factorized matrix too
//	if(pC!=pD)
//		sgecp_lib(m, n, 1.0, 0, pC, sdc, 0, pD, sdd);

	// minimum matrix size
	p = n<m ? n : m; // XXX

	// main loop
	// 4 columns at a time
	jj = 0;
	for(; jj<p-3; jj+=4) // XXX
		{
		// pivot & factorize & solve lower
		ii = jj;
		i0 = ii;
		for( ; ii<m-3; ii+=4)
			{
			kernel_sgemm_nn_4x4_lib4(jj, &dm1, &pD[ii*sdd], 0, &pD[jj*bs], sdd, &d1, &pD[jj*bs+ii*sdd], &pD[jj*bs+ii*sdd]);
			}
		if(m-ii>0)
			{
			kernel_sgemm_nn_4x4_vs_lib4(jj, &dm1, &pD[ii*sdd], 0, &pD[jj*bs], sdd, &d1, &pD[jj*bs+ii*sdd], &pD[jj*bs+ii*sdd], m-ii, 4);
			}
		kernel_sgetrf_pivot_4_lib4(m-i0, &pD[jj*bs+i0*sdd], sdd, &inv_diag_D[jj], &ipiv[i0]);
		ipiv[i0+0] += i0;
		if(ipiv[i0+0]!=i0+0)
			{
			srowsw_lib(jj, pD+(i0+0)/bs*bs*sdd+(i0+0)%bs, pD+(ipiv[i0+0])/bs*bs*sdd+(ipiv[i0+0])%bs);
			srowsw_lib(n-jj-4, pD+(i0+0)/bs*bs*sdd+(i0+0)%bs+(jj+4)*bs, pD+(ipiv[i0+0])/bs*bs*sdd+(ipiv[i0+0])%bs+(jj+4)*bs);
			}
		ipiv[i0+1] += i0;
		if(ipiv[i0+1]!=i0+1)
			{
			srowsw_lib(jj, pD+(i0+1)/bs*bs*sdd+(i0+1)%bs, pD+(ipiv[i0+1])/bs*bs*sdd+(ipiv[i0+1])%bs);
			srowsw_lib(n-jj-4, pD+(i0+1)/bs*bs*sdd+(i0+1)%bs+(jj+4)*bs, pD+(ipiv[i0+1])/bs*bs*sdd+(ipiv[i0+1])%bs+(jj+4)*bs);
			}
		ipiv[i0+2] += i0;
		if(ipiv[i0+2]!=i0+2)
			{
			srowsw_lib(jj, pD+(i0+2)/bs*bs*sdd+(i0+2)%bs, pD+(ipiv[i0+2])/bs*bs*sdd+(ipiv[i0+2])%bs);
			srowsw_lib(n-jj-4, pD+(i0+2)/bs*bs*sdd+(i0+2)%bs+(jj+4)*bs, pD+(ipiv[i0+2])/bs*bs*sdd+(ipiv[i0+2])%bs+(jj+4)*bs);
			}
		ipiv[i0+3] += i0;
		if(ipiv[i0+3]!=i0+3)
			{
			srowsw_lib(jj, pD+(i0+3)/bs*bs*sdd+(i0+3)%bs, pD+(ipiv[i0+3])/bs*bs*sdd+(ipiv[i0+3])%bs);
			srowsw_lib(n-jj-4, pD+(i0+3)/bs*bs*sdd+(i0+3)%bs+(jj+4)*bs, pD+(ipiv[i0+3])/bs*bs*sdd+(ipiv[i0+3])%bs+(jj+4)*bs);
			}

		// solve upper
		ll = jj+4;
		for( ; ll<n-3; ll+=4)
			{
			kernel_strsm_nn_ll_one_4x4_lib4(i0, &pD[i0*sdd], &pD[ll*bs], sdd, &pD[ll*bs+i0*sdd], &pD[ll*bs+i0*sdd], &pD[i0*bs+i0*sdd]);
			}
		if(n-ll>0)
			{
			kernel_strsm_nn_ll_one_4x4_vs_lib4(i0, &pD[i0*sdd], &pD[ll*bs], sdd, &pD[ll*bs+i0*sdd], &pD[ll*bs+i0*sdd], &pD[i0*bs+i0*sdd], 4, n-ll);
			}
		}
	if(m>=n)
		{
		if(n-jj>0)
			{
			goto left_n_4;
			}
		}
	else
		{
		if(m-jj>0)
			{
			goto left_m_4;
			}
		}

	// common return if jj==n
	return;

	// clean up

	left_n_4:
	// 1-4 columns at a time
	// pivot & factorize & solve lower
	ii = jj;
	i0 = ii;
	for( ; ii<m; ii+=4)
		{
		kernel_sgemm_nn_4x4_vs_lib4(jj, &dm1, &pD[ii*sdd], 0, &pD[jj*bs], sdd, &d1, &pD[jj*bs+ii*sdd], &pD[jj*bs+ii*sdd], m-ii, n-jj);
		}
	kernel_sgetrf_pivot_4_vs_lib4(m-i0, n-jj, &pD[jj*bs+i0*sdd], sdd, &inv_diag_D[jj], &ipiv[i0]);
	ipiv[i0+0] += i0;
	if(ipiv[i0+0]!=i0+0)
		{
		srowsw_lib(jj, pD+(i0+0)/bs*bs*sdd+(i0+0)%bs, pD+(ipiv[i0+0])/bs*bs*sdd+(ipiv[i0+0])%bs);
		srowsw_lib(n-jj-4, pD+(i0+0)/bs*bs*sdd+(i0+0)%bs+(jj+4)*bs, pD+(ipiv[i0+0])/bs*bs*sdd+(ipiv[i0+0])%bs+(jj+4)*bs);
		}
	if(n-jj>1)
		{
		ipiv[i0+1] += i0;
		if(ipiv[i0+1]!=i0+1)
			{
			srowsw_lib(jj, pD+(i0+1)/bs*bs*sdd+(i0+1)%bs, pD+(ipiv[i0+1])/bs*bs*sdd+(ipiv[i0+1])%bs);
			srowsw_lib(n-jj-4, pD+(i0+1)/bs*bs*sdd+(i0+1)%bs+(jj+4)*bs, pD+(ipiv[i0+1])/bs*bs*sdd+(ipiv[i0+1])%bs+(jj+4)*bs);
			}
		if(n-jj>2)
			{
			ipiv[i0+2] += i0;
			if(ipiv[i0+2]!=i0+2)
				{
				srowsw_lib(jj, pD+(i0+2)/bs*bs*sdd+(i0+2)%bs, pD+(ipiv[i0+2])/bs*bs*sdd+(ipiv[i0+2])%bs);
				srowsw_lib(n-jj-4, pD+(i0+2)/bs*bs*sdd+(i0+2)%bs+(jj+4)*bs, pD+(ipiv[i0+2])/bs*bs*sdd+(ipiv[i0+2])%bs+(jj+4)*bs);
				}
			if(n-jj>3)
				{
				ipiv[i0+3] += i0;
				if(ipiv[i0+3]!=i0+3)
					{
					srowsw_lib(jj, pD+(i0+3)/bs*bs*sdd+(i0+3)%bs, pD+(ipiv[i0+3])/bs*bs*sdd+(ipiv[i0+3])%bs);
					srowsw_lib(n-jj-4, pD+(i0+3)/bs*bs*sdd+(i0+3)%bs+(jj+4)*bs, pD+(ipiv[i0+3])/bs*bs*sdd+(ipiv[i0+3])%bs+(jj+4)*bs);
					}
				}
			}
		}

	// solve upper
	if(0) // there is no upper
		{
		ll = jj+4;
		for( ; ll<n; ll+=4)
			{
			kernel_strsm_nn_ll_one_4x4_vs_lib4(i0, &pD[i0*sdd], &pD[ll*bs], sdd, &pD[ll*bs+i0*sdd], &pD[ll*bs+i0*sdd], &pD[i0*bs+i0*sdd], m-i0, n-ll);
			}
		}
	return;


	left_m_4:
	// 1-4 rows at a time
	// pivot & factorize & solve lower
	ii = jj;
	i0 = ii;
	kernel_sgemm_nn_4x4_vs_lib4(jj, &dm1, &pD[ii*sdd], 0, &pD[jj*bs], sdd, &d1, &pD[jj*bs+ii*sdd], &pD[jj*bs+ii*sdd], m-ii, n-jj);
	kernel_sgetrf_pivot_4_vs_lib4(m-i0, n-jj, &pD[jj*bs+i0*sdd], sdd, &inv_diag_D[jj], &ipiv[i0]);
	ipiv[i0+0] += i0;
	if(ipiv[i0+0]!=i0+0)
		{
		srowsw_lib(jj, pD+(i0+0)/bs*bs*sdd+(i0+0)%bs, pD+(ipiv[i0+0])/bs*bs*sdd+(ipiv[i0+0])%bs);
		srowsw_lib(n-jj-4, pD+(i0+0)/bs*bs*sdd+(i0+0)%bs+(jj+4)*bs, pD+(ipiv[i0+0])/bs*bs*sdd+(ipiv[i0+0])%bs+(jj+4)*bs);
		}
	if(m-i0>1)
		{
		ipiv[i0+1] += i0;
		if(ipiv[i0+1]!=i0+1)
			{
			srowsw_lib(jj, pD+(i0+1)/bs*bs*sdd+(i0+1)%bs, pD+(ipiv[i0+1])/bs*bs*sdd+(ipiv[i0+1])%bs);
			srowsw_lib(n-jj-4, pD+(i0+1)/bs*bs*sdd+(i0+1)%bs+(jj+4)*bs, pD+(ipiv[i0+1])/bs*bs*sdd+(ipiv[i0+1])%bs+(jj+4)*bs);
			}
		if(m-i0>2)
			{
			ipiv[i0+2] += i0;
			if(ipiv[i0+2]!=i0+2)
				{
				srowsw_lib(jj, pD+(i0+2)/bs*bs*sdd+(i0+2)%bs, pD+(ipiv[i0+2])/bs*bs*sdd+(ipiv[i0+2])%bs);
				srowsw_lib(n-jj-4, pD+(i0+2)/bs*bs*sdd+(i0+2)%bs+(jj+4)*bs, pD+(ipiv[i0+2])/bs*bs*sdd+(ipiv[i0+2])%bs+(jj+4)*bs);
				}
			if(m-i0>3)
				{
				ipiv[i0+3] += i0;
				if(ipiv[i0+3]!=i0+3)
					{
					srowsw_lib(jj, pD+(i0+3)/bs*bs*sdd+(i0+3)%bs, pD+(ipiv[i0+3])/bs*bs*sdd+(ipiv[i0+3])%bs);
					srowsw_lib(n-jj-4, pD+(i0+3)/bs*bs*sdd+(i0+3)%bs+(jj+4)*bs, pD+(ipiv[i0+3])/bs*bs*sdd+(ipiv[i0+3])%bs+(jj+4)*bs);
					}
				}
			}
		}

	// solve upper
	ll = jj+4;
	for( ; ll<n; ll+=4)
		{
		kernel_strsm_nn_ll_one_4x4_vs_lib4(i0, &pD[i0*sdd], &pD[ll*bs], sdd, &pD[ll*bs+i0*sdd], &pD[ll*bs+i0*sdd], &pD[i0*bs+i0*sdd], m-i0, n-ll);
		}
	return;

	}



/****************************
* new interface
****************************/



#if defined(LA_HIGH_PERFORMANCE)



// dpotrf
void blasfeo_spotrf_l(int m, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj)
	{

	if(m<=0)
		return;

	if(ci!=0 | di!=0)
		{
		printf("\nblasfeo_spotrf_l: feature not implemented yet: ci=%d, di=%d\n", ci, di);
		exit(1);
		}

	const int ps = 4;

	int sdc = sC->cn;
	int sdd = sD->cn;
	float *pC = sC->pA + cj*ps;
	float *pD = sD->pA + dj*ps;
	float *dD = sD->dA;
	if(di==0 && dj==0) // XXX what to do if di and dj are not zero
		sD->use_dA = 1;
	else
		sD->use_dA = 0;

	int i, j, l;

	i = 0;
#if defined(TARGET_ARMV8A_ARM_CORTEX_A57) || defined(TARGET_ARMV8A_ARM_CORTEX_A53)
#if 1
	for(; i<m-15; i+=16)
		{
		j = 0;
		for(; j<i; j+=4)
			{
			kernel_strsm_nt_rl_inv_16x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+i*sdc], sdc, &pD[j*ps+i*sdd], sdd, &pD[j*ps+j*sdd], &dD[j]);
			}
		kernel_spotrf_nt_l_16x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+j*sdc], sdc, &pD[j*ps+j*sdd], sdd, &dD[j]);
		kernel_spotrf_nt_l_12x4_lib4(j+4, &pD[(i+4)*sdd], sdd, &pD[(j+4)*sdd], &pC[(j+4)*ps+(j+4)*sdc], sdc, &pD[(j+4)*ps+(j+4)*sdd], sdd, &dD[j+4]);
		kernel_spotrf_nt_l_8x4_lib4(j+8, &pD[(i+8)*sdd], sdd, &pD[(j+8)*sdd], &pC[(j+8)*ps+(j+8)*sdc], sdc, &pD[(j+8)*ps+(j+8)*sdd], sdd, &dD[j+8]);
		kernel_spotrf_nt_l_4x4_lib4(j+12, &pD[(i+12)*sdd], &pD[(j+12)*sdd], &pC[(j+12)*ps+(j+12)*sdc], &pD[(j+12)*ps+(j+12)*sdd], &dD[j+12]);
		}
	if(m>i)
		{
		if(m-i<=4)
			{
			goto left_4;
			}
		else if(m-i<=8)
			{
			goto left_8;
			}
		else if(m-i<=12)
			{
			goto left_12;
			}
		else
			{
			goto left_16;
			}
		}
#elif 0
	for(; i<m-11; i+=12)
		{
		j = 0;
		for(; j<i; j+=4)
			{
			kernel_strsm_nt_rl_inv_12x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+i*sdc], sdc, &pD[j*ps+i*sdd], sdd, &pD[j*ps+j*sdd], &dD[j]);
			}
		kernel_spotrf_nt_l_12x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+j*sdc], sdc, &pD[j*ps+j*sdd], sdd, &dD[j]);
		kernel_spotrf_nt_l_8x4_lib4(j+4, &pD[(i+4)*sdd], sdd, &pD[(j+4)*sdd], &pC[(j+4)*ps+(j+4)*sdc], sdc, &pD[(j+4)*ps+(j+4)*sdd], sdd, &dD[j+4]);
		kernel_spotrf_nt_l_4x4_lib4(j+8, &pD[(i+8)*sdd], &pD[(j+8)*sdd], &pC[(j+8)*ps+(j+8)*sdc], &pD[(j+8)*ps+(j+8)*sdd], &dD[j+8]);
		}
	if(m>i)
		{
		if(m-i<=4)
			{
			goto left_4;
			}
		else if(m-i<=8)
			{
			goto left_8;
			}
		else
			{
			goto left_12;
			}
		}
#else
	for(; i<m-7; i+=8)
		{
		j = 0;
		for(; j<i; j+=4)
			{
			kernel_strsm_nt_rl_inv_8x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+i*sdc], sdc, &pD[j*ps+i*sdd], sdd, &pD[j*ps+j*sdd], &dD[j]);
			}
		kernel_spotrf_nt_l_8x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+j*sdc], sdc, &pD[j*ps+j*sdd], sdd, &dD[j]);
		kernel_spotrf_nt_l_4x4_lib4(j+4, &pD[(i+4)*sdd], &pD[(j+4)*sdd], &pC[(j+4)*ps+(j+4)*sdc], &pD[(j+4)*ps+(j+4)*sdd], &dD[j+4]);
		}
	if(m>i)
		{
		if(m-i<=4)
			{
			goto left_4;
			}
		else
			{
			goto left_8;
			}
		}
#endif
#elif defined(TARGET_ARMV7A_ARM_CORTEX_A15)
	for(; i<m-11; i+=12)
		{
		j = 0;
		for(; j<i; j+=4)
			{
			kernel_strsm_nt_rl_inv_12x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+i*sdc], sdc, &pD[j*ps+i*sdd], sdd, &pD[j*ps+j*sdd], &dD[j]);
			}
		kernel_spotrf_nt_l_12x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+j*sdc], sdc, &pD[j*ps+j*sdd], sdd, &dD[j]);
		kernel_spotrf_nt_l_8x4_lib4(j+4, &pD[(i+4)*sdd], sdd, &pD[(j+4)*sdd], &pC[(j+4)*ps+(j+4)*sdc], sdc, &pD[(j+4)*ps+(j+4)*sdd], sdd, &dD[j+4]);
		kernel_spotrf_nt_l_4x4_lib4(j+8, &pD[(i+8)*sdd], &pD[(j+8)*sdd], &pC[(j+8)*ps+(j+8)*sdc], &pD[(j+8)*ps+(j+8)*sdd], &dD[j+8]);
		}
	if(m>i)
		{
		if(m-i<=4)
			{
			goto left_4;
			}
		else if(m-i<=8)
			{
			goto left_8;
			}
		else
			{
			goto left_12;
			}
		}
#elif defined(TARGET_ARMV7A_ARM_CORTEX_A9) | defined(TARGET_ARMV7A_ARM_CORTEX_A7)
	for(; i<m-7; i+=8)
		{
		j = 0;
		for(; j<i; j+=4)
			{
			kernel_strsm_nt_rl_inv_8x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+i*sdc], sdc, &pD[j*ps+i*sdd], sdd, &pD[j*ps+j*sdd], &dD[j]);
			}
		kernel_spotrf_nt_l_8x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+j*sdc], sdc, &pD[j*ps+j*sdd], sdd, &dD[j]);
		kernel_spotrf_nt_l_4x4_lib4(j+4, &pD[(i+4)*sdd], &pD[(j+4)*sdd], &pC[(j+4)*ps+(j+4)*sdc], &pD[(j+4)*ps+(j+4)*sdd], &dD[j+4]);
		}
	if(m>i)
		{
		if(m-i<=4)
			{
			goto left_4;
			}
		else
			{
			goto left_8;
			}
		}
#else
	for(; i<m-3; i+=4)
		{
		j = 0;
		for(; j<i; j+=4)
			{
			kernel_strsm_nt_rl_inv_4x4_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+i*sdc], &pD[j*ps+i*sdd], &pD[j*ps+j*sdd], &dD[j]);
			}
		kernel_spotrf_nt_l_4x4_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+j*sdc], &pD[j*ps+j*sdd], &dD[j]);
		}
	if(m>i)
		{
		goto left_4;
		}
#endif

	// common return if i==m
	return;

	// clean up loops definitions

#if defined(TARGET_ARMV8A_ARM_CORTEX_A57) || defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	left_16:
	j = 0;
	for(; j<i; j+=4)
		{
//		kernel_dtrsm_nt_rl_inv_16x4_vs_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+i*sdc], sdc, &pD[j*ps+i*sdd], sdd, &pD[j*ps+j*sdd], &dD[j], m-i, m-j);
		kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+i*sdc], &pD[j*ps+i*sdd], &pD[j*ps+j*sdd], &dD[j], m-i, m-j);
		kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+4)*sdd], &pD[j*sdd], &pC[j*ps+(i+4)*sdc], &pD[j*ps+(i+4)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-4, m-j);
		kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+8)*sdd], &pD[j*sdd], &pC[j*ps+(i+8)*sdc], &pD[j*ps+(i+8)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-8, m-j);
		kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+12)*sdd], &pD[j*sdd], &pC[j*ps+(i+12)*sdc], &pD[j*ps+(i+12)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-12, m-j);
		}
//	kernel_dpotrf_nt_l_16x4_vs_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+j*sdc], sdc, &pD[j*ps+j*sdd], sdd, &dD[j], m-i, m-j);
	kernel_spotrf_nt_l_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+j*sdc], &pD[j*ps+j*sdd], &dD[j], m-i, m-j);
	kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+4)*sdd], &pD[j*sdd], &pC[j*ps+(i+4)*sdc], &pD[j*ps+(i+4)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-4, m-j);
	kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+8)*sdd], &pD[j*sdd], &pC[j*ps+(i+8)*sdc], &pD[j*ps+(i+8)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-8, m-j);
	kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+12)*sdd], &pD[j*sdd], &pC[j*ps+(i+12)*sdc], &pD[j*ps+(i+12)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-12, m-j);
//	kernel_dpotrf_nt_l_12x4_vs_lib4(j+4, &pD[(i+4)*sdd], sdd, &pD[(j+4)*sdd], &pC[(j+4)*ps+(j+4)*sdc], sdc, &pD[(j+4)*ps+(j+4)*sdd], sdd, &dD[j+4], m-i-4, m-j-4);
	kernel_spotrf_nt_l_4x4_vs_lib4(j+4, &pD[(i+4)*sdd], &pD[(j+4)*sdd], &pC[(j+4)*ps+(j+4)*sdc], &pD[(j+4)*ps+(j+4)*sdd], &dD[j+4], m-i-4, m-j-4);
	kernel_strsm_nt_rl_inv_4x4_vs_lib4(j+4, &pD[(i+8)*sdd], &pD[(j+4)*sdd], &pC[(j+4)*ps+(i+8)*sdc], &pD[(j+4)*ps+(i+8)*sdd], &pD[(j+4)*ps+(j+4)*sdd], &dD[j+4], m-i-8, m-j-4);
	kernel_strsm_nt_rl_inv_4x4_vs_lib4(j+4, &pD[(i+12)*sdd], &pD[(j+4)*sdd], &pC[(j+4)*ps+(i+12)*sdc], &pD[(j+4)*ps+(i+12)*sdd], &pD[(j+4)*ps+(j+4)*sdd], &dD[j+4], m-i-12, m-j-4);
//	kernel_dpotrf_nt_l_8x4_vs_lib4(j+8, &pD[(i+8)*sdd], sdd, &pD[(j+8)*sdd], &pC[(j+8)*ps+(j+8)*sdc], sdc, &pD[(j+8)*ps+(j+8)*sdd], sdd, &dD[j+8], m-i-8, m-j-8);
	kernel_spotrf_nt_l_4x4_vs_lib4(j+8, &pD[(i+8)*sdd], &pD[(j+8)*sdd], &pC[(j+8)*ps+(i+8)*sdc], &pD[(j+8)*ps+(i+8)*sdd], &dD[j+8], m-i-8, m-j-8);
	kernel_strsm_nt_rl_inv_4x4_vs_lib4(j+8, &pD[(i+12)*sdd], &pD[(j+8)*sdd], &pC[(j+8)*ps+(i+12)*sdc], &pD[(j+8)*ps+(i+12)*sdd], &pD[(j+8)*ps+(j+8)*sdd], &dD[j+8], m-i-12, m-j-8);
	kernel_spotrf_nt_l_4x4_vs_lib4(j+12, &pD[(i+12)*sdd], &pD[(j+12)*sdd], &pC[(j+12)*ps+(i+12)*sdc], &pD[(j+12)*ps+(i+12)*sdd], &dD[j+12], m-i-12, m-j-12);
	return;
#endif

#if defined(TARGET_ARMV8A_ARM_CORTEX_A57) || defined(TARGET_ARMV8A_ARM_CORTEX_A53) || defined(TARGET_ARMV7A_ARM_CORTEX_A15)
	left_12:
	if(m-i==12)
		{
		j = 0;
		for(; j<i; j+=4)
			{
			kernel_strsm_nt_rl_inv_12x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+i*sdc], sdc, &pD[j*ps+i*sdd], sdd, &pD[j*ps+j*sdd], &dD[j]);
			}
		kernel_spotrf_nt_l_12x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+j*sdc], sdc, &pD[j*ps+j*sdd], sdd, &dD[j]);
		kernel_spotrf_nt_l_8x4_lib4(j+4, &pD[(i+4)*sdd], sdd, &pD[(j+4)*sdd], &pC[(j+4)*ps+(j+4)*sdc], sdc, &pD[(j+4)*ps+(j+4)*sdd], sdd, &dD[j+4]);
		kernel_spotrf_nt_l_4x4_lib4(j+8, &pD[(i+8)*sdd], &pD[(j+8)*sdd], &pC[(j+8)*ps+(i+8)*sdc], &pD[(j+8)*ps+(i+8)*sdd], &dD[j+8]);
		}
	else
		{
		j = 0;
		for(; j<i; j+=4)
			{
	//		kernel_dtrsm_nt_rl_inv_12x4_vs_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+i*sdc], sdc, &pD[j*ps+i*sdd], sdd, &pD[j*ps+j*sdd], &dD[j], m-i, m-j);
			kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+i*sdc], &pD[j*ps+i*sdd], &pD[j*ps+j*sdd], &dD[j], m-i, m-j);
			kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+4)*sdd], &pD[j*sdd], &pC[j*ps+(i+4)*sdc], &pD[j*ps+(i+4)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-4, m-j);
			kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+8)*sdd], &pD[j*sdd], &pC[j*ps+(i+8)*sdc], &pD[j*ps+(i+8)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-8, m-j);
			}
	//	kernel_dpotrf_nt_l_12x4_vs_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+j*sdc], sdc, &pD[j*ps+j*sdd], sdd, &dD[j], m-i, m-j);
		kernel_spotrf_nt_l_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+j*sdc], &pD[j*ps+j*sdd], &dD[j], m-i, m-j);
		kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+4)*sdd], &pD[j*sdd], &pC[j*ps+(i+4)*sdc], &pD[j*ps+(i+4)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-4, m-j);
		kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+8)*sdd], &pD[j*sdd], &pC[j*ps+(i+8)*sdc], &pD[j*ps+(i+8)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-8, m-j);
	//	kernel_dpotrf_nt_l_8x4_vs_lib4(j+4, &pD[(i+4)*sdd], sdd, &pD[(j+4)*sdd], &pC[(j+4)*ps+(j+4)*sdc], sdc, &pD[(j+4)*ps+(j+4)*sdd], sdd, &dD[j+4], m-i-4, m-j-4);
		kernel_spotrf_nt_l_4x4_vs_lib4(j+4, &pD[(i+4)*sdd], &pD[(j+4)*sdd], &pC[(j+4)*ps+(j+4)*sdc], &pD[(j+4)*ps+(j+4)*sdd], &dD[j+4], m-i-4, m-j-4);
		kernel_strsm_nt_rl_inv_4x4_vs_lib4(j+4, &pD[(i+8)*sdd], &pD[(j+4)*sdd], &pC[(j+4)*ps+(i+8)*sdc], &pD[(j+4)*ps+(i+8)*sdd], &pD[(j+4)*ps+(j+4)*sdd], &dD[j+4], m-i-8, m-j-4);
		kernel_spotrf_nt_l_4x4_vs_lib4(j+8, &pD[(i+8)*sdd], &pD[(j+8)*sdd], &pC[(j+8)*ps+(i+8)*sdc], &pD[(j+8)*ps+(i+8)*sdd], &dD[j+8], m-i-8, m-j-8);
		}
	return;
#endif

#if defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53) | defined(TARGET_ARMV7A_ARM_CORTEX_A15) | defined(TARGET_ARMV7A_ARM_CORTEX_A9) | defined(TARGET_ARMV7A_ARM_CORTEX_A7)
	left_8:
	if(m-i==8)
		{
		j = 0;
		for(; j<i; j+=4)
			{
			kernel_strsm_nt_rl_inv_8x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+i*sdc], sdc, &pD[j*ps+i*sdd], sdd, &pD[j*ps+j*sdd], &dD[j]);
			}
		kernel_spotrf_nt_l_8x4_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+j*sdc], sdc, &pD[j*ps+j*sdd], sdd, &dD[j]);
		kernel_spotrf_nt_l_4x4_lib4(j+4, &pD[(i+4)*sdd], &pD[(j+4)*sdd], &pC[(j+4)*ps+(i+4)*sdc], &pD[(j+4)*ps+(i+4)*sdd], &dD[j+4]);
		}
	else
		{
		j = 0;
		for(; j<i; j+=4)
			{
	//		kernel_dtrsm_nt_rl_inv_8x4_vs_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+i*sdc], sdc, &pD[j*ps+i*sdd], sdd, &pD[j*ps+j*sdd], &dD[j], m-i, m-j);
			kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+i*sdc], &pD[j*ps+i*sdd], &pD[j*ps+j*sdd], &dD[j], m-i, m-j);
			kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+4)*sdd], &pD[j*sdd], &pC[j*ps+(i+4)*sdc], &pD[j*ps+(i+4)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-4, m-j);
			}
	//	kernel_dpotrf_nt_l_8x4_vs_lib4(j, &pD[i*sdd], sdd, &pD[j*sdd], &pC[j*ps+j*sdc], sdc, &pD[j*ps+j*sdd], sdd, &dD[j], m-i, m-j);
		kernel_spotrf_nt_l_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+j*sdc], &pD[j*ps+j*sdd], &dD[j], m-i, m-j);
		kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[(i+4)*sdd], &pD[j*sdd], &pC[j*ps+(i+4)*sdc], &pD[j*ps+(i+4)*sdd], &pD[j*ps+j*sdd], &dD[j], m-i-4, m-j);
		kernel_spotrf_nt_l_4x4_vs_lib4(j+4, &pD[(i+4)*sdd], &pD[(j+4)*sdd], &pC[(j+4)*ps+(i+4)*sdc], &pD[(j+4)*ps+(i+4)*sdd], &dD[j+4], m-i-4, m-j-4);
		}
	return;
#endif

	left_4:
	if(m-i==4)
		{
		j = 0;
		for(; j<i; j+=4)
			{
			kernel_strsm_nt_rl_inv_4x4_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+i*sdc], &pD[j*ps+i*sdd], &pD[j*ps+j*sdd], &dD[j]);
			}
		kernel_spotrf_nt_l_4x4_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+j*sdc], &pD[j*ps+j*sdd], &dD[j]);
		}
	else
		{
		j = 0;
		for(; j<i; j+=4)
			{
			kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+i*sdc], &pD[j*ps+i*sdd], &pD[j*ps+j*sdd], &dD[j], m-i, m-j);
			}
		kernel_spotrf_nt_l_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*ps+j*sdc], &pD[j*ps+j*sdd], &dD[j], m-i, m-j);
		}
	return;

	}



// dpotrf
void blasfeo_spotrf_l_mn(int m, int n, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj)
	{

	if(m<=0 || n<=0)
		return;

	if(ci!=0 | di!=0)
		{
		printf("\nblasfeo_spotrf_l: feature not implemented yet: ci=%d, di=%d\n", ci, di);
		exit(1);
		}

	const int bs = 4;

	int sdc = sC->cn;
	int sdd = sD->cn;
	float *pC = sC->pA + cj*bs;
	float *pD = sD->pA + dj*bs;
	float *dD = sD->dA;
	if(di==0 && dj==0) // XXX what to do if di and dj are not zero
		sD->use_dA = 1;
	else
		sD->use_dA = 0;

	int i, j, l;

	i = 0;
	for(; i<m-3; i+=4)
		{
		j = 0;
		for(; j<i && j<n-3; j+=4)
			{
			kernel_strsm_nt_rl_inv_4x4_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+i*sdc], &pD[j*bs+i*sdd], &pD[j*bs+j*sdd], &dD[j]);
			}
		if(j<n)
			{
			if(j<i) // dtrsm
				{
				kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+i*sdc], &pD[j*bs+i*sdd], &pD[j*bs+j*sdd], &dD[j], m-i, n-j);
				}
			else // dpotrf
				{
				if(j<n-3)
					{
					kernel_spotrf_nt_l_4x4_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+j*sdc], &pD[j*bs+j*sdd], &dD[j]);
					}
				else
					{
					kernel_spotrf_nt_l_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+j*sdc], &pD[j*bs+j*sdd], &dD[j], m-i, n-j);
					}
				}
			}
		}
	if(m>i)
		{
		goto left_4;
		}

	// common return if i==m
	return;

	// clean up loops definitions

	left_4:
	j = 0;
	for(; j<i && j<n-3; j+=4)
		{
		kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+i*sdc], &pD[j*bs+i*sdd], &pD[j*bs+j*sdd], &dD[j], m-i, n-j);
		}
	if(j<n)
		{
		if(j<i) // dtrsm
			{
			kernel_strsm_nt_rl_inv_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+i*sdc], &pD[j*bs+i*sdd], &pD[j*bs+j*sdd], &dD[j], m-i, n-j);
			}
		else // dpotrf
			{
			kernel_spotrf_nt_l_4x4_vs_lib4(j, &pD[i*sdd], &pD[j*sdd], &pC[j*bs+j*sdc], &pD[j*bs+j*sdd], &dD[j], m-i, n-j);
			}
		}
	return;

	return;
	}



// dsyrk dpotrf
void blasfeo_ssyrk_spotrf_ln(int m, int k, struct blasfeo_smat *sA, int ai, int aj, struct blasfeo_smat *sB, int bi, int bj, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj)
	{
	if(ai!=0 | bi!=0 | ci!=0 | di!=0)
		{
		printf("\nblasfeo_ssyrk_spotrf_ln: feature not implemented yet: ai=%d, bi=%d, ci=%d, di=%d\n", ai, bi, ci, di);
		exit(1);
		}
	const int bs = 4;
	int sda = sA->cn;
	int sdb = sB->cn;
	int sdc = sC->cn;
	int sdd = sD->cn;
	float *pA = sA->pA + aj*bs;
	float *pB = sB->pA + bj*bs;
	float *pC = sC->pA + cj*bs;
	float *pD = sD->pA + dj*bs;
	float *dD = sD->dA; // XXX what to do if di and dj are not zero
	ssyrk_spotrf_nt_l_lib(m, m, k, pA, sda, pB, sdb, pC, sdc, pD, sdd, dD);
	if(di==0 && dj==0)
		sD->use_dA = 1;
	else
		sD->use_dA = 0;
	return;
	}



void blasfeo_ssyrk_spotrf_ln_mn(int m, int n, int k, struct blasfeo_smat *sA, int ai, int aj, struct blasfeo_smat *sB, int bi, int bj, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj)
	{
	if(ai!=0 | bi!=0 | ci!=0 | di!=0)
		{
		printf("\nblasfeo_ssyrk_spotrf_ln: feature not implemented yet: ai=%d, bi=%d, ci=%d, di=%d\n", ai, bi, ci, di);
		exit(1);
		}
	const int bs = 4;
	int sda = sA->cn;
	int sdb = sB->cn;
	int sdc = sC->cn;
	int sdd = sD->cn;
	float *pA = sA->pA + aj*bs;
	float *pB = sB->pA + bj*bs;
	float *pC = sC->pA + cj*bs;
	float *pD = sD->pA + dj*bs;
	float *dD = sD->dA; // XXX what to do if di and dj are not zero
	ssyrk_spotrf_nt_l_lib(m, n, k, pA, sda, pB, sdb, pC, sdc, pD, sdd, dD);
	if(di==0 && dj==0)
		sD->use_dA = 1;
	else
		sD->use_dA = 0;
	return;
	}



// dgetrf no pivoting
void blasfeo_sgetrf_np(int m, int n, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj)
	{
	if(ci!=0 | di!=0)
		{
		printf("\nblasfeo_sgetf_np: feature not implemented yet: ci=%d, di=%d\n", ci, di);
		exit(1);
		}
	const int bs = 4;
	int sdc = sC->cn;
	int sdd = sD->cn;
	float *pC = sC->pA + cj*bs;
	float *pD = sD->pA + dj*bs;
	float *dD = sD->dA; // XXX what to do if di and dj are not zero
	sgetrf_nn_nopivot_lib(m, n, pC, sdc, pD, sdd, dD);
	if(di==0 && dj==0)
		sD->use_dA = 1;
	else
		sD->use_dA = 0;
	return;
	}




// dgetrf row pivoting
void blasfeo_sgetrf_rp(int m, int n, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj, int *ipiv)
	{
	if(ci!=0 | di!=0)
		{
		printf("\nblasfeo_sgetrf_rp: feature not implemented yet: ci=%d, di=%d\n", ci, di);
		exit(1);
		}
	const int bs = 4;
	int sdc = sC->cn;
	int sdd = sD->cn;
	float *pC = sC->pA + cj*bs;
	float *pD = sD->pA + dj*bs;
	float *dD = sD->dA; // XXX what to do if di and dj are not zero
	// needs to perform row-excanges on the yet-to-be-factorized matrix too
	if(pC!=pD)
		blasfeo_sgecp(m, n, sC, ci, cj, sD, di, dj);
	sgetrf_nn_lib(m, n, pC, sdc, pD, sdd, dD, ipiv);
	if(di==0 && dj==0)
		sD->use_dA = 1;
	else
		sD->use_dA = 0;
	return;
	}



int blasfeo_sgeqrf_worksize(int m, int n)
	{
	return 0;
	}



void blasfeo_sgeqrf(int m, int n, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj, void *work)
	{
	if(m<=0 | n<=0)
		return;
#ifndef BENCHMARKS_MODE
	printf("\nblasfeo_sgeqrf: feature not implemented yet\n");
	exit(1);
#endif
	return;
	}



int blasfeo_sgelqf_worksize(int m, int n)
	{
	return 0;
	}



void blasfeo_sgelqf(int m, int n, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj, void *work)
	{
	if(m<=0 | n<=0)
		return;
#ifndef BENCHMARKS_MODE
	printf("\nblasfeo_sgelqf: feature not implemented yet\n");
	exit(1);
#endif
	return;
	}



// LQ factorization with positive diagonal elements
void blasfeo_sgelqf_pd(int m, int n, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj, void *work)
	{
	if(m<=0 | n<=0)
		return;
#ifndef BENCHMARKS_MODE
	printf("\nblasfeo_sgelqf_pd: feature not implemented yet\n");
	exit(1);
#endif
	return;
	}



// LQ factorization with positive diagonal elements, array of matrices
// [L, A] <= lq( [L. A] )
// L lower triangular, of size (m)x(m)
// A full of size (m)x(n1)
void blasfeo_sgelqf_pd_la(int m, int n1, struct blasfeo_smat *sD, int di, int dj, struct blasfeo_smat *sA, int ai, int aj, void *work)
	{
	if(m<=0)
		return;
#ifndef BENCHMARKS_MODE
	printf("\nblasfeo_sgelqf_pd_la: feature not implemented yet\n");
	exit(1);
#endif
	return;
	}



// LQ factorization with positive diagonal elements, array of matrices
// [L, L, A] <= lq( [L. L, A] )
// L lower triangular, of size (m)x(m)
// A full of size (m)x(n1)
void blasfeo_sgelqf_pd_lla(int m, int n1, struct blasfeo_smat *sD, int di, int dj, struct blasfeo_smat *sL, int li, int lj, struct blasfeo_smat *sA, int ai, int aj, void *work)
	{
	if(m<=0)
		return;
#ifndef BENCHMARKS_MODE
	printf("\nblasfeo_dgelqf_pd_lla: feature not implemented yet\n");
	exit(1);
#endif
	}



#else

#error : wrong LA choice

#endif



