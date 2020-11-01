[![N|Solid](https://landslides-and-rivers.sbg.ac.at/landrs/wp-content/uploads/2017/10/ricola_v3.jpg)](https://landslides-and-rivers.sbg.ac.at)

# Landslide-dammed lake simulator
The repository hosts a script to simulate landslides that dam river courses and create lakes in the Austrian Alps. 
The script is accompanied by a basic [Docker] container setup that allows very easy installation and deployment.
For more information on the results provided by this code, please refer to [this article][Arg20].

This script triggers landslides from a Digital Elevation Model (DEM), simulates their runouts and 
computes the potentially created lakes in the Austrian Alps, in 6 different steps which have to be
launched separately. By default, only one part of the DEM is selected to be processed for memory
reasons. More/Other parts can be chosen by modifying the variable `parts_nb` in the code.

# Authors
**Anne-Laure Argentin**: the bash script `main.sh` and Docker setup
**Stefan Hergarten**: the *Rockfall* (`rockfalls.cpp`, `image.cpp`, `track.cpp`) code and the Gerris parametrisation file `Debrisflow.gfs`
**Jörg Robl**: the Gerris parametrisation file `Debrisflow.gfs`

----

# Installation using Docker
### Prerequisites
In order to run this container you'll need docker installed.


* [Windows](https://docs.docker.com/windows/started)
* [OS X](https://docs.docker.com/mac/started/)
* [Linux](https://docs.docker.com/linux/started/)

### Command lines
1. Ensure `docker` and `docker-compose` are installed
1. Run `docker-compose up --build`

### What this does
1. Calls an Ubuntu image and installs GDAL, GRASS GIS, [GMT] and [Gerris].
1. Creates a `output_folder` in the container at `/app/output_container`.
1. Copies the bash script `main.sh`, the Gerris parametrisation file `Debrisflow.gfs`, the Austrian digital elevation model (DEM) `dhm_at_lamb_10m_2018.tif` and the Rockfall source files `rockfalls.cpp`, `image.cpp`, `track.cpp` from `scripts` into the container directory `/app/scripts`.
1. Runs the bash script `main.sh` with the option specified in the `Dockerfile`.

The results are available on the host machine at `./scripts/output`

# Running the script
The script necessitates a few external files and comes with different command lines for the 6 processing steps. All input files should be placed in the `scripts` repository.

### Inputs
- The freely available LiDAR-based [digital elevation model (DEM) of the Austrian Alps][DEM] with a spatial resolution of 10 m.
- An executable of the Rockfall code - TO COME.

### Options
Syntax: ./app/main.sh [-h] [-v] -A|B|X|C|D|E
- `-h`: Print the Help.
- `-v`: Runs in verbose mode.
- `-A`: First step of the workflow: cuts the Austrian DEM in 14 smaller parts for computational reasons.
        The extents of the regions are hard-coded. By default, only the first part is selected to be processed.
        Requirement: the Austrian DEM `dhm_at_lamb_10m_2018.tif` in the folder `scripts`.
- `-B`: Second step of the workflow: Fills the selected parts of the DEM and computes the gradient and
        accumulation. This step is carried out using TopoToolbox in the article but is done with GRASS GIS
        in this script.
- `-X`: Third step of the workflow: uses the Rockfall code from Hergarten (2012) to determine the landslide
        areas and volumes, then prepares the DEMs for input into Gerris. The slope thresholds for the 
        Rockfall algorithm are 45° and 79°. The number of seed pixels is derived from the DEM area. Only
        the landslides which volumes exceed 10^5 m^3 are retained for the following steps.
- `-C`: Fourth step of the workflow: launches Gerris. The landslide runouts are simulated for a period of
        6 minutes. No display is available (install gfsview2D for this). The code can be parallelized by enabling
        MPI in Gerris build.
- `-D`: Fifth step of the workflow: fill the landslide dams in GRASS GIS. The landslide deposits outputted 
        from Gerris are added to the topography and the DEM is filled to obtain the landslide-dammed 
        lake topography.
- `-E`: Sixth step of the workflow: computes the geomorphometrics of the landslide-dammed lakes. This 
        includes lake depth and volume, deposit position, height and volume, as well as several obstruction
        and stability indices. The results are outputed in `Computations.txt` files.
        Requirements: the vector files of the lithologies `LithUnits_*.gmt` and the glacial imprint 
        `lgm_global_Corrected_only_alps.gmt` in the folder `scripts` (gmt is the Generic Mapping Tool
        vector extension).

At least one of the `-A`, `-B`, `-X`, `-C`, `-D` or `-E` options is required.

### How to change the options
Go into the `dockercompose.yml` and change the line `command: ["/app/main.sh", -A, -v]` with the option desired before running the `docker-compose up --build` command line.

### Only 1rst part of the DEM
The code produces a lot of data and several GB of space are required to host the DEM and results. By default, only the first part of the DEM is processed. This can be changed inside the script 'main.sh'. Beware that the processing of all parts of Austria requires a non-negligible amount of free memory.

# Codes used in this script
   - Rockfall - Rockfall is a code developped by [Hergarten, (2012)][Her12], which determines landslides areas and volumes on a DEM based on slope thresholds.
   - [Gerris] - Gerris is a flow solver developped by [Popinet, (2003)][Pop03], adapted for steep terrains by [Hergarten and Robl, 2015][Her15]. We use this flow solver to simulate the landslide runout on the DEM. Gerris is automatically downloaded and compiled by the Dockerfile.

# Acknowledgments
This research has been supported by the [Austrian Academy of Sciences][OeAW] (ÖAW) through the [project RiCoLa][RiCoLa] (Detection and analysis of landslide-induced river course changes and lake formation).

# Bibliography

| Citation | Article |
| ------ | ------ |
| Popinet, 2003 | [Popinet, S.: Gerris: A tree-based adaptive solver for the incompressible Euler equations in complex geometries, Journal of ComputationalPhysics, 190, 572–600, https://doi.org/10.1016/S0021-9991(03)00298-5][Pop03] |
| Hergarten and Robl, 2015 | [Hergarten, S. and Robl, J.: Modelling rapid mass movements using the shallow water equations in Cartesian coordinates, Natural Hazardsand Earth System Sciences, 15, 671–685, https://doi.org/10.5194/nhess-15-671-2015, www.nat-hazards-earth-syst-sci.net/15/671/2015/, 2015][Her15] |
| Hergarten, 2012 | [Hergarten, S.: Topography-based modeling of large rockfalls and application to hazard assessment, Geophysical Research Letters, 39, https://doi.org/10.1029/2012GL052090, 2012][Her12] |

# License
This is free software delivered under the [MIT license][MIT].
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

**Free Software, Hell Yeah!**

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)

   [Docker]: <https://www.docker.com/>
   [Arg20]: <https://www.natural-hazards-and-earth-system-sciences.net/>
   [OeAW]: <https://www.oeaw.ac.at/>
   [RiCoLa]: <https://landslides-and-rivers.sbg.ac.at/>
   [Gerris]: <http://gfs.sourceforge.net/wiki/index.php/Main_Page>
   [Pop03]: <https://www.sciencedirect.com/science/article/pii/S0021999103002985?casa_token=uz7J-hc7FOcAAAAA:JJzVNIuCUK4cfmuJtNyJ-tnmkujNc9CNTtSEA5j7e-J6tNHrv9l74Y5W8eXmyJ1WGGoTJO5O3UY>
   [Her15]: <https://nhess.copernicus.org/articles/15/671/2015/nhess-15-671-2015.pdf>
   [Her12]: <https://agupubs.onlinelibrary.wiley.com/doi/epdf/10.1029/2012GL052090>
   [DEM]: <https://www.data.gv.at/katalog/dataset/dgm>
   [GMT]: <https://www.generic-mapping-tools.org/>
   [MIT]: <https://opensource.org/licenses/MIT>

