#!/bin/bash
#
# Author: Anne-Laure Argentin
# Purpose: Workflow to compute landslide-dammed lakes.  It uses the Rockfall code from Hergarten (2012)
# to determinate the landslides areas and volumes and Gerris (Popinet, 2003) to simulate the landslide runouts.
# There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# 
# ############################################################################################
#
# Copyright (c) 2020 Anne-Laure Argentin
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
# to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions
# of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
##############################################################################################

out=output_folder/
main=./
lgm_shp=${main}lgm_global_Corrected_only_alps.gmt

nb_cores_parallelization=20

gerris_end=360 # Duration of the Gerris simulation (360/60 => 6 min): "The end identifier specifies that the simulation should stop when the physical time reaches the given value."
# http://gfs.sourceforge.net/tutorial/tutorial/tutorial1.html

cell_size=10

# Buffer: We chose 512 because the refinement level of Gerris divides the map into a 1024x1024 pixels square. So it was the minimum.
buffer=512 # WARNING !!! Make sure this value is coherent with Gerris refinement level (2^10 = 1024 -> 512)
dem_width=$buffer*2 # Nb of pixels in the cut DEMs (squares)
length=$((($buffer*2+1)*$cell_size)) # 2*buffer + 1 x10 with *2 because on both sides of the origin pixel (+1), and *10 because 1 pixel = 10m.

declare -A part1 part2 part3 part4 part5 part6 part7 part8 part9 part10 part11 part12 part13 part14
part1=(["x1"]=107005 ["y1"]=422005 ["x2"]=200255 ["y2"]=324005)
part2=(["x1"]=190005 ["y1"]=418005 ["x2"]=285255 ["y2"]=315005)
part3=(["x1"]=275005 ["y1"]=385255 ["x2"]=365255 ["y2"]=298005)
part4=(["x1"]=275005 ["y1"]=434005 ["x2"]=365255 ["y2"]=375005)

part5=(["x1"]=355005 ["y1"]=385255 ["x2"]=455255 ["y2"]=283005)
part6=(["x1"]=355005 ["y1"]=463255 ["x2"]=455255 ["y2"]=375005)
part7=(["x1"]=350005 ["y1"]=536005 ["x2"]=455255 ["y2"]=453005)

part8=(["x1"]=445005 ["y1"]=385255 ["x2"]=545255 ["y2"]=270005)
part9=(["x1"]=445005 ["y1"]=465255 ["x2"]=545255 ["y2"]=375005)
part10=(["x1"]=445005 ["y1"]=538005 ["x2"]=545255 ["y2"]=455005)

part11=(["x1"]=535005 ["y1"]=385255 ["x2"]=635255 ["y2"]=298005)
part12=(["x1"]=535005 ["y1"]=466255 ["x2"]=635255 ["y2"]=375005)	
part13=(["x1"]=535005 ["y1"]=540005 ["x2"]=639255 ["y2"]=456005)

part14=(["x1"]=625005 ["y1"]=466255 ["x2"]=661005 ["y2"]=355005)


# Because of the pixel/area registration of the grids, beware of choosing numbers finishing with "5".

parts_nb=(1) # 2 3 4 5 6 7 8 9 10 11 12 13 14


landslide_volume_threshold_m3=100000
shoot_density_divider=100 # Using the surface as a proxy for the nb of shoots in Hergarten's code.


#######################################
# Gives info on the grid using gmt grdinfo
# Arguments:
#   $1 : the grid
#######################################
function Info()
{
	local grid=$1
	echo ""
	echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
	gmt grdinfo $grid
	echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
	echo ""
}
export -f Info

#######################################
# Cut one part of the DEM
# Arguments:
#   $1 : xmin
#   $2 : ymax
#   $3 : xmax
#   $4 : ymin
#   $5 : ID number attributed to the part
#######################################
function CutOnePart()
{
	local x1=$1 y1=$2 x2=$3 y2=$4 part_nb=$5
	echo "------------------------------------------------------------------------------------------------------------1 Cut one part of the DEM"
    gdal_translate -projwin $x1 $y1 $x2 $y2 -of GTiff ${main}dhm_at_lamb_10m_2018.tif ${out}dhm_at_lamb_10m_2018_part${part_nb}.tif -co tfw=yes
}

#######################################
# Print the variable in the Computations.txt file.
# If the variable is empty for whatever reason, 
# prints an error message to keep the result order in the file.
# Arguments:
#   $1 : the variable to print
#######################################
function PrintNonEmpty()
{
	local variable=$1
	if [ -z "$variable" ] # If string is empty
	then
		echo "\$variable is empty"
		echo -n "ErrorNoValue " >> ${subdirector}Computations.txt
	else
		echo -n "$variable " >> ${subdirector}Computations.txt
	fi
}

#######################################
# Import the topography, the slope and accumulation that were computed using Topotoolbox in the article and GRASS GIS in this script.
# Arguments:
#   $1 : region extent of the part
#   $2 : ID number attributed to the part
#   $3 : output directory
#######################################
function ImportTopographySlopeAndAccumulation()
{
	local rlocal=$1 jter=$2 directory=$3
	echo "------------------------------------------------------------------------------------------------------------2 Import topo with nodata=0" # From .ztlf to .grd
	gmt grd2xyz $verb ${out}dhm_at_lamb_10m_2018_part${jter}_ttb_filled.tif -ZTLf > ${out}dhm_at_lamb_10m_2018_part${jter}_filled.ztlf
	gmt xyz2grd $verb ${out}dhm_at_lamb_10m_2018_part${jter}_filled.ztlf -G"${directory}DEM_filled_NaNs.grd" $rlocal -I$cell_size -ZTLf -r
	PrintTIFF $directory "DEM_filled_NaNs.grd" dhm_at_lamb_10m_2018_part${jter}_filled.tif
	gmt grdmath $verb ${directory}DEM_filled_NaNs.grd 0 DENAN = ${directory}DEM_filled.grd
	Info ${directory}DEM_filled.grd

	gmt grdconvert $verb ${out}dhm_at_lamb_10m_2018_part${jter}_ttb_grad.tif=gd:"gTiff" ${directory}slope.grd
	gmt grdconvert $verb ${out}dhm_at_lamb_10m_2018_part${jter}_ttb_acc.tif=gd:"gTiff" ${directory}accumulation.grd
}

