// stb_dxt.h - v1.10 - DXT1/DXT5 compressor - public domain
// original by fabian "ryg" giesen - ported to C by stb
// use '#define STB_DXT_IMPLEMENTATION' before including to create the implementation
//
// USAGE:
//   call stb_compress_dxt_block() for every block (you must pad)
//     source should be a 4x4 block of RGBA data in row-major order;
//     Alpha channel is not stored if you specify alpha=0 (but you
//     must supply some constant alpha in the alpha channel).
//     You can turn on dithering and "high quality" using mode.
//
// version history:
//   v1.10  - (i.c) various small quality improvements
//   v1.09  - (stb) update documentation re: surprising alpha channel requirement
//   v1.08  - (stb) fix bug in dxt-with-alpha block
//   v1.07  - (stb) bc4; allow not using libc; add STB_DXT_STATIC
//   v1.06  - (stb) fix to known-broken 1.05
//   v1.05  - (stb) support bc5/3dc (Arvids Kokins), use extern "C" in C++ (Pavel Krajcevski)
//   v1.04  - (ryg) default to no rounding bias for lerped colors (as per S3TC/DX10 spec);
//            single color match fix (allow for inexact color interpolation);
//            optimal DXT5 index finder; "high quality" mode that runs multiple refinement steps.
//   v1.03  - (stb) endianness support
//   v1.02  - (stb) fix alpha encoding bug
//   v1.01  - (stb) fix bug converting to RGB that messed up quality, thanks ryg & cbloom
//   v1.00  - (stb) first release
//
// contributors:
//   Rich Geldreich (more accurate index selection)
//   Kevin Schmidt (#defines for "freestanding" compilation)
//   github:ppiastucki (BC4 support)
//   Ignacio Castano - improve DXT endpoint quantization
//
// LICENSE
//
//   See end of file for license information.

#ifndef STB_INCLUDE_STB_DXT_H
#define STB_INCLUDE_STB_DXT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef STB_DXT_STATIC
#define STBDDEF static
#else
#define STBDDEF extern
#endif

// compression mode (bitflags)
#define STB_DXT_NORMAL    0
#define STB_DXT_DITHER    1   // use dithering. dubious win. never use for normal maps and the like!
#define STB_DXT_HIGHQUAL  2   // high quality mode, does two refinement steps instead of 1. ~30-40% slower.

STBDDEF void stb_compress_dxt_block(unsigned char *dest, const unsigned char *src_rgba_four_bytes_per_pixel, int alpha, int mode);
STBDDEF void stb_compress_bc4_block(unsigned char *dest, const unsigned char *src_r_one_byte_per_pixel);
STBDDEF void stb_compress_bc5_block(unsigned char *dest, const unsigned char *src_rg_two_byte_per_pixel);

#define STB_COMPRESS_DXT_BLOCK

#ifdef __cplusplus
}
#endif
#endif // STB_INCLUDE_STB_DXT_H

#ifdef STB_DXT_IMPLEMENTATION

// configuration options for DXT encoder. set them in the project/makefile or just define
// them at the top.

// STB_DXT_USE_ROUNDING_BIAS
//     use a rounding bias during color interpolation. this is closer to what "ideal"
//     interpolation would do but doesn't match the S3TC/DX10 spec. old versions (pre-1.03)
//     implicitly had this turned on.
//
//     in case you're targeting a specific type of hardware (e.g. console programmers):
//     NVidia and Intel GPUs (as of 2010) as well as DX9 ref use DXT decoders that are closer
//     to STB_DXT_USE_ROUNDING_BIAS. AMD/ATI, S3 and DX10 ref are closer to rounding with no bias.
//     you also see "(a*5 + b*3) / 8" on some old GPU designs.
// #define STB_DXT_USE_ROUNDING_BIAS

#include <stdlib.h>

#if !defined(STBD_ABS) || !defined(STBI_FABS)
#include <math.h>
#endif

#ifndef STBD_ABS
#define STBD_ABS(i)           abs(i)
#endif

