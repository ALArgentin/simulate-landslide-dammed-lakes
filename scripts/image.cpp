#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <ctype.h>
#include <math.h>
#include <unistd.h>

#define Lint unsigned int
#define SetRGB(r,g,b) ((Lint)(r)<<20|(Lint)(g)<<10|(Lint)(b))
#define GetRed(im,i,j) (im->rgb[i][j]>>20&1023)
#define GetGreen(im,i,j) (im->rgb[i][j]>>10&1023)
#define GetBlue(im,i,j) (im->rgb[i][j]&1023)
#define GetGray(im,i,j) (((im->rgb[i][j]>>20&1023)+(im->rgb[i][j]>>10&1023)+(im->rgb[i][j]&1023)+1)/3)
#define getR(u) (u>>20&1023)
#define getG(u) (u>>10&1023)
#define getB(u) (u&1023)


struct image { int f; int h; int w; Lint **rgb; char *c; };


struct image * Resize ( struct image *, Lint, Lint );


#ifndef BUILDNAME

#define BUILDNAME

char *BuildName ( char *name, int number, char *ext, int ndigits, char *out )

{
  char 	format[10];
  int	i;

  if ( out == NULL )
    out = (char*) calloc ( strlen(name)+ndigits+strlen(ext)+1, 1 );
  strcpy ( out, name );
  sprintf ( format, "%%s%%%ii%%s", ndigits );
  sprintf ( out, format, name, number, ext );
  for ( i = 0; i < strlen(out); ++i )
    if ( out[i] == ' ' ) out[i] = '0';
  return out;
}

#endif


void Exit ( const char *text )

{
  printf ( "Error in function %s, program terminated\n", text );
  exit(-1);
}


Lint setHSB ( double h, double s, double v )
{
  double  r, g, b;

  while ( h < 0 )  ++h;
  while ( h >= 1 )  --h;
  h *= 6;
  switch ( (int)h )
  {
    case 0: r = 1;
            g = h;
            b = 0;
            break;
    case 1: r = 2-h;
            g = 1;
            b = 0;
            break;
    case 2: r = 0;
            g = 1;
            b = h-2;
            break;
    case 3: r = 0;
            g = 4-h;
            b = 1;
            break;
    case 4: r = h-4;
            g = 0;
            b = 1;
            break;
    case 5: r = 1;
            g = 0;
            b = 6-h;
            break;
  }
  r = v*(1+s*(r-1));  
  g = v*(1+s*(g-1));  
  b = v*(1+s*(b-1));  
  return SetRGB((int)(1023*r+0.5),(int)(1023*g+0.5),(int)(1023*b+0.5));
}

struct image * AllocImage ( Lint h, Lint w )

{
  Lint	  i;
  struct image  *im;
  
  im = (struct image*) calloc ( 1, sizeof(struct image) );
  if ( im == NULL )  Exit( "Alloc Image" );
  im->f = 1;
  im->h = h;
  im->w = w;
  im->rgb = (Lint**) calloc ( h, sizeof(Lint*) );
  if ( im->rgb == NULL )  Exit ( "AllocImage" );
  *(im->rgb) = (Lint*) calloc ( h*w, sizeof(Lint) );
  if ( *(im->rgb) == NULL )  Exit ( "AllocImage" );
  for ( i = 1; i < h; ++i )  im->rgb[i] = im->rgb[i-1] + w;
  im->c = (char*) calloc ( 1, sizeof(char) );
  im->c[0] = '\0';
  return im;
}


struct image * Copy ( struct image * im, double top, double left,
							 Lint h, Lint w )
{
  int           i, j, t, l, wi, wj, wt;
  struct image  *out;

  if ( im == NULL )  Exit( "Copy" );
  if ( h+top > im->h )  h = im->h-top;
  if ( w+left > im->w )  w = im->w-left;
  t = top;
  if ( top-t < 0.01 )    top = t;
  if ( t+1-top < 0.01 )  top = ++t;
  wi = fabs(top-t) > 0.005 ? 1024*(top-t) : 0;
  l = left;
  if ( left-l < 0.01 )    left = l;
  if ( l+1-left < 0.01 )  left = ++l;
  wj = fabs(left-l) > 0.005 ? 1024*(left-l) : 0;
  out = AllocImage ( wi ? h+1 : h, wj ? w+1 : w );
  for ( i = 0; i < h; ++i )
    for ( j = 0; j < w; ++j )  out->rgb[i][j] = im->rgb[t+i][l+j];
  if ( wi )
  {
    wt = 1024-wi;
    for ( i = 0; i < h; ++i )
      for ( j = 0; j < w; ++j )
        out->rgb[i][j] = SetRGB ( (wt*GetRed(out,i,j)+wi*GetRed(out,i+1,j))>>10,(wt*GetGreen(out,i,j)+wi*GetGreen(out,i+1,j))>>10,(wt*GetBlue(out,i,j)+wi*GetBlue(out,i+1,j))>>10);
    out->h = h;
  }  
  if ( wj )
  {
    wt = 1024-wj;
    for ( i = 0; i < out->h; ++i )
      for ( j = 0; j < out->w; ++j )
        out->rgb[i][j] = SetRGB ( (wt*GetRed(out,i,j)+wj*GetRed(out,i,j+1))>>10,(wt*GetGreen(out,i,j)+wj*GetGreen(out,i,j+1))>>10,(wt*GetBlue(out,i,j)+wj*GetBlue(out,i,j+1))>>10);
    out->w = w;
  }  
  return out;
}


void SetComment ( struct image * im, char * text )

{
  im->c = (char*) realloc ( im->c, strlen(text)+1 );
  strcpy ( im->c, text );
}

void addToComment ( struct image * im, const char * text )
{
  char  *tmp;

  tmp = (char*) calloc ( strlen(im->c)+strlen(text)+1, 1 );
  strcpy ( tmp, im->c );
  im->c = tmp;
  strcat ( im->c, text );
}

void DestroyImage ( struct image *im )

{
  if ( im->c != NULL )  free ( im->c );
  if ( im->f && *(im->rgb) != NULL )  free ( *(im->rgb) );
  if ( im->rgb != NULL )  free ( im->rgb );
  if ( im != NULL )  free ( im );
}


struct image * Resize ( struct image * im, Lint h, Lint w )

{
  struct image  *tmp, *out;
  Lint	  	i, j, k, ns, nd, *row, *weight, *sum, *line, *p, red, green,
									 blue;
  if ( h > 0 )
  {
    if ( w <= 0 )  w = (h*im->w)/im->h;
  }
  else
    if ( w > 0 )  h = (w*im->h)/im->w;
    else
    {
      h = im->h;
      w = im->w;
    }
  if ( h == im->h && w == im->w )  return Copy ( im, 0, 0, h, w );
  ns = h<im->h ? h : im->h;
  nd = h<im->h ? im->h : h;
  tmp = AllocImage ( h, im->w );
  row = (Lint*) calloc ( nd, sizeof(Lint) );
  weight = (Lint*) calloc ( nd, sizeof(Lint) );
  for ( i = 0; i < nd; ++i )
  {
    row[i] = (i*ns)/nd;
    weight[i] = nd*(row[i]+1)-i*ns;
  }
  if ( h >= im->h )
  {
    if ( row[nd-1] == ns-1 )  weight[nd-1] = nd;
    for ( i = 0; i < nd; ++i )
      for ( j = 0; j < im->w; ++j )
      {
        k = nd-weight[i];
        red = ( weight[i] * GetRed(im,row[i],j)
		      + ( k ? k * GetRed(im,row[i]+1,j) : 0 ) ) / nd;
        green = ( weight[i] * GetGreen(im,row[i],j)
		        + ( k ? k * GetGreen(im,row[i]+1,j) : 0 ) ) / nd;
        blue = ( weight[i] * GetBlue(im,row[i],j)
		       + ( k ? k * GetBlue(im,row[i]+1,j) : 0 ) ) / nd;
	tmp->rgb[i][j] = SetRGB(red,green,blue);
      }
  }
  else
  {
    sum = (Lint*) calloc ( ns, sizeof(Lint) );
    line = (Lint*) calloc ( 3*ns, sizeof(Lint) );
    for ( i = 0; i < nd; ++i )
    {
      sum[row[i]] += weight[i];
      if ( row[i] != ns-1 )  sum[row[i]+1] += nd-weight[i];
    }
    for ( j = 0; j < im->w; ++j )
    {
      for ( i = 0; i < nd; ++i )
      {
	p = line+3*row[i];
	*p++ += weight[i] * GetRed(im,i,j);
	*p++ += weight[i] * GetGreen(im,i,j);
	*p++ += weight[i] * GetBlue(im,i,j);
    	if ( row[i] != ns-1 )
	{
	  *p++ += (nd-weight[i]) * GetRed(im,i,j);
	  *p++ += (nd-weight[i]) * GetGreen(im,i,j);
	  *p++ += (nd-weight[i]) * GetBlue(im,i,j);
	}
      }
      p = line;
      for ( i = 0; i < ns; ++i )
      {
        red = *p/sum[i];
        *p++ = 0;
        green = *p/sum[i];
	*p++ = 0;
	blue = *p/sum[i];
	*p++ = 0;
	tmp->rgb[i][j] = SetRGB(red,green,blue);
      }
    }
    free ( line );
    free ( sum );
  }
  free ( weight );
  free ( row );
  ns = w<im->w ? w : im->w;
  nd = w<im->w ? im->w : w;
  out = AllocImage ( h, w );
  row = (Lint*) calloc ( nd, sizeof(Lint) );
  weight = (Lint*) calloc ( nd, sizeof(Lint) );
  for ( j = 0; j < nd; ++j )
  {
    row[j] = (j*ns)/nd;
    weight[j] = nd*(row[j]+1)-j*ns;
  }
  if ( w >= im->w )
  {
    if ( row[nd-1] == ns-1 )  weight[nd-1] = nd;
    for ( i = 0; i < h; ++i )
      for ( j = 0; j < nd; ++j )
      {
  	k = nd-weight[j];
	red = ( weight[j] * GetRed(tmp,i,row[j])
		      + ( k ? k * GetRed(tmp,i,row[j]+1) : 0 ) ) / nd;
        green = ( weight[j] * GetGreen(tmp,i,row[j])
		        + ( k ? k * GetGreen(tmp,i,row[j]+1) : 0 ) ) / nd;
	blue = ( weight[j] * GetBlue(tmp,i,row[j])
		      + ( k ? k * GetBlue(tmp,i,row[j]+1) : 0 ) ) / nd;
	out->rgb[i][j] = SetRGB(red,green,blue);
      }
  }
  else
  {
    sum = (Lint*) calloc ( ns, sizeof(Lint) );
    line = (Lint*) calloc ( 3*ns, sizeof(Lint) );
    for ( j = 0; j < nd; ++j )
    {
      sum[row[j]] += weight[j];
      if ( row[j] != ns-1 )  sum[row[j]+1] += nd-weight[j];
    }
    for ( i = 0; i < h; ++i )
    {
      for ( j = 0; j < nd; ++j )
      {
	p = line+3*row[j];
	*p++ += weight[j] * GetRed(tmp,i,j);
	*p++ += weight[j] * GetGreen(tmp,i,j);
	*p++ += weight[j] * GetBlue(tmp,i,j);
    	if ( row[j] != ns-1 )
	{
	  *p++ += (nd-weight[j]) * GetRed(tmp,i,j);
	  *p++ += (nd-weight[j]) * GetGreen(tmp,i,j);
	  *p++ += (nd-weight[j]) * GetBlue(tmp,i,j);
	}
      }
      p = line;
      for ( j = 0; j < ns; ++j )  
      {
        red = *p/sum[j];
        *p++ = 0;
        green = *p/sum[j];
        *p++ = 0;
        blue = *p/sum[j];
        *p++ = 0;
        out->rgb[i][j] = SetRGB(red,green,blue);
      }
    }
    free ( line );
    free ( sum );
  }
  free ( weight );
  free ( row );
  DestroyImage ( tmp );
  return out;
} 


