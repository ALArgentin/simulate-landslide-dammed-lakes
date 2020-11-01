#define DEG *M_PI/180

char *readFile ( char *filename, int &len )
{
  FILE *fp = fopen ( filename, "rb" );
  fseek ( fp, 0, SEEK_END );
  len = ftell ( fp );
  fseek ( fp, 0, SEEK_SET );
  char *buf = (char*) malloc ( len+1 );
  fread( buf, len, 1, fp );
  buf[len] = '\0';
  fclose ( fp );
  return buf;
}

char *readFile ( char *filename )
{
  int  len;
  return readFile ( filename, len );
}

class Point
{
  public:
  int     start;	  
  double  lon, lat;

  double distanceFromLine ( Point a, Point b )
  {
    b.lon -= a.lon;
    b.lat -= a.lat;
    a.lon -= lon;
    a.lat -= lat;
    double c = cos(lat DEG); 
    a.lon *= c;
    b.lon *= c;
    return 111111*sqrt(fabs(a.lon*a.lon+a.lat*a.lat-pow((a.lon*b.lon+a.lat*b.lat),2)/(b.lon*b.lon+b.lat*b.lat)));
  }

  double distanceFromLine ( Point a, Point b, Point &p, double &lambda )
  {
    b.lon -= a.lon;
    b.lat -= a.lat;
    a.lon -= lon;
    a.lat -= lat;
    double c = cos(lat DEG); 
    a.lon *= c;
    b.lon *= c;
    double den = b.lon*b.lon+b.lat*b.lat;
    lambda = den ? -(a.lon*b.lon+a.lat*b.lat)/den : 0.;
    if ( lambda < 0. )  lambda = 0.;
    else 
      if ( lambda > 1. )  lambda = 1.;
    p.lon = a.lon + lambda*b.lon;
    p.lat = a.lat + lambda*b.lat;
    double d = 111111*sqrt(p.lon*p.lon+p.lat*p.lat);
    if ( a.lon*b.lat > a.lat*b.lon )  d = -d;
    p.lon /= c;
    p.lon += lon;
    p.lat += lat;
    return d;
  }
};

class Track
{
  public:

  char           name[1024];
  vector<Point>  points;
  Point          ll, ur;

  Track()
  {
    *name = '\0';
  }

  struct image *computeMask ( int lat, int lon, int res )
  {
    FILE *fp = fopen ( (tmpdir+"polygon.ps").c_str(), "w" );
    fprintf ( fp, "%%!\n" );
    for ( int i = 0; i < points.size(); ++i )
    {
      if ( i && points[i].start )  fprintf ( fp, "closepath\n" );	      
      fprintf ( fp, "%f %f %s\n", 576*(points[i].lon-lon),
                                  576*(points[i].lat-lat),
                                  i && !points[i].start ? "lineto" : "moveto" );
    }
    fprintf ( fp, "closepath\nfill\nshowpage\n" );
    fclose ( fp );
    char  buf[1024];
    sprintf ( buf, "gs -sDEVICE=pbmraw -sPAPERSIZE=a4 -r%d -q -dNOPAUSE -sOutputFile=%spolygon.pbm -- %spolygon.ps", res/8, tmpdir.c_str(), tmpdir.c_str() );
    sprintf ( buf, "gs -sDEVICE=pbmraw -sPAPERSIZE=a4 -r%d -dNOPAUSE -sOutputFile=%spolygon.pbm -- %spolygon.ps", res/8, tmpdir.c_str(), tmpdir.c_str() );
//    sprintf ( buf, ("gs -sDEVICE=pbmraw -sPAPERSIZE=a4 -r%d -q -dNOPAUSE -sOutputFile="+tmpdir+"polygon.pbm -- "+tmpdir+"polygon.ps").c_str(), res/8 );
printf ( "%s\n", buf );
    system ( buf );
    struct image * im = ReadPPM ( (tmpdir+"polygon.pbm").c_str() );
    sprintf ( buf, ("rm "+tmpdir+"polygon.*").c_str() );
//    system ( buf );
    return im;
  }

