/*
 *  $Id: fbe.c,v 1.4 2000/12/03 12:25:47 tgkamp Exp $
 *
 *  picoTK generic Frame Buffer Emulator
 *
 *  Copyright (c) 2000 by Thomas Gallenkamp <tgkamp@users.sourceforge.net>
 *  Copyright (c) 2004 by Vince Busam <vince@sixpak.org>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
 /* Credits: I used the X11/Xlib demo application xscdemo v2.2b 
  *          (c) 1992 by Sudarshan Karkada, as a starting point 
  *          for this program.
  */

#define PROGNAME "fbe"
#define VERSION  "0.02"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

void X11_initialize();
void fbe_loop();

Display 	*display;
Colormap    colormap;
GC 		    gc;
Window		window, root, parent;
int 		depth, screen, visibility;

char *host  = NULL, *cmapbuf;

int CRTX=640;
int CRTY=480;
int ZOOM=1;
int CRTX_TOTAL=640;
int BITS_PER_PIXEL=8;
int lcd_look=0;
int redocmap = 0;

int PIXELS_PER_BYTE; 
int PIXEL_MASK;
int PIXELS_PER_LONG;

#define MAX_CRTX 640
#define MAX_CRTY 480
 
unsigned long *crtbuf;

#define CHUNKX 32
#define CHUNKY 20
  
unsigned long crcs[MAX_CRTX/CHUNKX][MAX_CRTY/CHUNKY];

unsigned long colors_24[256]=  /* contains 24 bit (00rrggbb) rgb color 
                                  mappings */ 
 { 
   0x00000000,0x000000c0,0x0000c000,0x0000c0c0,
   0x00c00000,0x00c000c0,0x00c0c000,0x00c0c0c0,
   0x00808080,0x000000ff,0x0000ff00,0x0000ffff,
   0x00ff0000,0x00ff00ff,0x00ffff00,0x00ffffff
 };

unsigned long colors_X11[256]; /* contains X11 variants, either 8,16 or
                                  24 bits */
void fbe_setcolors(void) {
  int i;
  unsigned short *cmap = (unsigned short *)cmapbuf, c;
  for (i=0;i<256;i++) {
    c = cmap[i];
    colors_24[i] = ((c&0xF800)<<8)|(((c>>5)&0x002F)<<10)|((c&0x001F)<<3);
  }
  if (BITS_PER_PIXEL==1) {
    if (lcd_look) {
      colors_24[0]=0x00a0c050;
      colors_24[1]=0x00808050;
    } else {
      colors_24[0]=0x00000000;
      colors_24[1]=0x00ff9000;
    }
  }
}

void fbe_calcX11colors(void) {
  int i;
  unsigned long c24;
  /*
   * Calculate X11 "pixel values" from 24 bit rgb (00rrggbb) color and
   * store them in colors_X11[} array for fast lookup. Mapping of rgb
   * colors to pixel values depends on the X11 Server color depth 
   */
 
  for (i=0; i<256; i++) {
    XColor xc;
          
    c24=colors_24[i];

    xc.red=   ((c24&0xff0000)>>16)*0x0101;
    xc.green= ((c24&0x00ff00)>> 8)*0x0101;
    xc.blue=  ((c24&0x0000ff)    )*0x0101;
    xc.flags= 0;
    XAllocColor(display,colormap,&xc);
    //XQueryColor(display,colormap,&xc);
    colors_X11[i]=xc.pixel;
  }
}

void fbe_init(void *crtbuf_)
{

  if(host == NULL)
    if(( host = (char *) getenv("DISPLAY")) == NULL)
      { fprintf(stderr, "%s", "Error: No environment variable DISPLAY\n");
      }

  PIXELS_PER_BYTE=(8/BITS_PER_PIXEL);
  PIXEL_MASK=((unsigned char *) 
	      "\x00\x01\x03\x07\x0f\x1f\x3f\x7f\xff") [BITS_PER_PIXEL];
  PIXELS_PER_LONG=PIXELS_PER_BYTE*4;
  X11_initialize();
  
  gc = XCreateGC(display, window, 0, NULL);
  crtbuf=crtbuf_;
  fbe_setcolors();
  fbe_calcX11colors();
  fbe_loop();
}