#######################################
# Launch Hergarten's code. Make sure to FILL YOUR DEM before using Hergarten's code.
# Only the landslides which volumes are above a given threshold are taken into account.  
# Arguments:
#   $1 : xmin
#   $2 : ymax
#   $3 : xmax
#   $4 : ymin
#   $5 : ID number attributed to the part
#   $6 : landslide volume threshold
#######################################
function RockfallsCode()
{
	local x1=$1 y1=$2 x2=$3 y2=$4 part_nb=$5 landslide_volume_threshold_m3=$6
	echo "------------------------------------------------------------------------------------------------------------3 Launch Hergarten's code"
	# Compute the area of the DEM, not including NaN areas from the borders.
    gmt grdmath $verb ${directory}DEM_filled.grd 0 NEQ = ${directory}ones.grd # "1" for all cells not equal to "0" (which were the "NaN" cells).
    local area=$(gmt grd2xyz $verb ${directory}ones.grd -Z | gmt math STDIN ABS SUM = | tail -1) # Sums all the "1" cells (the NaNs were converted to zeros)
	local whole_area=$((($x2-$x1)*($y1-$y2)))

	
	local columns_nb=$((($x2-$x1)/$cell_size))
	local nb_shoots=$(($area/$shoot_density_divider))
	local slope_min=$(echo "1*$cell_size" | bc -l) # Smin = 1 as recommended by Hergarten, 2012, but scaled to the pixel size.
	local slope_max=$(echo "5*$cell_size" | bc -l) # Smax = 5 as recommended by Hergarten, 2012, but scaled to the pixel size.
	local min_elev=0 # The minimum elevation of a triggering landslide for it to be considered. Since we want a homogeneous density of the shooting, we put the threshold to 0.
	local threshold=$(echo "$landslide_volume_threshold_m3/($cell_size^2)" | bc -l) # Hergarten's code threshold has to be scaled to the pixel size.
	echo "Nb of shoots:" $nb_shoots "Area:" $area "Area with NaNs:" $whole_area "m2" "Column nb:" $columns_nb

	# -t is the number of shoots to do, while -n is the number of landslides to SUCESSFULLY trigger (landslide_volume > 0).
	echo "${main}rockfalls ${out}dhm_at_lamb_10m_2018_part${part_nb}_filled.ztlf -c $columns_nb -s $slope_min -u $slope_max -h $min_elev -t ${nb_shoots} -o ${out}/rockfalls_part${part_nb}.csv -p ${out}/rockfalls_part${part_nb} $threshold"
	${main}rockfalls ${out}dhm_at_lamb_10m_2018_part${part_nb}_filled.ztlf -c $columns_nb -s $slope_min -u $slope_max -h $min_elev -t ${nb_shoots} -o ${out}/rockfalls_part${part_nb}.csv -p ${out}/rockfalls_part${part_nb} $threshold

	local nb_landslides=$(wc -l ${out}rockfalls_part${part_nb}.csv | awk '{print $1}')
	echo $nb_landslides "landslides were triggered with the Rocḱfall code."
}

################################################################################################################################################

#######################################
# Convert the GRD into a TIFF, and add the CRS info EPSG:31287 after.
# Nested to be exported together with the "task" function.
# Arguments:
#   $1 : the path to find all files
#   $2 : the input grd file
#   $3 : the output tif file
#######################################
function PrintTIFF()
{
    gmt grdconvert $verb ${1}$2 ${1}temp_nocrs.tif=gd:"gTiff"
    gdal_translate -a_srs EPSG:31287 ${1}temp_nocrs.tif ${1}$3
    rm ${1}temp_nocrs.tif
}
export -f PrintTIFF

#######################################
# Import landslide with nodata=NaN
# Arguments:
#######################################
function ImportLandslide()
{
	local i=$1 name=$2 subdirector=$3 directory=$4 rlocal=$5 x1=$6 y2=$7
    echo "------------------------------------------------------------------------------------------------------------5 Import landslide with nodata=NaN"
    rupture_surface=${directory}${name}.csv
    original_surface=${directory}${name}_original.csv
    echo $rupture_surface

    # Transform the .csv file into a .grd, with nodata values set by default to NaN (NaNs are needed for the grdmath operation AND)
    awk -v cell_size=$cell_size -v minx=$x1 -v miny=$y2 -F"," '{a=($2+0.5)*cell_size+minx; b=($1+0.5)*cell_size+miny; c=$3; print(a, b, c)}' $rupture_surface | # +0.5 because the grid is shifted.
	gmt xyz2grd $verb -G${subdirector}rockfall.grd -I$cell_size $rlocal -r
    awk -v cell_size=$cell_size -v minx=$x1 -v miny=$y2 -F"," '{a=($2+0.5)*cell_size+minx; b=($1+0.5)*cell_size+miny; c=$3; print(a, b, c)}' $original_surface | # +0.5 because the grid is shifted.
	gmt xyz2grd $verb -G${subdirector}rockfall_original.grd -I$cell_size $rlocal -r
	# From grid registered to pixel registered (like the DEM) PROBLEM, Question: creates negative values!!! and if not, the extremums are changed... OR IS IT OK NOW?
	PrintTIFF $subdirector "rockfall_original.grd" rockfall_original.tif

    Info ${subdirector}rockfall.grd
    Info ${subdirector}rockfall_original.grd
}
export -f ImportLandslide

#######################################
# Compute: subtract the rockfall depth
# Arguments:
#######################################
function ComputeDEMsRelatedToLandslideForGerris()
{
	local subdirector=$1 directory=$2
    echo "------------------------------------------------------------------------------------------------------------6 Compute: subtract the rockfall depth"
	PrintTIFF $directory "DEM_filled.grd" DEM_filled.tif
    gmt grdmath $verb ${subdirector}rockfall.grd ${directory}DEM_filled.grd AND = ${subdirector}without_rockfalls_DEM.grd
    gmt grdmath $verb ${directory}DEM_filled.grd ${subdirector}without_rockfalls_DEM.grd SUB = ${subdirector}only_rockfalls_DEM.grd
    gmt grdmath $verb ${subdirector}only_rockfalls_DEM.grd 0 DENAN = ${subdirector}only_rockfalls_DEM.grd
}
export -f ComputeDEMsRelatedToLandslideForGerris

#######################################
# Extract subregions
# Arguments:DEM.grd
#######################################
function ExtractSubregions()
{
	local out=$1 subdirector=$2 directory=$3 range=$4
    echo "------------------------------------------------------------------------------------------------------------8 Extract subregions"
    echo "This landslide is not too close to the borders."
    echo "out:" $out
    echo "subdirector:" $subdirector
    echo "directory:" $directory
    echo "range:" $range
    gmt grdcut $verb ${directory}DEM_filled.grd -G${subdirector}cut_DEM.grd $range
    gmt grdcut $verb ${directory}DEM_filled_NaNs.grd -G${subdirector}cut_DEM_NaNs.grd $range
    gmt grdcut $verb ${directory}slope.grd -G${subdirector}cut_slope.grd $range
    gmt grdcut $verb ${directory}accumulation.grd -G${subdirector}cut_accumulation.grd $range

    gmt grdcut $verb ${subdirector}rockfall.grd -G${subdirector}cut_rockfall.grd $range
    gmt grdcut $verb ${subdirector}without_rockfalls_DEM.grd -G${subdirector}cut_without_rockfalls_DEM.grd $range
    gmt grdcut $verb ${subdirector}only_rockfalls_DEM.grd -G${subdirector}cut_only_rockfalls_DEM.grd $range

    Info ${directory}DEM_filled.grd
	Info ${directory}accumulation.grd
	Info ${subdirector}rockfall.grd
	Info ${subdirector}cut_rockfall.grd
	Info ${subdirector}cut_accumulation.grd
	Info ${subdirector}cut_slope.grd
    Info ${subdirector}cut_only_rockfalls_DEM.grd
}
export -f ExtractSubregions