  Track shorten ( double d )
  {
    if ( points.size() == 0 )  return *this;
    Track  s;
    strcpy ( s.name, name );
    s.points.push_back(points[0]);
    int last = 0;
    for ( int i = last+2; i < points.size(); ++i ) 
      for ( int j = last+1; j < i; ++j )
        if ( points[j].distanceFromLine(points[last],points[i]) > d )
        {
          last = i-1;
          s.points.push_back(points[last]);
          i = last+2;
          break;
        }     
    s.points.push_back(points.back());
    return s;
  }

  Track shorten ( double d, int nmax )
  {
    int  n;
    for ( ; ; d *= 1.05*n/nmax )
    { 
      Track  s = shorten(d);
      n = s.points.size();
      printf ( "Shortening perimeter to resolution %.0f m (%i points).\n", d, n );
      if ( n <= nmax )  return s;
//      s.~Track();
    }
  }

  void writeGPX ( string filename )
  {
    char ml[8];
    FILE *fp = fopen ( filename.c_str(), "w" );
    fprintf ( fp, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
                  "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" "
                  "creator=\"Geomorphology tools written by Stefan Hergarten\" version=\"1.1\" "
                  "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                  "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n"
                  "  <trk>\n" );
    if ( strlen(name) )
      fprintf ( fp, "    <name>%s</name>\n", name );
    fprintf ( fp, "    <trkseg>\n" );
    for ( int i = 0; i < points.size(); ++i )
    {  
      fprintf ( fp, "      <trkpt lat=\"%f\" lon=\"%f\" />\n", points[i].lat, points[i].lon );
//      fprintf ( fp, "      </trkpt>\n" );
    }    
    fprintf ( fp, "    </trkseg>\n  </trk>\n</gpx>\n" );
    fclose ( fp );
  }

  void readGPX ( char *filename )
  {
    Point  c;
    char *p, *q, *r, *start, *end;
    char *buf = readFile ( filename );
    for ( p = buf; ( p = strstr(p,"<gpxx:rpt") ) != NULL; )
      memcpy ( p, "<trkpt   ", 9 );
    if ( ( start = strstr(buf,"<name") ) != NULL )
    {
      start = strchr(start,'>') + 1;
      while ( isspace(*start) )  ++start;
      end = strstr(start,"</name>") - 1;  
      while ( isspace(*end) )  --end;
      strncpy ( name, start, end+1-start );
      name[end+1-start] = '\0'; 
    }
    end = buf-1;
    c.start = 1;
    while( ( start = strstr(end+1,"<trkpt") ) != NULL )
    {
      int ct = 1;
      for ( end = start+1; ct; ++end )
      {
        if ( *end == '<' && *(end+1) != '/' )  ++ct;
        else
          if ( *end == '/' )  --ct;
      }
      *end = '\0';
      c.lat = 0.; 
      if ( ( p = strstr(start,"lat") ) != NULL && p < end ) 
      {
        while ( !isdigit(*p) && *p != '-' )  ++p;
        c.lat = atof(p); 
      }
      c.lon = 0.;
      if ( ( p = strstr(start,"lon") ) != NULL && p < end ) 
      {
        while ( !isdigit(*p) && *p != '-' )  ++p;
        c.lon = atof(p); 
      }
      points.push_back(c);    
      c.start = 0;
    }
  }

  void writeKML ( string filename )
  {
    FILE *fp = fopen ( filename.c_str(), "w" );
    fprintf ( fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                  "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n" 
                  "<Document>\n" 
                  "  <Placemark>\n" );
    if ( strlen(name) )
      fprintf ( fp, "  <name>%s</name>\n", name );
    fprintf ( fp, "    <LineString><coordinates>", name );
    for ( int i = 0; i < points.size(); ++i )
      fprintf ( fp, "%f,%f ", points[i].lon, points[i].lat );
    fprintf ( fp, "</coordinates></LineString>\n"
                  "  </Placemark>\n" 
                  "</Document>\n"
                  "</kml>\n" );
    fclose ( fp );
  }

