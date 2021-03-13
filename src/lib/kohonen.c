#include <stdio.h>
#include <stdlib.h>
#include "neuquant.h"

// kohonen neural net adapted from dekker's "kohonen neural networks for
// optimal colour quantization" (2009)

#define netsize 256
#define maxnetpos	(netsize-1)
#define netbiasshift	4			/* bias for color values */
#define ncycles		100			/* no. of learning cycles */

/* defs for freq and bias */
#define intbiasshift    16			/* bias for fractions */
#define intbias		(((int) 1)<<intbiasshift)
#define gammashift  	10			/* gamma = 1024 */
#define gamma   	(((int) 1)<<gammashift)
#define betashift  	10
#define beta		(intbias>>betashift)	/* beta = 1/1024 */
#define betagamma	(intbias<<(gammashift-betashift))

/* defs for decreasing radius factor */
#define initrad		(netsize>>3)		/* for 256 cols, radius starts */
#define radiusbiasshift	6			/* at 32.0 biased by 6 bits */
#define radiusbias	(((int) 1)<<radiusbiasshift)
#define initradius	(initrad*radiusbias)	/* and decreases by a */
#define radiusdec	30			/* factor of 1/30 each cycle */ 

/* defs for decreasing alpha factor */
#define alphabiasshift	10			/* alpha starts at 1.0 */
#define initalpha	(((int) 1)<<alphabiasshift)
int alphadec;					/* biased by 10 bits */

/* radbias and alpharadbias used for radpower calculation */
#define radbiasshift	8
#define radbias		(((int) 1)<<radbiasshift)
#define alpharadbshift  (alphabiasshift+radbiasshift)
#define alpharadbias    (((int) 1)<<alpharadbshift)

static int netindex[256];			/* for network lookup - really 256 */

static int bias [netsize];			/* bias and freq arrays for learning */
static int freq [netsize];
static int radpower[initrad];			/* radpower for precomputation */

kohonenctx* initnet(const void *data, int leny, int linesize, int lenx, int sample){
	kohonenctx* ret = malloc(sizeof(*ret));
	if(ret){
		int i;
		int *p;
		
		for (i=0; i<netsize; i++) {
			p = ret->network[i];
			p[0] = p[1] = p[2] = (i << (netbiasshift+8))/netsize;
			p[3] = 0;
			freq[i] = intbias/netsize;	/* 1/netsize */
			bias[i] = 0;
		}
		ret->data = data;
		ret->linesize = linesize;
		ret->samplefac = sample;
		ret->leny = leny;
		ret->lenx = lenx;
	}
	return ret;
}

	
/* Unbias network to give byte values 0..255 and record position i to prepare for sort
   ----------------------------------------------------------------------------------- */

void unbiasnet(kohonenctx* kctx){
	int i,j,temp;

	for (i=0; i<netsize; i++) {
		for (j=0; j<3; j++) {
			temp = (kctx->network[i][j] + (1 << (netbiasshift - 1))) >> netbiasshift;
			if (temp > 255) temp = 255;
			kctx->network[i][j] = temp;
		}
		kctx->network[i][3] = i;			/* record color no */
	}
}


void netcolor(const kohonenctx* kctx, int color, unsigned char rgb[static 3]){
	rgb[0] = kctx->network[color][0];
	rgb[1] = kctx->network[color][1];
	rgb[2] = kctx->network[color][2];
}

