#include <time.h>

#include "newGPU.h"
#include "minimal.h"

u16 bgr2rgb[65536];

///////////////////////////////////////////////////////////////////////////////
#include "raster.h"
#include "inner_Blit.h"

///////////////////////////////////////////////////////////////////////////////
void clutInit()
{
#if 0
	int r,g,b,i;
	
	i = 0;
	for(b = 0; b < 32; b++)
	{
		for(g = 0; g < 32; g++)
		{
			for(r = 0; r < 32; r++)
			{
				u16 pixbgr = (b<<10) | (g<<5) | (r);
				bgr2rgb[i] = ((pixbgr&(0x1f001f<<10))>>10) | ((pixbgr&(0x1f001f<<5))<<1) | ((pixbgr&(0x1f001f<<0))<<11);
				bgr2rgb[i+32768] = bgr2rgb[i];
				i++;
			}
		}
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
void  gpuDebugRect(u16 col, u32 x0, u32 y0, u32 x1, u32 y1)
{
  if(x0>x1 || y0>y1 || x1>1024 || y1>512)
    return;

  x0 = (x0* PSX4ALL_WIDTH)/1024;  x1 = (x1*PSX4ALL_WIDTH )/1024;
  y0 = (y0*PSX4ALL_HEIGHT)/ 512;  y1 = (y1*PSX4ALL_HEIGHT)/ 512;

  u16* dest_screen16 = gp2x_screen16;
  dest_screen16 += (y0*PSX4ALL_WIDTH);

  // top
  if(y0!=y1)
  {
    for(u32 x=x0;x<x1;++x)  dest_screen16[x] = col;
    dest_screen16 += PSX4ALL_WIDTH;
  }
  
  //  middle
  if(x1)  --x1;
  while(++y0<y1)
  {
    dest_screen16[x0] = dest_screen16[x1] = col;
    dest_screen16 += PSX4ALL_WIDTH;
  }
  ++x1;

  // bottom
  {
    for(u32 x=x0;x<x1;++x)  dest_screen16[x] = col;
  }
}
///////////////////////////////////////////////////////////////////////////////
void  gpuDisplayVideoMem()
{
	u16* dest_screen16 = gp2x_screen16;
  for(int y=0;y<240;++y)
  {
  	u16* src_screen16  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0,(y<<9)/PSX4ALL_HEIGHT)];
    GPU_BlitWSSWSSWSSWSSWSSS(	src_screen16, dest_screen16, false);
    dest_screen16 += PSX4ALL_WIDTH;
  }

  gpuDebugRect(0x00FF, DrawingArea[0],    DrawingArea[1],   DrawingArea[2],   DrawingArea[3]);
  gpuDebugRect(0xFF00, DisplayArea[0],    DisplayArea[1],   DisplayArea[0]+DisplayArea[2], DisplayArea[1]+DisplayArea[3]);
  gpuDebugRect(0x0FF0, DirtyArea[0],      DirtyArea[1],     DirtyArea[2],     DirtyArea[3]);
  gpuDebugRect(0xFFFF, LastDirtyArea[0],  LastDirtyArea[1], LastDirtyArea[2], LastDirtyArea[3]);

  gp2x_video_flip();
}


///////////////////////////////////////////////////////////////////////////////
void gpuVideoOutput(void)
{
	static s16 old_res_horz, old_res_vert, old_rgb24;
	s16 h0, x0, y0, w0, h1;

	x0 = DisplayArea[0];
	y0 = DisplayArea[1];

	w0 = DisplayArea[2];
	h0 = DisplayArea[3];  // video mode

	h1 = DisplayArea[7] - DisplayArea[5]; // display needed
	if (h0 == 480)
		h1 = Min2(h1*2,480);

#ifdef ZAURUS
	if(SDL_MUSTLOCK(gp2x_sdlwrapper_screen)) SDL_LockSurface(gp2x_sdlwrapper_screen);
#endif

	u16* dest_screen16 = gp2x_screen16;
	u16* src_screen16  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(x0,y0)];
	u32 isRGB24 = (GPU_GP1 & 0x00200000 ? 32 : 0);
	/* Clear the screen if resolution changed to prevent interlacing and clipping to clash */
	if( (w0 != old_res_horz || h1 != old_res_vert || (s16)isRGB24 != old_rgb24) )
	{
		// Update old resolution
		old_res_horz = w0;
		old_res_vert = h1;
		old_rgb24 = (s16)isRGB24;
		// Finally, clear the screen for this special case
		gp2x_video_RGB_clearscreen16();
	}

