/* 
Rockfall code
Purpose: determine landslide area and volume depending on topography.
Hergarten, S.: Topography-based modeling of large rockfalls and application to hazard assessment, Geophysical Research Letters, 39, https://doi.org/10.1029/2012GL052090, 2012
Author: Stefan Hergarten
Modified by: Anne-Laure Argentin
*/


#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>
#include <climits>
#include <cfloat>
#include <cmath>
#include <vector>
#include <assert.h>

#define DATATYPE float

using namespace std;

string tmpdir;

#include "image.cpp"
#include "track.cpp"

struct node { DATATYPE h; DATATYPE o; float s; };
struct index { int lat; int lon; };
struct Rockfall { int lat; int lon; double a; double v; vector <struct index> l; vector <struct index> p; vector <DATATYPE> h; vector <DATATYPE> o; }; // AL: modif, added "vector <DATATYPE> o;"

class CDEM
{ 
  struct node   ****n, b;

  public:
  int                   res, minlat, maxlat, minlon, maxlon;
  float                 minelev;
  double                *rx, ry, *rxy, c, p, a0;
  vector<struct index>  patches; 

  CDEM ( )
  {
    n = (struct node****) calloc ( 180, sizeof(struct node***) );
    *n = (struct node***) calloc ( 180*360, sizeof(struct node**) );
    for ( int i = 1; i < 180; ++i )  n[i] = n[i-1]+360;
    res = 0;
    b.h = b.o = 0;
    b.s = 0.;
    minlat = minlon = INT_MAX;
    maxlat = maxlon = -INT_MAX;
  }

  void read ( const char *filename )
  {
    int         row = -1, col = -1, valid = 1;
    const char  *p, *q;
    FILE        *fp;
    
    if ( ( fp = fopen ( filename, "rb" ) ) == NULL )
    {
      printf ( "File %s not found.\n", filename );
      return;
    }
    if ( ( q = strrchr(filename,'/') ) == NULL )  q = filename;
    if ( ( p = strchr(q,'N') ) != NULL )  row = atoi(p+1)+90;
    else
      if ( ( p = strchr(q,'S') ) != NULL )  row = -atoi(p+1)+90;
    if ( ( p = strchr(q,'E') ) != NULL )  col = atoi(p+1)+180;
    else
      if ( ( p = strchr(q,'W') ) != NULL )  col = -atoi(p+1)+180;
    if ( row < 0 || row >= 180 || col < 0 || col >= 360 )
    {
      printf ( "Position of file %s could not be recognized. Ignoring file.\n", filename );
      fclose ( fp );
      return;
    }
    if ( n[row][col] == NULL ) 
    {
      if ( res == 0 )
      {
        int datasize = strstr(filename,".hg") != NULL || strstr(filename,".dra") != NULL ? 2 : 1;
        fseek ( fp, 0, SEEK_END );
        int filesize = ftell ( fp );
        rewind ( fp );
        res = sqrt(filesize/datasize)-0.5;
        printf ( "Setting data resolution to %i points per degree\n",
                 res );
        ry = 4e7/(360*res);
        rx = (double*) calloc ( 90*res+1, sizeof(double) );
        rxy = (double*) calloc ( 90*res+1, sizeof(double) );
        for ( int i = 0; i <= 90*res; ++i )
        {
          rx[i] = ry*cos(i/(57.29578*res));
          rxy[i] = sqrt(rx[i]*rx[i]+ry*ry);
        }
      }
      struct node **p = (struct node**) calloc ( res, sizeof(struct node*) );
      *p = (struct node*) calloc ( res*res, sizeof(struct node) );
      for ( int i = 1; i < res; ++i )  p[i] = p[i-1] + res;
      n[row][col] = p;
      printf ( "Assigning new data segment for lat = %i, lon = %i.\n",
                row-90, col-180 ); 
    }
    if ( strstr(filename,".hgm") != NULL )
    { 
      short int *buf = (short int*) calloc ( res+1, sizeof(short int) );
      valid = valid && ( fread ( buf, sizeof(short int), (res+1), fp ) == (res+1) );
      for ( int i = 0; i < res; ++i )
      {
        valid = valid && ( fread ( buf, sizeof(short int), (res+1), fp ) == (res+1) );
        for ( int j = 0; j < res; ++j )
        {
          if ( buf[j] >= 16384 )  buf[j] -= 16384;
//          if ( buf[j] >= 16384 )  buf[j] = 0;
          if ( buf[j] < minelev )  buf[j] = minelev;
          n[row][col][res-1-i][j].h = buf[j];
        }
      }
      if ( res*(row-90) < minlat )  minlat = res*(row-90); 
      if ( res*(row-89)-1 > maxlat )  maxlat = res*(row-89)-1; 
      if ( res*(col-180) < minlon )  minlon = res*(col-180); 
      if ( res*(col-179)-1 > maxlon )  maxlon = res*(col-179)-1; 
      struct index  ind;
      ind.lat = res*(row-90);
      ind.lon = res*(col-180);
      patches.push_back(ind);
      if ( valid )
        printf ( "File %s successfully read and assigned to "
                 "lat = %i, lon = %i.\n",
                 filename, row-90, col-180 );
      else 
        printf ( "Warning: invalid file %s read and assigned to "
                 "lat = %i, lon = %i.\n",
                 filename, row-90, col-180 );
      free ( buf );
      fclose ( fp );
      return;
    }
    char *buf = (char*) calloc ( 2*(res+1), 1 );
    valid = valid && ( fread ( buf, 1, 2*(res+1), fp ) == 2*(res+1) );
    for ( int i = 0; i < res; ++i )
    {
      valid = valid && ( fread ( buf, 1, 2*(res+1), fp ) == 2*(res+1) );
      for ( int j = 0; j < res; ++j )
        n[row][col][res-1-i][j].o = 256*(signed char)buf[2*j]+(unsigned char)buf[2*j+1];
    }
    if ( res*(row-90) < minlat )  minlat = res*(row-90); 
    if ( res*(row-89)-1 > maxlat )  maxlat = res*(row-89)-1; 
    if ( res*(col-180) < minlon )  minlon = res*(col-180); 
    if ( res*(col-179)-1 > maxlon )  maxlon = res*(col-179)-1; 
    struct index  ind;
    ind.lat = res*(row-90);
    ind.lon = res*(col-180);
    patches.push_back(ind);
    if ( valid )
      printf ( "File %s successfully read and assigned to "
               "lat = %i, lon = %i.\n",
               filename, row-90, col-180 );
    else 
      printf ( "Warning: invalid file %s read and assigned to "
               "lat = %i, lon = %i.\n",
               filename, row-90, col-180 );
    free ( buf );
    fclose ( fp );
  }