#ifndef STBD_FABS
#define STBD_FABS(x)          fabs(x)
#endif

#ifndef STBD_MEMSET
#include <string.h>
#define STBD_MEMSET           memset
#endif

static unsigned char stb__Expand5[32];
static unsigned char stb__Expand6[64];
static unsigned char stb__OMatch5[256][2];
static unsigned char stb__OMatch6[256][2];
static unsigned char stb__QuantRBTab[256+16];
static unsigned char stb__QuantGTab[256+16];

static int stb__Mul8Bit(int a, int b)
{
  int t = a*b + 128;
  return (t + (t >> 8)) >> 8;
}

static void stb__From16Bit(unsigned char *out, unsigned short v)
{
   int rv = (v & 0xf800) >> 11;
   int gv = (v & 0x07e0) >>  5;
   int bv = (v & 0x001f) >>  0;

   out[0] = stb__Expand5[rv];
   out[1] = stb__Expand6[gv];
   out[2] = stb__Expand5[bv];
   out[3] = 0;
}

static unsigned short stb__As16Bit(int r, int g, int b)
{
   return (unsigned short)((stb__Mul8Bit(r,31) << 11) + (stb__Mul8Bit(g,63) << 5) + stb__Mul8Bit(b,31));
}

// linear interpolation at 1/3 point between a and b, using desired rounding type
static int stb__Lerp13(int a, int b)
{
#ifdef STB_DXT_USE_ROUNDING_BIAS
   // with rounding bias
   return a + stb__Mul8Bit(b-a, 0x55);
#else
   // without rounding bias
   // replace "/ 3" by "* 0xaaab) >> 17" if your compiler sucks or you really need every ounce of speed.
   return (2*a + b) / 3;
#endif
}

// lerp RGB color
static void stb__Lerp13RGB(unsigned char *out, unsigned char *p1, unsigned char *p2)
{
   out[0] = (unsigned char)stb__Lerp13(p1[0], p2[0]);
   out[1] = (unsigned char)stb__Lerp13(p1[1], p2[1]);
   out[2] = (unsigned char)stb__Lerp13(p1[2], p2[2]);
}

/****************************************************************************/

// compute table to reproduce constant colors as accurately as possible
static void stb__PrepareOptTable(unsigned char *Table,const unsigned char *expand,int size)
{
   int i,mn,mx;
   for (i=0;i<256;i++) {
      int bestErr = 256;
      for (mn=0;mn<size;mn++) {
         for (mx=0;mx<size;mx++) {
            int mine = expand[mn];
            int maxe = expand[mx];
            int err = STBD_ABS(stb__Lerp13(maxe, mine) - i);

            // DX10 spec says that interpolation must be within 3% of "correct" result,
            // add this as error term. (normally we'd expect a random distribution of
            // +-1.5% error, but nowhere in the spec does it say that the error has to be
            // unbiased - better safe than sorry).
            err += STBD_ABS(maxe - mine) * 3 / 100;

            if(err < bestErr)
            {
               Table[i*2+0] = (unsigned char)mx;
               Table[i*2+1] = (unsigned char)mn;
               bestErr = err;
            }
         }
      }
   }
}

static void stb__EvalColors(unsigned char *color,unsigned short c0,unsigned short c1)
{
   stb__From16Bit(color+ 0, c0);
   stb__From16Bit(color+ 4, c1);
   stb__Lerp13RGB(color+ 8, color+0, color+4);
   stb__Lerp13RGB(color+12, color+4, color+0);
}