#######################################
# Create Tiffs
# Arguments:
#   $1 : working subdirectory
#######################################
function CreateTiffs()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------9 Create tifs"
    bash -c "PrintTIFF $subdirector cut_DEM.grd DEM.tif"
    bash -c "PrintTIFF $subdirector cut_without_rockfalls_DEM.grd without_rockfalls_DEM.tif"
    bash -c "PrintTIFF $subdirector cut_only_rockfalls_DEM.grd only_rockfalls_DEM.tif"
}
export -f CreateTiffs

#######################################
# Create XYZ
# Arguments:
#   $1 : working subdirectory
#######################################
function CreateXYZ()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------10 Create XYZ"
    gmt grd2xyz $verb ${subdirector}cut_without_rockfalls_DEM.grd > ${subdirector}without_rockfalls_DEM.xyz
    gmt grd2xyz $verb ${subdirector}cut_only_rockfalls_DEM.grd > ${subdirector}only_rockfalls_DEM.xyz
}
export -f CreateXYZ

#######################################
# Create KDT
# Arguments:
#   $1 : working subdirectory
#######################################
function CreateKDT()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------11 Create KDT"
    xyz2kdt ${subdirector}without_rockfalls_DEM < ${subdirector}without_rockfalls_DEM.xyz
    xyz2kdt ${subdirector}only_rockfalls_DEM < ${subdirector}only_rockfalls_DEM.xyz
}
export -f CreateKDT

#######################################
# Launch Gerris
# Arguments:
#######################################
function LaunchGerris()
{
	local subdirector=$1 minx=$2 miny=$3 length=$4 out=$5 name=$6 gerris_end=$7
    echo "------------------------------------------------------------------------------------------------------------12 Launch Gerris"
    mkdir ${subdirector}ASC
    mkdir ${subdirector}VTK
    echo minx: $minx miny: $miny length: $length gerris_end: $gerris_end subdirector: $subdirector
    # BEWARE of NaN values, and of grid refinement level < study area width.
    # gfsview2D cannot be used without display
    gerris2D -m ${main}Debrisflow.gfs -v --define=PATH=$subdirector --define=TX=$minx --define=TY=$miny --define=LENGTH=$length --define=END=$gerris_end #| gfsview2D
}
export -f LaunchGerris

#######################################
# Resample Gerris output: the ASCII dam height file
# Arguments:
#######################################
function ResampleGerrisOutput()
{
	local subdirector=$1 range=$2
    echo "------------------------------------------------------------------------------------------------------------13 Resample the ASCII dam height file"
	# If the script doesn't go through this following line try stopping it, deleting gmt.history and launching it again.
    tail -n +7 ${subdirector}ASC/FlowDepth_${gerris_end}.asc | gmt xyz2grd $verb -di-9999 -G${subdirector}dam_height.grd -I$cell_size $range -ZTLA -r 
	bash -c "PrintTIFF $subdirector dam_height.grd dam_height.tif"
    # Question: Make sure those pb disappeared. PB 1 : plein d'artefacts, PB 2 : Tous les .asc ont des bandes noires sur les côtés
	# Question: Seems to present problems here sometimes, WHY?!!! WHAT'S HAPPENING? Because the dam is on top of the boundary??
}
export -f ResampleGerrisOutput

#######################################
# All landslide deposits which ended up outside Austria are erased to not disturb the water flow and the deposition height values of deposits IN Austria. 
# Arguments:
#   $1 : working subdirectory
#######################################
function GetRidOfDepositsOutsideAustria()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------prior-14 Get rid of deposits outside Austria"
	# Assert the dam / landslide is not in the NaN parts of the DEM.
	gmt grdmath 1 ${subdirector}cut_DEM_NaNs.grd ISNAN SUB = ${subdirector}DEM_mask.grd # 1 inside Autria, 0 outside
    Info ${subdirector}dam_height.grd
    gmt grdmath $verb ${subdirector}dam_height.grd ${subdirector}DEM_mask.grd MUL = ${subdirector}dam_height_austria.grd # No NaNs, but 0s.
}
export -f GetRidOfDepositsOutsideAustria

#######################################
# Add the dam to the topography
# Arguments:
#   $1 : working subdirectory
#######################################
function AddDamToTopo()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------14 Add dam to the topography"
    Info ${subdirector}dam_height_austria.grd
    gmt grdmath $verb ${subdirector}dam_height_austria.grd ${subdirector}cut_without_rockfalls_DEM.grd ADD = ${subdirector}final_DEM.grd # No NaNs, but 0s.
    bash -c "PrintTIFF $subdirector final_DEM.grd final_DEM.tif"
}
export -f AddDamToTopo

#######################################
# Fill the topography that now comprises the landslide dam deposits to create the landslide-dammed lake.
# Arguments:
#######################################
function FillDammedLake()
{
	local subdirector=$1 minx=$2 maxx=$3 miny=$4 maxy=$5 name=$6
    echo "------------------------------------------------------------------------------------------------------------15 Fill the dammed lake"
    # In Docker, we have to reclaim the ownership of the GRASS Mapset.
    me=$(whoami)
    chown -R $me ${out}PERMANENT/PERMANENT/
    echo "is who I am _______________________________"
    grass ${out}PERMANENT/PERMANENT/ --exec g.mapset PERMANENT            
    grass ${out}PERMANENT/PERMANENT/ --exec g.proj georef=${subdirector}final_DEM.tif -c
    grass ${out}PERMANENT/PERMANENT/ --exec g.region w=$minx e=$maxx s=$miny n=$maxy

	# Import
    grass ${out}PERMANENT/PERMANENT/ --exec r.in.gdal --verbose --overwrite input=${subdirector}final_DEM.tif output=${name}_init_mosaik_dem
	grass ${out}PERMANENT/PERMANENT/ --exec r.info --verbose map=${name}_init_mosaik_dem
    
	# Fill the DEM with landslide deposits, export flow direction file and filled DEM.
	grass ${out}PERMANENT/PERMANENT/ --exec r.terraflow -s --verbose --overwrite elevation=${name}_init_mosaik_dem filled=${name}_filled direction=${name}_direction
    grass ${out}PERMANENT/PERMANENT/ --exec r.info --verbose map=${name}_filled
    grass ${out}PERMANENT/PERMANENT/ --exec r.out.gdal --overwrite input=${name}_direction output=${subdirector}final_DEM_direction.tif format=GTiff
    grass ${out}PERMANENT/PERMANENT/ --exec r.out.gdal --overwrite input=${name}_filled output=${subdirector}final_DEM_filled.tif format=GTiff
}
export -f FillDammedLake