struct image * SelectSection ( struct image * src, Lint top, Lint left,
							Lint h, Lint w )
{
  Lint          i;
  struct image  *im;

  if ( src == NULL )  Exit( "SelectSection" );
  im = (struct image*) calloc ( 1, sizeof(struct image) );
  if ( im == NULL )  Exit ( "SelectSection" );
  im->f = 0;
  im->h = h+top <= src->h ? h : src->h-top;
  im->w = w+left <= src->w ? w : src->w-left;
  im->rgb = (Lint**) calloc ( h, sizeof(Lint*) );
  if ( im->rgb == NULL )  Exit ( "SelectSection" );
  for ( i = 0; i < im->h; ++i )  im->rgb[i] = src->rgb[i+top]+left;
  im->c = src->c;
/*
  im->c = (char*) calloc ( 1, sizeof(char) );
  im->c[0] = '\0';
*/
  return im;
}


struct image * ReadPPM ( const char *filename )

{
  FILE           *fp;
  char  	 text[1024], *rest, *co;
  unsigned char  *line, c;
  Lint	         i, j, w, h, r, l, m;
  struct image   *im;

  if ( ( fp = fopen ( filename, "rb" ) ) == NULL )   Exit ( "ReadPPM" );
  if ( fgets ( text, 1024, fp ) == NULL )  Exit ( "ReadPPM" );
  if ( text[0] - 'P' )  Exit ( "ReadPPM" );
  c = text[1];
  co = (char*) calloc ( 1024, sizeof(char) );
  *co = '\0';
  l = 1;
  do
  {
    if ( fgets(text,1024,fp) == NULL )  Exit ( "ReadPPM" );
    if ( strstr(text,"\r#") != NULL )  strcpy(strstr(text,"\r#"),"\n\0");
    if ( *text == '#' )
    {
      if ( strlen(co)+strlen(text) > 1024*l )
        co = (char*) realloc ( co, 1024*++l );
      for ( i = 1; isspace(text[i]); ++i ); 
      if ( strncmp ( text+i, "Copyright Stefan Hergarten", 28 ) )
        strcat ( co, text+i );
    }  	
  }
  while ( *text == '#' );
  do
  {
    w = strtol ( text, &rest, 10 );
    h = strtol ( rest, NULL, 10 );
    if ( w <= 0 || h <= 0 )
      if ( fgets(text,1024,fp) == NULL )  Exit ( "ReadPPM" );
  }
  while ( w <= 0 || h <= 0 );
  im = AllocImage ( h, w );
  free ( im->c );
  im->c = co;
  line = (unsigned char*) calloc ( 3*w, 1 );
  switch ( c )
  {
    case '4': for ( i = 0; i < h; ++i )
              {
                if ( fread ( line, 1, (w+7)/8, fp ) - (w+7)/8 )
                  Exit ( "ReadPPM" );
		m = 128;
	        for ( j = 0; j < im->w; ++j )
	        {
		  if ( !(line[j/8]&m) )
                    im->rgb[i][j] = SetRGB(1023,1023,1023);        
      		  if ( !(m>>=1) ) m = 128;
	        }	
	      }
	      break;
    case '5': if ( fgets ( text, 256, fp ) == NULL )  Exit ( "ReadPPM" );
              for ( i = 0; i < h; ++i )
              {
                if ( fread ( line, 1, w, fp ) - w )  Exit ( "ReadPPM" );
	        for ( j = 0; j < im->w; ++j )
	        {
	          r = line[j]<<2 | rand()&3;
		  im->rgb[i][j] = SetRGB(r,r,r);
	        }	
	      }
	      break;
    case '6': if ( fgets ( text, 256, fp ) == NULL )  Exit ( "ReadPPM" );
              for ( i = 0; i < h; ++i )
              {
                if ( fread ( line, 1, 3*w, fp ) - 3*w )  Exit ( "ReadPPM" );
                for ( j = 0; j < im->w; ++j )
                {
                  r = rand();
                  im->rgb[i][j] = SetRGB(line[3*j],line[3*j+1],line[3*j+2]) << 2
			    			    | r&0XC03 | (r&3) << 20;
    	        }
	      }
	      break;
    default : Exit ( "ReadPPM" );
  }
  free ( line );
  fclose ( fp );
  return im;
}

struct image * readImage ( const char *filename )
{
  char  com[1024];

  sprintf ( com, "convert %s /tmp/tmp.ppm", filename );
  system ( com );
  return ReadPPM ( "/tmp/tmp.ppm" );
}

struct image * ReadPPMfromCD ( char *filename )

{
  FILE           *fp;
  char  	 name[128];

  sprintf ( name, "/cdrom/%s", filename );
  while ( ( fp = fopen ( name, "rb" ) ) == NULL )
  {
    fclose ( fp );
    system ( "umount /cdrom" );
    printf ( "File %s not found. Replace CD and press the return key.\n",
    								filename ); 
    getchar();
    system ( "mount /cdrom" );
  }
  fclose ( fp );
  return ReadPPM ( name ); 
}


void WritePPM ( struct image * im, const char *filename )

{
  Lint	         i, j;
  unsigned char  text[256], *line;
  FILE	         *fp;

  fp = fopen ( filename, "wb" );
  if ( fp == NULL )  return;
  fprintf ( fp, "P6\n# " );
  for ( i = 0; im->c[i]-'\0'; ++i )
  {
    fputc ( im->c[i], fp );
    if ( im->c[i] == '\n' )  fprintf ( fp, "# " );
  }
  fprintf ( fp, "\n%1i %1i\n255\n", im->w, im->h );
  line = (unsigned char*) calloc ( 3*im->w, 1 );
  for ( i = 0; i < im->h; ++i )
  {
    for ( j = 0; j < im->w; ++j )
    {
      line[3*j] = im->rgb[i][j] >> 22;
      line[3*j+1] = im->rgb[i][j] >> 12;
      line[3*j+2] = im->rgb[i][j] >> 2;
    }
    fwrite ( line, 1, 3*im->w, fp );
  }
  free ( line );
  fclose ( fp );
}


void WritePGM ( struct image * im, char *filename )

{
  Lint	         i, j;
  unsigned char  text[256], *line;
  FILE	         *fp;

  fp = fopen ( filename, "wb" );
  if ( fp == NULL )  return;
  fprintf ( fp, "P5\n# " );
  for ( i = 0; im->c[i]-'\0'; ++i )
  {
    fputc ( im->c[i], fp );
    if ( im->c[i] == '\n' )  fprintf ( fp, "# " );
  }
  fprintf ( fp, "Copyright Stefan Hergarten\n%1i %1i\n255\n", im->w, im->h );
  line = (unsigned char*) calloc ( im->w, 1 );
  for ( i = 0; i < im->h; ++i )
  {
    for ( j = 0; j < im->w; ++j )  line[j] = GetGray(im,i,j) >> 2;
    fwrite ( line, 1, im->w, fp );
  }
  free ( line );
  fclose ( fp );
}


void WritePBM ( struct image * im, char *filename )

{
  Lint	         i, j, m, l;
  unsigned char  text[256], *line;
  FILE	         *fp;

  fp = fopen ( filename, "wb" );
  if ( fp == NULL )  return;
  fprintf ( fp, "P4\n# " );
  for ( i = 0; im->c[i]-'\0'; ++i )
  {
    fputc ( im->c[i], fp );
    if ( im->c[i] == '\n' )  fprintf ( fp, "# " );
  }
  fprintf ( fp, "Copyright Stefan Hergarten\n%1i %1i\n", im->w, im->h );
  line = (unsigned char*) calloc ( l=(im->w+7)/8, 1 );
  for ( i = 0; i < im->h; ++i )
  {
    m = 128;
    for ( j = 0; j < l; ++j )  line[j] = 0;
    for ( j = 0; j < im->w; ++j )
    {
      if ( GetGray(im,i,j) < 512 )  line[j/8] |= m;
      if ( !(m>>=1) ) m = 128;
    }  
    fwrite ( line, 1, l, fp );
  }
  free ( line );
  fclose ( fp );
}


struct image * ReadBMP ( char *filename )

{
  FILE           *fp;
  unsigned char  text[54], *line;
  int	         i, j, w, h, r, n;
  struct image   *im;

  if ( ( fp = fopen ( filename, "rb" ) ) == NULL )   Exit ( "ReadBMP" );
  if ( fread ( text, 1, 54, fp ) - 54 )  Exit ( "ReadBMP" );
  w = text[18]+256*text[19];
  h = text[22]+256*text[23];
  if ( w <= 0 || h <= 0 )  Exit( "ReadBMP" );
  im = AllocImage ( h, w );
  n = 4*((3*w+3)/4);
  line = (unsigned char*) calloc ( n, 1 );
  for ( i = h-1; i >= 0; --i )
  {
    if ( fread ( line, 1, n, fp ) - n )  Exit ( "ReadBMP" );
    for ( j = 0; j < w; ++j )
    {
      r = rand();
      im->rgb[i][j] = SetRGB(line[3*j+2],line[3*j+1],line[3*j]) << 2
			    			  | r&0XC03 | (r&3) << 20;
    }
  }
  free ( line );
  fclose ( fp );
  return im;
}


struct image * ReadYUV ( char *filename )

{
  unsigned char  line[1440];
  FILE           *fp;
  struct image   *im;
  long int       i, j, g, red, green, blue, wr = 306, wg = 601, wb = 117;
  