// Block dithering function. Simply dithers a block to 565 RGB.
// (Floyd-Steinberg)
static void stb__DitherBlock(unsigned char *dest, unsigned char *block)
{
  int err[8],*ep1 = err,*ep2 = err+4, *et;
  int ch,y;

  // process channels separately
  for (ch=0; ch<3; ++ch) {
      unsigned char *bp = block+ch, *dp = dest+ch;
      unsigned char *quant = (ch == 1) ? stb__QuantGTab+8 : stb__QuantRBTab+8;
      STBD_MEMSET(err, 0, sizeof(err));
      for(y=0; y<4; ++y) {
         dp[ 0] = quant[bp[ 0] + ((3*ep2[1] + 5*ep2[0]) >> 4)];
         ep1[0] = bp[ 0] - dp[ 0];
         dp[ 4] = quant[bp[ 4] + ((7*ep1[0] + 3*ep2[2] + 5*ep2[1] + ep2[0]) >> 4)];
         ep1[1] = bp[ 4] - dp[ 4];
         dp[ 8] = quant[bp[ 8] + ((7*ep1[1] + 3*ep2[3] + 5*ep2[2] + ep2[1]) >> 4)];
         ep1[2] = bp[ 8] - dp[ 8];
         dp[12] = quant[bp[12] + ((7*ep1[2] + 5*ep2[3] + ep2[2]) >> 4)];
         ep1[3] = bp[12] - dp[12];
         bp += 16;
         dp += 16;
         et = ep1, ep1 = ep2, ep2 = et; // swap
      }
   }
}

// The color matching function
static unsigned int stb__MatchColorsBlock(unsigned char *block, unsigned char *color,int dither)
{
   unsigned int mask = 0;
   int dirr = color[0*4+0] - color[1*4+0];
   int dirg = color[0*4+1] - color[1*4+1];
   int dirb = color[0*4+2] - color[1*4+2];
   int dots[16];
   int stops[4];
   int i;
   int c0Point, halfPoint, c3Point;

   for(i=0;i<16;i++)
      dots[i] = block[i*4+0]*dirr + block[i*4+1]*dirg + block[i*4+2]*dirb;

   for(i=0;i<4;i++)
      stops[i] = color[i*4+0]*dirr + color[i*4+1]*dirg + color[i*4+2]*dirb;

   // think of the colors as arranged on a line; project point onto that line, then choose
   // next color out of available ones. we compute the crossover points for "best color in top
   // half"/"best in bottom half" and then the same inside that subinterval.
   //
   // relying on this 1d approximation isn't always optimal in terms of euclidean distance,
   // but it's very close and a lot faster.
   // http://cbloomrants.blogspot.com/2008/12/12-08-08-dxtc-summary.html

   c0Point   = (stops[1] + stops[3]);
   halfPoint = (stops[3] + stops[2]);
   c3Point   = (stops[2] + stops[0]);

   if(!dither) {
      // the version without dithering is straightforward
      for (i=15;i>=0;i--) {
         int dot = dots[i]*2;
         mask <<= 2;

         if(dot < halfPoint)
           mask |= (dot < c0Point) ? 1 : 3;
         else
           mask |= (dot < c3Point) ? 2 : 0;
      }
  } else {
      // with floyd-steinberg dithering
      int err[8],*ep1 = err,*ep2 = err+4;
      int *dp = dots, y;

      c0Point   <<= 3;
      halfPoint <<= 3;
      c3Point   <<= 3;
      for(i=0;i<8;i++)
         err[i] = 0;

      for(y=0;y<4;y++)
      {
         int dot,lmask,step;

         dot = (dp[0] << 4) + (3*ep2[1] + 5*ep2[0]);
         if(dot < halfPoint)
           step = (dot < c0Point) ? 1 : 3;
         else
           step = (dot < c3Point) ? 2 : 0;
         ep1[0] = dp[0] - stops[step];
         lmask = step;

         dot = (dp[1] << 4) + (7*ep1[0] + 3*ep2[2] + 5*ep2[1] + ep2[0]);
         if(dot < halfPoint)
           step = (dot < c0Point) ? 1 : 3;
         else
           step = (dot < c3Point) ? 2 : 0;
         ep1[1] = dp[1] - stops[step];
         lmask |= step<<2;

         dot = (dp[2] << 4) + (7*ep1[1] + 3*ep2[3] + 5*ep2[2] + ep2[1]);
         if(dot < halfPoint)
           step = (dot < c0Point) ? 1 : 3;
         else
           step = (dot < c3Point) ? 2 : 0;
         ep1[2] = dp[2] - stops[step];
         lmask |= step<<4;

         dot = (dp[3] << 4) + (7*ep1[2] + 5*ep2[3] + ep2[2]);
         if(dot < halfPoint)
           step = (dot < c0Point) ? 1 : 3;
         else
           step = (dot < c3Point) ? 2 : 0;
         ep1[3] = dp[3] - stops[step];
         lmask |= step<<6;

         dp += 4;
         mask |= lmask << (y*8);
         { int *et = ep1; ep1 = ep2; ep2 = et; } // swap
      }
   }

   return mask;
}

