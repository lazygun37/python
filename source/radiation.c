
/***********************************************************/
/** @file  radiation.c
 * @author ksl
 * @date   April, 2018
 *
 * @brief  Routines to update radiation field parameters and to
 * calculate various opacities (free-free, bound-free, etc.
 *
 ***********************************************************/

//OLD /***********************************************************
//OLD                                        Space Telescope Science Institute
//OLD 
//OLD  Synopsis:
//OLD 	 radiation(p,ds) updates the radiation field parameters in the wind and reduces 
//OLD 	 the weight of the photon as a result of the effect free free and photoabsorption.
//OLD 	 radiation also keeps track of the number of photoionizations of h and he in the
//OLD 	 cell. 
//OLD Arguments:
//OLD 
//OLD 	PhotPtr p;	the photon
//OLD 	double ds	the distance the photon has travelled in the cell
//OLD 
//OLD Returns:
//OLD 	Always returns 0.  The pieces of the wind structure which are updated are
//OLD 	j,ave_freq,ntot, heat_photo, heat_ff, heat_h, heat_he1, heat_he2, heat_z,
//OLD 	nioniz, and ioniz[].
//OLD  
//OLD Description:	 
//OLD Notes:
//OLD 	
//OLD 	The # of ionizations of a specific ion = (w(0)-w(s))*n_i sigma_i/ (h nu * kappa_tot).  (The # of ionizations
//OLD 	is just given by the integral of n_i sigma_i w(s) / h nu ds, but if the density is assumed to
//OLD 	be constant and sigma is also constant [therefy ignoring velocity shifts in a grid cell], then
//OLD 	n_i sigma and h nu can be brought outside the integral.  Hence the integral is just over w(s),
//OLD 	but that is just given by (w(0)-w(smax))/kappa_tot.)  The routine calculates the number of ionizations per
//OLD 	unit volume.
//OLD 	
//OLD 	This routine is very time counsuming for our normal file which has a large number of x-sections.  The problem
//OLD 	is going through all the do loops, not insofar as I can determine anything else.  Trying to reduce the calucatiions
//OLD 	by using a density criterion is a amall effect.  One really needs to avoid the calculations, either by avoiding
//OLD 	the do loops altogether or by reducing the number of input levels.  It's possible that if one were clever about
//OLD 	the thresholds (as we are on the lines that one could figure out a wininning strategy as it is all brute force
//OLD         do loops..  
//OLD 
//OLD 	57h -- This routine was very time consuming.  The time spent is/was well out of proportion to
//OLD 	the affect free-bound processes have on the overall spectrum, and so signifincant changes have been made
//OLD 	to the earlier versions of the routine.  The fundamental problem before 57h was that every time one
//OLD 	entered the routine (which was every for every movement of a photon) in the code.  Basically there were
//OLD 	3  changes made:
//OLD 		1. During the detailed spectrum cycle, the code avoids the portions of the routine that are
//OLD 	only needed during the ionization cycles.
//OLD 		2. Switches have been installed tha skip the free-bound section altogether if PHOT_DEN_MIN is
//OLD 	less than zero.  It is very reasonable to do this for the detailed spectrum calculation if one is
//OLD 	doing a standard 
//OLD 	were to skip portions of when they were not needed 
//OLD 
//OLD 	?? Would it be more natural to include electron scattering here in Radiation as well ??
//OLD 	?? Radiation needs a major overhaul.  A substantial portion of this routine is not needed in the extract 
//OLD 	?? portion of the calculation.  In addition the do loops go through all of the ions checking one at a time
//OLD 	?? whether they are above the frequencey threshold.  
//OLD 	?? The solution I believe is to include some kind of switch that tells the routine when one is doing
//OLD 	?? the ionization calculation and to skip the unnecessary sections when extract is being carried out.
//OLD 	?? In addition, if there were a set of ptrs to the photionization structures that were orded by frequency,
//OLD 	?? similar to the line list, one could then change to loops so that one only had to check up to the
//OLD 	?? first x-section that had a threshold up to the photon frequency, and not all of the rest.
//OLD 	?? At present, I have simply chopped the photoionizations being considered to include only thresholds
//OLD         ?? shortward of the Lyman limit...e.g. 1 Rydberg, but this makes it impossible to discuss the Balmer jump
//OLD History:
//OLD  	97jan	ksl	Coded as part of python effort
//OLD 	98jul	ksl	Almost entirely recoded to eliminate arbitrary split between
//OLD 			the several preexisting routines.
//OLD 	98nov	ksl	Added back tracking of number of h and he ionizations
//OLD 	99jan	ksl	Increased COLMIN from 1.e6 to 1.e10 and added checks so
//OLD 			that one does not attempt to calculate photoionization
//OLD 			cross-sections below threshold.  Both of these changes
//OLD 			are intended to speed this routine.
//OLD     01oct   ksl     Modifications begun to incorporate Topbase photoionization
//OLD                         x-sections.
//OLD     01oct   ksl     Moved fb_cooling to balance_abso.  It's only used by
//OLD                         balance and probably should not be there either.
//OLD 	02jun	ksl	Improved/fixed treatment of calculation of number of ionizations.
//OLD 	04apr	ksl	SS had modified this routine to allow for macro-atoms in python_47, but his modifications
//OLD 			left very little for radiation to accomplish.  I have returned to the old version of 
//OLD 			routine, and moved the little bit that needed to be done in this routine for
//OLD 			the macro approach to the calling routine.  Once we abandon the old approach this
//OLD 			routine can be deleted.
//OLD 	04aug	ksl	Fixed an error which had crept into the program between 51 and 51a that caused the
//OLD 			disk models to be wrong.  The problem was that there are two places where the
//OLD 			same frequency limit should have been used, but the limits had been set differently.
//OLD 	04dec	ksl	54a -- Miniscule change to eliminate -03 warnings.  Also eliminate some variables
//OLD 			that were not really being used.
//OLD 	06may	ksl	57+ -- Began modifications to allow for splitting the wind and plasma structures
//OLD 	06jul	ksl	57h -- Made various changes intended to speed up this routine. These include
//OLD 			skipping sections of the routine in the spectrum generation
//OLD 			phase that are not used, allowing control over whether fb opacities
//OLD 			are calculated at all, and using frequency ordered pointer arrays
//OLD 			to shorten the loops.
//OLD 	11aug	nsh	70 changes made to radiation to allow compton cooling to be computed
//OLD 	11aug	nsh	70 Changed printout of spectra in selected regions so it is always
//OLD 			the midpoint of the wind
//OLD 	12may	nsh	72 Added induced compton
//OLD 	12jun 	nsh	72 Added lines to write out photon stats to a file dirung diagnostics. This is
//OLD 			to allow us to see how well spectra are being modelled by power law W and alpha
//OLD 	1405    JM corrected error (#73) so that photon frequency is shifted to the rest frame of the 
//OLD 	        cell in question. Also added check which checks if a photoionization edge is crossed
//OLD 	        along ds.
//OLD 	1508	NSH slight modification to mean that compton scattering no longer reduces the weight of
//OLD 			the photon in this part of the code. It is now done when the photon scatters.
//OLD **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "atomic.h"
#include "python.h"

