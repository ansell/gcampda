/** 
 *   csi: 
 *
 *
 *   (c) Copyright 2010 Federico Bareilles <bareilles@gmail.com>,
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *     
 *   The author does NOT admit liability nor provide warranty for any
 *   of this software. This material is provided "AS-IS" in the hope
 *   that it may be useful for others.
 *
 **/
#define _GNU_SOURCE

#include "pakbus.h"



#if 1
/** 
 signature algorithm. 
 
 Standard signature is initialized with a seed of 0xaaaa. Returns
 signature. If the function is called on a partial data set, the
 return value should be used as the seed for the remainder
**/
uint16_t calc_sig_for(void const *buff, uint32_t len, uint16_t seed)
{
	uint16_t n;
	uint16_t j;
	uint16_t rtn = seed;
	unsigned char const *str = (unsigned char const *) buff;
	// calculate a representative number for the byte
	// block using the CSI signature algorithm.
	for (n = 0; n < len; n++) {
		j = rtn;
		rtn = (rtn << 1) & (uint16_t) 0x01ff;
		if (rtn >= 0x100)
			rtn++;
		rtn = (((rtn + (j >> 8) + str[n]) & (uint16_t) 0xff) | (j << 8));
	}

	return rtn;
}


uint16_t calc_sig_nullifier(uint16_t sig)
{
	/* calculate the value for the most significant byte then run
	 this value through the signature algorithm using the
	 specified signature as seed.  The calculation is designed to
	 cause the least significant byte in the signature to become
	 zero.
	*/
	uint16_t new_seed = (sig << 1)&0x1FF;
	unsigned char null1, null2;
	uint16_t new_sig = sig;
	if(new_seed >= 0x0100)
		new_seed++;
	null1 = (unsigned char) (0x0100 - (new_seed + (sig >> 8)));
	new_sig = calc_sig_for(&null1, 1, sig);
	/* now perform the same calculation for the most significant
	 byte in the signature. This time we will use the signature
	 that was calculated using the first null byte
	*/
	new_seed = (new_sig << 1)&0x01FF;
	if(new_seed >= 0x0100)
		new_seed++;
	null2 = (unsigned char) (0x0100 - (new_seed + (new_sig >> 8)));
	/* now form the return value placing null in the most
	   significant byte location */
	uint16_t rtn = null1;
	rtn <<= 8;
	rtn += null2;

	return rtn;
}



#elif 0
/**
 * Function to compute the signature of a byte sequence using the 
 * CSI algorithm.
 */
uint16_t calc_sig_for(const void *buf, uint32_t len, uint16_t seed)
{
	uint32_t n;
	uint16_t j;
	uint16_t ret = seed;
	char *ptr = (char *) buf;
	
	for (n = 0; n < len; n++) {
		j = ret;
		ret = (ret << 1) & (uint16_t) 0x01ff;
		if(ret >= 0x100){
			ret++;
		}
		ret = (((ret + (j >> 8) + ptr[n]) & (uint16_t)0xff) | (j << 8));
	}

	return ret;
}

/**
 * This function computes the nullifier of a 2-byte signature computed
 * by the CSI algorithm.
 */
uint16_t calc_sig_nullifier(uint16_t sig)
{
    uint16_t tmp, signull;
    char  null0, null1, msb;

    tmp = (uint16_t)(0x1ff & (sig << 1));
    if (tmp >= 0x100) {
        tmp += 1;
    }
    null1 = (0x00ff & (0x100 - (sig >> 8) - (0x00ff & tmp)));
    msb = (char) (0x00ff & sig);
    null0 = (0x00ff & (0x100 - ((0xff & msb))));
    signull = 0xffff & ((null1 << 8) + null0);

    return signull;
}
#else


// signature algorithm.
// Standard signature is initialized with a seed of
// 0xaaaa. Returns signature. If the function is called
// on a partial data set, the return value should be
// used as the seed for the remainder
typedef unsigned short uint2;
typedef unsigned long uint4;
typedef unsigned char byte;

uint2 calcSigFor(void const *buff, uint4 len, uint2 seed)
{
  uint2 j, n;
  uint2 rtn = seed;
  byte const *str = (byte const *)buff;
  // calculate a representative number for the byte
  // block using the CSI signature algorithm.
  for (n = 0; n < len; n++)
  {
    j = rtn;
    rtn = (rtn << 1) & (uint2)0x01FF;
    if (rtn >= 0x100)
    rtn++;
    rtn = (((rtn + (j >> 8) + str[n]) & (uint2)0xFF) | (j << 8));
  }
  return rtn;
} // calcSigFor


uint2 calcSigNullifier(uint2 sig)
{
  // calculate the value for the most significant
  // byte then run this value through the signature
  // algorithm using the specified signature as seed.
  // The calculation is designed to cause the least
  // significant byte in the signature to become zero.
  uint2 new_seed = (sig << 1)&0x1FF;
  byte null1;
  uint2 new_sig = sig;
  if(new_seed >= 0x0100)
     new_seed++;
  null1 = (byte)(0x0100 - (new_seed + (sig >> 8)));
  new_sig = calcSigFor(&null1,1,sig);
  // now perform the same calculation for the most significant byte
  // in the signature. This time we will use the signature that was
  // calculated using the first null byte
  byte null2;
  new_seed = (new_sig << 1)&0x01FF;
  if(new_seed >= 0x0100)
     new_seed++;
  null2 = (byte)(0x0100 - (new_seed + (new_sig >> 8)));
  // now form the return value placing null in the most
  // significant byte location
  uint2 rtn = null1;
  rtn <<= 8;
  rtn += null2;
  return rtn;
}// calcSigNullifier


#endif