// The color optimization function. (Clever code, part 1)
static void stb__OptimizeColorsBlock(unsigned char *block, unsigned short *pmax16, unsigned short *pmin16)
{
  int mind = 0x7fffffff,maxd = -0x7fffffff;
  unsigned char *minp, *maxp;
  double magn;
  int v_r,v_g,v_b;
  static const int nIterPower = 4;
  float covf[6],vfr,vfg,vfb;

  // determine color distribution
  int cov[6];
  int mu[3],min[3],max[3];
  int ch,i,iter;

  for(ch=0;ch<3;ch++)
  {
    const unsigned char *bp = ((const unsigned char *) block) + ch;
    int muv,minv,maxv;

    muv = minv = maxv = bp[0];
    for(i=4;i<64;i+=4)
    {
      muv += bp[i];
      if (bp[i] < minv) minv = bp[i];
      else if (bp[i] > maxv) maxv = bp[i];
    }

    mu[ch] = (muv + 8) >> 4;
    min[ch] = minv;
    max[ch] = maxv;
  }

  // determine covariance matrix
  for (i=0;i<6;i++)
     cov[i] = 0;

  for (i=0;i<16;i++)
  {
    int r = block[i*4+0] - mu[0];
    int g = block[i*4+1] - mu[1];
    int b = block[i*4+2] - mu[2];

    cov[0] += r*r;
    cov[1] += r*g;
    cov[2] += r*b;
    cov[3] += g*g;
    cov[4] += g*b;
    cov[5] += b*b;
  }

  // convert covariance matrix to float, find principal axis via power iter
  for(i=0;i<6;i++)
    covf[i] = cov[i] / 255.0f;

  vfr = (float) (max[0] - min[0]);
  vfg = (float) (max[1] - min[1]);
  vfb = (float) (max[2] - min[2]);

  for(iter=0;iter<nIterPower;iter++)
  {
    float r = vfr*covf[0] + vfg*covf[1] + vfb*covf[2];
    float g = vfr*covf[1] + vfg*covf[3] + vfb*covf[4];
    float b = vfr*covf[2] + vfg*covf[4] + vfb*covf[5];

    vfr = r;
    vfg = g;
    vfb = b;
  }

  magn = STBD_FABS(vfr);
  if (STBD_FABS(vfg) > magn) magn = STBD_FABS(vfg);
  if (STBD_FABS(vfb) > magn) magn = STBD_FABS(vfb);

   if(magn < 4.0f) { // too small, default to luminance
      v_r = 299; // JPEG YCbCr luma coefs, scaled by 1000.
      v_g = 587;
      v_b = 114;
   } else {
      magn = 512.0 / magn;
      v_r = (int) (vfr * magn);
      v_g = (int) (vfg * magn);
      v_b = (int) (vfb * magn);
   }

   // Pick colors at extreme points
   for(i=0;i<16;i++)
   {
      int dot = block[i*4+0]*v_r + block[i*4+1]*v_g + block[i*4+2]*v_b;

      if (dot < mind) {
         mind = dot;
         minp = block+i*4;
      }

      if (dot > maxd) {
         maxd = dot;
         maxp = block+i*4;
      }
   }

   *pmax16 = stb__As16Bit(maxp[0],maxp[1],maxp[2]);
   *pmin16 = stb__As16Bit(minp[0],minp[1],minp[2]);
}

