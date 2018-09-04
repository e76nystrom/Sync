#define DBGTRK 1

#if DBGTRK

EXT boolean dbgTrk;
#define TRKBUFSIZE (4*128)
EXT int16_t trkidx;
EXT int16_t trkbuf[TRKBUFSIZE];

#define DBGTRK2WL0 0            /* encoder capature */
#define DBGTRK2WL1 1            /* interal capature */

#define DBGTRK1W 0
#define dbgTrk1W(val1)

#define DBGTRK1L 0
#define dbgTrk1L(val1)

#define DBGTRK2L 0
#define dbgTrk2L(val1, val2)

#define DBGTRK2WL 1
#define dbgTrk2WL(val1, val2, val3) \
{\
 if (dbgTrk)\
 {\
  int16_t *p = (int16_t *) &trkbuf[trkidx];\
  *p++ = val1;\
  *p++ = val2;\
  *((int32_t *) p) = val3;\
  trkidx += 4;\
  trkidx &= (TRKBUFSIZE - 1);\
 }\
}

#endif /* DBGTRK */