#define COLMIN	0.01

int iicount = 0;


/**********************************************************/
/** @name      radiation
 * @brief      updates the  field parameters in the wind and reduces 
 * the weight of the photon as a result of the effects of  free free, bound-free and 
 * Compton scattering. The routine  
 * also keeps track of the number of photoionizations for H and He in the
 * cell.
 *
 * @param [in,out] PhotPtr  p   the photon
 * @param [in] double  ds   the distance the photon has travelled in the cell
 * @return     Always returns 0.  The pieces of the wind structure which are updated are
 * 	j,ave_freq,ntot, heat_photo, heat_ff, heat_h, heat_he1, heat_he2, heat_z,
 * 	nioniz, and ioniz[].
 *
 * @details
 *
 * ### Notes ###
 * The # of ionizations of a specific ion = (w(0)-w(s))*n_i sigma_i/ (h nu * kappa_tot).  (The # of ionizations
 * is just given by the integral of n_i sigma_i w(s) / h nu ds, but if the density is assumed to
 * be constant and sigma is also constant [therefy ignoring velocity shifts in a grid cell], then
 * n_i sigma and h nu can be brought outside the integral.  Hence the integral is just over w(s),
 * but that is just given by (w(0)-w(smax))/kappa_tot.)  The routine calculates the number of ionizations per
 * unit volume.
 * 	
 *
 **********************************************************/

