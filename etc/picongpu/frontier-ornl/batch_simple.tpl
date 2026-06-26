#!/usr/bin/env bash
# Copyright 2013-2024 Axel Huebl, Richard Pausch, Rene Widera, Sergei Bastrakov, Klaus Steinger, Alexander Debus
#
# This file is part of PIConGPU.
#
# PIConGPU is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# PIConGPU is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with PIConGPU.
# If not, see <http://www.gnu.org/licenses/>.
#


# PIConGPU batch script for frontier's SLURM batch system

#SBATCH --account=!TBG_nameProject
#SBATCH --partition=!TBG_queue
#SBATCH --time=!TBG_wallTime
# Sets batch job's name
#SBATCH --job-name=!TBG_jobName
#SBATCH --nodes=!TBG_nodes_adjusted
#SBATCH --ntasks=!TBG_tasks_adjusted
#SBATCH --mem-per-gpu=!TBG_memPerDevice
#SBATCH --ntasks-per-gpu=1
#SBATCH --gpu-bind=closest
#SBATCH --gres=gpu:!TBG_devicesPerNode

#SBATCH --mail-type=!TBG_mailSettings
#SBATCH --mail-user=!TBG_mailAddress
#SBATCH --chdir=!TBG_dstPath

#SBATCH --open-mode=append
#SBATCH -o stdout
#SBATCH -e stderr

## calculations will be performed by tbg ##
.TBG_queue="batch"

# settings that can be controlled by environment variables before submit
.TBG_mailSettings=${MY_MAILNOTIFY:-"NONE"}
.TBG_mailAddress=${MY_MAIL:-"someone@example.com"}
.TBG_author=${MY_NAME:+--author \"${MY_NAME}\"}
.TBG_nameProject=${PROJID:-""}
.TBG_profile=${PIC_PROFILE:-"~/picongpu.profile"}

# number of available/hosted devices per node in the system
.TBG_numHostedDevicesPerNode=8

# host memory per device
.TBG_memPerDevice=64000

# number of CPU cores to block per GPU
# we have 8 CPU cores per GPU (64cores/8gpus ~ 8cores)
# but one core (= core 0) is reserved for system processes
# accordign to OLCF docs
.TBG_coresPerGPU=7

# Assign one OpenMP thread per available core per GPU (=task)
#export OMP_NUM_THREADS=!TBG_coresPerGPU

# required GPUs per node for the current job
.TBG_devicesPerNode=$(if [ $TBG_tasks -gt $TBG_numHostedDevicesPerNode ] ; then echo $TBG_numHostedDevicesPerNode; else echo $TBG_tasks; fi)

# We only start 1 MPI task per device
.TBG_mpiTasksPerNode="$(( TBG_devicesPerNode * 1 ))"

# use ceil to caculate nodes
.TBG_nodes="$((( TBG_tasks + TBG_devicesPerNode - 1 ) / TBG_devicesPerNode))"

# oversubscribe the node allocation by N per thousand
# The default can be overwritten by setting the environment variable PIC_NODE_OVERSUBSCRIPTION_PT
.TBG_node_oversubscription_pt=${PIC_NODE_OVERSUBSCRIPTION_PT:-2}

# adjust number of nodes for fault tolerance adjustments
.TBG_nodes_adjusted=$((!TBG_nodes * (1000 + !TBG_node_oversubscription_pt) / 1000))
.TBG_tasks_adjusted=$((!TBG_nodes_adjusted * !TBG_numHostedDevicesPerNode))

## end calculations ##

echo 'Start job with !TBG_nodes_adjusted nodes. Required are !TBG_nodes nodes.'

TBG_dstPath="!TBG_dstPath"
cd $TBG_dstPath

export MODULES_NO_OUTPUT=1
source !TBG_profile
if [ $? -ne 0 ] ; then
    echo "Error: PIConGPU environment profile under \"!TBG_profile\" not found!"
    exit 1
fi
unset MODULES_NO_OUTPUT

# set user rights to u=rwx;g=r-x;o=---
umask 0027

mkdir simOutput 2> /dev/null
cd simOutput
ln -s ../stdout output

# number of broken nodes
n_broken_nodes=0

# return code of cuda_memcheck
node_check_err=1

# Run PIConGPU
echo "Start PIConGPU."

srun -n !TBG_tasks --nodes=!TBG_nodes  -K1 $TBG_dstPath/input/bin/picongpu --mpiDirect !TBG_author !TBG_programParams
