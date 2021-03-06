#!/bin/bash -l
#

# output directory from startup arguments
output_dir=${1:-"/tmp/foil4_001/"}

if [ "$output_dir" = "-h" ] || [ "$output_dir" = "--help" ]
then
  echo "Usage:"
  echo "  $0 [output_directory]"
fi

#isaac &
#server_id=$!

echo ""
#echo "Let's watch a laser-plasma movie!"
#echo "  http://laser.plasma.ninja/ngc/interface.htm"
echo "Let's create some openPMD HDF5 files from a novel"
echo "plasma ion accelerator driven by a short, intense"
echo "laser pulse!"
echo ""

# wait until server is up
sleep 5

# start PIConGPU
cd /opt/picInputs/foil
tbg \
  -f \
  -s "bash -l" \
  -c etc/picongpu/4.cfg \
  -t etc/picongpu/bash/mpirun.tpl \
  $output_dir

# kill the isaac server after tbg returns
#kill $server_id

echo ""
echo "Simulation finished! See the created output in:"
echo "    $output_dir"
echo ""