  void read ( vector<const char*> &filename, int cols = 0, double f = 0. )
  {
    struct node  *p;

    for ( vector<const char*>::iterator it = filename.begin(); it != filename.end(); ++it )
      if ( cols > 0 )
        read ( *it, cols );
      else
        read( *it );

    if ( res > 1 )
      for ( int lat = minlat; lat <= maxlat; ++lat )
        for ( int lon = minlon; lon <= maxlon; ++lon )
          if ( (p=getNode(lat,lon))->h )
            p->o = p->h >= 16384 ? p->h-16384-p->o : 0;
          else
          {
            p->h = p->o;
            p->o = 0;
          }  
    if ( f )
    {
    for ( int lat = minlat; lat <= maxlat; ++lat )
      for ( int lon = minlon; lon <= maxlon; ++lon )
      {
        if ( (p=getNode(lat,lon))->o > 0 )
          for ( int i = lat-(int)(f*p->o/ry); i <= lat-(int)(f*p->o/ry); ++i )
            for ( int j = lon-(int)(f*p->o/rx[abs(lat)]); j <= lon+(int)(f*p->o/rx[abs(lat)]); ++j )
              if ( getNode(i,j)->h < 16384 )  getNode(i,j)->h += 16384;
//        printf ( "%i %i %i %i\n", lat, lon, p->h-16384, p->o );  
        p->o = 0;
      }
    }
  }
  
