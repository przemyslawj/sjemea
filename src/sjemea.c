#include <R.h>
#include <S.h>			/* for seed_in, seed_out */

/* We use this SMALLVAL to test whether two floating point values can
 * be regarded as "equal": we see if the absolute difference between
 * the two values is less than this SMALLVAL. */
#define SMALLVAL 1e-12


/* Theory of binning numbers.  Seems quite straightforward, but there
 * is a little thing to worry about it: numbers falling onto a bin
 * edge.  If we want to bin a set of numbers that vary between MIN and
 * MAX into n bins, we make each bin of width w:
 *   w = (MAX-MIN) / n
 * and then for a value x, we increment bin b, where
 *   b =  (int) ( (x - MIN)/ w)
 * where (int) rounds down to nearest integer.  This makes each bin of
 * the type [low, high).  So, for MIN=0, MAX=3, n=3, the overall range
 * is divided into say 3 bins like: [0, 1) [1, 2) [2, 3).  This means
 * that the overall range of the histogram will be [0,3).  So, if we
 * try to bin a value of x=3 (the max value), it falls outside the
 * last bin.  In R, this problem is overcome using the INCLUDE.LOWEST
 * variable.  In this C code, we explicitly test for this case, so the
 * max value is included in the last bin.
 *
 * For more information on binning, see David Young's help for the
 * POP-11 routine array_hist, included at the end of this file.
 */
 

void count_overlap(Sfloat *a, int *pna, Sfloat *b, int *pnb, Sfloat *pdt,
		  int *res)
{
  /* A[i] is the time of the ith spike in cell A.  A has *PNA spikes.
   * Likewise for cell B.  Assume that spike times are ordered,
   * earliest first.  We return (in *RES) the number of spikes in B
   * that occur within +/- dt of a spike in A.
   */

  int count=0;
  int sa, sb, low;
  Sfloat alow, ahigh, dt;
  int na, nb;

  na = *pna; nb = *pnb; dt=*pdt;

  /* low is the index of first spike time in train b that could be
   * withing the range [a-dt, a+dt] of the spike in a.  We rely on the
   * spike times being ordered to use this trick.
   */
   
  low = 0;
  for( sa=0; sa < na; sa++) {
    alow = a[sa]-dt; ahigh= a[sa]+dt;
    for(sb=low; sb < nb;sb++) {
      if (b[sb] > ahigh)
	sb = nb;		/* stop checking this train */
      else			/* spike could be in [alow, ahigh] */
	if (b[sb] >= alow)	/* equality holds when delta_t is -tmax */
	  count++;
        else
	  low = sb;		/* can ignore this spike in B from now on. */
    }
  }
  *res = count;
}

void bin_overlap(Sfloat *a, int *pna, Sfloat *b, int *pnb, Sfloat *pdt,
		 int *bins, int *pnbins)
{
  /* bin_overlap is similar to count_overlap except that we return a
   * histogram which bins the time difference between spikes into one
   * of several time bins.  The maximum absolute time difference is
   * *PDT; the histogram *INS returned is on length *PBINS.  Here we
   * ignore whether the time difference is +ve or -ve.  To check that
   * the binning is working, run the R function test.count.hist.nab().
   * Each histogram bin is of the form [low1, high1) with the last bin
   * specially set to [lown,highn], so the overall range of this
   * histogram is [0,T], where T=*PDT.
   */
  
  int sa, sb, low;
  Sfloat alow, ahigh, dt, delta_t, bin_wid, max_val;
  int na, nb;
  int bin_num, nbins;

  na = *pna; nb = *pnb; dt=*pdt; nbins = *pnbins;
  max_val = dt;
  bin_wid = dt/(double)nbins;
  /* low is the index of first spike time in train b that could be
   * withing the range [a-dt, a+dt] of the spike in a.  We rely on the
   * spike times being ordered to use this trick.
   */
   
  low = 0;
  for( sa=0; sa < na; sa++) {
    alow = a[sa]-dt; ahigh= a[sa]+dt;
    for(sb=low; sb < nb;sb++) {
      if (b[sb] > ahigh)
	sb = nb;		/* stop checking this train */
      else			/* spike could be in [alow, ahigh] */
	if (b[sb] >= alow) {	/* equality when (deltat == -tmax) */
	  /* need to bin this value. */
	  delta_t = fabs( b[sb] - a[sa]);
	  bin_num = (int) (delta_t / bin_wid);
	  if ((bin_num == nbins) &&(fabs(delta_t - max_val) < SMALLVAL))
	    bin_num--;		/* put max value into largest bin. */
	  if ( (bin_num <0 ) || (bin_num >= nbins))
	    Rprintf("bin number wrong %f %d\n", delta_t, bin_num);
	  else {
	    bins[bin_num]++;
	  }
	} else
	  low = sb;		/* can ignore this spike in B from now on. */
    }
  }
}