static const float midpoints5[32] = {
   0.015686f, 0.047059f, 0.078431f, 0.111765f, 0.145098f, 0.176471f, 0.207843f, 0.241176f, 0.274510f, 0.305882f, 0.337255f, 0.370588f, 0.403922f, 0.435294f, 0.466667f, 0.5f,
   0.533333f, 0.564706f, 0.596078f, 0.629412f, 0.662745f, 0.694118f, 0.725490f, 0.758824f, 0.792157f, 0.823529f, 0.854902f, 0.888235f, 0.921569f, 0.952941f, 0.984314f, 1.0f
};

static const float midpoints6[64] = {
   0.007843f, 0.023529f, 0.039216f, 0.054902f, 0.070588f, 0.086275f, 0.101961f, 0.117647f, 0.133333f, 0.149020f, 0.164706f, 0.180392f, 0.196078f, 0.211765f, 0.227451f, 0.245098f,
   0.262745f, 0.278431f, 0.294118f, 0.309804f, 0.325490f, 0.341176f, 0.356863f, 0.372549f, 0.388235f, 0.403922f, 0.419608f, 0.435294f, 0.450980f, 0.466667f, 0.482353f, 0.500000f,
   0.517647f, 0.533333f, 0.549020f, 0.564706f, 0.580392f, 0.596078f, 0.611765f, 0.627451f, 0.643137f, 0.658824f, 0.674510f, 0.690196f, 0.705882f, 0.721569f, 0.737255f, 0.754902f,
   0.772549f, 0.788235f, 0.803922f, 0.819608f, 0.835294f, 0.850980f, 0.866667f, 0.882353f, 0.898039f, 0.913725f, 0.929412f, 0.945098f, 0.960784f, 0.976471f, 0.992157f, 1.0f
};

static unsigned short stb__Quantize5(float x)
{
   unsigned short q;
   x = x < 0 ? 0 : x > 1 ? 1 : x;  // saturate
   q = (unsigned short)(x * 31);
   q += (x > midpoints5[q]);
   return q;
}

static unsigned short stb__Quantize6(float x)
{
   unsigned short q;
   x = x < 0 ? 0 : x > 1 ? 1 : x;  // saturate
   q = (unsigned short)(x * 63);
   q += (x > midpoints6[q]);
   return q;
}