#######################################
# Compute the lake grid, then the lake volume.
# Arguments:
#   $1 : working subdirectory
#######################################
function ComputeLakeVolume()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------16 Compute the lake grid, then the lake volume in m3"
	header+="| 1. Lake volume (m3) "
    gmt grdconvert $verb ${subdirector}final_DEM_filled.tif=gd:"gTiff" ${subdirector}final_DEM_filled.grd
    gmt grdmath $verb ${subdirector}final_DEM_filled.grd ${subdirector}final_DEM.grd SUB = ${subdirector}lake.grd
	bash -c "PrintTIFF $subdirector lake.grd lake.tif"

    lake_vol=$(gmt grd2xyz $verb ${subdirector}lake.grd -Z | gmt math STDIN ABS SUM = | tail -1)
	lake_vol=$(echo "$lake_vol*$cell_size*$cell_size" | bc -l) # Pixel*Meter to Meter³
	echo "lake volume:" $lake_vol
	PrintNonEmpty $lake_vol

	rm ${subdirector}final_DEM_filled.grd ${subdirector}final_DEM.grd
}
export -f ComputeLakeVolume

#######################################
# Computes the position of the deposits (the maximum height). Creates a grid to be used to find the slope in the process.
# Arguments:
#   $1 : working subdirectory
#######################################
function ComputeDepositsPosition()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------17 Compute the deposits position and height (x y z (m))"
	header+="| 2,3,4. Deposits position and height (x y z (m)) "
	deposits_height=$(gmt grd2xyz $verb ${subdirector}dam_height_austria.grd -Z | gmt math STDIN UPPER = | tail -1)
	echo "Deposits height:" $deposits_height 

	gmt grdmath $verb ${subdirector}dam_height_austria.grd DUP EXTREMA 2 EQ MUL $deposits_height EQ = ${subdirector}dam_center.grd # The pixel with highest dam height == 1, the rest == 0. Used afterward for the slope.
	gmt grdmath $verb ${subdirector}dam_center.grd 0 NAN $deposits_height MUL = temp.nc # The pixel with highest value == highest value, the rest == NaN.
	gmt grd2xyz temp.nc -s | xargs echo
	deposits_x=$(gmt grd2xyz temp.nc -s | awk '{print $1}')
	deposits_y=$(gmt grd2xyz temp.nc -s | awk '{print $2}')
	deposits_z=$(gmt grd2xyz temp.nc -s | awk '{print $3}')
	PrintNonEmpty $deposits_x
	PrintNonEmpty $deposits_y
	PrintNonEmpty $deposits_z

	rm temp.nc
}
export -f ComputeDepositsPosition

#######################################
# Compute the dam volume.
# Arguments:
#   $1 : working subdirectory
#   $2 : landslide source volume, as computed by the code from Hergarten
#######################################
function ComputeDepositsVolume()
{
	local subdirector=$1 hergarten_landslide_volume=$2
    echo "------------------------------------------------------------------------------------------------------------18 Compute the dam volume in m3 (DBI, BI, MOI - Vlandslide==Vdam, HDSI)"
	header+="| 5,6,7,8. Landslide volume (m3)(Rockfall code, before Gerris, after Gerris, after Gerris in Austria only) "
	before_gerris_landslide_vol=$(gmt grd2xyz $verb ${subdirector}only_rockfalls_DEM.grd -Z | gmt math STDIN ABS SUM 100 MUL = | tail -1)
	after_gerris_landslide_vol=$(gmt grd2xyz $verb ${subdirector}dam_height.grd -Z | gmt math STDIN ABS SUM 100 MUL = | tail -1)
	after_gerris_austria_landslide_vol=$(gmt grd2xyz $verb ${subdirector}dam_height_austria.grd -Z | gmt math STDIN ABS SUM 100 MUL = | tail -1)

    PrintNonEmpty $hergarten_landslide_volume
    PrintNonEmpty $before_gerris_landslide_vol
    PrintNonEmpty $after_gerris_landslide_vol
    PrintNonEmpty $after_gerris_austria_landslide_vol

	header+="| 9,10,11. Landslide/deposits volume differences (%) (Rockfall/Before Gerris, Before/After Gerris, All/Inside Austria after Gerris) "
	percentage=100 # To convert to %
	diff1=$(echo "(($hergarten_landslide_volume - $before_gerris_landslide_vol) / $hergarten_landslide_volume) *$percentage" | bc -l)
	diff2=$(echo "(($before_gerris_landslide_vol - $after_gerris_landslide_vol) / $after_gerris_landslide_vol) *$percentage" | bc -l)
	diff3=$(echo "(($after_gerris_landslide_vol - $after_gerris_austria_landslide_vol) / $after_gerris_austria_landslide_vol) *$percentage" | bc -l)
    PrintNonEmpty $diff1
    PrintNonEmpty $diff2
    PrintNonEmpty $diff3

	#Do not remove unless you want to recompute the last step: ${subdirector}only_rockfalls_DEM.grd
}
export -f ComputeDepositsVolume

#######################################
# Compute the upstream catchment area.
# Arguments:
#   $1 : working subdirectory
#######################################
function ComputeUpstreamArea()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------19 Compute the upstream catchment area in km2 (for use in DBI, BI, HDSI)"
	header+="| 12. Upstream area at deposits max height [USED] (km2) "
    gmt grdmath $verb ${subdirector}dam_height_austria.grd 0 GT = ${subdirector}dam_mask.grd # 1 where dam, 0 elsewhere
    gmt grdmath $verb ${subdirector}dam_mask.grd ${subdirector}cut_accumulation.grd MUL = ${subdirector}masked_accumulation.grd # Accumulation values where dam, 0 elsewhere

	m2_to_km2=1000000
	upstream_flow_accumulation=$(gmt grd2xyz $verb ${subdirector}masked_accumulation.grd -Z | gmt math STDIN UPPER = | tail -1) # Maximum of the accumulation values covered by the dam
	upstream_area=$(echo "$upstream_flow_accumulation*$cell_size*$cell_size/$m2_to_km2" | bc -l) # Pixel to Meter² to Kilometer²
    PrintNonEmpty $upstream_area

	#Do not remove unless you want to recompute the last step: ${subdirector}cut_accumulation.grd ${subdirector}masked_accumulation.grd ${subdirector}dam_height_austria.grd
}
export -f ComputeUpstreamArea

#######################################
# Compute the blockage index and dimensionless blockage index (BI and DBI).
# Arguments:
#   $1 : working subdirectory
#######################################
function ComputeBlockageIndexes()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------20 Compute the blockage index and dimensionless blockage index (BI and DBI)"
	header+="| 13,14,15. Blockage Index, Dimensionless Blockage Index, Impoundment Index "
	echo $landslide_volume $upstream_area $deposits_height
	blockage_index=$(echo "l($after_gerris_austria_landslide_vol/$upstream_area)/l(10)" | bc -l)
	dimensionless_blockage_index=$(echo "l(($upstream_area*$deposits_height)/$after_gerris_austria_landslide_vol)/l(10)" | bc -l)
	if [ $lake_vol -eq 0 ]; then 
		impoundment_index="NoValueSinceLakeEmpty"
	else
		impoundment_index=$(echo "l($after_gerris_austria_landslide_vol/$lake_vol)/l(10)" | bc -l)
	fi
    PrintNonEmpty $blockage_index
    PrintNonEmpty $dimensionless_blockage_index
    PrintNonEmpty $impoundment_index
}
export -f ComputeBlockageIndexes