void bin2_overlap(Sfloat *a, int *pna, Sfloat *b, int *pnb, Sfloat *pdt,
		 int *bins, int *pnbins)
{
  /* bin2_overlap is a bidirectional version of bin_overlap.
   * This time the sign of the time difference between spikes is important.
   * Each histogram bin is of the form [low1, high1) with the last bin
   * specially set to [lown,highn], so the overall range of this
   * histogram is [-T,T] where T=*PDT.
   */

  int sa, sb, low;
  Sfloat alow, ahigh, dt, delta_t, bin_wid, min_val, max_val;
  int na, nb;
  int bin_num, nbins, bin_numi;

  na = *pna; nb = *pnb; dt=*pdt; nbins = *pnbins;
  min_val = 0.0-dt;		/* smallest value that we will bin. */
  max_val = dt;
  /* the range of times is now [-dt,dt], so we need to double dt. */
  bin_wid = (2.0 * dt)/nbins;
  /*Rprintf("bin_wid %f\n", bin_wid);*/
  /* low is the index of first spike time in train b that could be
   * withing the range [a-dt, a+dt] of the spike in a.  We rely on the
   * spike times being ordered to use this trick.
   */
   
  low = 0;
  for( sa=0; sa < na; sa++) {
    alow = a[sa]-dt; ahigh= a[sa]+dt;
    for(sb=low; sb < nb;sb++) {
      if (b[sb] > ahigh)	/* >= for case when at max. */
	sb = nb;		/* stop checking this train */
      else			/* spike could be in [alow, ahigh] */
	if (b[sb] >= alow) {	/* equality when (deltat == -tmax) */
	  /* need to bin this value. */
	  delta_t = ( b[sb] - a[sa]);

	  /* Compute bin number using both floor and casting to an
	   * int.  This is temporary code as I think I found that
	   * sometimes the "self spike" for auto-correlations was put
	   * in the wrong bin.  If we get this error, we are in
	   * trouble.
	   */
	  bin_numi = (int)((delta_t - min_val)/ bin_wid);
	  bin_num = (int)floor((delta_t - min_val)/ bin_wid);
	  /*
	  if (bin_num != bin_numi) {
	    Rprintf("XXX different bin numbers: dt %f min %f wid %f floor %d (int) %d %f\n",
		    delta_t, min_val, bin_wid,
		    bin_num, bin_numi, delta_t);
	  }
	  */
	  if ( (bin_num == nbins) &&(fabs(delta_t - max_val) < SMALLVAL))
	    bin_num--;		/* fits into largest bin. */
	  if ( (bin_num <0 ) || (bin_num >= nbins))
	    Rprintf("bin2: number wrong %f %d %f\n",delta_t, bin_num, bin_wid);
	  else {
	    /* When making auto-correlation, see which bin the
	     * "self-comparison" is put into:
	     */
	    /* if (sa==sb) */
	    /*Rprintf("%d: when binning own spike (time %f), dt %f bin %d\n",*/
	    /*sa, a[sa], delta_t, bin_num);*/

	    bins[bin_num]++;
	  }
	} else
	  low = sb;		/* can ignore this spike in B from now on. */
    }
  }
}