#if 0 //def PANDORA
	if(h0==256)
	{
		h0 = 240;
		src_screen16  += ((h1-h0)>>1)*1024;
		h1 = h0;
	}
#else
	//  Height centering
  int sizeShift = 1;
  if(h0==256)
    h0 = 240;
  else
  if(h0==480)
    sizeShift = 2;

  if(h1>h0)
  {
		src_screen16  += ((h1-h0)>>sizeShift)*1024;
    h1 = h0;
  }
  else
  if(h1<h0)
    dest_screen16 += ((h0-h1)>>sizeShift)*PSX4ALL_WIDTH;

	/* Main blitter */
  int incY = (h0==480) ? 2 : 1;
#endif

#if 0 //def PANDORA
//printf("w0 %u h1 %u isRGB24 %u\n", w0, h1, isRGB24);
	void (*GPU_Blitter)(void* src, u16* dst16, u32 height);

	switch( w0 )
	{
	case 256:
			if(h1 <= 240)
			{
				GPU_Blitter = GPU_Blit_256_240_ROT90CW_2X;
			}
			else if(h1 <= 480) 
			{
				GPU_Blitter = GPU_Blit_256_480_ROT90CW_2X;
			}
		break;
	case 368:
			if(h1 <= 240) 
			{
				GPU_Blitter = GPU_Blit_368_240_ROT90CW_2X;
			}
			else if(h1 <= 480) 
			{
				GPU_Blitter = GPU_Blit_368_480_ROT90CW_2X;
			}
		break;
	case 320:
			if(h1 <= 240) 
			{
				GPU_Blitter = GPU_Blit_320_240_ROT90CW_2X;
			}
			else if(h1 <= 480) 
			{
				GPU_Blitter = GPU_Blit_320_480_ROT90CW_2X;
			}
		break;
	case 384:
			if(h1 <= 240)
			{
				GPU_Blitter = GPU_Blit_384_240_ROT90CW_2X;
			}
			else if(h1 <= 480) 
			{
				GPU_Blitter = GPU_Blit_384_480_ROT90CW_2X;
			}
		break;
	case 512:
			if(h1 <= 240)
			{
				GPU_Blitter = GPU_Blit_512_240_ROT90CW_2X;
			}
			else if(h1 <= 480)
			{
				GPU_Blitter = GPU_Blit_512_480_ROT90CW_2X;
			}
		break;
	case 640:
			if(h1 <= 480) GPU_Blitter = GPU_Blit_640_480_ROT90CW;
			else return;
		break;
	default: return;
	}
  /* Blit span */
	GPU_Blitter(src_screen16,	dest_screen16, h1);
#else
//y0 -= framesProgresiveInt ? incY : 0;
  for(int y1=y0+h1; y0<y1; y0+=incY)
	{
		/* Blit span */
		switch( w0 )
		{
		case 256:
			if( 0 == (y0&linesInterlace) )
				GPU_BlitWWDWW(	src_screen16,	dest_screen16, isRGB24);
			break;
		case 368:
			if( 0 == (y0&linesInterlace) )
				GPU_BlitWWWWWWWWS(	src_screen16,	dest_screen16, isRGB24, 4);
			break;
		case 320:
			if( 0 == (y0&linesInterlace) )
				GPU_BlitWW(	src_screen16,	dest_screen16, isRGB24);
			break;
		case 384:
			if( 0 == (y0&linesInterlace) )
				GPU_BlitWWWWWS(	src_screen16,	dest_screen16, isRGB24);
			break;
		case 512:
			if( 0 == (y0&linesInterlace) )
				GPU_BlitWWSWWSWS(	src_screen16, dest_screen16, isRGB24);
			break;
		case 640:
			if( 0 == (y0&linesInterlace) )
				GPU_BlitWS(	src_screen16, dest_screen16, isRGB24);
			break;
		}

		dest_screen16 += PSX4ALL_WIDTH;
		src_screen16  += h0==480 ? 2048 : 1024;
	}
#endif

#ifdef ZAURUS
	if(SDL_MUSTLOCK(gp2x_sdlwrapper_screen)) SDL_UnlockSurface(gp2x_sdlwrapper_screen);
#endif