#######################################
# Determine which lithology there is at the dam area.
# Arguments:
#   $1 : working subdirectory
#######################################
function DetermineLithologyAtDepositsArea()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------21 Determine lithology at deposits area"
	header+="| 16. DepositsAreaLithologyNb "
	> litho_test.txt # Clear file
	# For each LithUnits gmt file, ...
	files=$(ls -1 ${main}LithUnits_*.gmt)
	for file in $files; do
		echo $file
		echo "$deposits_x $deposits_y"
		echo "$deposits_x $deposits_y" | gmt spatial $verb -N${file}+z -aZ=LithNr -o2 >> litho_test.txt
		# ABSOLUTELY make sure that the attribute "LithNr" is indicated as a "integer/double" and not a "string" in the "LithUnits_*.gmt" file!!!
	done
	local lithology=$(awk 'BEGIN {max = -inf} { if ($1 > max) {max = $1; line = $0} else if ($1 > 0) { print "Another lithology detected:", $1 } } END { print line }' litho_test.txt)
    PrintNonEmpty $lithology
}
export -f DetermineLithologyAtDepositsArea

#######################################
# Retrieve the release area centroid from the correct file.
# Dumb reverse computation. I should have saved the exact coordinates.
# Arguments:
#######################################
function RetrieveReleaseAreaCentroid()
{
	local subdirector=$1 minx=$2 maxx=$3 miny=$4 maxy=$5
    echo "------------------------------------------------------------------------------------------------------------22 Retrieve release area centroid"
	header+="| 17,18. ReleaseAreaCentroidCoordinates "
	release_x=$(bc <<< "( $minx + $maxx ) / 2")
	release_y=$(bc <<< "( $miny + $maxy ) / 2")
    PrintNonEmpty $release_x
    PrintNonEmpty $release_y
}
export -f RetrieveReleaseAreaCentroid

#######################################
# Determine which lithology there is at the release area.
# Arguments:
#   $1 : working subdirectory
#######################################
function DetermineLithologyAtReleaseArea()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------23 Determine lithology at release area"
	header+="| 19. ReleaseAreaLithologyNb "
	> litho_test.txt # Clear file
	# For each LithUnits gmt file, ...
	files=$(ls -1 ${main}LithUnits_*.gmt)
	for file in $files; do
		echo $file
		echo "$release_x $release_y"
		echo "$release_x $release_y" | gmt spatial $verb -N${file}+z -aZ=LithNr -o2 >> litho_test.txt
		# ABSOLUTELY make sure that the attribute "LithNr" is indicated as a "integer/double" and not a "string" in the "LithUnits_*.gmt" file!!!
	done
	local lithology=$(awk 'BEGIN {max = -inf} { if ($1 > max) {max = $1; line = $0} else if ($1 > 0) { print "Another lithology detected:", $1 } } END { print line }' litho_test.txt)
    PrintNonEmpty $lithology
}
export -f DetermineLithologyAtReleaseArea

#######################################
# Determine if the local landscape was glacially imprinted or not.
# Notes: The LGM file was modified so that the 1rst polygon has its ID now refering to 301 and not 0 anymore (there are 301 polygons in the file).
# We did this because the points not included in any polygons return 0 to gmt spatial
# Arguments:
#   $1 : working subdirectory
#######################################
function DetermineIfGlaciallyImprinted()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------24 Determine glacial imprint at deposits area"
	header+="| 20. GlaciallyImprinted "
	# If the polygon ID is bigger than 0, it means the point is contained by a polygon.
	lgm=$(echo "$deposits_x $deposits_y" | gmt spatial $verb -N${lgm_shp}+z -aZ=FID -o2 | awk '{ if ($1 > 0){ print 1 } else { print 0 } }')
    PrintNonEmpty $lgm
}
export -f DetermineIfGlaciallyImprinted

#######################################
# Retrieve the local slope at the deposits maximum height.
# Arguments:
#   $1 : working subdirectory
#######################################
function ComputeMaxDepositsPositionSlope()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------25 Fetch the slope at the maximum deposits position in m/m (HDSI)"
	header+="| 21. Max deposits slope (m/m) "
	gmt grdmath $verb ${subdirector}dam_center.grd 0 NAN ${subdirector}cut_slope.grd MUL = slope.nc # The pixel with highest value == highest value, the rest == NaN.
	gmt grd2xyz slope.nc -s | xargs echo
	local max_deposits_slope=$(gmt grd2xyz slope.nc -s | awk '{print $3}')
	PrintNonEmpty $max_deposits_slope
	#rm ${subdirector}dam_center.grd ${subdirector}cut_slope.grd slope.nc
}
export -f ComputeMaxDepositsPositionSlope

#######################################
# Compute the Backstow Index Is, Basin Index Ia, and Relief Index Ir, from Korup, 2004.
# Arguments:
#   $1 : working subdirectory
#######################################
function ComputeKorupIndexesIsIa()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------26 Compute the Korup indexes Is & Ia"
	header+="| 22, 23. Backstow Index Is & Basin Index Ia "
	echo $upstream_area $deposits_height $lake_vol
	if [ $lake_vol -eq 0 ]; then 
		Is="NoValueSinceLakeEmpty"
	else
		Is=$(echo "l(($deposits_height^3)/$lake_vol)/l(10)" | bc -l)
	fi
	
	Ia=$(echo "l(($deposits_height^2)/$upstream_area)/l(10)" | bc -l)
	PrintNonEmpty $Is
	PrintNonEmpty $Ia
}
export -f ComputeKorupIndexesIsIa

#######################################
# Computes the extent of the dam in the subregion to make sure the subregion is big enough.
# Arguments:
#   $1 : working subdirectory
#######################################
function ComputeDepositsExtent()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------27 Compute the deposits extents"
	header+="| 24,25,26,27,28,29. Deposits extent (xmin xmax ymin ymax), pixels in NaN part of DEM, and valid (1) or invalid (0) "
	ComputeAndAssertDepositsOrLakeExtent ${subdirector} dam_mask.grd
}
export -f ComputeDepositsExtent

#######################################
# Computes the extent of the lake in the subregion to make sure the subregion is big enough.
# Arguments:
#   $1 : working subdirectory
#######################################
function ComputeLakeExtent()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------28 Compute the lake extents"
	header+="| 30,31,32,33,34,35. Lake extent (xmin xmax ymin ymax), pixels in NaN part of DEM, and valid (1) or invalid (0) "
    gmt grdmath $verb ${subdirector}lake.grd 0 GT = ${subdirector}lake_mask.grd # 1 where lake, 0 elsewhere
	ComputeAndAssertDepositsOrLakeExtent ${subdirector} lake_mask.grd
}
export -f ComputeLakeExtent