  im = AllocImage ( 576, 720 );
  fp = fopen ( filename, "rb" );
  if ( fp == NULL )  Exit ( "ReadYUV" );
  for ( i = 0; i < 576; ++i )
  {
    fread ( line, 1, 1440, fp );
    for ( j = 0; j < 720; ++j )
    {
      g = (int)line[2*j+1]<<2 | rand()&3;	
      blue = g + ((wr+wg)*((int)line[(j>>1)<<2]-128))/128;
      red = g + ((wb+wg)*((int)line[((j>>1)<<2)+2]-128))/128; 
      green = (1024*g-wb*blue-wr*red) / wg;
      if ( red < 0 )       red = 0;
      if ( red > 1023 )    red = 1023;
      if ( green < 0 )     green = 0;
      if ( green > 1023 )  green = 1023;
      if ( blue < 0 )      blue = 0;
      if ( blue > 1023 )   blue = 1023;
      im->rgb[i][j] = SetRGB(red,green,blue);
    }
  }
  fclose ( fp );
  return im;
}

    
void WriteYUV ( struct image * im, char *filename )

{
  Lint                  i, j, red, green, blue, tmp,
  			wr = 306, wg = 601, wb = 117;
  char			com[64];
  static unsigned char  line[1440];
  FILE           	*fp;

  if ( im->h-576 || im->w-720 )  Exit ( "WriteYUV" ); 
  fp = fopen ( filename, "wb" );
  if ( fp == NULL )  return;
  for ( i = 0; i < 576; ++i )
  {
    for ( j = 0; j < 720; j+=2 )
    {
      red = GetRed(im,i,j) + GetRed(im,i,j+1);
      green = GetGreen(im,i,j) + GetGreen(im,i,j+1);
      blue = GetBlue(im,i,j) + GetBlue(im,i,j+1);
      line[2*j] = (wr*(2304+blue-red)+wg*(2304+blue-green)) / (18*(wr+wg));
      line[2*j+1] = (wr*GetRed(im,i,j)+wg*GetGreen(im,i,j)
	    			+wb*GetBlue(im,i,j)) / 4769 + 16;
      line[2*j+2] = (wb*(2304+red-blue)+wg*(2304+red-green)) / (18*(wb+wg));
      line[2*j+3] = (wr*GetRed(im,i,j+1)+wg*GetGreen(im,i,j+1)
	                        +wb*GetBlue(im,i,j+1)) / 4769 + 16;
      }
    fwrite ( line, 1, 1440, fp );
  }
  fclose ( fp );
}


struct image * ReadPS ( const char *filename, int col, int bgred, int bggreen,
		        int bgblue, int dpi, double b )
{
  int		i, j, left = 32767, right = 0, bottom = 0, top = 32767, border;
  char  	text[256], name[L_tmpnam];
  struct image  *im, *out;

  strcpy ( name, "/tmp/readps.ppm" );
  if ( col )
    sprintf ( text, "gs -sDEVICE=ppmraw -sPAPERSIZE=bbox -r%d -q -dNOPAUSE -sOutputFile=%s -- %s", dpi, name, filename );
  else
    sprintf ( text, "gs -sDEVICE=pgmraw -sPAPERSIZE=a4 -r%d -q -dNOPAUSE -sOutputFile=%s -- %s", dpi, name, filename );
  system ( text );
  im = ReadPPM ( name );
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
      if ( abs((int)(GetRed(im,i,j)-bgred)) > 7 || abs((int)(GetGreen(im,i,j)-bggreen)) > 7
      		|| abs((int)(GetBlue(im,i,j)-bgblue)) > 7 )
      {
        if ( i < top )  top = i;
	if ( i > bottom )  bottom = i;
	if ( j < left )  left = j;
	if ( j > right )  right = j;
      }	
  if ( bottom <= top || right <= left )
  {
    left = top = 0;
    right = bottom = 1;
  }
  if ( b > 0. )
  {
    border = bottom-top;
    if ( right-left > border )  border = right-left;
    border = b*border+2;
    if ( (top-=border) < 0 )  top = 0;
    if ( (left-=border) < 0 )  left = 0;
    if ( (bottom+=border) >= im->h )  bottom = im->h-1;
    if ( (right+=border) >= im->w )  right = im->w-1;
  }
  out = Copy ( im, top, left, bottom-top+1, right-left+1 );
  DestroyImage ( im );
  unlink ( name );
  return out;
}


/*
struct image * ReadPS ( char *filename, int col, int bgred, int bggreen,
		        int bgblue, int page, int dpi, int nogs, double b )
{
  int		i, j, left = 32767, right = 0, bottom = 0, top = 32767, border;
  char  	text[256];
  struct image  *im, *out;

  if ( !nogs )
  {
    if ( col )
      sprintf ( text, "gs -sDEVICE=ppmraw -sPAPERSIZE=a4 -r%d -q -dNOPAUSE -sOutputFile=/tmp/slidelib%%d.ppm -- %s", dpi, filename );
    else
      sprintf ( text, "gs -sDEVICE=pgmraw -sPAPERSIZE=a4 -r%d -q -dNOPAUSE -sOutputFile=/tmp/slidelib%%d.ppm -- %s", dpi, filename );
    system ( text );
  }
  sprintf ( text, "/tmp/slidelib%d.ppm", page );
  im = ReadPPM ( text );
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
      if ( abs(GetRed(im,i,j)-bgred) > 7 || abs(GetGreen(im,i,j)-bggreen) > 7
      		|| abs(GetBlue(im,i,j)-bgblue) > 7 )
      {
        if ( i < top )  top = i;
	if ( i > bottom )  bottom = i;
	if ( j < left )  left = j;
	if ( j > right )  right = j;
      }	
  border = bottom-top;
  if ( right-left > border )  border = right-left;
  border = b*border+2;
  if ( (top-=border) < 0 )  top = 0;
  if ( (left-=border) < 0 )  left = 0;
  if ( (bottom+=border) >= im->h )  bottom = im->h-1;
  if ( (right+=border) >= im->w )  right = im->w-1;
  out = Copy ( im, top, left, bottom-top+1, right-left+1 );
  DestroyImage ( im );
  return out;
}
*/


struct image * RotateImage ( struct image * im )

{
  struct image  *out;
  Lint	        i, j;

  out = AllocImage ( im->w, im->h );
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )  out->rgb[im->w-j-1][i] = im->rgb[i][j];
  return out;
}


void WritePS ( struct image * im, char *filename, int dpi )

{
  FILE          *fp;
  int	        i, j, k, h, w, hoff, voff;
  struct image  *out;
  
  out = im->w > im->h && im->w > 7.5*dpi ? RotateImage ( im ) : im; 
  w = (72*out->w+dpi-1)/dpi;
  h = (72*out->h+dpi-1)/dpi;
  hoff = (595-w)/2;
  if ( hoff < 25 )  hoff = 25;
  voff = (842-h)/2;
  if ( voff < 25 )  voff = 25;
  fp = fopen ( filename, "w" );
  fprintf ( fp, "%%!PS-Adobe-2.0 EPSF-2.0\n" );
  fprintf ( fp, "%%%%Creator: Stefan Hergarten\n" );
  fprintf ( fp, "%%%%BoundingBox: %i %i %i %i\n", hoff, voff, hoff+w, voff+h );
  fprintf ( fp, "%%%%EndComments\n" );
  fprintf ( fp, "/origstate save def\n" );
  fprintf ( fp, "20 dict begin\n" );
  fprintf ( fp, "/pix %i string def\n", 3*out->w );
  fprintf ( fp, "/grays %i string def\n", out->w );
  fprintf ( fp, "/npixls 0 def\n" );
  fprintf ( fp, "/rgbindx 0 def\n" );
  fprintf ( fp, "%i %i translate\n", hoff, voff );
  fprintf ( fp, "%f %f scale\n", (72.*out->w)/dpi, (72.*out->h)/dpi );
  fprintf ( fp, "/colorimage where\n" );
  fprintf ( fp, "  { pop }\n" );
  fprintf ( fp, "  {\n" );
  fprintf ( fp, "    /colortogray {\n" );
  fprintf ( fp, "      /rgbdata exch store\n" );
  fprintf ( fp, "      rgbdata length 3 idiv\n" );
  fprintf ( fp, "      /npixls exch store\n" );
  fprintf ( fp, "      /rgbindx 0 store\n" );
  fprintf ( fp, "0 1 npixls 1 sub {\n" );
  fprintf ( fp, "        grays exch\n" );
  fprintf ( fp, "        rgbdata rgbindx       get 20 mul\n" );
  fprintf ( fp, "        rgbdata rgbindx 1 add get 37 mul\n" );
  fprintf ( fp, "        rgbdata rgbindx 2 add get 7 mul\n" );
  fprintf ( fp, "        add add 64 idiv\n" );
  fprintf ( fp, "        put\n" );
  fprintf ( fp, "        /rgbindx rgbindx 3 add store\n" );
  fprintf ( fp, "      } for\n" );
  fprintf ( fp, "      grays 0 npixls getinterval\n" );
  fprintf ( fp, "    } bind def\n" );
  fprintf ( fp, "    /mergeprocs {\n" );
  fprintf ( fp, "      dup length 3 -1 roll dup length\n" );
  fprintf ( fp, "      dup 5 1 roll\n 3 -1 roll add array cvx\n" );
  fprintf ( fp, "      dup 3 -1 roll 0 exch putinterval dup 4 2 roll putinterval\n" );
  fprintf ( fp, "    } bind def\n" );
  fprintf ( fp, "    /colorimage { pop pop {colortogray} mergeprocs image } bind def \n" );
  fprintf ( fp, "  } ifelse\n\n" );
  fprintf ( fp, "%i %i 8\n", out->w, out->h );
  fprintf ( fp, "[%i 0 0 %i 0 0]\n", out->w, out->h );
  fprintf ( fp, "{ currentfile pix readhexstring pop }\n" );
  fprintf ( fp, "false 3 colorimage\n" );
  for ( i = 0; i < out->h; ++i )
  {
    k = out->h-1-i;
    for ( j = 0; j < out->w; ++j )
      fprintf ( fp, "%x%x%x%x%x%x", out->rgb[k][j]>>26&15, out->rgb[k][j]>>22&15,
				    out->rgb[k][j]>>16&15, out->rgb[k][j]>>12&15,
				    out->rgb[k][j]>>6&15, out->rgb[k][j]>>2&15 );
    fprintf ( fp, "\n" );
  }
  fprintf ( fp, "showpage\n" );
  fprintf ( fp, "end\n" );
  fprintf ( fp, "origstate restore\n" );
  fprintf ( fp, "%%%%Trailer\n" );
  fclose ( fp );
  if ( im->w > im->h && im->w > 7.5*dpi )  DestroyImage ( out );
}


void Paste ( struct image * dest, struct image * src, int ioff, int joff )