int
radiation (p, ds)
     PhotPtr p;
     double ds;
{
  TopPhotPtr x_top_ptr;

  WindPtr one;
  PlasmaPtr xplasma;

  double freq;
  double kappa_tot, frac_tot, frac_ff;
  double frac_z, frac_comp;     /* frac_comp - the heating in the cell due to compton heating */
  double frac_ind_comp;         /* frac_ind_comp - the heating due to induced compton heating */
  double frac_auger;
  double frac_tot_abs,frac_auger_abs,z_abs;
  double kappa_ion[NIONS];
  double frac_ion[NIONS];
  double density, ft, tau, tau2;
  double energy_abs;
  int n, nion;
  double q, x, z;
  double w_ave, w_in, w_out;
  double den_config ();
  int nconf;
//  double weight_of_packet, y;  //to do with augerion calcs, now deprecated
  double v_inner[3], v_outer[3], v1, v2;
  double freq_inner, freq_outer;
  double freq_min, freq_max;
  double frac_path, freq_xs;
  struct photon phot;
  int ndom;

  one = &wmain[p->grid];        /* So one is the grid cell of interest */

  ndom = one->ndom;
  xplasma = &plasmamain[one->nplasma];
  check_plasma (xplasma, "radiation");

  /* JM 140321 -- #73 Bugfix
     In previous version we were comparing these frequencies in the global rest frame
     The photon frequency should be shifted to the rest frame of the cell in question
     We currently take the average of this frequency along ds. In principle
     this could be improved, so we throw an error if the difference between v1 and v2 is large */

  /* calculate velocity at original position */
  vwind_xyz (ndom, p, v_inner); // get velocity vector at new pos
  v1 = dot (p->lmn, v_inner);   // get direction cosine

  /* Create phot, a photon at the position we are moving to 
   *  note that the actual movement of the photon gets done after 
   *  the call to radiation 
   */ 

  stuff_phot (p, &phot);        // copy photon ptr
  move_phot (&phot, ds);        // move it by ds
  vwind_xyz (ndom, &phot, v_outer);     // get velocity vector at new pos
  v2 = dot (phot.lmn, v_outer); // get direction cosine

  /* calculate photon frequencies in rest frame of cell */

  freq_inner = p->freq * (1. - v1 / C);
  freq_outer = phot.freq * (1. - v2 / C);

  /* take the average of the frequencies at original position and original+ds */
  freq = 0.5 * (freq_inner + freq_outer);

  /* calculate free-free, compton and ind-compton opacities 
     note that we also call these with the average frequency along ds */

  kappa_tot = frac_ff = kappa_ff (xplasma, freq);       /* Add ff opacity */
  kappa_tot += frac_comp = kappa_comp (xplasma, freq);  /* Calculate compton opacity, 
                                                           store it in kappa_comp and also add it to kappa_tot, 
                                                           the total opacity for the photon path */

  kappa_tot += frac_ind_comp = kappa_ind_comp (xplasma, freq);

  frac_tot = frac_z = 0;        /* 59a - ksl - Moved this line out of loop to avoid warning, but notes 
                                   indicate this is all diagnostic and might be removed */
  frac_auger = 0;
  frac_tot_abs=frac_auger_abs=0.0;

  /* Check which of the frequencies is larger.  */

  if (freq_outer > freq_inner)
  {
    freq_max = freq_outer;
    freq_min = freq_inner;
  }
  else
  {
    freq_max = freq_inner;
    freq_min = freq_outer;
  }

  if (freq > phot_freq_min)

  {
    if (geo.ioniz_or_extract)   
    {                           // Initialize during ionization cycles only
      for (nion = 0; nion < nions; nion++)
      {
        kappa_ion[nion] = 0;
        frac_ion[nion] = 0;
      }
    }

    /* Next section is for photoionization with Topbase.  There may be more
       than one x-section associated with an ion, and so one has to keep track
       of the energy that goes into heating electrons carefully.  */

    /* JM 1405 -- I've added a check here that checks if a photoionization edge has been crossed.
       If it has, then we multiply sigma*density by a factor frac_path, which is equal to the how far along 
       ds the edge occurs in frequency space  [(ft - freq_min) / (freq_max - freq_min)] */


    /* Next steps are a way to avoid the loop over photoionization x sections when it should not matter */
    if (DENSITY_PHOT_MIN > 0)   
    {                           // Initialize during ionization cycles only


      /* Loop over all photoionization xsections */
      for (n = 0; n < nphot_total; n++)
      {
        x_top_ptr = phot_top_ptr[n];
        ft = x_top_ptr->freq[0];
        if (ft > freq_min && ft < freq_max)
        {
          /* then the shifting of the photon causes it to cross an edge. 
             Find out where between fmin and fmax the edge would be in freq space.
             frac_path is the fraction of the total path length above the absorption edge
             freq_xs is freq halfway between the edge and the max freq if an edge gets crossed */
          frac_path = (freq_max - ft) / (freq_max - freq_min);
          freq_xs = 0.5 * (ft + freq_max);
        }

        else if (ft > freq_max)
          break;                // The remaining transitions will have higher thresholds

        else if (ft < freq_min)
        {
          frac_path = 1.0;      // then the frequency of the photon is above the threshold all along the path
          freq_xs = freq;       // use the average frequency
        }

        if (freq_xs < x_top_ptr->freq[x_top_ptr->np - 1])
        {
          /* Need the appropriate density at this point. 
          how we get this depends if we have a topbase (level by level) 
             or vfky cross-section (ion by ion) */

          nion = x_top_ptr->nion;
          if (ion[nion].phot_info > 0)  // topbase or hybrid
          {
            nconf = x_top_ptr->nlev;
            density = den_config (xplasma, nconf);
          }

          else if (ion[nion].phot_info == 0)    // verner
            density = xplasma->density[nion];

          else
          {                    
            Error ("radiation.c: No type (%i) for xsection!\n");
            density = 0.0;
          }

          if (density > DENSITY_PHOT_MIN)
          {

            /* Note that this includes a filling factor  */
            kappa_tot += x = sigma_phot (x_top_ptr, freq_xs) * density * frac_path * zdom[ndom].fill;


            if (geo.ioniz_or_extract)   
            {                   // Calculate during ionization cycles only

              //This is the heating effect - i.e. the absorbed photon energy less the binding energy of the lost electron
              frac_tot += z = x * (freq_xs - ft) / freq_xs; 
              //This is the absorbed energy fraction
              frac_tot_abs += z_abs = x; 
			  
              if (nion > 3)
              {
                frac_z += z;
              }

              frac_ion[nion] += z;
              kappa_ion[nion] += x;
            }

          }


        }
      }                         
      
      /* Loop over all inner shell cross sections as well! But only for VFKY ions - topbase has those edges in */

      if (freq > inner_freq_min)
      {
        for (n = 0; n < n_inner_tot; n++)
        {
          if (ion[inner_cross[n].nion].phot_info != 1)  
          {
            x_top_ptr = inner_cross_ptr[n];
            if (x_top_ptr->n_elec_yield != -1)  //Only any point in doing this if we know the energy of elecrons
            {
              ft = x_top_ptr->freq[0];

              if (ft > freq_min && ft < freq_max)
              {
                frac_path = (freq_max - ft) / (freq_max - freq_min);
                freq_xs = 0.5 * (ft + freq_max);
              }
              else if (ft > freq_max)
                break;          // The remaining transitions will have higher thresholds
              else if (ft < freq_min)
              {
                frac_path = 1.0;        // then all frequency along ds are above edge
                freq_xs = freq; // use the average frequency
              }
              if (freq_xs < x_top_ptr->freq[x_top_ptr->np - 1])
              {
                nion = x_top_ptr->nion;
                if (ion[nion].phot_info == 0)   // verner only ion
                {
                  density = xplasma->density[nion];     //All these rates are from the ground state, so we just need the density of the ion.
                }
                else
                {
                  nconf = phot_top[ion[nion].ntop_ground].nlev; //The lower level of the ground state Pi cross section (should be GS!)
                  density = den_config (xplasma, nconf);
                }
                if (density > DENSITY_PHOT_MIN)
                {
                  kappa_tot += x = sigma_phot (x_top_ptr, freq_xs) * density * frac_path * zdom[ndom].fill;
                  if (geo.ioniz_or_extract && x_top_ptr->n_elec_yield != -1)    // Calculate during ionization cycles only
                  {
                    frac_auger += z = x * (inner_elec_yield[x_top_ptr->n_elec_yield].Ea / EV2ERGS) / (freq_xs * HEV);
	                frac_auger_abs += z_abs = x; //This is the absorbed energy fraction
					
                    if (nion > 3)
                    {
                      frac_z += z;
                    }
                    frac_ion[nion] += z;
                    kappa_ion[nion] += x;
                  }
                }
              }
            }
          }
        }
      }
    }
  }



  /* finished looping over cross-sections to calculate bf opacity 
     we can now reduce weights and record certain estimators */



  tau = kappa_tot * ds;
  w_in = p->w;

  if (sane_check (tau))
  {
    Error ("Radiation:sane_check CHECKING ff=%e, comp=%e, ind_comp=%e\n", frac_ff, frac_comp, frac_ind_comp);
  }
/* Calculate the heating effect*/

  if (tau > 0.0001)
  {                             /* Need differentiate between thick and thin cases */
    x = exp (-tau);
    energy_abs = w_in * (1. - x);
  }
  else
  {
    tau2 = tau * tau;
    energy_abs = w_in * (tau - 0.5 * tau2);
  }

  /* Calculate the reduction in weight - compton scattering is not included, it is now included at scattering
     however induced compton heating is not implemented at scattering, so it should remain here for the time being
     to maimtain consistency. */

  tau = (kappa_tot - frac_comp) * ds;

  if (sane_check (tau))
  {
    Error ("Radiation:sane_check CHECKING ff=%e, comp=%e, ind_comp=%e\n", frac_ff, frac_comp, frac_ind_comp);
  }
  /* Calculate the reduction in the w of the photon bundle along with the average
     weight in the cell */

  if (tau > 0.0001)
  {                             /* Need differentiate between thick and thin cases */
    x = exp (-tau);
    p->w = w_out = w_in * x;
    w_ave = (w_in - w_out) / tau;
  }
  else
  {
    tau2 = tau * tau;
    p->w = w_out = w_in * (1. - tau + 0.5 * tau2);      /*Calculate to second order */
    w_ave = w_in * (1. - 0.5 * tau + 0.1666667 * tau2);
  }


  if (sane_check (p->w))
  {
    Error ("Radiation:sane_check photon weight is %e for tau %e\n", p->w, tau);
  }

  if (geo.ioniz_or_extract == 0)
    return (0);                 // 57h -- ksl -- 060715

/* Everything after this is only needed for ionization calculations */
/* Update the radiation parameters used ultimately in calculating t_r */

  xplasma->ntot++;

/* NSH 15/4/11 Lines added to try to keep track of where the photons are coming from, 
 * and hence get an idea of how 'agny' or 'disky' the cell is. */
/* ksl - 1112 - Fixed this so it records the number of photon bundles and not the total
 * number of photons.  Also used the PTYPE designations as one should as a matter of 
 * course
 */

  if (p->origin == PTYPE_STAR)
    xplasma->ntot_star++;
  else if (p->origin == PTYPE_BL)
    xplasma->ntot_bl++;
  else if (p->origin == PTYPE_DISK)
    xplasma->ntot_disk++;
  else if (p->origin == PTYPE_WIND)
    xplasma->ntot_wind++;
  else if (p->origin == PTYPE_AGN)
    xplasma->ntot_agn++;



  if (p->freq > xplasma->max_freq)      // check if photon frequency exceeds maximum frequency
    xplasma->max_freq = p->freq;

  if (modes.save_cell_stats && ncstat > 0)
  {
    save_photon_stats (one, p, ds, w_ave);      // save photon statistics (extra diagnostics)
  }


  /* JM 1402 -- the banded versions of j, ave_freq etc. are now updated in update_banded_estimators,
     which also updates the ionization parameters and scattered and direct contributions */

  update_banded_estimators (xplasma, p, ds, w_ave);


  if (sane_check (xplasma->j) || sane_check (xplasma->ave_freq))
  {
    Error ("radiation:sane_check Problem with j %g or ave_freq %g\n", xplasma->j, xplasma->ave_freq);
  }

  if (kappa_tot > 0)
  {
	  
    //If statement added 01mar18 ksl to correct problem of zero divide
    //  in odd situations where no continuum opacity
    z = (energy_abs) / kappa_tot;
    xplasma->heat_ff += z * frac_ff;
    xplasma->heat_tot += z * frac_ff;
	xplasma->abs_tot += z * frac_ff;   /* The energy absorbed from the photon field in this cell */
	
    xplasma->heat_comp += z * frac_comp;        /* Calculate the heating in the cell due to compton heating */
    xplasma->heat_tot += z * frac_comp; /* Add the compton heating to the total heating for the cell */
	xplasma->abs_tot += z * frac_comp;   /* The energy absorbed from the photon field in this cell */
	xplasma->abs_tot += z * frac_ind_comp;   /* The energy absorbed from the photon field in this cell */

    xplasma->heat_tot += z * frac_ind_comp;     /* Calculate the heating in the celldue to induced compton heating */
    xplasma->heat_ind_comp += z * frac_ind_comp;        /* Increment the induced compton heating counter for the cell */
    if (freq > phot_freq_min)
    {
		xplasma->abs_photo += z * frac_tot_abs;  //Here we store the energy absorbed from the photon flux - different from the heating by the binding energy
		xplasma->abs_auger += z * frac_auger_abs; //same for auger
		xplasma->abs_tot += z * frac_tot_abs;   /* The energy absorbed from the photon field in this cell */
		xplasma->abs_tot += z * frac_auger_abs;   /* The energy absorbed from the photon field in this cell */

      xplasma->heat_photo += z * frac_tot;
      xplasma->heat_z += z * frac_z;
      xplasma->heat_tot += z * frac_tot;        //All of the photoinization opacities
      xplasma->heat_auger += z * frac_auger;
      xplasma->heat_tot += z * frac_auger;      //All the inner shell opacities

      /* Calculate the number of photoionizations per unit volume for H and He 
         */
      xplasma->nioniz++;
      q = (z) / (H * freq * xplasma->vol);
      /* So xplasma->ioniz for each species is just 
         (energy_abs)*kappa_h/kappa_tot / H*freq / volume
         or the number of photons absorbed in this bundle per unit volume by this ion
       */

      for (nion = 0; nion < nions; nion++)
      {
        xplasma->ioniz[nion] += kappa_ion[nion] * q;
        xplasma->heat_ion[nion] += frac_ion[nion] * z;
      }

    }
  }

  /* Now for contribution to inner shell ionization estimators (SS, Dec 08) */
  /*. Commented out by NSH 2018 */
//  for (n = 0; n < nauger; n++)
//  {
//    ft = augerion[n].freq_t;
//    if (p->freq > ft)
//    {

//      weight_of_packet = w_ave;
//      x = sigma_phot_verner (&augerion[n], freq);       //this is the cross section
//      y = weight_of_packet * x * ds;

//      xplasma->gamma_inshl[n] += y / (freq * H * xplasma->vol);
//    }
//  }



  return (0);
}