// The refinement function. (Clever code, part 2)
// Tries to optimize colors to suit block contents better.
// (By solving a least squares system via normal equations+Cramer's rule)
static int stb__RefineBlock(unsigned char *block, unsigned short *pmax16, unsigned short *pmin16, unsigned int mask)
{
   static const int w1Tab[4] = { 3,0,2,1 };
   static const int prods[4] = { 0x090000,0x000900,0x040102,0x010402 };
   // ^some magic to save a lot of multiplies in the accumulating loop...
   // (precomputed products of weights for least squares system, accumulated inside one 32-bit register)

   float f;
   unsigned short oldMin, oldMax, min16, max16;
   int i, akku = 0, xx,xy,yy;
   int At1_r,At1_g,At1_b;
   int At2_r,At2_g,At2_b;
   unsigned int cm = mask;

   oldMin = *pmin16;
   oldMax = *pmax16;

   if((mask ^ (mask<<2)) < 4) // all pixels have the same index?
   {
      // yes, linear system would be singular; solve using optimal
      // single-color match on average color
      int r = 8, g = 8, b = 8;
      for (i=0;i<16;++i) {
         r += block[i*4+0];
         g += block[i*4+1];
         b += block[i*4+2];
      }

      r >>= 4; g >>= 4; b >>= 4;

      max16 = (stb__OMatch5[r][0]<<11) | (stb__OMatch6[g][0]<<5) | stb__OMatch5[b][0];
      min16 = (stb__OMatch5[r][1]<<11) | (stb__OMatch6[g][1]<<5) | stb__OMatch5[b][1];
   } else {
      At1_r = At1_g = At1_b = 0;
      At2_r = At2_g = At2_b = 0;
      for (i=0;i<16;++i,cm>>=2) {
         int step = cm&3;
         int w1 = w1Tab[step];
         int r = block[i*4+0];
         int g = block[i*4+1];
         int b = block[i*4+2];

         akku    += prods[step];
         At1_r   += w1*r;
         At1_g   += w1*g;
         At1_b   += w1*b;
         At2_r   += r;
         At2_g   += g;
         At2_b   += b;
      }

      At2_r = 3*At2_r - At1_r;
      At2_g = 3*At2_g - At1_g;
      At2_b = 3*At2_b - At1_b;

      // extract solutions and decide solvability
      xx = akku >> 16;
      yy = (akku >> 8) & 0xff;
      xy = (akku >> 0) & 0xff;

      f = 3.0f / 255.0f / (xx*yy - xy*xy);

      max16 =  stb__Quantize5((At1_r*yy - At2_r * xy) * f) << 11;
      max16 |= stb__Quantize6((At1_g*yy - At2_g * xy) * f) << 5;
      max16 |= stb__Quantize5((At1_b*yy - At2_b * xy) * f) << 0;

      min16 =  stb__Quantize5((At2_r*xx - At1_r * xy) * f) << 11;
      min16 |= stb__Quantize6((At2_g*xx - At1_g * xy) * f) << 5;
      min16 |= stb__Quantize5((At2_b*xx - At1_b * xy) * f) << 0;
   }

   *pmin16 = min16;
   *pmax16 = max16;
   return oldMin != min16 || oldMax != max16;
}

// Color block compression
static void stb__CompressColorBlock(unsigned char *dest, unsigned char *block, int mode)
{
   unsigned int mask;
   int i;
   int dither;
   int refinecount;
   unsigned short max16, min16;
   unsigned char dblock[16*4],color[4*4];

   dither = mode & STB_DXT_DITHER;
   refinecount = (mode & STB_DXT_HIGHQUAL) ? 2 : 1;

   // check if block is constant
   for (i=1;i<16;i++)
      if (((unsigned int *) block)[i] != ((unsigned int *) block)[0])
         break;

   if(i == 16) { // constant color
      int r = block[0], g = block[1], b = block[2];
      mask  = 0xaaaaaaaa;
      max16 = (stb__OMatch5[r][0]<<11) | (stb__OMatch6[g][0]<<5) | stb__OMatch5[b][0];
      min16 = (stb__OMatch5[r][1]<<11) | (stb__OMatch6[g][1]<<5) | stb__OMatch5[b][1];
   } else {
      // first step: compute dithered version for PCA if desired
      if(dither)
         stb__DitherBlock(dblock,block);

      // second step: pca+map along principal axis
      stb__OptimizeColorsBlock(dither ? dblock : block,&max16,&min16);
      if (max16 != min16) {
         stb__EvalColors(color,max16,min16);
         mask = stb__MatchColorsBlock(block,color,dither);
      } else
         mask = 0;

      // third step: refine (multiple times if requested)
      for (i=0;i<refinecount;i++) {
         unsigned int lastmask = mask;

         if (stb__RefineBlock(dither ? dblock : block,&max16,&min16,mask)) {
            if (max16 != min16) {
               stb__EvalColors(color,max16,min16);
               mask = stb__MatchColorsBlock(block,color,dither);
            } else {
               mask = 0;
               break;
            }
         }

         if(mask == lastmask)
            break;
      }
  }

  // write the color block
  if(max16 < min16)
  {
     unsigned short t = min16;
     min16 = max16;
     max16 = t;
     mask ^= 0x55555555;
  }

  dest[0] = (unsigned char) (max16);
  dest[1] = (unsigned char) (max16 >> 8);
  dest[2] = (unsigned char) (min16);
  dest[3] = (unsigned char) (min16 >> 8);
  dest[4] = (unsigned char) (mask);
  dest[5] = (unsigned char) (mask >> 8);
  dest[6] = (unsigned char) (mask >> 16);
  dest[7] = (unsigned char) (mask >> 24);
}