{
  int           i, j;

  for ( i = 0; i < src->h; ++i )
    for ( j = 0; j < src->w; ++j )  dest->rgb[i+ioff][j+joff] = src->rgb[i][j];
}


struct image * Enlarge ( struct image * im, int h, int w, int mode )

{
  int	        i, j, ioff, joff;
  struct image  *out;
  
  if ( h < im->h )  h = im->h;
  if ( w < im->w )  w = im->w;
  out = AllocImage ( h, w );
  if ( mode == 1 )
    for ( i = 0; i < out->h; ++i )
      for ( j = 0; j < out->w; ++j )  out->rgb[i][j] = im->rgb[0][0];
  ioff = (h-im->h)/2;
  joff = (w-im->w)/2;
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )  out->rgb[i+ioff][j+joff] = im->rgb[i][j];
  if ( mode == 2 )
  {
    for ( i = 0; i < ioff; ++i )
      for ( j = 0; j < out->w; ++j )
	out->rgb[i][j] = out->rgb[2*ioff-1-i][j];
    for ( i = ioff+im->h; i < out->h; ++i )
      for ( j = 0; j < out->w; ++j )
	out->rgb[i][j] = out->rgb[2*(ioff+im->h)-1-i][j];
    for ( j = 0; j < joff; ++j )
      for ( i = 0; i < out->h; ++i )
	out->rgb[i][j] = out->rgb[i][2*joff-1-j];
    for ( j = joff+im->w; j < out->w; ++j )
      for ( i = 0; i < out->h; ++i )
	out->rgb[i][j] = out->rgb[i][2*(joff+im->w)-1-j];
  }
  return out;
}


struct image * BlurDark ( struct image * im )

{
  struct image  *out;
  Lint		i, j, k, l, red, green, blue;

  out = AllocImage ( im->h, im->w );
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j ) 
    {
      red = green = blue = 1023;
      for ( k = i-1; k <= i+1; ++k )
        if ( k >= 0 && k < im->h )
	  for ( l = j-1; l <= j+1; ++l )
	    if ( l >= 0 && l < im->w )
	    {
	      if ( GetRed(im,k,l) < red )  red = GetRed(im,k,l);
              if ( GetGreen(im,k,l) < green )  green = GetGreen(im,k,l);
	      if ( GetBlue(im,k,l) < blue )  blue = GetBlue(im,k,l);
            }
      out->rgb[i][j] = SetRGB(red,green,blue);
    }  
  return out;	      
}


struct image * Average ( struct image * im, int m )

{
  struct image  *out;
  int		i, j, k, l, k1, k2, red, green, blue;

  out = AllocImage ( im->h, im->w );
  for ( i = 0; i < im->h; ++i )
  {
    k1 = i-m > 0 ? i-m : 0;
    k2 = i+m < im->h-1 ? i+m : im->h-1;
    red = green = blue = 0;
    for ( j = -m; j < im->w; ++j )
    { 
      if ( (l=j-m-1) >= 0 )
        for ( k = k1; k <= k2; ++k )
        {       
          red -= GetRed(im,k,l); 
          green -= GetGreen(im,k,l);
          blue -= GetBlue(im,k,l);
        }
      if ( (l=j+m) >= 0 && l < im->w )
        for ( k = k1; k <= k2; ++k )
        {       
          red += GetRed(im,k,l); 
          green += GetGreen(im,k,l);
          blue += GetBlue(im,k,l);
        }
      if ( j >= 0 )
      {
        l = ( ( i+m+1 < im->h ? i+m+1 : im->h ) - ( i-m > 0 ? i-m : 0 ) )
          * ( ( j+m+1 < im->w ? j+m+1 : im->w ) - ( j-m > 0 ? j-m : 0 ) );
        out->rgb[i][j] = SetRGB(red/l,green/l,blue/l);
      }
    }
  }       
  return out;
}


void Sharpen ( struct image * im, int m, int f, int t )

{
  struct image  *av;
  int		i, j, k, k1, k2, l, l1, l2, rmin, rmax, gmin, gmax, bmin, bmax,
                red, green, blue;
  int ct = 0;

  av = Average ( im, m );
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
    {
      red = GetRed(im,i,j) - GetRed(av,i,j);
      green = GetGreen(im,i,j) - GetGreen(av,i,j);
      blue = GetBlue(im,i,j) - GetBlue(av,i,j);
/*      if ( abs(red) >= t || abs(green) >= t || abs(blue) >= t )
*/
      av->rgb[i][j] = im->rgb[i][j];
      if ( abs((int)(red+green+blue)) >= t )

      { 
        ++ct;
        red = GetRed(im,i,j) + f*red;
        green = GetGreen(im,i,j) + f*green;
        blue = GetBlue(im,i,j) + f*blue;
        k1 = i-m > 0 ? i-m : 0;
        k2 = i+m+1 < im->h ? i+m+1 : im->h;
        l1 = j-m > 0 ? j-m : 0;
        l2 = j+m+1 < im->w ? j+m+1 : im->w;
        rmin = gmin = bmin = 1023;
        rmax = gmax = bmax = 0;
        for ( k = k1; k < k2; ++k )
          for ( l = l1; l < l2; ++l )
          {
            if ( GetRed(im,k,l) < rmin )  rmin = GetRed(im,k,l);  
            if ( GetRed(im,k,l) > rmax )  rmax = GetRed(im,k,l);  
            if ( GetGreen(im,k,l) < gmin )  gmin = GetGreen(im,k,l);  
            if ( GetGreen(im,k,l) > gmax )  gmax = GetGreen(im,k,l);  
            if ( GetBlue(im,k,l) < bmin )  bmin = GetBlue(im,k,l);  
            if ( GetBlue(im,k,l) > bmax )  bmax = GetBlue(im,k,l);  
          }
        if ( red < rmin )  red = rmin;
        if ( red > rmax )  red = rmax;
        if ( green < gmin )  green = gmin;
        if ( green > gmax )  green = gmax;
        if ( blue < bmin )  blue = bmin;
        if ( blue > bmax )  blue = bmax;
        av->rgb[i][j] = SetRGB(red,green,blue);
      }
    }
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )  im->rgb[i][j] = av->rgb[i][j];
  DestroyImage ( av );
printf ( "%i\n", ct );
}


void Gamma ( struct image * im, double g )

{
  Lint	   i, j, k, red, green, blue, fac, list[3070];

  if ( g == 1. )  return;
  list[0] = 0;
  for ( i = 1; i < 3070; ++i ) 
    list[i] = 0X1000L * exp(log(i/3069.)*(1/g-1)) + 0.5;
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
    {
      red = GetRed(im,i,j);
      green = GetGreen(im,i,j);
      blue = GetBlue(im,i,j);
      fac = list[red+green+blue];
      red = ( fac*red + 0X7FF ) >> 12;
      if ( red > 1023 )  red = 1023;
      green = ( fac*green + 0X7FF ) >> 12;
      if ( green > 1023 )  green = 1023;
      blue = ( fac*blue + 0X7FF ) >> 12;
      if ( blue > 1023 )  blue = 1023;
      im->rgb[i][j] = SetRGB(red,green, blue);
    }
}


void Invert ( struct image * im )

{
  int  i, j;
  
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
      im->rgb[i][j] = ~im->rgb[i][j];
}


void IncreaseSaturation ( struct image * im, int chg )

{
  long int  i, j, red, green, blue, gray, min;
  
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
    {
      red = GetRed(im,i,j);
      green = GetGreen(im,i,j);
      blue = GetBlue(im,i,j);
      gray = red+green+blue;
      if ( blue < ( min = red<green ? red : green ) )  min = blue;
      if ( gray )
      {
      	min = (chg*(gray-3*min)*min)/gray;     
      	red += ((3*red-gray)*min)/(25*gray);
      	if ( red < 0 )  red = 0;
      	if ( red > 1023 )  red = 1023;
      	green += ((3*green-gray)*min)/(25*gray);
      	if ( green < 0 )  green = 0;
      	if ( green > 1023 )  green = 1023;
      	blue += ((3*blue-gray)*min)/(25*gray);
      	if ( blue < 0 )  blue = 0;
      	if ( blue > 1023 )  blue = 1023;
      	im->rgb[i][j] = SetRGB(red,green,blue);
      }
    }
}


void IncreaseSatCol ( struct image * im, int red, int green, int blue, int chg )

{
  long int  i, j, gray, min;
  int       h, d, refh, cchg[1025];
  
  for ( i = 0; i < 512; ++i )  cchg[1024-i] = cchg[i] = chg*exp(-2.7e-5*i*i);
  refh = green > blue ?
  	 163 * atan((0.57735*(2*red-green-blue))/(green-blue)) + 256:
	 green < blue ?
	 163 * atan((0.57735*(2*red-green-blue))/(green-blue)) + 768 :
	 2*red > green+blue ? 512 : 0; 
	 
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
    {
      red = GetRed(im,i,j);
      green = GetGreen(im,i,j);
      blue = GetBlue(im,i,j);
      h = green > blue ?
  	 163 * atan((0.57735*(2*red-green-blue))/(green-blue)) + 256:
	 green < blue ?
	 163 * atan((0.57735*(2*red-green-blue))/(green-blue)) + 768 :
	 2*red > green+blue ? 512 : 0; 
      d = abs((int)(h-refh));
      if ( i == 107 && j == 411 )
        printf ( "%i\t%i\t%i\n", h, refh, cchg[d] );
      gray = red+green+blue;
      if ( blue < ( min = red<green ? red : green ) )  min = blue;
      if ( gray )
      {
      	min = (cchg[d]*(gray-3*min)*min)/gray;     
      	red += ((3*red-gray)*min)/(25*gray);
      	if ( red < 0 )  red = 0;
      	if ( red > 1023 )  red = 1023;
      	green += ((3*green-gray)*min)/(25*gray);
      	if ( green < 0 )  green = 0;
      	if ( green > 1023 )  green = 1023;
      	blue += ((3*blue-gray)*min)/(25*gray);
      	if ( blue < 0 )  blue = 0;
      	if ( blue > 1023 )  blue = 1023;
      	im->rgb[i][j] = SetRGB(red,green,blue);
      }
    }
}


double Brightness ( struct image * im, double * sp )