  void read ( const char *filename, int cols )
  {
    res = 1;	  
    FILE  *fp; 
    DATATYPE *buf = (DATATYPE*) calloc ( cols, sizeof(DATATYPE) );
    if ( ( fp = fopen ( filename, "rb" ) ) == NULL  )
    {
      printf ( "Error: File %s not found.\n", filename );
      exit ( -1 );
    }
    fseek ( fp, 0, SEEK_END );
    int len = ftell ( fp );
    int rows = len/(cols*sizeof(DATATYPE));
    if ( rows*cols*sizeof(DATATYPE) != len )
    {
      printf ( "Error: File %s has inconsistent length.\n", filename );
      exit ( -1 );
    }
    fseek ( fp, 0, SEEK_SET );
    struct node **p = (struct node**) calloc ( rows, sizeof(struct node*) );
    *p = (struct node*) calloc ( rows*cols, sizeof(struct node) );
      for ( int i = 1; i < rows; ++i )  p[i] = p[i-1] + cols;
    n[0][0] = p;
    for ( int i = rows-1; i >= 0; --i )
      for ( int j = 0; j < cols; ++j )
      {
        fread ( &n[0][0][i][j].h, sizeof(DATATYPE), 1, fp );
	if ( n[0][0][i][j].h < 0 )  n[0][0][i][j].h = 0;
      }
    fclose ( fp );
    printf ( "Read file %s with %i rows and %i columns\n", filename, rows, cols );
    printf ( "Heeeyyyyy!\n" );
    minlat = 0;
    maxlat = rows-1;
    minlon = 0;
    maxlon = cols-1;
    ry = 1.;
    rx = (double*) calloc ( rows, sizeof(double) );
    rxy = (double*) calloc ( rows, sizeof(double) );
    for ( int i = 0; i < rows; ++i )
    {
      rx[i] = 1.;
      rxy[i] = sqrt(2.);
    }
  }

  void write ( const char *filename )
  {
    int         row = -1, col = -1, valid = 1;
    const char  *p, *q;
    FILE        *fp;
    
    if ( ( fp = fopen ( filename, "wb" ) ) == NULL )
    {
      printf ( "File %s could not be opened for writing.\n", filename );
      return;
    }
    if ( ( q = strrchr(filename,'/') ) == NULL )  q = filename;
    if ( res > 1 )
    {
      if ( ( p = strchr(q,'N') ) != NULL )  row = atoi(p+1)+90;
      else
        if ( ( p = strchr(q,'S') ) != NULL )  row = -atoi(p+1)+90;
      if ( ( p = strchr(q,'E') ) != NULL )  col = atoi(p+1)+180;
      else
        if ( ( p = strchr(q,'W') ) != NULL )  col = -atoi(p+1)+180;
      if ( row < 0 || row >= 180 || col < 0 || col >= 360 )
      {
        printf ( "Position of file %s could not be recognized. Ignoring file.\n", filename );
        fclose ( fp );
        return;
      }
      if ( n[row][col] == NULL ) 
      {
        printf ( "No data for file %s.\n", filename );
        fclose ( fp );
        return;
      }
      if ( strstr(filename,".hgm") != NULL )
      {
        for ( int i = 0; i <= res; ++i )
          for ( int j = 0; j <= res; ++j )
            fwrite ( &(getNode(res*(row-89)-i,res*(col-180)+j)->h), sizeof(short int), 1, fp );
      }
      else
      {
        char *buf = (char*) calloc ( 2*(res+1), 1 );
        for ( int i = 0; i <= res; ++i )
        {
          for ( int j = 0; j <= res; ++j )
          {
            buf[2*j] = (unsigned short int)getNode(res*(row-89)-i,res*(col-180)+j)->h/256;
            buf[2*j+1] = (unsigned short int)getNode(res*(row-89)-i,res*(col-180)+j)->h%256;
          }
          fwrite ( buf, 1, 2*(res+1), fp );
        }
        free ( buf );
      }
      printf ( "Successfully written file %s (lat = %i, lon = %i).\n",
               filename, row-90, col-180 );
    }
    else
      for ( int i = maxlat; i >= minlat; --i )
        for ( int j = minlon; j <= maxlon; ++j )
          fwrite ( &n[0][0][i][j].h, sizeof(DATATYPE), maxlon+1, fp );
    fclose ( fp );
  }

  struct node * getNode ( int lat, int lon )
  {
    if ( res == 1 )  return lat >= minlat && lat <= maxlat && lon >= minlon && lat <= maxlon ? &n[0][0][lat][lon] : &b;
    int row = (lat+res*90)/res;
    int col = (lon+res*180)/res;
    return n[row][col] != NULL ? &n[row][col][(lat+res*60)%res][(lon+res*180)%res] : &b;
  }      

  struct node * getNode ( struct index ind )
  {
    return getNode ( ind.lat, ind.lon );
  }

  struct index getRandomPoint ( int minelev )
  {
    struct index ind;	  
    do
    {
      ind.lat = minlat + (double)(maxlat-minlat)*rand()/RAND_MAX; 	    
      ind.lon = minlon + (double)(maxlon-minlon)*rand()/RAND_MAX;
    }
    while ( getNode(ind)->h < minelev );
    return ind;    
  }	  