//OLD /***********************************************************
//OLD                                        Space Telescope Science Institute
//OLD 
//OLD  Synopsis: 
//OLD 	double kappa_ff(w,freq) calculates the free-free opacity allowing for stimulated emission
//OLD  	
//OLD Arguments:
//OLD 
//OLD Returns:
//OLD 	
//OLD Description:	 
//OLD 	Formula from Allen
//OLD 	Includes only singly ionized H and doubly ionized he 	
//OLD 
//OLD Notes:
//OLD 
//OLD History:
//OLD 	98aug	ksl	Coded as part of python effort
//OLD         04Apr   SS      If statement added for cases with hydrogen only.
//OLD                         Note: this routine assumes that H I is the first ion
//OLD                         and that He II is the fourth ion.
//OLD 	05may	ksl	57+ -- Modified to eliminate WindPtr in favor
//OLD 			of PlasmaPtr
//OLD    	12oct	nsh	73 -- Modified to use a prefector summed over all ions, calculated prior
//OLD 			to the photon flight
//OLD 
//OLD **************************************************************/



/**********************************************************/
/** @name      kappa_ff
 * @brief      calculates the free-free opacity allowing for stimulated emission
 *
 * @param [in] PlasmaPtr  xplasma   The plasma cell where the free-free optacity is to be calculated
 * @param [in] double  freq   The frequency at which kappa_ff is to be calculated
 * @return     kappa           
 *
 * @details
 * Uses the formula from Allen
 *
 * ### Notes ###
 *
 * The routine originally only includes the effect of singly ionized H and doubly ionized He
 * and did not include a gaunt factor
 *
 * More recent versions include all ions and a gaunt factor, as calculated in 
 * pop_kappa_ff_array and stored in kappa_ff_factor. The gaunt factor as currewntly
 * implemented is a frequency averaged one, and so is approximate (but better than 
 * just using 1). A future upgrade would use a more complex implementation where we 
 * use the frequency dependant gaunt factor.
 *
 **********************************************************/