void X11_initialize()
{
  XSetWindowAttributes attr;
  char name[80];
  Pixmap iconPixmap;
  XWMHints xwmhints;

  if((display = XOpenDisplay(host)) == NULL) 
    {
      fprintf(stderr,"Error: Connection could not be made.\n");
      exit(1);
    }

  screen = DefaultScreen(display);
  colormap=DefaultColormap(display,screen);
  parent = root = RootWindow(display,screen);
  depth  = DefaultDepth(display,screen);

  XSelectInput(display,root,SubstructureNotifyMask);

  attr.event_mask = ExposureMask;

  attr.background_pixel=BlackPixel(display,screen);
   
  {
    window = XCreateWindow(display,root,0,
			   0,
			   CRTX*ZOOM,CRTY*ZOOM,       
			   0,depth,InputOutput, DefaultVisual(display,screen),
			   CWEventMask|CWBackPixel,&attr);

    sprintf(name,"fbe %d * %d * %d bpp",CRTX,CRTY,BITS_PER_PIXEL);

    XChangeProperty(display,window,XA_WM_NAME,XA_STRING,8,
		    PropModeReplace,name,strlen(name));
    XMapWindow(display,window);
  }

  xwmhints.icon_pixmap   = iconPixmap;
  xwmhints.initial_state = NormalState;
  xwmhints.flags         = IconPixmapHint | StateHint;

  XSetWMHints(display, window, &xwmhints );
  XClearWindow(display,window);
  XSync(display, 0);

}

  
unsigned long calc_patch_crc(int ix, int iy) { 
  unsigned long crc;
  int x, y;
  int off;
 
  off=(ix*CHUNKX)/PIXELS_PER_LONG+iy*CHUNKY*(CRTX_TOTAL/PIXELS_PER_LONG);
  crc=0x8154711;
 
  for (x=0; x<CHUNKX/PIXELS_PER_LONG; x++) 
    for (y=0; y<CHUNKY; y++) {
      unsigned long dat;
      dat=crtbuf[off+x+y*CRTX_TOTAL/PIXELS_PER_LONG];
     
      /*     crc^=((crc^dat)<<1)^((dat&0x8000) ? 0x1048:0); 
       */  
      crc+=(crc%211+dat);
      /* crc=(crc<<1)+((crc&0x80000000) ? 1:0);  */
    }
  return crc;
}

int repaint;
Pixmap pixmap;

void check_and_paint(int ix, int iy) {
  unsigned long crc;
  int x;
  int y;
  int off;
  int color;
 
  crc=calc_patch_crc(ix,iy);
 
  if (!repaint && crc==crcs[ix][iy]) return;
  off=ix*(CHUNKX/PIXELS_PER_BYTE)
    +iy*CHUNKY*(CRTX_TOTAL/PIXELS_PER_BYTE);
 
  XSetForeground(display,gc,0x000000);
  XFillRectangle(display, pixmap, gc, 0, 0, CHUNKX*ZOOM, CHUNKY*ZOOM);
  for (y=0; y<CHUNKY; y++) 
    for (x=0; x<CHUNKX; x++) {
      unsigned char data;
      data=(((unsigned char *)crtbuf)[off+x/PIXELS_PER_BYTE+y*(CRTX_TOTAL/PIXELS_PER_BYTE)]
	    );
      color=(data>>( ((PIXELS_PER_BYTE-1)-(x&(PIXELS_PER_BYTE-1)))*BITS_PER_PIXEL)) 
	& PIXEL_MASK;
      XSetForeground(display,gc,colors_X11[color]);
      if (ZOOM>1)
	XFillRectangle(display, pixmap, gc, x*ZOOM, y*ZOOM, 2, 2);
      else
	XDrawPoint(display, pixmap, gc, x, y);   
    }
 
  XCopyArea(display, pixmap, window, gc, 0, 0, CHUNKX*ZOOM,
	    CHUNKY*ZOOM, ix*CHUNKX*ZOOM, iy*CHUNKY*ZOOM);
  crcs[ix][iy]=crc; 
}
         

void fbe_loop()
{
  pixmap=XCreatePixmap(display,window,CHUNKX*ZOOM,CHUNKY*ZOOM,depth);
  repaint=0;
  while(1) {
    int x;
    int y;
    repaint=0;
    /*
      Check if to force complete repaint because of window 
      expose event
    */
    while((XPending(display) > 0)) {
      XEvent event;
      XNextEvent(display,&event);
      if (event.type==Expose) repaint=1;
    }
    
    /* 
       Sample all chunks for changes in shared memory buffer and
       eventually repaint individual chunks. Repaint everything if
       repaint is true (see above)
    */
    for (y=0; y<CRTY/CHUNKY; y++)
      for (x=0; x<CRTX/CHUNKX; x++)
	check_and_paint(x,y);
    usleep(1000);
    
    /* re-set color map */
    if (redocmap) {
      fbe_setcolors();
      fbe_calcX11colors();  
      redocmap = 0;
    }
  }
}