  void exclude ( )
  {
    for ( int lat = minlat; lat <= maxlat; ++lat )
      for ( int lon = minlon; lon <= maxlon; ++lon )
        getNode(lat,lon)->o = getNode(lat,lon)->h;
    for ( int lat = minlat; lat <= maxlat; ++lat )
      for ( int lon = minlon; lon <= maxlon; ++lon )
        if ( getNode(lat,lon)->o <= 0 || getNode(lat,lon)->o >= 16384 )
          for ( int i = lat-16; i <= lat+16; ++i )
            if ( getNode(i,lon)->h > 0 && getNode(i,lon)->h < 16384 )  getNode(i,lon)->h += 16384;
    for ( int lat = minlat; lat <= maxlat; ++lat )
      for ( int lon = minlon; lon <= maxlon; ++lon )
        getNode(lat,lon)->o = getNode(lat,lon)->h;
    for ( int lat = minlat; lat <= maxlat; ++lat )
      for ( int lon = minlon; lon <= maxlon; ++lon )
        if ( getNode(lat,lon)->o <= 0 || getNode(lat,lon)->o >= 16384 )
          for ( int i = lon-24; i <= lon+24; ++i )
            if ( getNode(lat,i)->h > 0 && getNode(lat,i)->h < 16384 )  getNode(lat,i)->h += 16384;
    for ( int lat = minlat; lat <= maxlat; ++lat )
      for ( int lon = minlon; lon <= maxlon; ++lon )
        getNode(lat,lon)->o = 0;
  }

  int destabilize ( int lat, int lon, double smin, double smax )
  {
    double         s, sh = 0., prob;
    struct node    *p, *q, *l;

    p = getNode ( lat, lon );
    if ( !p->h )  return 0;
    double h = p->h >= 16384 ? p->h-16384 : p->h;  
//    if ( !p->h || p->h >= 16384 )  return 0;
//    for ( int i = lat-11; i <= lat+11; ++i )
//      for ( int j = lon-16; j <= lon+16; ++j )
//        if ( getNode(i,j)->h >= 16384 || getNode(i,j)->h <= 0 )  return 0;
    q = getNode ( lat+1, lon+1 );
    if ( q->h && q->h < 16384 && ( s = (h-q->h)/rxy[abs(lat)] ) > sh )
    {
      sh = s;
      l = q;
    }
    q = getNode ( lat, lon+1 );
    if ( q->h && q->h < 16384 && ( s = (h-q->h)/rx[abs(lat)] ) > sh )
    {
      sh = s;
      l = q;
    }
    q = getNode ( lat-1, lon+1 );
    if ( q->h && q->h < 16384 && ( s = (h-q->h)/rxy[abs(lat)] ) > sh )
    {
      sh = s;
      l = q;
    }
    q = getNode ( lat-1, lon );
    if ( q->h && q->h < 16384 && ( s = (h-q->h)/ry ) > sh )
    {
      sh = s;
      l = q;
    }
    q = getNode ( lat-1, lon-1 );
    if ( q->h && q->h < 16384 && ( s = (h-q->h)/rxy[abs(lat)] ) > sh )
    {
      sh = s;
      l = q;
    }
    q = getNode ( lat, lon-1 );
    if ( q->h && q->h < 16384 && ( s = (h-q->h)/rx[abs(lat)] ) > sh )
    {
      sh = s;
      l = q;
    }
    q = getNode ( lat+1, lon-1 );
    if ( q->h && q->h < 16384 && ( s = (h-q->h)/rxy[abs(lat)] ) > sh )
    {
      sh = s;
      l = q;
    }
    q = getNode ( lat+1, lon );
    if ( q->h && q->h < 16384 && ( s = (h-q->h)/ry ) > sh )
    {
      sh = s;
      l = q;
    }
    prob = ( sh - ( p->s > smin? p->s : smin ) ) / ( smax - smin );
    if ( prob < 0. )  return 0;
    if ( rand() >= prob*RAND_MAX )
    {
      p->s = sh;
//if ( p->s < smin ) { printf ( "%e\n", p->s ); exit ( 0 ); }
      return 0;
    }  
//  printf ( "%i %i %g %g %i %i", lat, lon, sh, prob, l->h, p->h ); 
    if ( !p->o )  p->o = p->h >= 16384 ? p->h-16384 : p->h;  
    p->h = l->h + (h-l->h)*smin/sh;
// printf ( " %i\n", p->h ); 
    return 1;
  }