#######################################
# Computes the extent of the dam/lake in the subregion and makes sure the subregion is big enough.
# Arguments:
#   $1 : working subdirectory
#   $2 : mask of the landslide deposits or lake extents
#######################################
function ComputeAndAssertDepositsOrLakeExtent()
{
	local subdirector=$1 dam_or_lake_mask=$2
	directory=${subdirector%${name}*}
	echo $subdirector $dam_or_lake_mask $directory

	gmt grdmath $verb ${subdirector}${dam_or_lake_mask} 0 NAN = ${subdirector}dam_or_lake_mask_nan.grd # 1 where dam/lake, NaN elsewhere
	gmt grdmath $verb ${subdirector}dam_or_lake_mask_nan.grd XCOL MUL = ${subdirector}temp.grd
	xmin=$(gmt grd2xyz $verb ${subdirector}temp.grd -Z | gmt math STDIN LOWER = | tail -1)
	xmax=$(gmt grd2xyz $verb ${subdirector}temp.grd -Z | gmt math STDIN UPPER = | tail -1)
	gmt grdmath $verb ${subdirector}dam_or_lake_mask_nan.grd YROW MUL = ${subdirector}temp.grd
	ymin=$(gmt grd2xyz $verb ${subdirector}temp.grd -Z | gmt math STDIN LOWER = | tail -1)
	ymax=$(gmt grd2xyz $verb ${subdirector}temp.grd -Z | gmt math STDIN UPPER = | tail -1)

	# Assert the dam / landslide is not in the NaN parts of the DEM.
	gmt grdmath ${subdirector}cut_DEM_NaNs.grd ISNAN ${subdirector}${dam_or_lake_mask} MUL = ${subdirector}intersection.grd
	# Intersection of Lake/Landslide and the NaNs of the DEM
	incorrect_pixels=$(gmt grd2xyz $verb ${subdirector}intersection.grd -Z | gmt math STDIN ABS SUM = | tail -1) # Number of incorrect pixels

	# Assert the dam / landslide is not in contact with the Gerris borders.
	max=$(echo "$dem_width - 1" | bc -l) # DEMs pixels range from (0) to ($dem_width - 1).
	# So if one of the extents of the dams/lake is lower or equal to 0, or greater or equal to ($dem_width - 1), the DEM extent is not enough.
	echo $xmin $ymin $xmax $ymax $max $incorrect_pixels
	if [ $xmin -gt 0 ] || [ $ymin -gt 0 ] || [ $xmax -lt $max ] || [ $ymax -lt $max ] || [ $incorrect_pixels -eq 0 ]; then 
		assert=1
	else
		assert=0 # No good!
	fi

	PrintNonEmpty $xmin
	PrintNonEmpty $xmax
	PrintNonEmpty $ymin
	PrintNonEmpty $ymax
	PrintNonEmpty $incorrect_pixels
	PrintNonEmpty $assert
	
	rm ${subdirector}dam_or_lake_mask_nan.grd ${subdirector}temp.grd
}
export -f ComputeAndAssertDepositsOrLakeExtent

#######################################
# If the rockfall surface is higher than the topography, compute the number of affected pixels.
# Arguments:
#   $1 : working subdirectory
#   $2 : name of the landslide, which is a number, in fact...
#######################################
function ComputeIfIncorrectRuptureSurface()
{
	local subdirector=$1 name=$2
    echo "------------------------------------------------------------------------------------------------------------29 Check if the rupture surface is correct"
	header+="| 36. Nb of incorrect pixels "
	directory=${subdirector%${name}*}
	temp=${directory%/}
	part_nb=${temp##*results_part}
	echo "$part_nb $name $directory"
    gmt grdmath $verb ${subdirector}rockfall.grd ${directory}DEM_filled.grd GT = ${subdirector}ones.grd # 1 for all incorrect places where rupture surface > topography.
    incorrect_pixels=$(gmt grd2xyz $verb ${subdirector}ones.grd -Z | gmt math STDIN ABS SUM = | tail -1) # Number of pixels where rupture surface > topography
	PrintNonEmpty $incorrect_pixels

	#rm ${subdirector}rockfall.grd ${subdirector}ones.grd # DO NOT DELETE IF YOU WANT TO RUN IT SEVERAL TIMES
}
export -f ComputeIfIncorrectRuptureSurface

#######################################
# Computes the position of the deposits with highest accumulation (so the most downstream pixel of the dam).
# Arguments:
#   $1 : working subdirectory
#######################################
function ComputeMostDownstreamDepositsPosition()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------30 Compute the most downstream deposits position (x y z (m)) and slope (m/m)"
	header+="| 37,38,39,40,41. Most downstream deposits position (x y accumulation height (m) slope [USED] (m/m)) "
	gmt grdmath $verb ${subdirector}masked_accumulation.grd DUP EXTREMA 2 EQ MUL $upstream_flow_accumulation EQ = ${subdirector}dam_center2.grd # The pixel with highest dam height == 1, the rest == 0. Used afterward for the slope.
	gmt grdmath $verb ${subdirector}dam_center2.grd 0 NAN $upstream_flow_accumulation MUL = temp.nc # The pixel with highest value == highest value, the rest == NaN.
	echo 'X, Y and Accumulation at most downstream deposits pixel:'
	gmt grd2xyz temp.nc -s | xargs echo
	local most_downstream_deposits_x=$(gmt grd2xyz temp.nc -s | awk '{print $1}')
	local most_downstream_deposits_y=$(gmt grd2xyz temp.nc -s | awk '{print $2}')
	local most_downstream_deposits_accu=$(gmt grd2xyz temp.nc -s | awk '{print $3}')
	PrintNonEmpty $most_downstream_deposits_x
	PrintNonEmpty $most_downstream_deposits_y
	PrintNonEmpty $most_downstream_deposits_accu

	gmt grdmath $verb ${subdirector}dam_center2.grd 0 NAN ${subdirector}dam_height_austria.grd MUL = temp.nc # The pixel with highest value == highest value, the rest == NaN.
	echo 'Deposits height at most downstream deposits pixel:'
	gmt grd2xyz temp.nc -s | xargs echo
	local most_downstream_deposits_z=$(gmt grd2xyz temp.nc -s | awk '{print $3}')
	PrintNonEmpty $most_downstream_deposits_z

	gmt grdmath $verb ${subdirector}dam_center2.grd 0 NAN ${subdirector}cut_slope.grd MUL = temp.nc # The pixel with highest value == highest value, the rest == NaN.
	echo 'Slope at most downstream deposits pixel:'
	gmt grd2xyz temp.nc -s | xargs echo
	most_downstream_deposits_slope=$(gmt grd2xyz temp.nc -s | awk '{print $3}')
	PrintNonEmpty $most_downstream_deposits_slope

	rm temp.nc
}
export -f ComputeMostDownstreamDepositsPosition

#######################################
# Compute the Hydromorphological Dam Stability Index (HDSI).
# Arguments:
#   $1 : working subdirectory
#######################################
function ComputeHDSI()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------31 Compute the HDSI"
	header+="| 42. HDSI "
	echo $after_gerris_austria_landslide_vol $upstream_area $most_downstream_deposits_slope
	if [ $most_downstream_deposits_slope -eq 0 ]; then    # Cannot divide by 0
		hdsi=1000
	else 
		hdsi=$(echo "l($after_gerris_austria_landslide_vol/($upstream_area*$most_downstream_deposits_slope))/l(10)" | bc -l)
	fi
	PrintNonEmpty $hdsi
}
export -f ComputeHDSI