{
  Lint    i, j, *wi, *wj, sum, anz[3070];
  double  av;
  
  wi = (Lint*) calloc ( im->h, sizeof(Lint) );
  for ( i = 0; i < im->h; ++i )
    wi[i] = (50*i*(im->h-1-i))/(im->h*im->h);
  wj = (Lint*) calloc ( im->w, sizeof(Lint) );
  for ( j = 0; j < im->w; ++j )  
    wj[j] = (50*j*(im->w-1-j))/(im->w*im->w);
  for ( i = 0; i < 3070; ++i )  anz[i] = 0;
  for ( i = 0; i < im->h; ++i )  
    for ( j = 0; j < im->w; ++j )
      anz[GetRed(im,i,j)+GetGreen(im,i,j)+GetBlue(im,i,j)] += wi[i]*wj[j];
  sum = 0;
  for ( i = 0; i < 3070; ++i )  sum += anz[i];
  av = 0.;
  if ( sp != NULL )
    for ( i = 0; i < 3070; ++i )
    {
      sp[i] = (double)anz[i]/sum;
      av += i*sp[i];
    }
  else
  {
    for ( i = 0; i < 3070; ++i )  av += i*anz[i];
    av /= sum;
  }
  free ( wj );
  free ( wi );
  return av/3069;
}


double Spectrum ( struct image * im, double * sp )

{
  Lint    i, j, max, red, green, blue, *wi, *wj, sum, anz[1024];
  double  av;
  
  wi = (Lint*) calloc ( im->h, sizeof(Lint) );
  for ( i = 0; i < im->h; ++i )
    wi[i] = (50*i*(im->h-1-i))/(im->h*im->h);
  wj = (Lint*) calloc ( im->w, sizeof(Lint) );
  for ( j = 0; j < im->w; ++j )  
    wj[j] = (50*j*(im->w-1-j))/(im->w*im->w);
  for ( i = 0; i < 1024; ++i )  anz[i] = 0;
  for ( i = 0; i < im->h; ++i )  
    for ( j = 0; j < im->w; ++j )
    {
      red = GetRed(im,i,j);
      green = GetGreen(im,i,j);
      blue = GetBlue(im,i,j);
      max = red > green ? red : green;
      if ( blue > max )  max = blue;
      anz[max] += wi[i]*wj[j];
    }
  sum = 0;
  for ( i = 0; i < 1024; ++i )  sum += anz[i];
  av = 0.;
  if ( sp != NULL )
    for ( i = 0; i < 1024; ++i )
    {
      sp[i] = (double)anz[i]/sum;
      av += i*sp[i];
    }
  else
  {
    for ( i = 0; i < 1024; ++i )  av += i*anz[i];
    av /= sum;
  }
  free ( wj );
  free ( wi );
  return av/1023;
}


void OptimizeContrast ( struct image * im )

{
  int	  i, j, min, max, red, green, blue, m, f[1024];
  double  sp[1024], sum, av, av2, g;

  Spectrum ( im, sp );
  sum = 0.;
  for ( min = 0; (sum+=sp[min]) < 1.e-4; ++min );
  sum = 0.;
  for ( max = 1023; (sum+=sp[max]) < 1.e-4; --max );
  av = av2 = 0.;
  for ( i = 0; i < 1024; ++i )
  {
    av += i*sp[i];  
    av2 += i*i*sp[i];
  }
  g = (double)((int)av*(max-min)-1023*((int)av-min))
                 / ((int)av2-(int)av*(min+max)+min*max);
  if ( g*(max-min) > 1023 )  g = 1023./(max-min);
  if ( g*(max-min) < -1023 )  g = -1023./(max-min);
  for ( i = 0; i <= min; ++i )  f[i] = 0;
  for ( i = min+1; i < max; ++i )
    f[i] = 1023*(i-min) + g*(i*i-i*(min+max)+min*max);
  for ( i = max; i < 1024; ++i )  f[i] = 1023*(i-min);
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
    {
      red = GetRed(im,i,j);
      green = GetGreen(im,i,j);
      blue = GetBlue(im,i,j);
      m = red > green ? red : green;
      if ( blue > m )  m = blue;
      if ( m )
      {
        red = m > min ? (f[m]*red)/(m*(max-min)) : 0;
        if ( red > 1023 )  red = 1023;
        green = m > min ? (f[m]*green)/(m*(max-min)) : 0;
        if ( green > 1023 )  green = 1023;
        blue = m > min ? (f[m]*blue)/(m*(max-min)) : 0;
	if ( blue > 1023 )  blue = 1023;
        im->rgb[i][j] = SetRGB(red,green,blue);
      }  	
    }
}


void Brighten ( struct image * im, int chg )

{
  long int  i, j, f, red, green, blue, gray;
  
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
    {
      red = GetRed(im,i,j);
      green = GetGreen(im,i,j);
      blue = GetBlue(im,i,j);
      gray = red+green+blue;
      f = (chg*gray*(3069-gray))/3069;
      red += (red*f)/76725;
      if ( red < 0 )  red = 0;
      if ( red > 1023 )  red = 1023;
      green += (green*f)/76725;
      if ( green < 0 )  green = 0;
      if ( green > 1023 )  green = 1023;
      blue += (blue*f)/76725;
      if ( blue < 0 )  blue = 0;
      if ( blue > 1023 )  blue = 1023;
      im->rgb[i][j] = SetRGB(red,green,blue);
    }
}

int computeMedian ( int l, int *a )
{
  int  i, m;

  do
  {
    m = -1;	
    for ( i = 1; i < l; ++i )
      if ( getR(a[i])+getG(a[i])+getB(a[i]) 
           < getR(a[i-1])+getG(a[i-1])+getB(a[i-1]) )
      { m = a[i]; a[i] = a[i-1]; a[i-1] = m; }
  }
  while ( m >= 0 );
  return a[l/2];
}    

struct image *applyMedianFilter ( struct image *im, int s )
{
  int           i, j, k, l, r, n, *a;	
  struct image  *out; 

  out = AllocImage ( im->h, im->w );
  a = (int*) calloc ( s*s, sizeof(int) );
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
    {
      r = s/2;
      if ( r > i )  r = i;
      if ( r > j )  r = j;
      if ( r >= im->h-i )  r = im->h-i-1;
      if ( r >= im->w-j )  r = im->w-j-1;
      n = 0;
      for ( k = i-r; k <= i+r; ++k )
        for ( l = j-r; l <= j+r; ++l )  a[n++] = im->rgb[k][l];
     out->rgb[i][j] = computeMedian ( n, a );
    }
  return out;
}

int Median ( int l, int *a )
{
  int  i, m = 0;

  for ( i = -l; i <= l; ++i )  m += i*a[i];
  if ( m > 0 )
    do
    {
      m = -1;	
      for ( i = -l; i < l; ++i )
        if ( a[i] > a[i+1] )  { m = a[i]; a[i] = a[i+1]; a[i+1] = m; }
    }
    while ( m >= 0 );
  else
    do
    {
      m = -1;	
      for ( i = -l; i < l; ++i )
        if ( a[i] < a[i+1] )  { m = a[i]; a[i] = a[i+1]; a[i+1] = m; }
    }
    while ( m >= 0 );
  return *a;
}


Lint MedianFilterPoint ( struct image *im, int l, int i, int j,
         int *r, int *g, int *b )
{
  int   k, m, n, di, dj, rm, gm, bm, red = 1023, green = 1023, blue = 1023;	
  
  for ( di = 0; di <= 1; ++di )
    for ( dj = di ? -1 : 1; dj <= 1; ++dj )
    {
      for ( k = -l; k <= l; ++k )
      {
        m = i+k*di;
        if ( m < 0 )  m = 0;
        if ( m >= im->h )  m = im->h-1;
        n = j+k*dj;
        if ( n < 0 )  n = 0;
        if ( n >= im->w )  n = im->w-1;
        r[k] = GetRed(im,m,n);
        g[k] = GetGreen(im,m,n);
        b[k] = GetBlue(im,m,n);
      }  
      if ( ( rm = Median ( l, r ) ) < GetRed(im,i,j) - 20 
	|| ( gm = Median ( l, g ) ) < GetGreen(im,i,j) - 20 
	|| ( bm = Median ( l, b ) ) < GetBlue(im,i,j) - 20
	|| rm+bm+gm <= GetRed(im,i,j)+GetGreen(im,i,j)+GetBlue(im,i,j) )
	  return im->rgb[i][j];
      if ( rm < red )  red = rm;
      if ( gm < green )  green = gm;
      if ( bm < blue )  blue = bm;
    }      
  return SetRGB(red,green,blue);
}

  
void Filter ( struct image *im )

{
  int   i, j, k, l, imin, imax, jmin, jmax, lv, *r, *g, *b;	
  Lint  gr;
  
  r = (int*) calloc ( 21, sizeof(int) ) + 10;
  g = (int*) calloc ( 21, sizeof(int) ) + 10;
  b = (int*) calloc ( 21, sizeof(int) ) + 10;

  for ( k = 0; k < 32; ++k )
    for ( l = 0; l < 32; ++l )
    {
      imin = (k*im->h)/32;
      imax = ((k+1)*im->h)/32;
      jmin = (l*im->w)/32;
      jmax = ((l+1)*im->w)/32;
      gr = 0;
      for ( i = imin+1; i < imax; ++i )   
      {
        for ( j = jmin+1; j < jmax; ++j )
          gr += ((signed)GetRed(im,i,j)-(signed)GetRed(im,i,j-1))
			*((signed)GetRed(im,i,j)-(signed)GetRed(im,i,j-1)) 
              + ((signed)GetRed(im,i,j)-(signed)GetRed(im,i-1,j))
			*((signed)GetRed(im,i,j)-(signed)GetRed(im,i-1,j)) 
              + ((signed)GetGreen(im,i,j)-(signed)GetGreen(im,i,j-1))
			*((signed)GetGreen(im,i,j)-(signed)GetGreen(im,i,j-1)) 
              + ((signed)GetGreen(im,i,j)-(signed)GetGreen(im,i-1,j))
			*((signed)GetGreen(im,i,j)-(signed)GetGreen(im,i-1,j)) 
              + ((signed)GetBlue(im,i,j)-(signed)GetBlue(im,i,j-1))
			*((signed)GetBlue(im,i,j)-(signed)GetBlue(im,i,j-1)) 
              + ((signed)GetBlue(im,i,j)-(signed)GetBlue(im,i-1,j))
			*((signed)GetBlue(im,i,j)-(signed)GetBlue(im,i-1,j));
      }
      lv = log(im->h*im->w/(double)gr)+5;
      if ( lv > 0 )
      {
        if ( lv > 10 )  lv = 10;   
        for ( i = imin; i < imax; ++i )   
          for ( j = jmin; j < jmax; ++j )
            im->rgb[i][j] = MedianFilterPoint ( im, lv, i, j, r, g, b );
      }
    }
  free ( b-10 );
  free ( g-10 );
  free ( r-10 );
}