  struct index getNeighbor ( struct index ind, unsigned char d )
  {
    if ( d && d != 255 )
    {
      ind.lon = d >= 1 && d <= 4 ? ind.lon+1 : d >= 16 && d <= 64 ? ind.lon-1 : ind.lon;
      ind.lat = d == 1 || d >= 64 ? ind.lat+1 : d >= 4 && d <= 16 ? ind.lat-1 : ind.lat;
    }
    return ind;
  }

  struct Rockfall rockfall ( int lat, int lon, double smin, double smax, int d, FILE *fp = NULL )
  {
    double  s;	  
    struct index ind, north, neigh;
    struct node  *p;
    struct Rockfall r;
    vector <struct index>  l;

    r.lat = ind.lat = lat;
    r.lon = ind.lon = lon;
    r.a = r.v = 0.;
    l.push_back(ind);
    int gen = 1, gmarker = 0;
    for ( int i = 0; i < l.size(); ++i )
    {
      if ( destabilize ( l[i].lat, l[i].lon, smin, smax ) )
      {
        if ( fp != NULL )
          fprintf ( fp, "%i %i %i %f\n", gen, l[i].lat, l[i].lon, (double)getNode(l[i].lat,l[i].lon)->h );
        for ( ind.lat = l[i].lat-1; ind.lat <= l[i].lat+1; ++ind.lat )
          for ( ind.lon = l[i].lon-1; ind.lon <= l[i].lon+1; ++ind.lon )
	    if ( ind.lat != l[i].lat || ind.lon != l[i].lon )
	      l.push_back(ind); 
//printf ( "%i %i %i\n", i, gmarker, gen );
        if ( i >= gmarker ) 
        {
          gmarker = l.size()-1;
          ++gen;
        }
      }  
//  printf ( "%i\n", l.size() );
    }
    north.lat = INT_MIN;
    double al_volume = 0;
    for ( int i = 0; i < l.size(); ++i )
    {
      p = getNode(l[i]);
      if ( p->o > 0 )
      {
        r.l.push_back(l[i]);
        r.h.push_back(p->h);
        r.o.push_back(p->o); // AL: modif
        r.a += rx[abs(l[i].lat)]*ry; // AL: Compute the landslide area here.
        r.v += rx[abs(l[i].lat)]*ry*(p->o-p->h); // AL: Compute the landslide volume here.
        al_volume += (p->o-p->h);
        if ( !d )  p->h = p->o;
	p->o = -1;
        if ( l[i].lat > north.lat )  north = l[i];
      }	    
      p->s = 0.;
    }
    if ( r.v != al_volume )
	{
		printf( "Volume AL %f vs. Herg %f", al_volume, r.v );
	}
    //assert( r.v*100 == al_volume );
    if ( r.l.size() )
    {
      ind = north;
      d = 32;
      do
      {
        r.p.push_back(ind);
//printf ( "Perimeter %i %i\n", ind.lat, ind.lon );
        d = (d<<2)%255;
//printf ( "Direction %i\n", d );
        while ( getNode(neigh=getNeighbor(ind,d))->o == 0 
                && getNode(neigh.lat,neigh.lon+1)->o == 0
                && getNode(neigh.lat+1,neigh.lon)->o == 0
                && getNode(neigh.lat+1,neigh.lon+1)->o == 0 )
        {
          d >>= 1;
          if ( !d )  d = 128;
        }
//printf ( "Direction %i\n", d );
        ind = neigh;  
      }
      while ( ind.lat != north.lat || ind.lon != north.lon );
      for ( int i = 0; i < r.l.size(); ++i )
        getNode(r.l[i])->o = 0;
    }
    return r;
  }

};