void inxbuild(kohonenctx* kctx){
	int i,j,smallpos,smallval;
	int *p,*q;
	int previouscol,startpos;

	previouscol = 0;
	startpos = 0;
	for (i=0; i<netsize; i++) {
		p = kctx->network[i];
		smallpos = i;
		smallval = p[1];			/* index on g */
		/* find smallest in i..netsize-1 */
		for (j=i+1; j<netsize; j++) {
			q = kctx->network[j];
			if (q[1] < smallval) {		/* index on g */
				smallpos = j;
				smallval = q[1];	/* index on g */
			}
		}
		q = kctx->network[smallpos];
		/* swap p (i) and q (smallpos) entries */
		if (i != smallpos) {
			j = q[0];   q[0] = p[0];   p[0] = j;
			j = q[1];   q[1] = p[1];   p[1] = j;
			j = q[2];   q[2] = p[2];   p[2] = j;
			j = q[3];   q[3] = p[3];   p[3] = j;
		}
		/* smallval entry is now in position i */
		if (smallval != previouscol) {
			netindex[previouscol] = (startpos+i)>>1;
			for (j=previouscol+1; j<smallval; j++) netindex[j] = i;
			previouscol = smallval;
			startpos = i;
		}
	}
	netindex[previouscol] = (startpos+maxnetpos)>>1;
	for (j=previouscol+1; j<256; j++) netindex[j] = maxnetpos; /* really 256 */
}


int inxsearch(kohonenctx* kctx, int r, int g, int b){
	int i,j,dist,a,bestd;
	int *p;
	int best;

	bestd = 1000;		/* biggest possible dist is 256*3 */
	best = -1;
	i = netindex[g];	/* index on g */
	j = i-1;		/* start at netindex[g] and work outwards */

	while ((i<netsize) || (j>=0)) {
		if (i<netsize) {
			p = kctx->network[i];
			dist = p[1] - g;		/* inx key */
			if (dist >= bestd) i = netsize;	/* stop iter */
			else {
				i++;
				if (dist<0) dist = -dist;
				a = p[0] - r;   if (a<0) a = -a;
				dist += a;
				if (dist<bestd) {
					a = p[2] - b;   if (a<0) a = -a;
					dist += a;
					if (dist<bestd) {bestd=dist; best=p[3];}
				}
			}
		}
		if (j>=0) {
			p = kctx->network[j];
			dist = g - p[1]; /* inx key - reverse dif */
			if (dist >= bestd) j = -1; /* stop iter */
			else {
				j--;
				if (dist<0) dist = -dist;
				a = p[0] - r;   if (a<0) a = -a;
				dist += a;
				if (dist<bestd) {
					a = p[2] - b;   if (a<0) a = -a;
					dist += a;
					if (dist<bestd) {bestd=dist; best=p[3];}
				}
			}
		}
	}
	return(best);
}


static int
contest(kohonenctx* kctx, int r, int g, int b){
	/* finds closest neuron (min dist) and updates freq */
	/* finds best neuron (min dist-bias) and returns position */
	/* for frequently chosen neurons, freq[i] is high and bias[i] is negative */
	/* bias[i] = gamma*((1/netsize)-freq[i]) */

	int i,dist,a,biasdist,betafreq;
	int bestpos,bestbiaspos,bestd,bestbiasd;
	int *p,*f, *n;

	bestd = ~(((int) 1)<<31);
	bestbiasd = bestd;
	bestpos = -1;
	bestbiaspos = bestpos;
	p = bias;
	f = freq;

	for (i=0; i<netsize; i++) {
		n = kctx->network[i];
		dist = n[0] - r;   if (dist<0) dist = -dist;
		a = n[1] - g;   if (a<0) a = -a;
		dist += a;
		a = n[2] - b;   if (a<0) a = -a;
		dist += a;
		if (dist<bestd) {bestd=dist; bestpos=i;}
		biasdist = dist - ((*p)>>(intbiasshift-netbiasshift));
		if (biasdist<bestbiasd) {bestbiasd=biasdist; bestbiaspos=i;}
		betafreq = (*f >> betashift);
		*f++ -= betafreq;
		*p++ += (betafreq<<gammashift);
	}
	freq[bestpos] += beta;
	bias[bestpos] -= betagamma;
	return(bestbiaspos);
}


/* Move neuron i towards biased (r,g,b) by factor alpha
   ---------------------------------------------------- */

