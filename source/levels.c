
/***********************************************************/
/** @file  levels.c
 * @author ksl
 * @date   January, 2018
 *
 * @brief  This file contains a routine to calculate level populations
 * in ions for LTE and related assumptions.
 *
 ***********************************************************/

//OLD /**************************************************************************
//OLD                     Space Telescope Science Institute
//OLD 
//OLD 
//OLD   Synopsis:
//OLD 	levels (xplasma, mode) calculates the fractional occupation numbers of
//OLD 	the various levels of atomic configurations as designated in
//OLD 	the atomic data files
//OLD 
//OLD   Description:
//OLD 
//OLD 	mode	0	LTE with t_r
//OLD 		1	LTE with t_e
//OLD 		2	Non-LTE (reduced by weighted BB)
//OLD 
//OLD   Arguments:
//OLD 
//OLD   Returns:
//OLD 
//OLD   Notes:
//OLD 
//OLD 	0808 - ksl - levels populates the levden array in the Plasma pointer.  It
//OLD 		is called from ion_abundances in python, and is called directly
//OLD 		from my diagnostic routine balance.  It's closely related to
//OLD 		another but separate routine which calculates the partition
//OLD 		functions, callled partition
//OLD 
//OLD   History:
//OLD 	01sep23	ksl	Began work
//OLD 	01oct10	ksl	Modified so modes matched python ionization modes
//OLD 			more exactly.
//OLD 	01dec03	ksl	Modified to simplify so modes match those of
//OLD 			nebular concentrations
//OLD 	01dec12	ksl	Modified to react to changes which split "nlte"
//OLD 			and "lte" levels.  Levels is explicitly for so-
//OLD 			called "nlte" levels, which are tracked in the
//OLD 			Wind structure
//OLD 	04Apr   SS      If statement added to avoid this routine changing
//OLD                         macro atom level populations.
//OLD         04May   SS      The if statment added above is modified for the case
//OLD                         of all "simple" ions.
//OLD 	06may	ksl	57+ -- Modified to make use of plasma structue
//OLD 	080810	ksl	62 - Fixed problem with how the levden array was
//OLD 			indexed.  The problem was that the index into the
//OLD 			levden array is not the same as the index into
//OLD 			the so called nlte configurations.
//OLD 			Also made the actual use of variables
//OLD 			like nion,n,m resemble that in partitions so the
//OLD 			routine was easier to compae
//OLD 
//OLD  ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "atomic.h"
#include "python.h"


/**********************************************************/
/** 
 * @brief      Calculates the fractional occupation numbers of
 * 	the various  of atomic configurations assuming LTE or various
 * 	alternatives to LTE
 *
 * @param [in] PlasmaPtr  xplasma   A single element of the Plasma structure
 * @param [in] int  mode  An integer which determines the way in which the partition funciton will be calculated 
 * @return     Always returns 0
 *
 * @details
 * 	The possibilites are:
 *
 * * NEBULARMODE_TR 0        LTE using t_r
 * * NEBULARMODE_TE 1        LTE using t_e
 * * NEBULARMODE_ML93 2      ML93 using a nebular approximation correction to LTE
 * * NEBULARMODE_NLTE_SIM 3  // Non_LTE with SS modification (Probably could be removed)
 * * NEBULARMODE_LTE_GROUND 4        // A test mode which forces all levels to the GS (Probably could be removed)
 *
 * levels populates the levden array in the Plasma pointer.  It
 * is called from the routine partition
 *
 * ### Notes ###
 *
 * @bug  Some rethinking of the whole level density approach needs to be done.  It's not
 * clear how these functions are being used and what difference if any that they make.  
 *
 **********************************************************/

int
levels (xplasma, mode)
     PlasmaPtr xplasma;
     int mode;
{
  double t, weight;
  int n, m;
  int nion;
  int m_ground;
  int nlevden;
  double kt;
  double z;

  if (mode == NEBULARMODE_TR)   // LTE with t_r
  {
    t = xplasma->t_r;
    weight = 1;
  }
  else if (mode == NEBULARMODE_TE)      // LTE with t_e
  {
    t = xplasma->t_e;
    weight = 1;
  }
  else if (mode == NEBULARMODE_ML93)    // non_LTE with t_r and weights
  {
    t = xplasma->t_r;
    weight = xplasma->w;
  }
  else if (mode == NEBULARMODE_NLTE_SIM)        /* non_LTE with SS modification NSH 120912 - This mode is more or less defunct. 
                                                   It can be romoved once all the viestiges of the original PL ioinzation scheme are removed */
  {
    t = xplasma->t_e;
    weight = 1;
  }
  else if (mode == NEBULARMODE_LTE_GROUND)      /* A test mode - this is to allow all levels to be set to GS, in the event we dont have a 
                                                   good idea of what the radiation field shoulb be. */
  {
    t = xplasma->t_e;
    weight = 0;
  }
  else
  {
    Error ("levels: Could not calculate levels for mode %d\n", mode);
    exit (0);
  }

  /* Next calculation should be almost identical to that contained in
     the routine partition.  An error in one place implies one in the
     other.  It seems like that this routine should be subsumed into
     that one ????  */

  kt = BOLTZMANN * t;

  for (nion = 0; nion < nions; nion++)
  {
    if (ion[nion].nlte > 0)
      /* Extra if statement added to prevent changing levden of macro atom populations (SS, Apr04) */
    {
      if (ion[nion].macro_info == 0 || geo.macro_ioniz_mode == 0)
      {                         //Then calculate levels for this ion 

        z = xplasma->partition[nion];

        /* N.B. partition functions will most likely have been 
           calculated from "lte" levels, at least for * now ??  */

        m = ion[nion].first_nlte_level;
        m_ground = m;           //store the ground state index - allow for gs energy neq 0 (SS) 
        nlevden = ion[nion].first_levden;
        xplasma->levden[nlevden] = config[m].g / z;     //Assumes first level is ground state
        for (n = 1; n < ion[nion].nlte; n++)
        {
          m++;
          nlevden++;
          xplasma->levden[nlevden] = weight * config[m].g * exp ((-config[m].ex + config[m_ground].ex) / kt) / z;
        }
      }
    }
  }

  return (0);

}