/**********************************************************************/
/*
HELP ARRAY_HIST                             David Young, January 1994


LIB *array_hist provides a procedure for obtaining a histogram of the
values in a region of an array.  All the values must be numbers.

         CONTENTS - (Use <ENTER> g to access required sections)

 -- Procedure array_hist
 -- Counting integer values
 -- Counting floating point values
 -- External optimisation
 -- Re-using histogram vectors
 -- Offsetting results in the vector

-- Procedure array_hist -----------------------------------------------

array_hist(__array, region, low, __nbins, _high) -> (_nlow, _hist, __nhigh)

        The histogram is formed for values in the part of __array
        specified by region.  The list region is in *boundslist style
        (i.e. first two elements give range of indices in first
        dimension, next two give range in second dimension, etc.). If
        region is false, the whole of the array is examined.

        The numbers low and _high give the overall range of values to
        count in the histogram. (The procedure *array_mxmn may be useful
        for obtaining these in the general case.) The range between low
        and _high is divided into __nbins equal parts.

        The bin width (the range of values that get counted in one bin)
        is given by

            __binwidth = (_high - low) / __nbins

        The result _hist is a vector containing counts of the values in
        each bin. The results _nlow and __nhigh return the counts of values
        that fell outside the range covered by _hist.

        To be precise, low is the smallest value to get counted in the
        first bin and _high is the smallest value _just too __large to get
        counted in the last bin. (This means that the treatment of
        integers and floats can be consistent.) A value _V from the array
        is treated as follows:

            _V < low:        increment _nlow
            _V >= _high:      increment __nhigh
            otherwise:      increment _hist(_I) where

                 _I = floor( (_V - low) * __nbins / (high - low) ) + 1

        Apart from rounding errors, this means that in the last case _I
        is chosen such that

                low + __binwidth * (_I - 1) <= _V < low + __binwidth * _I

        (The floor function returns the largest integer less than or
        equal to its argument.)

        It is possible to re-use vectors and to place the counts in the
        vector starting from some element other than the first. These
        options are described below.

-- Counting integer values --------------------------------------------

Suppose the values in __array are integers in the range 0 ... 255, and we
want to know how many of each there are.  The correct call is

    array_hist(array, false, 0, 256, 256) -> (nlow, hist, nhigh);

The __nbins argument is 256 because there are 256 different values to
count. Note that _high is 256, not 255, because it must be the next value
above the top of the histogram range. To make the bin width equal to 1,
we need

    _high = low + __nbins

The element _hist(_I) will contain the number of values in the array equal
to _I-1.  The -1 is necessary because the values start at 0, but vectors
are indexed from 1.

In general, for integer values to be counted properly, with _K different
values counted in each bin, we need

    _high = low + _K * __nbins

and the _I'th element of _hist will contain the count for values in the
range low + _K * (_I-1) to low + (_K+1) * (_I-1) - 1.

To sum up, to count integers in the range __N0 to __N1 inclusive you should
use:

    low = __N0
    _high = __N1 + _1

and the number of different values counted in each bin will be

    _K = (_high - low) / __nbins

with __nbins chosen to make _K an integer.

For example, we can look at the performance of the POP-11 random number
generator by filling an array with random numbers in the range 1 to 16
and looking at its histogram.

    vars arr, nlo, hist, nhi;
    newarray([1 1000], erase <> random(% 16 %)) -> arr;
    array_hist(arr, false, 1, 16, 17) -> (nlo, hist, nhi);

    nlo =>
    ** 0
    hist =>
    ** {59 62 67 75 60 59 66 61 56 67 47 64 60 70 58 69}
    nhi =>
    ** 0

As expected, no values are less then 1 or greater than or equal to 17,
and the 1000 values are reasonably evenly distributed. (You will not get
an identical distribution if you try this.)

-- Counting floating point values -------------------------------------

Counting floating point values ("decimals" in POP-11) is usually
simpler, as low and _high then normally correspond exactly to the range
of interest.  For example, to test the performance of the random number
generator on floats, we can fill an array with numbers from 0.0 to 1.0
and look at its histogram in much the same way as before:

    newarray([1 1000], erase <> random0(% 1.0 %)) -> arr;
    array_hist(arr, false, 0.0, 16, 1.0) -> (nlo, hist, nhi);

The results will be similar to the previous example. The bin width in
this case is 0.0625 - sixteen of these cover the range from 0.0 to 1.0.

Rounding errors mean that values on, or very close to, bin boundaries
may get counted in the wrong bin.  This risk is inevitable with floating
point calculations.  If the values fall into natural groups, the problem
can be eliminated by putting the bin boundaries firmly into the gaps.
For example, if the values are whole numbers (although represented as
floats) in the range A0 to A1 inclusive, and the bin width is to be 1,
then it would be sensible to use

    low = __A0 - 0.5
    _high = __A1 + 0.5
    __nbins = round(_high - low)

However, this should not be done if the values are actually represented
as integers - see the section above.

-- External optimisation ----------------------------------------------

Two cases are dealt with using external code, for much increased speed:

    1. __array is a packed array of single precision floating point
    values, as produced for example by *newsfloatarray.

    2. __array is a packed array of bytes, as produced for example by
    *newbytearray, and both low and _high are integers.

The result _hist will be an *INTVEC.

-- Re-using histogram vectors -----------------------------------------

It is possible to re-use a histogram vector to avoid creating garbage,
by passing it as an argument as __nbins (instead of an integer as above).
The counts will be stored in it and it will be returned.  The length of
the vector becomes the number of bins.

If the conditions for an external procedure call are satisfied, then it
will be most efficient to make the vector an *INTVEC.

-- Offsetting results in the vector -----------------------------------

It may be useful to place the counts in part of the vector, not
necessarily starting at the first element.  This can be done by passing
a list as __nbins, with three elements:

    _startindex: the index of the first bin
    __nbins: the number of bins
    veclen: the length of the vector

The _hist result will then be of length veclen with the counts in the
elements from _startindex to _startindex + __nbins - 1.

If a vector is to be re-used, it can be given as the third element of
the list, in place of veclen.
*/