double
kappa_ff (xplasma, freq)
     PlasmaPtr xplasma;
     double freq;
{
  double x;
  double exp ();
  double x1, x2, x3;
  int ndom;

  /* get the domain number */
  ndom = wmain[xplasma->nwind].ndom;

  if (gaunt_n_gsqrd == 0)       //Maintain old behaviour
  {
    if (nelements > 1)
    {
      x = x1 = 3.692e8 * xplasma->ne * (xplasma->density[1] + 4. * xplasma->density[4]);
    }
    else
    {
      x = x1 = 3.692e8 * xplasma->ne * (xplasma->density[1]);
    }
  }
  else
  {
    x = x1 = xplasma->kappa_ff_factor;
  }
  x *= x2 = (1. - exp (-H_OVER_K * freq / xplasma->t_e));
  x /= x3 = (sqrt (xplasma->t_e) * freq * freq * freq);

  x *= zdom[ndom].fill;         // multiply by the filling factor- should cancel with density enhancement

  return (x);
}



//OLD /***********************************************************
//OLD 				       Space Telescope Science Institute
//OLD 
//OLD  Synopsis:
//OLD 	double sigma_phot(x_ptr,freq)	calculates the
//OLD 	photionization crossection due to the transition associated with
//OLD 	x_ptr at frequency freq
//OLD Arguments:
//OLD      struct topbase_phot *x_ptr;
//OLD      double freq;
//OLD 
//OLD Returns:
//OLD 
//OLD Description:
//OLD 	sigma_phot uses the Topbase x-sections to calculate the bound free
//OLD 	(or photoionization) xsection.	The data must have been into the
//OLD 	photoionization structures xphot with get_atomic_data and the
//OLD 	densities of individual ions must have been calculated previously.
//OLD 
//OLD Notes:
//OLD 
//OLD History:
//OLD 	01Oct	ksl	Coded as part of general move to use Topbase data
//OLD 			(especially partial xsections, which did not exist 
//OLD 			in the Verner et al prescriptions
//OLD 	02jul	ksl	Fixed error in the way fraction being applied.
//OLD 			Sigh! and then modified program to use linterp
//OLD 
//OLD **************************************************************/


/**********************************************************/
/** @name      sigma_phot
 * @brief      calculates the
 * 	photionization crossection due to a Topbase level associated with
 * 	x_ptr at frequency freq
 *
 * @param [in,out] struct topbase_phot *  x_ptr   The structure that contains
 * TopBase information about the photoionization x-section
 * @param [in] double  freq   The frequency where the x-section is to be calculated
 *
 * @return     The x-section   
 *
 * @details
 * sigma_phot uses the Topbase x-sections to calculate the bound free
 * (or photoionization) xsection.	The data must have been into the
 * photoionization structures xphot with get_atomic_data and the
 * densities of individual ions must have been calculated previously.
 *
 * ### Notes ###
 *
 **********************************************************/

double
sigma_phot (x_ptr, freq)
     struct topbase_phot *x_ptr;
     double freq;
{
  int nmax;
  double xsection;
  double frac, fbot, ftop;
  int linterp ();
  int nlast;

  if (freq < x_ptr->freq[0])
    return (0.0);               // Since this was below threshold
  if (freq == x_ptr->f)
    return (x_ptr->sigma);      // Avoid recalculating xsection

  if (x_ptr->nlast > -1)
  {
    nlast = x_ptr->nlast;
    if ((fbot = x_ptr->freq[nlast]) < freq && freq < (ftop = x_ptr->freq[nlast + 1]))
    {
      frac = (log (freq) - log (fbot)) / (log (ftop) - log (fbot));
      xsection = exp ((1. - frac) * log (x_ptr->x[nlast]) + frac * log (x_ptr->x[nlast + 1]));
      //Store the results
      x_ptr->sigma = xsection;
      x_ptr->f = freq;
      return (xsection);
    }
  }

/* If got to here, have to go the whole hog in calculating the x-section */
  nmax = x_ptr->np;
  x_ptr->nlast = linterp (freq, &x_ptr->freq[0], &x_ptr->x[0], nmax, &xsection, 1);     //call linterp in log space


  //Store the results
  x_ptr->sigma = xsection;
  x_ptr->f = freq;


  return (xsection);

}

//OLD /***********************************************************
//OLD 
//OLD   Synopsis: 
//OLD  	double sigma_phot_verner(x_ptr,freq)	calculates the photionization crossection due to the transition 
//OLD  	associated with x_ptr at frequency freq
//OLD  Arguments:
//OLD  
//OLD  Returns:
//OLD  
//OLD  Description:	 
//OLD         Same as sigma_phot but using the older compitation from Verner that includes inner shells
//OLD  
//OLD  Notes:
//OLD  
//OLD  History:
//OLD  	08dec	SS	Coded (actually mostly copied from sigma_phot)
//OLD  
//OLD **************************************************************/


