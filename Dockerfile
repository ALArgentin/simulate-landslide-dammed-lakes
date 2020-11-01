# The code was developed on a server under Red Hat Enterprise Linux Server, version 7.6 (Maipo)
FROM ubuntu

# Contact details
LABEL maintainer="anne-laure.argentin@sbg.ac.at"

# Change to the work directory this is an arbitory directory
WORKDIR /app
# Copy the script across syntax COPY LOCAL REMOTE
# The main script
COPY scripts/main.sh .
# The Austrian DEM
COPY scripts/dhm_at_lamb_10m_2018.tif .
# The files required to compile the Rockfalls executable
COPY scripts/rockfalls.cpp .
COPY scripts/track.cpp .
COPY scripts/image.cpp .
# The parametrisation file for Gerris
COPY scripts/Debrisflow.gfs .

# Update repositories and install dependancies 
# Make sure all of this is on the same line otherwise the update is embeded in a different layer
# add unless doing a force rebuild will not systematically be updated
# yum is the Red Hat Enterprise Linux equivalent of apt-get
# -y is needed for the install as `apt install` expects a user prompt
#RUN yum update -y && yum install -y gdal-bin
RUN apt-get update -y && apt-get install -y gdal-bin
RUN apt-get update -y && DEBIAN_FRONTEND=noninteractive apt-get install -y grass
RUN apt-get update -y && DEBIAN_FRONTEND=noninteractive apt-get install -y gmt gmt-dcw gmt-gshhg
RUN apt-get update -y && DEBIAN_FRONTEND=noninteractive apt-get install -y bc
RUN apt-get update -y && DEBIAN_FRONTEND=noninteractive apt-get install -y g++

# Compiling the Rockfalls code
RUN ls
RUN g++ -o /app/rockfalls /app/rockfalls.cpp

# We need to compile Gerris from source code http://gfs.sourceforge.net/wiki/index.php/Installing_from_source
# instead of just doing 'apt-get install -y gerris' to install the required module Terrain
# Dependent packages
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y libglib2.0-dev libnetpbm10-dev m4 libproj-dev \
libgsl0-dev libnetcdf-dev libode-dev libfftw3-dev libhypre-dev libgtkglext1-dev libstartup-notification0-dev ffmpeg

# Compiling and installing GTS
RUN curl -L -O http://gts.sf.net/gts-snapshot.tar.gz
RUN tar -xzf gts-snapshot.tar.gz
RUN cd gts-snapshot-* && ./configure && make && make install #(or just 'make install' if installing locally)
RUN echo "/usr/local/lib" >> /etc/ld.so.conf
RUN cat /etc/ld.so.conf
RUN /sbin/ldconfig

# Compiling and installing Gerris
RUN curl -L -O http://gerris.dalembert.upmc.fr/gerris/gerris-snapshot.tar.gz
RUN tar -xzf gerris-snapshot.tar.gz
# For Gerris in parallel
# The Gerris snapshot uses a old version of OpenMPI and this variable 'MPI_Errhandler_set' has been removed and replaced in MPI-3.0.
RUN sed -i 's/MPI_Errhandler_set/MPI_Comm_set_errhandler/g' ./gerris-snapshot-*/src/init.c
RUN apt-get install -y openmpi-bin libopenmpi-dev
RUN cd gerris-snapshot-* && ./configure && make && make install
RUN /sbin/ldconfig

# Run a command in occurence to make sure the file is executable
RUN chmod +x main.sh

# Set the default command that will be run when a container is spun up
CMD ["/bin/bash", "/app/main.sh"]