int main ( int argc, char **argv )
{
  int                  minelev = 10, n = 0, d = 0, w = 0, cols = 0;
  long int             nt = 0;
  double               smin = 1., smax = 5., vsum = 0., f = 0., minvol = 0.;
  char                 *dir = NULL, com[1024];
  vector<const char*>  filename;
  FILE                 *out = stdout, *per = NULL, *hlog = NULL;

  srand(getpid());
  for ( int i = 1; i < argc; ++i )
    if ( argv[i][0] == '-' )
      switch ( argv[i][1] )
      {
        case 'c': cols = atoi(argv[++i]); //
                  break;
        case 's': smin = atof(argv[++i]);
                  break;
        case 'u': smax = atof(argv[++i]);
                  break;
        case 'h': minelev = atoi(argv[++i]);
                  break;
        case 'n': n = atoi(argv[++i]);
                  break;
        case 't': nt = atoi(argv[++i]);
                  break;
        case 'f': f = atof(argv[++i]);
                  break;
        case 'd': d = 1;
                  break;
        case 'w': w = 1;
                  break;
        case 'o': out = fopen ( argv[++i], "w" );
                  break;
        case 'p': dir = argv[++i];
                  sprintf ( com, "mkdir %s", dir );
                  system ( com );
                  sprintf ( com, "%s/all.ps", dir );
                  per = fopen ( com, "w" );
                 
                  fprintf ( per, "%%!\n"
                                 "/LL { exch 47 sub 840 mul exch 9 sub 573 mul exch } def\n"
                                 "0.1 setlinewidth\n"
                                 "1 0 0 setrgbcolor\n" );
 printf("Enter Section\n");  
                  minvol = atof(argv[++i]);
                  break;
 
      }
    else
 filename.push_back(argv[i]);

  tmpdir = "/tmp/";
  CDEM *cdem = new CDEM ( );
  cdem->read ( filename, cols, f );


//  cdem->exclude();
  for ( int i = 0; n > 0 || nt > 0; ++i )
  {
//    if ( per != NULL )  hlog = fopen ( "hlog", "w" );
    struct index ind = cdem->getRandomPoint ( minelev );	  
    struct Rockfall r = cdem->rockfall ( ind.lat, ind.lon, smin, smax, d, NULL );
//    if ( hlog != NULL )  fclose(hlog);
    --nt;
    if ( r.v ) // AL: IF there was a landslide triggered by the shoot (i.e. if the landslide volume > 0)
    {
      --n;
      vsum += r.v;
      fprintf ( out, "%i %f %f %g %g %g %g %g\n",
                     i, (double)r.lon/cdem->res, (double)r.lat/cdem->res,
		     (double)cdem->getNode(r.lat,r.lon)->h, r.a, r.v, r.v/r.a, vsum ); // AL: then this landslide is printed in the (-p option).csv file from the options. (Which means all the landslide indices missing are due to no triggering success.)
      if ( per != NULL && r.v >= minvol ) // AL: if in addition the landslide volume is bigger than the threshold (-> last number of the commandline), the rupture surface is printed in a (landslide_nb).csv file. 
      {
                 
        Track  perimeter;
        for ( int j = 0; j < r.p.size(); ++j )
        {
          Point  p;
          p.lon = (r.p[j].lon+0.5)/cdem->res;
          p.lat = (r.p[j].lat+0.5)/cdem->res;
          perimeter.points.push_back(p);
          fprintf ( per, "%i %f %f %g %g %g %g %g pop pop pop pop pop pop pop pop ",
                         i, (double)r.lon/cdem->res, (double)r.lat/cdem->res,
	                 (double)cdem->getNode(r.lat,r.lon)->h, r.a, r.v, r.v/r.a, vsum ); 
          fprintf ( per, "%f %f LL %s\n", p.lat, p.lon, j ? "lineto" : "moveto" ); 
        }
        fprintf ( per, "%i %f %f %g %g %g %g %g pop pop pop pop pop pop pop pop ",
                       i, (double)r.lon/cdem->res, (double)r.lat/cdem->res,
                       (double)cdem->getNode(r.lat,r.lon)->h, r.a, r.v, r.v/r.a, vsum ); 
        fprintf ( per, "closepath stroke\n" );
        sprintf ( com, "%s/%i.kml", dir, i );
        perimeter.writeKML ( com );
        sprintf ( com, "%s/%i.csv", dir, i );
        FILE *ev = fopen ( com, "w" );
        for ( int j = 0; j < r.l.size(); ++j )
          fprintf ( ev, "%i,%i,%g\n", r.l[j].lat, r.l[j].lon, (double)r.h[j] );
        fclose ( ev );
        sprintf ( com, "%s/%i_original.csv", dir, i ); // AL: modif
        FILE *ev2 = fopen ( com, "w" ); // AL: modif
        for ( int j = 0; j < r.l.size(); ++j ) // AL: modif
          fprintf ( ev2, "%i,%i,%g\n", r.l[j].lat, r.l[j].lon, (double)r.o[j] ); // AL: modif
        fclose ( ev2 ); // AL: modif
//        sprintf ( com, "mv hlog %s/%i.csv", dir, i );
//        system(com);
      }
//      if ( w && !n )
//        for ( vector<const char*>::iterator it = filename.begin(); it != filename.end(); ++it )
//          cdem->write(*it);
    }
  }  
  exit ( 0 );
}  