/**********************************************************/
/** @name      sigma_phot_verner
 * @brief      double (x_ptr,freq)	calculates the photionization crossection due to the transition 
 *  	associated with x_ptr at frequency freq (when the data is in the form of the Verner x-sections
 *
 * @param [in] struct innershell *  x_ptr   The stucture that contains information in the format of Verner for 
 * a particular ion level
 * @param [in,out] double  freq   The frequency where the x-section is calculated
 * @return     The photoinization x-section
 *
 * @details
 * Same as sigma_phot but using the older compilation from Verner that includes inner shells
 *
 * ### Notes ###
 * 
 * I (NSH) think this routine has been largely superceeded by the new inner shell formulation of auger ionization.
 * At some poi nt we may wish to expunge the old augerion perts of python.
 *
 **********************************************************/

double
sigma_phot_verner (x_ptr, freq)
     struct innershell *x_ptr;
     double freq;
{
  double ft;
  double y;
  double f1, f2, f3;
  double xsection;

  ft = x_ptr->freq_t;           /* threshold frequency */

  if (ft < freq)
  {
    y = freq / x_ptr->E_0 * HEV;

    f1 = ((y - 1.0) * (y - 1.0)) + (x_ptr->yw * x_ptr->yw);
    f2 = pow (y, 0.5 * x_ptr->P - 5.5 - x_ptr->l);
    f3 = pow (1.0 + sqrt (y / x_ptr->ya), -x_ptr->P);
    xsection = x_ptr->Sigma * f1 * f2 * f3;     // the photoinization xsection

    return (xsection);
  }
  else
    return (0.0);
}


//OLD /***********************************************************
//OLD 				       Space Telescope Science Institute
//OLD 
//OLD Synopsis:
//OLD 
//OLD double den_config(one,nconf) returns the precalculated density
//OLD 	of a particular "nlte" level.	If levels are not defined for an ion it
//OLD 	assumes that all ions of are in the ground state.
//OLD 
//OLD Arguments:
//OLD 
//OLD Returns:
//OLD 
//OLD Description: The first case is when the density of the particular level
//OLD is in the wind array The second caseis when the density of the levels
//OLD for an ion are not tracked, and hence only the total density for the
//OLD ion is known.  In that case, we assume the ion is in it's ground state.
//OLD 
//OLD 
//OLD Notes:
//OLD 
//OLD History:
//OLD 	01oct	ksl	Coded as part of effort to add topbase
//OLD 			xsections and detailed balance to python
//OLD 	05may	ksl	57+ -- Recoded to use PlasmaPtr
//OLD 
//OLD **************************************************************/


/**********************************************************/
/** @name      den_config
 * @brief      returns the precalculated density
 * 	of a particular "nlte" level.	
 *
 * @param [in] PlasmaPtr  xplasma   The Plasma structure containing information about
 * the cell of interest. 
 * @param [in] int  nconf   The running index that describes which level we are interested in
 * @return     The density for a particular level of an ion in the cell
 *
 * @details
 * The first case is when the density of the particular level
 * is in the wind array The second case is when the density of the levels
 * for an ion are not tracked, and hence only the total density for the
 * ion is known.  In that case, we assume the ion is in it's ground state.
 *
 * If levels are not defined for an ion it
 * assumes that all ions of are in the ground state.
 *
 * ### Notes ###
 *
 **********************************************************/

double
den_config (xplasma, nconf)
     PlasmaPtr xplasma;
     int nconf;
{
  double density;
  int nnlev, nion;

  nnlev = config[nconf].nden;
  nion = config[nconf].nion;

  if (nnlev >= 0)
  {                             // Then a "non-lte" level with a density
    density = xplasma->levden[nnlev] * xplasma->density[nion];
  }
  else if (nconf == ion[nion].firstlevel)
  {
/* Then we are using a Topbase photoionization x-section for this ion, 
but we are not storing any densities, and so we assume it is completely in the
in the ground state */
    density = xplasma->density[nion];
  }
  else
  {
    density = 0;
  }

  return (density);


}


//OLD /***********************************************************
//OLD                 Southampton University
//OLD 
//OLD Synopsis: pop_kappa_ff_array populates the multiplicative 	
//OLD 		factor used in the FF calculation. This depends only
//OLD 		on the densities of ions in the cell, and the electron
//OLD 		temperature (which feeds into the gaunt factor) so it
//OLD 		saves time to calculate all this just the once. This
//OLD 		is called in python.c, just before the photons are 
//OLD 		sent thruogh the wind.
//OLD 
//OLD Arguments:		
//OLD 
//OLD Returns:
//OLD  
//OLD Description:	
//OLD 
//OLD Notes:
//OLD 
//OLD 
//OLD History:
//OLD    12oct           nsh     coded 
//OLD    1212	ksl	Added sane check; note that this routine
//OLD    		is poorly documented.  Somewhere this 
//OLD 		should be discribed better.  
//OLD    1407 nsh	changed loop to only go over NPLASMA cells not NPLASMA+1
//OLD  
//OLD **************************************************************/



/**********************************************************/
/** @name      pop_kappa_ff_array
 * @brief      populates the multiplicative 	
 * constant, including a gaunt factor, to be  used in the 
 * calculating free free  emission (and absorption). 
 *
 *
 *
 *
 * @details
 * The routine populates plasmamain[].kappa_ff_factor
 *
 *
 * The free-free multiplicative constant  depends only
 * on the densities of ions in the cell, and the electron
 * temperature (which feeds into the gaunt factor) so it
 * saves time to calculate all this just the once. 
 *
 * ### Notes ###
 * This routine
 * is called just before the photons are 
 * sent through the wind.
 *
 **********************************************************/

double
pop_kappa_ff_array ()
{

  double gsqrd, gaunt, sum;
  int i, j;


  for (i = 0; i < NPLASMA; i++) //Changed to loop only up to NPLASMA, not NPLASMA+1
  {
    sum = 0.0;
    for (j = 0; j < nions; j++)
    {
      if (ion[j].istate != 1)   //The neutral ion does not contribute
      {
        gsqrd = ((ion[j].istate - 1) * (ion[j].istate - 1) * RYD2ERGS) / (BOLTZMANN * plasmamain[i].t_e);
        gaunt = gaunt_ff (gsqrd);
        sum += plasmamain[i].density[j] * (ion[j].istate - 1) * (ion[j].istate - 1) * gaunt;
        if (sane_check (sum))
        {
          Error ("pop_kappa_ff_array:sane_check sum is %e this is a problem, possible in gaunt %3\n", sum, gaunt);
        }
      }
      else
      {
        sum += 0.0;             //add nothing to the sum if we have a neutral ion
      }

    }
    plasmamain[i].kappa_ff_factor = plasmamain[i].ne * sum * 3.692e8;
  }

  return (0);
}