static void
altersingle(kohonenctx* kctx, int alpha, int i, int r, int g, int b){
	int *n;

	n = kctx->network[i];				/* alter hit neuron */
	*n -= (alpha*(*n - r)) / initalpha;
	n++;
	*n -= (alpha*(*n - g)) / initalpha;
	n++;
	*n -= (alpha*(*n - b)) / initalpha;
}


/* Move adjacent neurons by precomputed alpha*(1-((i-j)^2/[r]^2)) in radpower[|i-j|]
   --------------------------------------------------------------------------------- */

static void
alterneigh(kohonenctx* kctx, int rad, int i, int r, int g, int b){
	int j, k, lo, hi, a;
	int *p, *q;

	lo = i - rad;   if (lo<-1) lo=-1;
	hi = i + rad;   if (hi>netsize) hi=netsize;

	j = i + 1;
	k = i - 1;
	q = radpower;
	while ((j < hi) || (k > lo)) {
		a = (*(++q));
		if (j<hi) {
			p = kctx->network[j];
			*p -= (a*(*p - r)) / alpharadbias;
			p++;
			*p -= (a*(*p - g)) / alpharadbias;
			p++;
			*p -= (a*(*p - b)) / alpharadbias;
			j++;
		}
		if (k>lo) {
			p = kctx->network[k];
			*p -= (a*(*p - r)) / alpharadbias;
			p++;
			*p -= (a*(*p - g)) / alpharadbias;
			p++;
			*p -= (a*(*p - b)) / alpharadbias;
			k--;
		}
	}
}


/* Main Learning Loop
   ------------------ */

static inline
const unsigned char* get_pixel(const kohonenctx* kctx, int pixel){
	int line = pixel / kctx->lenx;
	int offx = pixel % kctx->lenx;
	return kctx->data + kctx->linesize * line + offx * 4;
}

void learn(kohonenctx* kctx){
	int radius,rad,alpha,step,delta,samplepixels;
	int i,j;

	alphadec = 30 + ((kctx->samplefac - 1) / 3);
	int pixel = 0;
	int lim = kctx->leny * kctx->lenx;
	samplepixels = lim / (3 * kctx->samplefac);
	delta = samplepixels / ncycles;
	alpha = initalpha;
	radius = initradius;
	
	rad = radius >> radiusbiasshift;
	if (rad <= 1) rad = 0;
	for (i=0; i<rad; i++) 
		radpower[i] = alpha*(((rad*rad - i*i)*radbias)/(rad*rad));
	
	if ((lim % prime1) != 0) step = prime1;
	else {
		if ((lim % prime2) !=0) step = prime2;
		else {
			if ((lim % prime3) !=0) step = prime3;
			else step = prime4;
		}
	}
	
	i = 0;
	while (i < samplepixels) {
		const unsigned char* p = get_pixel(kctx, pixel);
		int r = p[0] << netbiasshift;
		int g = p[1] << netbiasshift;
		int b = p[2] << netbiasshift;
fprintf(stderr, "BIAS %4d %4d %4d SRC %3d %3d %3d\n", r, g, b, p[0], p[1], p[2]);
		j = contest(kctx, r, g, b);

		altersingle(kctx, alpha, j, r, g, b);
		if(rad){
			alterneigh(kctx, rad, j, r, g, b);   /* alter neighbours */
		}

		pixel += step;
    if(pixel >= lim){
			pixel -= lim;
		}
	
		i++;
		if(i % delta == 0){
			alpha -= alpha / alphadec;
			radius -= radius / radiusdec;
			rad = radius >> radiusbiasshift;
			if(rad <= 1){
				rad = 0;
			}
			for(j=0; j<rad; j++){
				radpower[j] = alpha*(((rad*rad - j*j)*radbias)/(rad*rad));
			}
		}
	}
}

void freenet(kohonenctx* kctx){
	free(kctx);
}