#######################################
# Compute the lake depth.
# Arguments:
#   $1 : working subdirectory
#######################################
function ComputeLakeDepth()
{
	local subdirector=$1
    echo "------------------------------------------------------------------------------------------------------------32 Compute the lake depth in m (DBI)"
	header+="| 43. Lake depth (m) "
	local lake_depth=$(gmt grd2xyz $verb ${subdirector}lake.grd -Z | gmt math STDIN UPPER = | tail -1)
	echo "Lake depth:" $lake_depth 
    PrintNonEmpty $lake_depth
}
export -f ComputeLakeDepth

#######################################
# Process one landslide
# Arguments:
#######################################
function task()
{
    local jter=$1 name=$2 rlocal=$3 lon=$4 lat=$5 buffer=$6 out=$7 subdirector=$8 directory=$9 rockfalls=${10} x1=${11} x2=${12} y1=${13} y2=${14} landslide_volume=${15}

	ImportLandslide $jter $name $subdirector $rockfalls $rlocal $x1 $y2
	ComputeDEMsRelatedToLandslideForGerris $subdirector $directory

    echo "------------------------------------------------------------------------------------------------------------7 Compute extent subregions"
    local x=$(bc -l <<< "($lon*$cell_size) + $x1") # x, y are cell-centered.
    local y=$(bc -l <<< "($lat*$cell_size) + $y2")
    echo $x " " $y
	# Dividing in bc with 1 allows to get the floor() of the value. (Only works if the -l option is not used)
    local minx=$(bc <<< "($x - ($buffer*$cell_size)) / 1")
    local miny=$(bc <<< "($y - ($buffer*$cell_size)) / 1")
    local maxx=$(bc <<< "($x + ($buffer*$cell_size)) / 1")
    local maxy=$(bc <<< "($y + ($buffer*$cell_size)) / 1")
	echo $minx $miny $maxx $maxy
    local range=-R$minx/$maxx/$miny/$maxy

    if (( $minx > $x1 )) && (( $miny > $y2 )) && (( $maxx < $x2 )) && (( $maxy < $y1 )); # The landslides close to the borders are not modelised. 
    then
		echo $subdirector $minx $maxx $miny $maxy $name $landslide_volume >> ${out}before_gerris.txt

		ExtractSubregions $out $subdirector $directory $range
		CreateTiffs $subdirector
		CreateXYZ $subdirector
		CreateKDT $subdirector
    fi
}
export -f task

#######################################
# The first step of this script: 
# Cuts the DEM into several parts sufficiently small to be processed.
# Arguments: GLOBAL variables
#######################################
function Workflow1()
{
	out=${out}/
    echo $out
    pwd
	> ${out}alpine_parts_extensions.txt # Clear file
	echo $length >> ${out}alpine_parts_extensions.txt # We use this file to control the extensions of the different parts (they have to overlap sufficiently).

 	for jter in "${parts_nb[@]}"
	do
		local x1=part$jter["x1"] x2=part$jter["x2"] y1=part$jter["y1"] y2=part$jter["y2"]
		local rlocal="-R${!x1}/${!x2}/${!y2}/${!y1}"

		echo ${!x1} ${!x2} ${!y1} ${!y2} $jter >> ${out}alpine_parts_extensions.txt
		echo "------------------------------------------------------------------------------------------------------------Part $jter, region = $rlocal"
		
		CutOnePart ${!x1} ${!y1} ${!x2} ${!y2} $jter
	done
}

#######################################
#######################################
function FillWithGRASSGIS()
{
 	for jter in "${parts_nb[@]}"
	do
        grass -c ${out}dhm_at_lamb_10m_2018_part${jter}.tif ${out}PERMANENT/
		echo "1 -"
		grass ${out}PERMANENT/PERMANENT/ --exec g.proj georef=${out}dhm_at_lamb_10m_2018_part${jter}.tif -c
		echo "2 -"
		grass ${out}PERMANENT/PERMANENT/ --exec r.in.gdal --verbose --overwrite input=${out}dhm_at_lamb_10m_2018_part${jter}.tif output=init_mosaik_dem
		echo "3 -"
		grass ${out}PERMANENT/PERMANENT/ --exec g.region raster=init_mosaik_dem
		grass ${out}PERMANENT/PERMANENT/ --exec r.terraflow -s --verbose --overwrite elevation=init_mosaik_dem filled=filled accumulation=accumulation
		grass ${out}PERMANENT/PERMANENT/ --exec r.out.gdal --overwrite input=accumulation output=${out}dhm_at_lamb_10m_2018_part${jter}_ttb_acc.tif format=GTiff
		grass ${out}PERMANENT/PERMANENT/ --exec r.out.gdal --overwrite input=filled output=${out}dhm_at_lamb_10m_2018_part${jter}_ttb_filled.tif format=GTiff
		echo "4 -"
		grass ${out}PERMANENT/PERMANENT/ --exec r.slope.aspect --verbose --overwrite elevation=init_mosaik_dem slope=slope format=percent
		grass ${out}PERMANENT/PERMANENT/ --exec r.out.gdal --overwrite input=slope output=${out}dhm_at_lamb_10m_2018_part${jter}_ttb_grad.tif format=GTiff
	done
}

#######################################
# The second step of this script: 
# The DEM parts have to be filled with Topotoolbox between the first and this second step.
# This step launches the rockfall code from Hergarten, and then converts the output into data usable by Gerris for landslide simulation.
# Arguments: GLOBAL variables
#######################################
function Workflow2()
{
	> ${out}before_gerris.txt # Clear file
 	for jter in "${parts_nb[@]}"
	do
		local x1=part$jter["x1"] x2=part$jter["x2"] y1=part$jter["y1"] y2=part$jter["y2"]
		local rlocal="-R${!x1}/${!x2}/${!y2}/${!y1}"
		local rockfalls=${out}rockfalls_part${jter}/
		local directory=${out}results_part${jter}/

		mkdir $directory

		ImportTopographySlopeAndAccumulation $rlocal $jter $directory
		RockfallsCode ${!x1} ${!y1} ${!x2} ${!y2} $jter $landslide_volume_threshold_m3

		echo "------------------------------------------------------------------------------------------------------------4 Select big landslides"
		while read document; do
			IFS=' ' read -r -a line <<< "$document"
			local nb=${line[0]} lon=${line[1]%.*} lat=${line[2]%.*} landslide_volume=${line[5]}
			landslide_volume=$(echo "$landslide_volume*$cell_size*$cell_size" | bc -l) # Pixel*Meter to Meter³
			echo lon, lat: "$lon," "$lat," nb: "$nb," "landslide volume (m³)": "$landslide_volume"

			if (( $(bc -l <<<"$landslide_volume >= $landslide_volume_threshold_m3") )); # The landslides under 0.001 km3 in volume are not modelled (N.B.: For the Alps DEM, 1 Pixel: 10m horiz., 1m vert., so 100m3 volume). 
			then
				local subdirector=${out}results_part${jter}/$nb/
				mkdir $subdirector
				echo "task $jter $nb $rlocal $lon $lat $buffer $out $subdirector $directory $rockfalls ${!x1} ${!x2} ${!y1} ${!y2} $landslide_volume"
				task $jter $nb $rlocal $lon $lat $buffer $out $subdirector $directory $rockfalls ${!x1} ${!x2} ${!y1} ${!y2} $landslide_volume #> ${subdirector}log.out &
			else
				echo "Landslide not voluminous enough. Threshold = $landslide_volume_threshold_m3."
			fi
		done <${out}rockfalls_part${jter}.csv
	done
}