void Merge ( struct image *a, struct image *b,
             int ia, int ib, int den, struct image *out )
{
  int	        i, j, f, red, green, blue;
  struct image  *tmp;

  if ( ia > den )  ia = den;
  if ( ib > den )  ib = den;
  if ( ia < ib )
  {
    tmp = a;
    a = b;
    b = tmp;
    i = ia;
    ia = ib;
    ib = i;
  }
  if ( ia < 0 )  Exit ( "Merge" );
  if ( ia )
  {
    if ( ib )
    {
      if ( a->h-b->h || a->w-b->w || a->h-out->h || a->w-out->w )
	Exit ( "Merge" );
      if ( ib > 0 )
      {
        if ( ia+ib > den )
        {
          ia -= (ia+ib-den)/2;
          ib = den-ia;
        }
        for ( i = 0; i < out->h; ++i )
          for ( j = 0; j < out->w; ++j )
	    out->rgb[i][j] = SetRGB((ia*GetRed(a,i,j)+ib*GetRed(b,i,j))/den,(ia*GetGreen(a,i,j)+ib*GetGreen(b,i,j))/den,(ia*GetBlue(a,i,j)+ib*GetBlue(b,i,j))/den);    
      }
      else
      {
        if ( ib < -1023 )  ib = -1023;
        for ( i = 0; i < out->h; ++i )
          for ( j = 0; j < out->w; ++j )
          {
            red = GetRed(b,i,j);
            green = GetGreen(b,i,j);
            blue = GetBlue(b,i,j);
            f = red > green ? red > blue ? red : blue
                              : green > blue ? green : blue;
            if ( f < 4 )  f = 0;
            f = (ia*(f+ib))/ib;
            if ( f < 0 )  f = 0;
            red += (f*GetRed(a,i,j))/den;
            if ( red < 0 )  red = 0;
            if ( red > 1023 )  red = 1023;
            green += (f*GetGreen(a,i,j))/den;
            if ( green < 0 )  green = 0;
            if ( green > 1023 )  green = 1023;
            blue += (f*GetBlue(a,i,j))/den;
            if ( blue < 0 )  blue = 0;
            if ( blue > 1023 )  blue = 1023;   
/*
            f = (1023-f)/4; 
	    red = ((double)ia*f*GetRed(a,i,j))/(255*den);
	    green = ((double)ia*f*GetGreen(a,i,j))/(255*den);
	    blue = ((double)ia*f*GetBlue(a,i,j))/(255*den);
if ( i == 0 && j == 0 )  printf ( "%i\n", f );
*/
            out->rgb[i][j] = SetRGB(red,green,blue);

         }
      }
    }
    else
    {
      if ( a->h-out->h || a->w-out->w )  Exit ( "Merge" );
      for ( i = 0; i < out->h; ++i )
        for ( j = 0; j < out->w; ++j )
	  out->rgb[i][j] = SetRGB((ia*GetRed(a,i,j))/den,(ia*GetGreen(a,i,j))/den,(ia*GetBlue(a,i,j))/den);    
    }
  }
  else
  {
    if ( ib )
    {
      if ( b->h-out->h || b->w-out->w )  Exit ( "Merge" );
      for ( i = 0; i < out->h; ++i )
        for ( j = 0; j < out->w; ++j )
	  out->rgb[i][j] = b->rgb[i][j];    
    }
    else
      for ( i = 0; i < out->h; ++i )
        for ( j = 0; j < out->w; ++j )  out->rgb[i][j] = 0;
  }
}

    
Lint ContrastColor ( struct image * im )

{
  int	  i, j, r, g, b, col;
  double  dist, max;

  max = 0.;
  for ( r = 0; r < 1024; r += 341 )
    for ( g = 0; g < 1024; g += 341 )
      for ( b = 0; b < 1024; b += 341 )
      {
        dist = 0.;
        for ( i = 0; i < im->h; ++i )
          for ( j = 0; j < im->w; ++j )
            dist += abs((int)(r-GetRed(im,i,j))) + abs((int)(g-GetGreen(im,i,j)))
                  + abs((int)(b-GetBlue(im,i,j)));
        if ( dist > max )
        {
          max = dist;
          col = SetRGB(r,g,b);
        }
      }
  return col;
}  


void Roll ( struct image *im, int border, int pos )

{
  int		i, j, left, right, x, y, d, g; 
  double	r, p, q, x3d, y3d, z3d, s, n1[3], n2[3];

  r = 0.05*im->h*sqrt((im->h-0.75*pos)/im->h);
  d = im->h;
  left = border;
  right = im->w-border-1;
  for ( i = 0; i < im->h; ++i )
  {
    for ( j = 0; j < left; ++j )  im->rgb[i][j] = 0;
    for ( j = right+1; j < im->w; ++j )  im->rgb[i][j] = 0;
  }
  for ( i = pos+1 > 0 ? pos+1 : 0; i < im->h; ++i )
    for ( j = left; j <= right; ++j )  im->rgb[i][j] = 0;
  if ( pos+r < 0 || pos-r > im->h || r <= 0. )  return;
  pos -= im->h/2;
  for ( i = 0; i < im->h; ++i )
  {
    y = i-im->h/2;
    p = (y*(double)(d*(d+r)+pos*y))/((d+r)*(d+r)+y*y);
    q = y*y*(r*r-d*d-pos*pos)/((d+r)*(d+r)+y*y);
    if ( p*p+q >= 0. )
    {
      y3d = p > 0 ? p-sqrt(p*p+q) : p+sqrt(p*p+q);
      z3d =  r*r-(y3d-pos)*(y3d-pos);
      if ( z3d < 0. )  continue;
      z3d = fabs(y3d)*d <= abs(y)*(d-r) ? r+sqrt(z3d) : r-sqrt(z3d); 
      n1[0] = 0;
      n1[1] = y3d-pos;
      n1[2] = z3d-r;
      s = sqrt(n1[1]*n1[1]+n1[2]*n1[2]);
      n1[1] /= s;
      n1[2] /= s;
      for ( j = 0; j < im->w; ++j )
      { 
        x = j-im->w/2;
        x3d = x*(d-z3d)/d;
        if ( fabs(x3d) > im->w/2-border )  continue;
        n2[0] = -x3d;
        n2[1] = -y3d;
        n2[2] = d-r-z3d;
        s = sqrt(n2[0]*n2[0]+n2[1]*n2[1]+n2[2]*n2[2]);
	if ( s <= 0. )  continue;
        s = (n1[1]*n2[1]+n1[2]*n2[2])/s;
        g = 1023*s*s;
        if ( g < 16 )  g = 16;
        im->rgb[i][j] = SetRGB(g,g,g);
      }
    }
  }
}


struct image * MakeRunText ( char *filename )

{
  struct image *im, *res; 

  im = ReadPS ( filename, 0, 1023, 1023, 1023, 300, 0.01 );
  res = Resize ( im, (562.5*im->h)/im->w, 600 );
  DestroyImage ( im );
  Invert ( res );
  im = Enlarge ( res, res->h+1152, 720, 0 );
  DestroyImage ( res );
  return im;   
}

Lint scaleGray ( Lint u, int f )
/* f = 4096 erhaelt den Grauwert */ 
{
  Lint  red, green, blue;  

  if ( f < 0 )  f = 0;
  red = ((u>>20&1023)*f)>>12;
  if ( red > 1023 )
  {
    f = (f*1023)/red;
    red = 1023;
  }
  green = ((u>>10&1023)*f)>>12;
  if ( green > 1023 )
  {
    f = (f*1023)/green;
    red = ((u>>20&1023)*f)>>12;
    green = 1023;
  }
  blue = ((u&1023)*f)>>12;
  if ( blue > 1023 )
  {
    f = (f*1023)/blue;
    red = ((u>>20&1023)*f)>>12;
    green = ((u>>10&1023)*f)>>12;
    blue = 1023;
  }
  return  SetRGB ( red, green, blue ); 
}     

void gradualFilter ( struct image *im, double mid, double width, double f )
{
  int     i, j, a, *g;  
  double  integ1, integ2, *ata;

  mid = 1-mid;
  g = (int*) calloc ( im->h, sizeof(int) );
  ata = (double*) calloc ( im->h, sizeof(double) );
  integ1 = integ2 = 0.;
  for ( i = 0; i < im->h; ++i )
  {
    g[i] = 0;
    for ( j = 0; j < im->w; ++j )  g[i] += GetGray(im,i,j);
    g[i] /= im->w;
/* printf ( "%i\t%i\n", i, g[i] ); */
    ata[i] = atan(2*((double)i/im->h-mid)/width);
    integ1 += g[i]*g[i]*ata[i];
    integ2 += g[i]*g[i]*ata[i]*ata[i];
  }
  for ( i = 0; i < im->h; ++i )
  {
    a = (1-f*integ1/integ2*ata[i])*4096;
    for ( j = 0; j < im->w; ++j )
      im->rgb[i][j] = scaleGray ( im->rgb[i][j], a );
  }
  free ( ata );
  free ( g );
}

void gradualSky ( struct image *im, double mid, double width, double f )
{
  int     i, j, r, g, dr, dg;  
  double  a;

  width *= 0.36;
/* von 80 auf 20 Prozent */
  for ( i = 0; i < im->h; ++i )
  {
    a = f*(0.5-atan(((double)i/im->h-mid)/width)/3.141592);
    for ( j = 0; j < im->w; ++j )
    {
      dr = GetBlue(im,i,j) - GetRed(im,i,j);
      if ( dr <= 0 )  continue;
      dg = GetBlue(im,i,j) - GetGreen(im,i,j);
      if ( dg <= 0 )  continue;
      r = GetRed(im,i,j) - a*dr;
      if ( r < 0 )  r = 0;
      g = GetGreen(im,i,j) - a*dg;
      if ( g < 0 )  g = 0;
      im->rgb[i][j] = SetRGB ( r, g, GetBlue(im,i,j) );
    }
  }
}

int computeDifference ( Lint u, Lint v )
{
  int  dif, tmp;
 
  dif = abs((int)(getR(u)-getR(v)));
  tmp = abs((int)(getG(u)-getG(v)));
  if ( tmp > dif )  dif = tmp;
  tmp = abs((int)(getB(u)-getB(v)));
  if ( tmp > dif )  dif = tmp;
  return dif;
}

struct image *smoothImage ( struct image *im, int t )
{
/* Idee: t = 4sqrt(iso) */

  int           i, j, k, l, kmin, kmax, lmin, lmax, ref, r, s, d, f, g, b, ct,
                gr[3][3];
  struct image  *lev, *out;