//OLD /***********************************************************
//OLD                 Southampton University
//OLD 
//OLD Synopsis: 
//OLD 	update_banded_estimators updates the estimators required for
//OLD 	mode 7 ionization- the Power law, pairwise, modified saha ionization solver.
//OLD 	It also records the values of IP.
//OLD 
//OLD Arguments:	
//OLD 	xplasma		PlasmaPtr for the cell
//OLD 	p 			Photon pointer
//OLD 	ds 			ds travelled
//OLD 	w_ave 		the weight of the photon. 
//In non macro atom mode,
//OLD 	            this is an average weight (passed as w_ave), but 
//OLD 	            in macro atom mode weights are never reduced (so p->w 
//OLD 	            is used).
//OLD 
//OLD Returns:
//OLD 	increments the estimators in xplasma in the following modes:
//
//
//OLD  
//OLD Description:	
//OLD 	This routine does not contain new code on initial writing, but instead 
//OLD 	moves duplicated code from increment_bf_estimators and radiation() 
//OLD 	to here, as duplicating code is bad practice.
//OLD 
//OLD Notes:
//OLD 
//OLD 
//OLD History:
//OLD    1402 JM 		Coding began
//OLD  
//OLD **************************************************************/


/**********************************************************/
/** @name      update_banded_estimators
 * @brief      updates the estimators required for determining crude
 * spectra in each Plasma cell
 *
 * @param [in,out] PlasmaPtr  xplasma   PlasmaPtr for the cell of interest
 * @param [in] PhotPtr  p   Photon pointer
 * @param [in] double  ds   ds travelled
 * @param [in] double  w_ave   the weight of the photon in the cell. 
 *
 * @return  Always returns 0
 *
 *
 *
 * @details
 * 
 * Increments the estimators that allow one to construct a crude
 * spectrum in each cell of the wind.  The frequency intervals
 * in which the spectra are constructed are in geo.xfreq. This information
 * is used in different ways (or not at all) depending on the ionization mode.
 *
 * It also records the values of IP.
 *
 * ### Notes ###
 * In non macro atom mode, w_ave
 * this is an average weight (passed as w_ave), but 
 * in macro atom mode weights are never reduced (so p->w 
 * is used).
 *
 * This routine is called from bf_estimators in macro_atom modes
 * and from radiation (above).  Although the historical documentation
 * suggests it is only called for certain ionization modes, it appears
 * to be called in all cases, though clearly it is only provides diagnostic
 * information in some of them.
 *
 **********************************************************/

int
update_banded_estimators (xplasma, p, ds, w_ave)
     PlasmaPtr xplasma;
     PhotPtr p;
     double ds;
     double w_ave;
{
  int i;

  /*photon weight times distance in the shell is proportional to the mean intensity */
  xplasma->j += w_ave * ds;

  if (p->nscat == 0)
  {
    xplasma->j_direct += w_ave * ds;
  }
  else
  {
    xplasma->j_scatt += w_ave * ds;
  }



/* frequency weighted by the weights and distance       in the shell .  See eqn 2 ML93 */
  xplasma->mean_ds += ds;
  xplasma->n_ds++;
  xplasma->ave_freq += p->freq * w_ave * ds;



  /* 1310 JM -- The next loop updates the banded versions of j and ave_freq, analogously to routine inradiation
     nxfreq refers to how many frequencies we have defining the bands. So, if we have 5 bands, we have 6 frequencies, 
     going from xfreq[0] to xfreq[5] Since we are breaking out of the loop when i>=nxfreq, this means the last loop 
     will be i=nxfreq-1 */

  /* note that here we can use the photon weight and don't need to calculate anm attenuated average weight
     as energy packets are indisivible in macro atom mode */


  for (i = 0; i < geo.nxfreq; i++)
  {
    if (geo.xfreq[i] < p->freq && p->freq <= geo.xfreq[i + 1])
    {

      xplasma->xave_freq[i] += p->freq * w_ave * ds;    /* frequency weighted by weight and distance */
      xplasma->xsd_freq[i] += p->freq * p->freq * w_ave * ds;   /* input to allow standard deviation to be calculated */
      xplasma->xj[i] += w_ave * ds;     /* photon weight times distance travelled */
      xplasma->nxtot[i]++;      /* increment the frequency banded photon counter */

      /* work out the range of frequencies within a band where photons have been seen */
      if (p->freq < xplasma->fmin[i])
      {
        xplasma->fmin[i] = p->freq;
      }
      if (p->freq > xplasma->fmax[i])
      {
        xplasma->fmax[i] = p->freq;
      }

    }
  }

  /* NSH 131213 slight change to the line computing IP, we now split out direct and scattered - this was 
     mainly for the progha_13 work, but is of general interest */
  /* 70h -- nsh -- 111004 added to try to calculate the IP for the cell. Note that 
   * this may well end up not being correct, since the same photon could be counted 
   * several times if it is rattling around.... */

  /* 1401 JM -- Similarly to the above routines, this is another bit of code added to radiation
     which originally did not get called in macro atom mode. */

  /* NSH had implemented a scattered and direct contribution to the IP. This doesn't really work 
     in the same way in macro atoms, so should instead be thought of as 
     'direct from source' and 'reprocessed' radiation */

  if (HEV * p->freq > 13.6)     // only record if above H ionization edge
  {

    /* IP needs to be radiation density in the cell. We sum contributions from
       each photon, then it is normalised in wind_update. */
    xplasma->ip += ((w_ave * ds) / (H * p->freq));

    if (HEV * p->freq < 13600)  //Tartar et al integrate up to 1000Ryd to define the ionization parameter
    {
      xplasma->xi += (w_ave * ds);
    }

    if (p->nscat == 0)
    {
      xplasma->ip_direct += ((w_ave * ds) / (H * p->freq));
    }
    else
    {
      xplasma->ip_scatt += ((w_ave * ds) / (H * p->freq));
    }
  }

  return (0);
}



