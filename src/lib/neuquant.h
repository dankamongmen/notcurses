#ifndef NOTCURSES_NEUQUANT
#define NOTCURSES_NEUQUANT

typedef struct kohonenctx {
	const unsigned char* data;
  int leny, lenx;
	int linesize;
  int samplefac;   /* sampling factor 1..30 */
  int network[256][4];			/* the network itself */
} kohonenctx;

/* For 256 colours, fixed arrays need 8kb, plus space for the image
   ---------------------------------------------------------------- */


/* four primes near 500 - assume no image has a length so large */
/* that it is divisible by all four primes */
#define prime1		499
#define prime2		491
#define prime3		487
#define prime4		503

#define minpicturebytes	(3*prime4)		/* minimum size for input image */


kohonenctx* initnet(const void *data, int leny, int linesize, int lenx, int sample);
void freenet(kohonenctx* kctx);
		
void unbiasnet(kohonenctx* kctx);	/* can edit this function to do output of colour map */

void netcolor(const kohonenctx* kctx, int color, unsigned char rgb[static 3]);

void inxbuild(kohonenctx* kctx);

int inxsearch(kohonenctx* kctx, int r, int g, int b);

void learn(kohonenctx* kctx);

#endif