  out = AllocImage ( im->h, im->w );
  lev = AllocImage ( im->h, im->w );
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
    {
      for ( k = 0; k < 3; ++k )
      {
        r = i+k-1;
        if ( r < 0 )  r = 0;
        if ( r > im->h-1 )  r = im->h-1;
        for ( l = 0; l < 3; ++l )
        {
          s = j+l-1;
          if ( s < 0 )  s = 0;
          if ( s > im->w-1 )  s = im->w-1;
          gr[k][l] = getR(im->rgb[r][s]) + getG(im->rgb[r][s])
                                         + getB(im->rgb[r][s]);
        }
      }
      d = abs((int)(2*(gr[2][1]-gr[1][1])+gr[2][0]-gr[1][0]+gr[2][2]-gr[1][2]));
      f = abs((int)(2*(gr[0][1]-gr[1][1])+gr[0][0]-gr[1][0]+gr[0][2]-gr[1][2]));
      if ( f > d )  d = f;
      f = abs((int)(2*(gr[1][2]-gr[1][1])+gr[2][2]-gr[2][1]+gr[0][2]-gr[0][1]));
      if ( f > d )  d = f;
      f = abs((int)(2*(gr[1][0]-gr[1][1])+gr[2][0]-gr[2][1]+gr[2][2]-gr[0][1]));
      if ( f > d )  d = f;
      lev->rgb[i][j] = d > 12*t ? 0 : 1;
/*
      ref = im->rgb[i][j];
      kmin = i ? i-1 : 0;
      kmax = im->h-i-1 ? i+2 : im->h; 
      lmin = j ? j-1 : 0;
      lmax = im->w-j-1 ? j+2 : im->w; 
      for ( k = kmin; k < kmax; ++k )
      {
        for ( l = lmin; l < lmax; ++l )
          if ( ( d = computeDifference ( im->rgb[k][l], ref ) ) > t )  break;
        if ( d > t )  break;
      }
      lev->rgb[i][j] = d > t ? 0 : 1;
*/
lev->rgb[i][j] *= 1023;
    }


WritePPM ( lev, "test.ppm" );
exit ( 0 );
  do
  {
    f = 0;
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )
        if ( lev->rgb[i][j] && lev->rgb[i][j] <= 5 )
        {
          kmin = i ? i-1 : 0;
          kmax = im->h-i-1 ? i+2 : im->h; 
          lmin = j ? j-1 : 0;
          lmax = im->w-j-1 ? j+2 : im->w; 
          g = 1;
          for ( k = kmin; k < kmax; ++k )
            for ( l = lmin; l < lmax; ++l )
              if ( lev->rgb[k][l] < lev->rgb[i][j] )  g = 0;
          if ( g )
          {
            ++lev->rgb[i][j];
            ++f;
          }
        }
  }
  while ( f );
  for ( f = ct = 1; ct ; ++f )  
  {
    ct = 0;
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )
        if ( lev->rgb[i][j] >= f )
        {
          ++ct;
          kmin = i ? i-1 : 0;
          kmax = im->h-i-1 ? i+2 : im->h; 
          lmin = j ? j-1 : 0;
          lmax = im->w-j-1 ? j+2 : im->w; 
          r = g = b = 0;
          for ( k = kmin; k < kmax; ++k )
            for ( l = lmin; l < lmax; ++l )
            {
              r += getR(im->rgb[k][l]);
              g += getG(im->rgb[k][l]);
              b += getB(im->rgb[k][l]);
            }
          d = (kmax-kmin)*(lmax-lmin);
          out->rgb[i][j] = SetRGB ( r/d, g/d, b/d );
        }
        else  out->rgb[i][j] = im->rgb[i][j];
    Paste ( im, out, 0, 0 ); 
  }
  DestroyImage ( lev );
  return out;
}

int computeMaximumOverFrame ( struct image *im, int i, int j, int r )
{
  int  k, l, max, tmp;

  max = 0;
  for ( k = i-r; k <= i+r; k += 2*r )
    for ( l = j-r; l <= j+r; ++l )
      if ( ( tmp = getB(im->rgb[k][l]) ) > max )  max = tmp;   
  for ( l = j-r; l <= j+r; l += 2*r )
    for ( k = i-r+1; k < i+r; ++k )
      if ( ( tmp = getB(im->rgb[k][l]) ) > max )  max = tmp;   
  return max;
}

void removeMaximums ( struct image *im )
{
  int  i, j, k, max, m, tmp;

  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
      im->rgb[i][j] = SetRGB(0,im->rgb[i][j],im->rgb[i][j]);
  for ( i = 2; i < im->h-2; ++i )
    for ( j = 2; j < im->w-2; ++j )
      if ( computeMaximumOverFrame ( im, i, j, 1 ) <= getG(im->rgb[i][j]) )   
      {
        if ( ( m = computeMaximumOverFrame ( im, i, j, 2 ) )
                                           < getB(im->rgb[i-1][j-1]) ) 
        {
          max = getB(im->rgb[i-2][j-2]);
          for ( k = 1; k <= 2; ++k )
          {
            if ( ( tmp = getB(im->rgb[i-2+k][j-2]) ) > max )  max = tmp;
            if ( ( tmp = getB(im->rgb[i-2][j-2+k]) ) > max )  max = tmp;
          }
          im->rgb[i-1][j-1] = SetRGB(0,max,getB(im->rgb[i-1][j-1]));
        }
        if ( m < getG(im->rgb[i-1][j+1]) ) 
        {
          max = getB(im->rgb[i-2][j+2]);
          for ( k = 1; k <= 2; ++k )
          {
            if ( ( tmp = getB(im->rgb[i-2+k][j+2]) ) > max )  max = tmp;
            if ( ( tmp = getB(im->rgb[i-2][j+2-k]) ) > max )  max = tmp;
          }
          im->rgb[i-1][j+1] = SetRGB(0,max,getB(im->rgb[i-1][j+1]));
        }
        if ( m < getG(im->rgb[i+1][j-1]) ) 
        {
          max = getB(im->rgb[i+2][j-2]);
          for ( k = 1; k <= 2; ++k )
          {
            if ( ( tmp = getB(im->rgb[i+2-k][j-2]) ) > max )  max = tmp;
            if ( ( tmp = getB(im->rgb[i+2][j-2+k]) ) > max )  max = tmp;
          }
          im->rgb[i+1][j-1] = SetRGB(0,max,getB(im->rgb[i+1][j-1]));
        }
        if ( m < getG(im->rgb[i+1][j+1]) ) 
        {
          max = getB(im->rgb[i+2][j+2]);
          for ( k = 1; k <= 2; ++k )
          {
            if ( ( tmp = getB(im->rgb[i+2-k][j+2]) ) > max )  max = tmp;
            if ( ( tmp = getB(im->rgb[i+2][j+2-k]) ) > max )  max = tmp;
          }
          im->rgb[i+1][j+1] = SetRGB(0,max,getB(im->rgb[i+1][j+1]));
        }
        if ( m < getG(im->rgb[i-1][j]) ) 
        {
          max = 0;;
          for ( k = -1; k <= 1; ++k )
            if ( ( tmp = getB(im->rgb[i-2][j+k]) ) > max )  max = tmp;
          im->rgb[i-1][j] = SetRGB(0,max,getB(im->rgb[i-1][j]));
        }
        if ( m < getG(im->rgb[i+1][j]) ) 
        {
          max = 0;;
          for ( k = -1; k <= 1; ++k )
            if ( ( tmp = getB(im->rgb[i+2][j+k]) ) > max )  max = tmp;
          im->rgb[i+1][j] = SetRGB(0,max,getB(im->rgb[i+1][j]));
        }
        if ( m < getG(im->rgb[i][j-1]) ) 
        {
          max = 0;;
          for ( k = -1; k <= 1; ++k )
            if ( ( tmp = getB(im->rgb[i+k][j-2]) ) > max )  max = tmp;
          im->rgb[i][j-1] = SetRGB(0,max,getB(im->rgb[i][j-1]));
        }
        if ( m < getG(im->rgb[i][j+1]) ) 
        {
          max = 0;;
          for ( k = -1; k <= 1; ++k )
            if ( ( tmp = getB(im->rgb[i+k][j+2]) ) > max )  max = tmp;
          im->rgb[i][j+1] = SetRGB(0,max,getB(im->rgb[i][j+1]));
        }
      }
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j ) 
      im->rgb[i][j] = SetRGB(0,getG(im->rgb[i][j]),getG(im->rgb[i][j]));
  for ( i = 1; i < im->h-1; ++i )
    for ( j = 1; j < im->w-1; ++j )
      if ( ( m = computeMaximumOverFrame ( im, i, j, 1 ) )
                                          < getG(im->rgb[i][j]) )   
        im->rgb[i][j] = SetRGB(0,m,getB(im->rgb[i][j]));
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j ) 
      im->rgb[i][j] = SetRGB(0,0,getG(im->rgb[i][j]));
}

void removeExtremes ( struct image *im )
{ 
  int           i, j, k;
  struct image  *buf;

  buf = AllocImage ( im->h, im->w );
  for ( k = 0; k < 2; ++k )
  {
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )  buf->rgb[i][j] = getR(im->rgb[i][j]);
    removeMaximums ( buf );    
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )
        im->rgb[i][j] = SetRGB(buf->rgb[i][j],
                               getG(im->rgb[i][j]),getB(im->rgb[i][j]));
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )  buf->rgb[i][j] = getG(im->rgb[i][j]);
    removeMaximums ( buf );    
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )
        im->rgb[i][j] = SetRGB(getR(im->rgb[i][j]),
                               buf->rgb[i][j],getB(im->rgb[i][j]));
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )  buf->rgb[i][j] = getB(im->rgb[i][j]);
    removeMaximums ( buf );    
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )
        im->rgb[i][j] = SetRGB(getR(im->rgb[i][j]),
                               getG(im->rgb[i][j]),buf->rgb[i][j]);
    Invert ( im );
  }
  DestroyImage ( buf );
/*
  WritePPM ( im, "test.ppm" );
  exit ( 0 );
*/
}