#######################################
# The third step of this script: 
# Arguments: GLOBAL variables
#######################################
function GerrisParallel()
{
	awk 'NR == 1 || NR % 8 == 0 {printf "%s\n", $0}' ${out}before_gerris.txt > ${out}after_gerris.txt # Select randomly the landslides I will effectively simulate.
	while read document; do
		IFS=' ' read -r -a line <<< "$document"
		local subdirector=${line[0]} minx=${line[1]} miny=${line[3]} name=${line[5]}
		echo $subdirector
		((i=i%$nb_cores_parallelization)); ((i++==0)) && wait
		#nohup bash -c "LaunchGerris $subdirector $minx $miny $length $out $name $gerris_end" > ${subdirector}log.out &
		echo "LaunchGerris $subdirector $minx $miny $length $out $name $gerris_end"
		LaunchGerris $subdirector $minx $miny $length $out $name $gerris_end
	done <${out}after_gerris.txt
	echo ${out}after_gerris.txt
}

#######################################
# The fourth step of this script: 
# WARNING: You have to launch this step in the GRASS GIS terminal (use "grass76 -text")
# Arguments: GLOBAL variables
#######################################
function FillDamsWithGRASSGIS()
{
	while read document; do
		IFS=' ' read -r -a line <<< "$document"
		local subdirector=${line[0]} minx=${line[1]} maxx=${line[2]} miny=${line[3]} maxy=${line[4]} name=${line[5]}
		echo $subdirector
		local range=-R$minx/$maxx/$miny/$maxy

		ResampleGerrisOutput $subdirector $range
		GetRidOfDepositsOutsideAustria $subdirector
		AddDamToTopo $subdirector
		FillDammedLake $subdirector $minx $maxx $miny $maxy $name # Uses GRASS, not parallelizable.
		
	done <${out}after_gerris.txt
}

#######################################
# The fifth and last step of this script: 
# Arguments: GLOBAL variables
#######################################
function Computations()
{
	while read document; do
		IFS=' ' read -r -a line <<< "$document"
		local subdirector=${line[0]} minx=${line[1]} maxx=${line[2]} miny=${line[3]} maxy=${line[4]} name=${line[5]} landslide_volume=${line[6]}
		> ${subdirector}Computations.txt # Clear file
		header="\n"
		ComputeLakeVolume $subdirector
		ComputeDepositsPosition $subdirector
		ComputeDepositsVolume $subdirector $landslide_volume
		ComputeUpstreamArea $subdirector
		ComputeBlockageIndexes $subdirector
		DetermineLithologyAtDepositsArea $subdirector
		RetrieveReleaseAreaCentroid $subdirector $minx $maxx $miny $maxy
		DetermineLithologyAtReleaseArea $subdirector
		DetermineIfGlaciallyImprinted $subdirector
		ComputeMaxDepositsPositionSlope $subdirector
		ComputeKorupIndexesIsIa $subdirector
		ComputeDepositsExtent $subdirector
		ComputeLakeExtent $subdirector
		ComputeIfIncorrectRuptureSurface $subdirector $name
		ComputeMostDownstreamDepositsPosition $subdirector
		ComputeHDSI $subdirector
		ComputeLakeDepth $subdirector
		echo -e $header >> ${subdirector}Computations.txt
	done <${out}after_gerris.txt
	echo ${out}after_gerris.txt
}
export -f Computations

#######################################
# Help/Documentation when calling the command line: 
# Arguments:
#######################################
Help()
{
    # Display Help
    cat << EOF
This script triggers landslides from a Digital Elevation Model (DEM), simulates their runouts and 
computes the potentially created lakes in the Austrian Alps, in 6 different steps which have to be
launched separately. By default, only one part of the DEM is selected to be processed for memory
reasons. More/Other parts can be chosen by modifying the variable `parts_nb` in the code.

Syntax: ./app/main.sh [-h] [-v] -A|B|X|C|D|E
options:
h       Print this Help.
v       Runs in verbose mode.
A       First step of the workflow: cuts the Austrian DEM in 14 smaller parts for computational reasons.
        The extents of the regions are hard-coded. By default, only the first part is selected to be processed.
        Requirement: the Austrian DEM `dhm_at_lamb_10m_2018.tif` in the folder `scripts`.
X       Second step of the workflow: Fills the selected parts of the DEM and computes the gradient and
        accumulation. This step is carried out using TopoToolbox in the article but is done with GRASS GIS
        in this script.
B       Third step of the workflow: uses the Rockfall code from Hergarten (2012) to determine the landslide
        areas and volumes, then prepares the DEMs for input into Gerris. The slope thresholds for the 
        Rockfall algorithm are 45° and 79°. The number of seed pixels is derived from the DEM area. Only
        the landslides which volumes exceed 10^5 m^3 are retained for the following steps.
C       Fourth step of the workflow: launches Gerris. The landslide runouts are simulated for a period of
        6 minutes. No display is available (install gfsview2D for this). The code can be parallelized by enabling
        MPI in Gerris build.
D       Fifth step of the workflow: fill the landslide dams in GRASS GIS. The landslide deposits outputted 
        from Gerris are added to the topography and the DEM is filled to obtain the landslide-dammed 
        lake topography.
E       Sixth step of the workflow: computes the geomorphometrics of the landslide-dammed lakes. This 
        includes lake depth and volume, deposit position, height and volume, as well as several obstruction
        and stability indices. The results are outputed in `Computations.txt` files.
        Requirements: the vector files of the lithologies `LithUnits_*.gmt` and the glacial imprint 
        `lgm_global_Corrected_only_alps.gmt` in the folder `scripts` (gmt is the Generic Mapping Tool
        vector extension).

This is free script delivered under the MIT license; see the source for copying conditions.  There is 
NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
EOF
}

#######################################
# Process the input options. Add options as needed. Getopts does not support long options (--my_option) 
# Arguments:
#######################################
# Get the options
step=0
while getopts "hvAXBCDE" option; do
    case $option in
        h) # display Help
            Help
            exit;;
        v) # verbose mode
            verb="-V" # "-V" for verbose output, "" otherwise.
            exit;;
        A) # launch first step
            Workflow1
            step=1
            exit;;
        X) # fill with GRASSGIS, while I filled with TopoToolbox for the article results.
            FillWithGRASSGIS
            step=6
            exit;;
        B) # launch second step
            Workflow2
            step=2
            exit;;
        C) # launch third step
            GerrisParallel
            step=3
            exit;;
        D) # launch fourth step
            FillDamsWithGRASSGIS
            step=4
            exit;;
        E) # launch fifth step
            Computations
            step=5
            exit;;
        \?) # incorrect option
            echo "Error: Invalid option"
            Help
            exit;;
        esac
done
if [ $step -eq 0 ]
then
    echo "ERROR: You have to specify the step to be carried out. Add -h for help."
    exit
fi