//OLD /*************************************************************
//OLD Synopsis: 
//OLD 	mean_intensity returns a value for the mean intensity 
//OLD 
//OLD Arguments:	
//OLD 	xplasma 		PlasmaPtr for the cell - supplies spectral model
//OLD 	freq 			the frequency at which we want to get a value of J
//OLD 	mode 			mode 1=use BB if we have not yet completed a cycle
//OLD 				and so dont have a spectral model, mode 2=never use BB
//OLD 
//OLD Returns:
//OLD  
//OLD Description:
//OLD    This subroutine returns a value for the mean intensity J at a 
//OLD    given frequency, using either a dilute blackbody model
//OLD    or a spectral model depending on the value of geo.ioniz_mode. 
//OLD    to avoid code duplication.
//OLD 
//OLD Notes:
//OLD    This subroutine was produced
//OLD    when we started to use a spectral model to populaste the upper state of a
//OLD    two level atom, as well as to calculate induced compton heating. This was
//OLD 
//OLD History:
//OLD    1407 NSH 		Coding began
//OLD  
//OLD **************************************************************/




/**********************************************************/
/** @name      mean_intensity
 * @brief      returns a value for the mean intensity
 *
 * @param [in] PlasmaPtr  xplasma   PlasmaPtr for the cell - supplies spectral model
 * @param [in] double  freq   the frequency at which we want to get a value of J
 * @param [in] int  mode   mode 1=use BB if we have not yet completed a cycle
 * @return     The mean intensity at a specific frequency
 *
 * @details
 * This subroutine returns a value for the mean intensity J at a 
 * given frequency, using either a dilute blackbody model
 * or a spectral model depending on the value of geo.ioniz_mode. 
 * to avoid code duplication.
 *
 * For ionization models that make use of the crude spectra accumulated
 * in crude spectral bands, the routine uses these bands to
 * get the mean intensity.  If however, one is using one of the other
 * (older) ionization modes, then the input variable mode drives how
 * the mean intensity is calculated.mode appears to be used 
 *
 * ### Notes ###
 * This subroutine was produced
 * when we started to use a spectral model to populate the upper state of a
 * two level atom, as well as to calculate induced compton heating. 
 *
 * @bug   The routine refers to a mode 5, which does not appear to 
 * exist, or at least it is not one that is included in python.h
 * Note also that the logic of this appears overcomplicated, reflecting
 * the evolution of banding, and various ionization modes being added
 * without looking at trying to make this simpler.
 *
 **********************************************************/

double
mean_intensity (xplasma, freq, mode)
     PlasmaPtr xplasma;         // Pointer to current plasma cell
     double freq;               // Frequency of the current photon being tracked
     int mode;                  // mode 1=use BB if no model, mode 2=never use BB

{
  int i;
  double J;
  double expo;

  J = 0.0;                      // Avoid 03 error


  if (geo.ioniz_mode == 5 || geo.ioniz_mode == IONMODE_PAIRWISE_SPECTRALMODEL || geo.ioniz_mode == IONMODE_MATRIX_SPECTRALMODEL)        /*If we are using power law ionization, use PL estimators */
  {
    if (geo.spec_mod > 0)       /* do we have a spextral model yet */
    {
      for (i = 0; i < geo.nxfreq; i++)
      {
        if (geo.xfreq[i] < freq && freq <= geo.xfreq[i + 1])    //We have found the correct model band
        {
          if (xplasma->spec_mod_type[i] > 0)    //Only bother if we have a model in this band
          {

            if (freq > xplasma->fmin_mod[i] && freq < xplasma->fmax_mod[i])     //The spectral model is defined for the frequency in question
            {

              if (xplasma->spec_mod_type[i] == SPEC_MOD_PL)     //Power law model
              {
                J = pow (10, (xplasma->pl_log_w[i] + log10 (freq) * xplasma->pl_alpha[i]));
              }

              else if (xplasma->spec_mod_type[i] == SPEC_MOD_EXP)       //Exponential model
              {
                J = xplasma->exp_w[i] * exp ((-1 * H * freq) / (BOLTZMANN * xplasma->exp_temp[i]));
              }
              else
              {
                Error ("mean_intensity: unknown spectral model (%i) in band %i\n", xplasma->spec_mod_type[i], i);
                J = 0.0;        //Something has gone wrong
              }
            }

            else
            {
              /* We have a spectral model, but it doesnt apply to the frequency 
                 in question. clearly this is a slightly odd situation, where last
                 time we didnt get a photon of this frequency, but this time we did. 
                 Still this should only happen in very sparse cells, so induced compton 
                 is unlikely to be important in such cells. We generate a warning, just 
                 so we can see if this is happening a lot */
              J = 0.0;

              /* JM140723 -- originally we threw an error here. No we count these errors and 
                 in wind_updates because you actually expect 
                 it to happen in a converging run */
              nerr_Jmodel_wrong_freq++;
            }
          }
          else                  /* There is no model in this band - this should not happen very often  */
          {
            J = 0.0;            //There is no model in this band, so the best we can do is assume zero J

            /* JM140723 -- originally we threw an error here. No we count these errors and 
               in wind_updates because you actually expect 
               it to happen in a converging run */
            nerr_no_Jmodel++;
          }



        }
      }
    }
    else                        //We have not completed an ionization cycle, so no chance of a model
    {
      if (mode == 1)            //We need a guess, so we use the initial guess of a dilute BB
      {
        expo = (H * freq) / (BOLTZMANN * xplasma->t_r);
        J = (2 * H * freq * freq * freq) / (C * C);
        J *= 1 / (exp (expo) - 1);
        J *= xplasma->w;
      }
      else                      //A guess is not a good idea (i.e. we need the model for induced compton), so we return zero.
      {
        J = 0.0;
      }

    }
  }

  else                          /*Else, use dilute BB estimator of J */
  {
    expo = (H * freq) / (BOLTZMANN * xplasma->t_r);
    J = (2 * H * freq * freq * freq) / (C * C);
    J *= 1 / (exp (expo) - 1);
    J *= xplasma->w;
  }

  return J;
}