// Alpha block compression (this is easy for a change)
static void stb__CompressAlphaBlock(unsigned char *dest,unsigned char *src, int stride)
{
   int i,dist,bias,dist4,dist2,bits,mask;

   // find min/max color
   int mn,mx;
   mn = mx = src[0];

   for (i=1;i<16;i++)
   {
      if (src[i*stride] < mn) mn = src[i*stride];
      else if (src[i*stride] > mx) mx = src[i*stride];
   }

   // encode them
   dest[0] = (unsigned char)mx;
   dest[1] = (unsigned char)mn;
   dest += 2;

   // determine bias and emit color indices
   // given the choice of mx/mn, these indices are optimal:
   // http://fgiesen.wordpress.com/2009/12/15/dxt5-alpha-block-index-determination/
   dist = mx-mn;
   dist4 = dist*4;
   dist2 = dist*2;
   bias = (dist < 8) ? (dist - 1) : (dist/2 + 2);
   bias -= mn * 7;
   bits = 0,mask=0;

   for (i=0;i<16;i++) {
      int a = src[i*stride]*7 + bias;
      int ind,t;

      // select index. this is a "linear scale" lerp factor between 0 (val=min) and 7 (val=max).
      t = (a >= dist4) ? -1 : 0; ind =  t & 4; a -= dist4 & t;
      t = (a >= dist2) ? -1 : 0; ind += t & 2; a -= dist2 & t;
      ind += (a >= dist);

      // turn linear scale into DXT index (0/1 are extremal pts)
      ind = -ind & 7;
      ind ^= (2 > ind);

      // write index
      mask |= ind << bits;
      if((bits += 3) >= 8) {
         *dest++ = (unsigned char)mask;
         mask >>= 8;
         bits -= 8;
      }
   }
}

static void stb__InitDXT()
{
   int i;
   for(i=0;i<32;i++)
      stb__Expand5[i] = (unsigned char)((i<<3)|(i>>2));

   for(i=0;i<64;i++)
      stb__Expand6[i] = (unsigned char)((i<<2)|(i>>4));

   for(i=0;i<256+16;i++)
   {
      int v = i-8 < 0 ? 0 : i-8 > 255 ? 255 : i-8;
      stb__QuantRBTab[i] = stb__Expand5[stb__Mul8Bit(v,31)];
      stb__QuantGTab[i] = stb__Expand6[stb__Mul8Bit(v,63)];
   }

   stb__PrepareOptTable(&stb__OMatch5[0][0],stb__Expand5,32);
   stb__PrepareOptTable(&stb__OMatch6[0][0],stb__Expand6,64);
}

void stb_compress_dxt_block(unsigned char *dest, const unsigned char *src, int alpha, int mode)
{
   unsigned char data[16][4];
   static int init=1;
   if (init) {
      stb__InitDXT();
      init=0;
   }

   if (alpha) {
      int i;
      stb__CompressAlphaBlock(dest,(unsigned char*) src+3, 4);
      dest += 8;
      // make a new copy of the data in which alpha is opaque,
      // because code uses a fast test for color constancy
      memcpy(data, src, 4*16);
      for (i=0; i < 16; ++i)
         data[i][3] = 255;
      src = &data[0][0];
   }

   stb__CompressColorBlock(dest,(unsigned char*) src,mode);
}

void stb_compress_bc4_block(unsigned char *dest, const unsigned char *src)
{
   stb__CompressAlphaBlock(dest,(unsigned char*) src, 1);
}

void stb_compress_bc5_block(unsigned char *dest, const unsigned char *src)
{
   stb__CompressAlphaBlock(dest,(unsigned char*) src,2);
   stb__CompressAlphaBlock(dest + 8,(unsigned char*) src+1,2);
}
#endif // STB_DXT_IMPLEMENTATION

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