  void readKML ( char *filename )
  {
    Point  c;
    char     *p, *q, *coord;
    double   lon, lat;
    char *buf = readFile ( filename );
    strcpy ( name, filename );
    while ( ( coord = strstr(buf,"<coordinates>") ) != NULL )
    {	    
      p = coord + 13;
      coord = strstr(coord,"</coordinates>")+1;
      *coord = '\0';
      buf = coord+1;
      c.start = 1;
      while ( 1 )
      {
        c.lon = strtod ( p, &q );
        if ( *q == '\0' || p == q )  break;
        p = q+1;
        c.lat = strtod ( p, &q );
        if ( *q == '\0' || p == q )  break;
        p = q+1;
        for ( q = p-1; *q != '\0' && !isdigit(*q); ++q )
          if ( *q == ',' )
          {
            strtod ( q+1, &p );
//            if ( *q == '\0' || p == q )  break;
            p = p+1;
            break;
         }
//        strtod ( p, &q );
//        if ( *q == '\0' || p == q )  break;
//        p = q+1;
        points.push_back(c);
        c.start = 0;	
      }	
    }
  } 

  void writeCSV ( string filename )
  {
    FILE *fp = fopen ( filename.c_str(), "w" );
    for ( int i = 0; i < points.size(); ++i )
      fprintf ( fp, "%f,%f\n", points[i].lon, points[i].lat );
    fclose ( fp );
  }

  void readCSV ( char *filename )
  {
    Point  c;
    char *buf = readFile ( filename );
    c.start = 1;
    for ( char *p = buf; *p != '\0'; ++p )
      if ( *p == ',' || *p == ';' )  *p = ' ';
    do
    {
      if ( sscanf ( buf, "%le%le", &c.lon, &c.lat ) == 2 )  points.push_back(c);
      c.start = 0;
    }
    while ( ( buf = strchr(buf,'\n') )++ != NULL );
  }

  void write ( string filename )
  {
    if ( strstr(filename.c_str(),".gpx") != NULL || strstr(filename.c_str(),".GPX") != NULL )
      writeGPX ( filename );
    else
      if ( strstr(filename.c_str(),".kml") != NULL || strstr(filename.c_str(),".KML") != NULL )
        writeKML ( filename );
      else
        if ( strstr(filename.c_str(),".csv") != NULL || strstr(filename.c_str(),".CSV") != NULL )
          writeCSV ( filename );
        else
        {
          writeGPX ( filename+".gpx" );
          writeKML ( filename+".kml" );
          writeCSV ( filename+".csv" );
        }
  }

  void read ( char *filename )
  {
    if ( strstr(filename,".gpx") != NULL || strstr(filename,".GPX") != NULL )
      readGPX ( filename );
    else
      if ( strstr(filename,".kml") != NULL || strstr(filename,".KML") != NULL )
        readKML ( filename );
      else
        readCSV ( filename );
    if ( points.size() )
      printf ( "Read track from file %s with %i points.\n", filename, points.size() );
  }

  void computeBBox()
  {
    ll.lon = 180;
    ll.lat = 90;
    ur.lon = -180;
    ur.lat = -90;
    for ( int i = 0; i < points.size(); ++i )
    {
      if ( points[i].lon < ll.lon )  ll.lon = points[i].lon;
      if ( points[i].lat < ll.lat )  ll.lat = points[i].lat;
      if ( points[i].lon > ur.lon )  ur.lon = points[i].lon;
      if ( points[i].lat > ur.lat )  ur.lat = points[i].lat;
    }
  }

  double distanceFromPoint ( Point x, Point &p )
  {
    int    ibest;
    double dist, lambda, lambdabest, mindist = 1e100;
    Point a;
    for ( int i = 1; i < points.size(); ++i )
    {
      double d = x.distanceFromLine ( points[i-1], points[i], a, lambda );
      if ( fabs(d) < fabs(mindist) )
      {
        mindist = d;
        ibest = i;
        lambdabest = lambda;
        p = a;
//        printf ( "Found nearest point in Interval [%i,%i]\n"
//                 "lambda = %g, dist = %g\n"
//                 "lon = %g, lat = %g\n", i-1, i, lambda, mindist, p.lon, p.lat );
      }
    }
    if ( ibest == 1 && lambdabest == 0. 
         || ibest == points.size()-1 && lambdabest == 1. )  mindist = 1e100;
    return mindist;
  }


};