/*
void removeMaximums ( struct image *im )
{
  int  i, j, k, max;

  for ( i = 0; i < im->h-3; ++i )
    for ( j = 0; j < im->w; ++j )
    {
      max = im->rgb[i][j];
      for ( k = 1; k < 4; ++k )
        if ( im->rgb[i+k][j] > max )  max = im->rgb[i+k][j];
      im->rgb[i][j] = SetRGB(max,0,im->rgb[i][j]);
    }
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w-3; ++j )
    {
      max = getB(im->rgb[i][j]);
      for ( k = 1; k < 4; ++k )
        if ( getB(im->rgb[i][j+k]) > max )  max = getB(im->rgb[i][j+k]);
      im->rgb[i][j] = SetRGB(getR(im->rgb[i][j]),max,getB(im->rgb[i][j]));
    }
  for ( i = 0; i < im->h-3; ++i )
    for ( j = 0; j < im->w-3; ++j )
    {
      max = getR(im->rgb[i][j]);
      if ( getR(im->rgb[i][j+3]) > max )  max = getR(im->rgb[i][j+3]);
      if ( getG(im->rgb[i][j]) > max )  max = getG(im->rgb[i][j]);
      if ( getG(im->rgb[i+3][j]) > max )  max = getG(im->rgb[i+3][j]);
      im->rgb[i][j] = SetRGB(max,0,getB(im->rgb[i][j]));
    }
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j ) 
      {
        im->rgb[i][j] = SetRGB(getR(im->rgb[i][j]),
                               getB(im->rgb[i][j]),getB(im->rgb[i][j]));
        if ( i && i < im->h-3 && j && j < im->w-3 
             && getR(im->rgb[i-1][j-1]) < getG(im->rgb[i][j]) )
        {
          max = getB(im->rgb[i-1][j-1]);
          for ( k = 1; k <= 2; ++k )
          {
            if ( getB(im->rgb[i-1+k][j-1]) > max )
              max = getB(im->rgb[i-1+k][j-1]);
            if ( getB(im->rgb[i-1][j-1+k]) > max )
              max = getB(im->rgb[i-1][j-1+k]);
          }
          im->rgb[i][j] = SetRGB( getR(im->rgb[i][j]),
                                  max, getB(im->rgb[i][j]) );
        }
        if ( i > 1 && i < im->h-2 && j && j < im->w-3 
             && getR(im->rgb[i-2][j-1]) < getG(im->rgb[i][j]) )
        {
          max = getB(im->rgb[i+1][j-1]);
          for ( k = 1; k <= 2; ++k )
          {
            if ( getB(im->rgb[i+1-k][j-1]) > max )
              max = getB(im->rgb[i+1-k][j-1]);
            if ( getB(im->rgb[i+1][j-1+k]) > max )
              max = getB(im->rgb[i+1][j-1+k]);
          }
          im->rgb[i][j] = SetRGB( getR(im->rgb[i][j]),
                                  max, getB(im->rgb[i][j]) );
        }
        if ( i && i < im->h-3 && j > 1 && j < im->w-2 
             && getR(im->rgb[i-1][j-2]) < getG(im->rgb[i][j]) )
        {
          max = getB(im->rgb[i-1][j+1]);
          for ( k = 1; k <= 2; ++k )
          {
            if ( getB(im->rgb[i-1+k][j+1]) > max )
              max = getB(im->rgb[i-1+k][j+1]);
            if ( getB(im->rgb[i+1][j-1+k]) > max )
              max = getB(im->rgb[i+1][j-1+k]);
          }
          im->rgb[i][j] = SetRGB( getR(im->rgb[i][j]),
                                  max, getB(im->rgb[i][j]) );
        }
        if ( i > 1 && i < im->h-2 && j >1 && j < im->w-2 
             && getR(im->rgb[i-2][j-2]) < getG(im->rgb[i][j]) )
        {
          max = getB(im->rgb[i+1][j+1]);
          for ( k = 1; k <= 2; ++k )
          {
            if ( getB(im->rgb[i+1-k][j+1]) > max )
              max = getB(im->rgb[i+1-k][j+1]);
            if ( getB(im->rgb[i+1][j+1-k]) > max )
              max = getB(im->rgb[i+1][j+1-k]);
          }
          im->rgb[i][j] = SetRGB( getR(im->rgb[i][j]),
                                  max, getB(im->rgb[i][j]) );
        }
      }
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j ) 
        im->rgb[i][j] = SetRGB(0,0,getG(im->rgb[i][j]));
}

void removeExtremes ( struct image *im )
{ 
  int           i, j, k;
  struct image  *buf;

  buf = AllocImage ( im->h, im->w );
  for ( k = 0; k < 2; ++k )
  {
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )  buf->rgb[i][j] = getR(im->rgb[i][j]);
    removeMaximums ( buf );    
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )
        im->rgb[i][j] = SetRGB(buf->rgb[i][j],
                               getG(im->rgb[i][j]),getB(im->rgb[i][j]));
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )  buf->rgb[i][j] = getG(im->rgb[i][j]);
    removeMaximums ( buf );    
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )
        im->rgb[i][j] = SetRGB(getR(im->rgb[i][j]),
                               buf->rgb[i][j],getB(im->rgb[i][j]));
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )  buf->rgb[i][j] = getB(im->rgb[i][j]);
    removeMaximums ( buf );    
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )
        im->rgb[i][j] = SetRGB(getR(im->rgb[i][j]),
                               getG(im->rgb[i][j]),buf->rgb[i][j]);
    Invert ( im );
  }

  WritePPM ( im, "test.ppm" );
  exit ( 0 );

}
xxx
*/



struct image *reduceNoise ( struct image *im, int t )
{
/* Idee: t = 6sqrt(iso) */

  int           i, j, k, l, c, kmin, kmax, lmin, lmax, ref, r, d, f, g, b, ct;
  struct image  *lev, *out, *res;

  t *= 12;
  res = AllocImage ( im->h/2, im->w/2 );
  for ( i = 0; i < im->h; ++i )
    for ( j = 0; j < im->w; ++j )
      res->rgb[i/2][j/2] += getR(im->rgb[i][j]) + getG(im->rgb[i][j])
                                                + getB(im->rgb[i][j]);
  lev = AllocImage ( res->h, res->w );
  for ( i = 0; i < res->h; ++i )
    for ( j = 0; j < res->w; ++j )
    {
      kmin = i ? i-1 : 0;
      kmax = res->h-i-1 ? i+2 : res->h; 
      lmin = j ? j-1 : 0;
      lmax = res->w-j-1 ? j+2 : res->w; 
      for ( k = kmin; k < kmax; ++k )
      {
        for ( l = lmin; l < lmax; ++l )
          if ( ( d = abs ( (signed int)(res->rgb[k][l]-res->rgb[i][j]) ) ) > t )  break;
        if ( d > t )  break;
      }
      lev->rgb[i][j] = d > t ? 0 : 1;
    }
  DestroyImage ( res ); 
  for ( i = 0; i < lev->h; ++i )
    for ( j = 0; j < lev->w; ++j )
      if ( !lev->rgb[i][j] )
      {
        kmin = i ? i-1 : 0;
        kmax = lev->h-i-1 ? i+2 : lev->h; 
        lmin = j ? j-1 : 0;
        lmax = lev->w-j-1 ? j+2 : lev->w; 
        c = 0;
        for ( k = kmin; k < kmax; ++k )
          for ( l = lmin; l < lmax; ++l )  if ( lev->rgb[k][l] != 1 )  ++c;
        if ( c < 3 )  lev->rgb[i][j] = 2;
      }
  for ( i = 0; i < lev->h; ++i )
    for ( j = 0; j < lev->w; ++j )
      if ( lev->rgb[i][j] == 2 )  lev->rgb[i][j] = 1;
  do
  {
    f = 0;
    for ( i = 0; i < lev->h; ++i )
      for ( j = 0; j < lev->w; ++j )
        if ( lev->rgb[i][j] && lev->rgb[i][j] <= 2 )
        {
          kmin = i ? i-1 : 0;
          kmax = lev->h-i-1 ? i+2 : lev->h; 
          lmin = j ? j-1 : 0;
          lmax = lev->w-j-1 ? j+2 : lev->w; 
          g = 1;
          for ( k = kmin; k < kmax; ++k )
            for ( l = lmin; l < lmax; ++l )
            {
              c = lev->rgb[k][l];
              if ( abs(c) < lev->rgb[i][j] )  g = 0;
            }
          if ( g )
          {
            lev->rgb[i][j] = -lev->rgb[i][j];
            ++f;
          }
        }
    for ( i = 0; i < lev->h; ++i )
      for ( j = 0; j < lev->w; ++j )
      {
        c = lev->rgb[i][j];
        if ( c < 0 )  lev->rgb[i][j] = -c+1; 
      }
  }
  while ( f );
  for ( i = 0; i < lev->h; ++i )
    for ( j = 0; j < lev->w; ++j )  if ( lev->rgb[i][j] )  --lev->rgb[i][j];
  out = AllocImage ( im->h, im->w );
  for ( f = ct = 1; ct ; ++f )  
  {
    ct = 0;
    for ( i = 0; i < im->h; ++i )
      for ( j = 0; j < im->w; ++j )
        if ( 2*lev->rgb[i/2][j/2] >= f )
        {
          ++ct;
          kmin = i ? i-1 : 0;
          kmax = im->h-i-1 ? i+2 : im->h; 
          lmin = j ? j-1 : 0;
          lmax = im->w-j-1 ? j+2 : im->w; 
          r = g = b = 0;
          for ( k = kmin; k < kmax; ++k )
            for ( l = lmin; l < lmax; ++l )
            {
              r += getR(im->rgb[k][l]);
              g += getG(im->rgb[k][l]);
              b += getB(im->rgb[k][l]);
            }
          d = (kmax-kmin)*(lmax-lmin);
          out->rgb[i][j] = SetRGB ( r/d, g/d, b/d );
        }
        else  out->rgb[i][j] = im->rgb[i][j];
    Paste ( im, out, 0, 0 ); 
  }
/*
  for ( i = 0; i < lev->h; ++i )
    for ( j = 0; j < lev->w; ++j )
      lev->rgb[i][j] = SetRGB(lev->rgb[i][j]*255,lev->rgb[i][j]*255,lev->rgb[i][j]*255);
  WritePPM ( lev, "lev.ppm" );
*/
  DestroyImage ( lev );
  return out;
}

char * getImParam ( const char *filename, const char *imname, const char *paramname )
{
  FILE  *fp;
  char  *buf, *p, nbuf[128]; 
 
  strcpy ( nbuf, imname );
  if ( ( p = strchr(nbuf,'.') ) != NULL )  *p = '\0';
  buf = (char*) calloc ( 256, 1 );
  fp = fopen ( filename, "r" );
  if ( fp == NULL )  return NULL;
  while ( fgets ( buf, 256, fp ) != NULL && strstr ( buf, nbuf ) == NULL );
  while ( fgets ( buf, 256, fp ) != NULL )
    if ( ( p = strstr ( buf, paramname ) ) != NULL )
    { 
      p += strlen ( paramname );
      while ( !isspace ( *p ) )  ++p;
      while ( isspace ( *p ) )  ++p;
      fclose ( fp );
      p[strlen(p)-2] = '\0';
      return p;
    }
  fclose ( fp );
  return NULL;
}

void writeImageData ( struct image *im, const char *filename, const char *imname )
{
  char  buf[1024];

  sprintf ( buf, "%s\n%s\nf = %s\nISO %s\nA = %s\nt = %s\nEV = %s\nD = %s\nBlitz %s\n", 
    imname,
  getImParam ( filename, imname, "Datum" ),
  getImParam ( filename, imname, "Brennweite" ),
  getImParam ( filename, imname, "ISO-Wert" ),
  getImParam ( filename, imname, "Blende:" ),
  getImParam ( filename, imname, "Belichtungszeit" ),
  getImParam ( filename, imname, "EV" ),
  getImParam ( filename, imname, "Objektentfernung" ),
  getImParam ( filename, imname, "Blitz" ) );
  SetComment ( im, buf );
}