/* DEBUG
gp2x_printf(NULL, 0, 10, "WIDTH %d HEIGHT %d", w0, h1 );
gp2x_video_flip();
*/
}

u64 curDelay     = 0 ;
u64 delayIncrement = 0;
timespec time_a, time_b, timediff;
int curDelay_inc = gp2x_timer_raw_second()/1000;
int skCount = 0;
int skRate  = 0;


timespec tsSubtract(timespec time1, timespec time2)
{
    struct  timespec  result ;

    if ((time1.tv_sec < time2.tv_sec) ||
        ((time1.tv_sec == time2.tv_sec) &&
         (time1.tv_nsec <= time2.tv_nsec))) {		/* TIME1 <= TIME2? */
        result.tv_sec = result.tv_nsec = 0 ;
    } else {						/* TIME1 > TIME2 */
        result.tv_sec = time1.tv_sec - time2.tv_sec ;
        if (time1.tv_nsec < time2.tv_nsec) {
            result.tv_nsec = time1.tv_nsec + 1000000000L - time2.tv_nsec ;
            result.tv_sec-- ;				/* Borrow a second. */
        } else {
            result.tv_nsec = time1.tv_nsec - time2.tv_nsec ;
        }
    }

    return (result) ;
}
///////////////////////////////////////////////////////////////////////////////
void  gpuSkipUpdate()
{

	u64    newtime;
	u64    diffintime = 0;

  ++frameRateCounter;

  isNewDisplay = false;

  static u32 s_LastFrame=0;
  u32 curFlip  = gp2x_timer_raw();
  u32 curFrame = curFlip-s_LastFrame;
  //isSkipOutput = isSkip;
  if(!isSkipOutput || !hardframeskip_blit)
  {
    ++frameRealCounter;
    //DEBUGF("doing frame rate %d real %d", frameRateCounter, frameRealCounter);
  	gpuVideoOutput();

#ifndef PANDORA
    if(DisplayArea[3] == 480)  // if 480 we only nees 1 of of 2 lines
    {
      linesInterlace = 1;
    }
    else
#endif
    if( linesInterlace != linesInterlace_user )
    {
      linesInterlace = linesInterlace_user;
      gp2x_video_RGB_clearscreen16();
    }
  }
  curFlip  = gp2x_timer_raw()-curFlip;
  if( displayFrameInfo && (!isSkipOutput || !hardframeskip_blit))
  {
    int ypos = 0;
    //gp2x_printf(NULL, 0, ypos,"VS:%04.4g fps:%04.4g real:%04.4g fs(%d/%d) (%3d,%2d,%2d)ms", float(vsincRate)/100.0f, float(frameRate)/100.0f, float(realRate)/100.0f, skipCount, skipRate, gp2x_timer_raw_to_ticks(curFrame),gp2x_timer_raw_to_ticks(curFlip),gp2x_timer_raw_to_ticks(curDelay));
    gp2x_printf(NULL, 0, ypos,"VS:%4d fps:%4d real:%4d fs(%d/%d) (%3d,%2d,%2d)ms", vsincRate, frameRate, realRate, skipCount, skipRate, gp2x_timer_raw_to_ticks(curFrame),gp2x_timer_raw_to_ticks(curFlip),gp2x_timer_raw_to_ticks(curDelay));
  #ifdef ENABLE_GPU_PRIM_STATS
    int polis = statF3 + statFT3 + statG3 + statGT3;
    gp2x_printf(NULL, 0,(ypos+=10),"PPF (%4d): PPS (%5d): ", polis,  (polis*realRate)/100 );
    gp2x_printf(NULL, 0,(ypos+=10),"types F(%4d) FT(%4d) G(%4d)  GT(%4d)", statF3, statFT3, statG3, statGT3);
  #endif
  #ifdef ENABLE_GPU_PROFILLER
    if(displayGpuStats)
    {
      gp2x_printf(NULL, 0,(ypos+=10),"dmaChainTime (%5d): %04.4g%% (%3dms)", dmaChainCount,  PROFILE_RATIO((100*dmaChainTime ),curFrame), gp2x_timer_raw_to_ticks(dmaChainTime) );
      gp2x_printf(NULL, 0,(ypos+=10),"gpuPolyTime  (%5d): %04.4g%% (%3dms)", gpuPolyCount,   PROFILE_RATIO((100*gpuPolyTime  ),curFrame), gp2x_timer_raw_to_ticks(gpuPolyTime)  );
      gp2x_printf(NULL, 0,(ypos+=10),"gpuPixelTime (%5d): %04.4g%% (%3dms)", gpuPixelCount,  PROFILE_RATIO((100*gpuPixelTime ),curFrame), gp2x_timer_raw_to_ticks(gpuPixelTime) );
      gp2x_printf(NULL, 0,(ypos+=10),"gpuRasterTime(%5d): %04.4g%% (%3dms)", gpuRasterCount, PROFILE_RATIO((100*gpuRasterTime),curFrame), gp2x_timer_raw_to_ticks(gpuRasterTime));
      gp2x_printf(NULL, 0,(ypos+=10),"dmaMemTime   (%5d): %04.4g%% (%3dms)", dmaMemCount,    PROFILE_RATIO((100*dmaMemTime),   curFrame), gp2x_timer_raw_to_ticks(dmaMemTime)   );
    }
  #endif
  }
  PROFILE_RESET(gpuPolyTime,gpuPolyCount);
  PROFILE_RESET(gpuRasterTime,gpuRasterCount);
  PROFILE_RESET(gpuPixelTime,gpuPixelCount);
  PROFILE_RESET(dmaChainTime,dmaChainCount);
  PROFILE_RESET(dmaMemTime,dmaMemCount);
  statF3 = statFT3 = statG3 = statGT3 = 0;
  statLF = statLG  = statS  = statT   = 0;

  if(!isSkipOutput || !hardframeskip_blit)
    gp2x_video_flip();

  s_LastFrame = gp2x_timer_raw();

  if (!skCount && !skRate) {
  	skCount = skipCount;
  	skRate = skipRate - skipCount;
  }
  
  if (skCount > skRate)
  {
    isSkip = isSkipOutput = 1;
    //DEBUGF("skipping frame");
    skCount--;
  }
  else
  {
    isSkip = isSkipOutput = 0;
    //DEBUGF("not skipping frame");
    skRate--;
  }

  clock_gettime(CLOCK_MONOTONIC, &time_a);
  timediff = tsSubtract(time_a, time_b);
  timespec Hz;
  Hz.tv_nsec = (isPAL?20000000L:16666666L);
  timespec sleep = tsSubtract(Hz,timediff);
  if (enableFrameLimit && sleep.tv_sec < 1) //Make sure values are sane
	  nanosleep (&sleep, NULL);
  clock_gettime(CLOCK_MONOTONIC, &time_b);

  /*newtime = SDL_GetTicks()*1000;

  if(((curDelay>0) && (curDelay < 1000000)) && enableFrameLimit)
  {
	  int a;
	  //while (SDL_GetTicks()*1000 < newtime+curDelay) {a++;}
	    SDL_Delay(curDelay/1000);

	  //printf("leftover %i %i %i\n", (SDL_GetTicks()*1000),newtime,curDelay);
	  delayIncrement += curDelay-((SDL_GetTicks()*1000)-newtime);
  }

  u64 d = (isPAL?20000:16666);

  diffintime = newtime - systime;
  vsincRate = delayIncrement;
  frameRate = curDelay;
  realRate = diffintime;

  u64 dinc = 0;

  if(enableFrameLimit)
  {
	  if (diffintime < d)
	  {
		  curDelay = d - diffintime;
		  if (delayIncrement <= curDelay)
		  {
			  curDelay -= delayIncrement;
			  delayIncrement=0;
		  }
		  else
		  {
			  dinc = delayIncrement-curDelay;
			  curDelay -= delayIncrement;
			  delayIncrement-=dinc;
		  }
	  }
	  else
	  {
		  delayIncrement += (diffintime - d);
		  curDelay = 0;
	  }
  }
  systime = SDL_GetTicks()*1000;*/

}

///////////////////////////////////////////////////////////////////////////////
void  NEWGPU_vSinc(void)
{
/*	u64    newtime;
	u64    diffintime = 0;*/

	/*	NOTE: GP1 must have the interlace bit toggle here,
		since it shouldn't be in readStatus as it was previously */
  GPU_GP1 ^= 0x80000000;

  if ( (GPU_GP1&0x08000000) ) // dma transfer NO update posible...
  		return;

  if ( (GPU_GP1&0x00800000) ) // Display disabled
  		return;

  ++vsincRateCounter;
  if (!skipCount)
	  gpuSkipUpdate();

}