void usr1_handler(int sig) {
  redocmap = 1;
}

int main(int argc, char **argv) {
  int fd, cfd;
  int i;
  int leave,ok,help;
  char *arg,*argp,buf[64];
  unsigned char *fbuf;
  FILE *fp;

  help=0;
  for (i=1; i<argc; i++) {
    arg=argv[i];
    if (arg[0]=='-') {
      arg++;
      leave=0;
      do
	switch(*arg) {
	  /* Non Parameter options */
	case 'h': help=1;
	  break;
	case 'l': lcd_look=1;
	  break;
	default : leave=1;
	}
      while(!leave && *(++arg));
      /* Prepare for parameter option */
      if (*arg) {
	if      (arg[1]) argp=arg+1;
	else if (i<argc-1 && argv[i+1][0]) argp=argv[++i];
	else if (strchr("xytdz",*arg))
	  { fprintf(stderr,PROGNAME "-F-Use option -%c with parameter\n",*arg);
	  return(1); }
	ok=0;
	switch(*arg) {
	  /* Parameter options */
	case 'x': ok=sscanf(argp,"%d",&CRTX) == 1;
	  break;
	case 'y': ok=sscanf(argp,"%d",&CRTY) == 1;
	  break;
	case 'd': ok=sscanf(argp,"%d",&BITS_PER_PIXEL) == 1;
	  break;
	case 'z': ok=sscanf(argp,"%d",&ZOOM) == 1;
	  break;
	case 't': ok=sscanf(argp,"%d",&CRTX_TOTAL) == 1;
	  break;
	default:  fprintf(stderr,
			  PROGNAME "-F-Use of unrecognized option -%c\n",*arg);
	  help=1;
	  ok=1;
	  break;
	}

	if (!ok) {fprintf(stderr,
			  PROGNAME
			  "-F-Illegal option parameter %s for option -%c\n",
			  argp,*arg);
	return 1; }

      }
    }/* else
	fname=arg; */
  }

  if (CRTX_TOTAL<CRTX) CRTX_TOTAL = CRTX;

  if (!ok || help) {
    printf(PROGNAME " " VERSION " "
	   " Frame Buffer Emulator\n"
	   "\n"
	   "Usage: " PROGNAME " [-<options>]\n"
	   "   Options:\n"
	   "       -x   X size            [%3d]\n"
	   "       -y   Y Size            [%3d]\n"
	   "       -t   X Total Size      [%3d]\n"
	   "       -d   Color depths bpp  [%d] \n"
	   "       -z   Zoom factor       [%d] \n"
	   "       -l   LCD look for 1bpp      \n",
	   CRTX,CRTY,CRTX_TOTAL,BITS_PER_PIXEL,ZOOM);
    return(1); }

  /*
   * Create shared memory dummy file if not already existent 
   */
  fd=open("/tmp/fbe_buffer",O_RDONLY);
  if (fd>0) {
    close(fd); 
  } else {
    fd=open("/tmp/fbe_buffer",O_CREAT|O_WRONLY,0777);
    for(i=0; i<(CRTX_TOTAL*CRTY*BITS_PER_PIXEL/8); i++) write(fd,"\000",1);
    close(fd);
  }
 
  fd=open("/tmp/fbe_buffer",O_RDWR);
  fbuf=mmap(NULL, (CRTX_TOTAL*CRTY*BITS_PER_PIXEL/8), PROT_READ|PROT_WRITE, MAP_SHARED,fd,0);

  cfd=open("/tmp/fbe_cmap",O_RDONLY);
  if (cfd>0) {
    close(cfd); 
  } else {
    cfd=open("/tmp/fbe_cmap",O_CREAT|O_WRONLY,0777);
    for(i=0; i<512; i++) write(cfd,"\000",1);
    close(cfd);
  }
  
  cfd=open("/tmp/fbe_cmap",O_RDWR);
  cmapbuf=mmap(NULL, 512, PROT_READ|PROT_WRITE, MAP_SHARED,cfd,0);
  signal(SIGUSR1,usr1_handler);

  fp = fopen("/tmp/fbe.pid","w");
  if (fp) {
    sprintf(buf,"%d",getpid());
    fputs(buf,fp);
    fclose(fp);
  }
 
  fbe_init(fbuf);

  return 0;
}
