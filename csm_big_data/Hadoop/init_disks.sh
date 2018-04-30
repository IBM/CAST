#!/bin/bash
# encoding: utf-8
# ================================================================================
#
#    init_disks.sh
#
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
# ================================================================================

#=========================================================
# Return Codes
#=========================================================
#: The directories and mounts were properly built.
RET_COMPLETE=0

#: Exit code for input events.
RET_INPUT_EXIT=1

#: The specified name node drive had an existing mount.
RET_NAME_MOUNTED=2

#: One of the specified data node drive had an existing mount.
RET_DATA_MOUNTED=3

#: Everything exited clean, but an NFS mount was detected.
RET_NFS_MOUNT=4

#=========================================================

commands="hn:d:a"

# Configurable data options.
#: The name node drive.
name_node_drive="sdn"

#: The drives to prepare for hadoop.
data_node_drives=("sdp" "sdq" "sdr" )

#: Flag for whether this run should clear aacraid settings.
#: Should only be called if the node has no hardware arrays
aacraid_clear=0

usage()
{
    cat << EOF >&2
    Usage $0
    =========
    -h              Displays this message.
    -a              A flag that, when set, triggers the aacraid clear.
    -n <device>     The block device for the name node.
    -d <devices>    A space separated list of data node devices.

    Error Codes
    ===========
    0 - Script executed with no problems.
    1 - Execution terminated in the execution of getopts.
    2 - The name node device has something mounted.
    3 - One of the data node devices has something mounted.
    4 - An NFS mount was detected.
EOF
}

# Echo to stderr.
echoe()
{
    >&2 echo $@
}

# Makes a directory hadoop ready.
# 1 - Directory
# 2 - User Name
hadoopify()
{
    mkdir -p $1
    chown -R $2:hadoop $1
    chmod -R 755 $1
}

# Partitions a drive for hadoop and adds it to the fstab.
partition_hadoop_drive()
{
    block_dev=$1
    mount_dir=$2

    echoe "Partitioning ${block_dev}"
    yes | parted ${block_dev} mklabel gpt -s 
    yes | parted ${block_dev} mkpart primary "1 -1" -s 
    mkfs.ext4 ${block_dev}1
    
    echoe "Adding ${block_dev} to fstab."
    sed -ni '\:'${mount_dir}':!p' /etc/fstab
    UUID=$(blkid ${block_dev}1 | sed 's:.* UUID=\"\([^"]*\)\".*:\1:g')
    echo "UUID=${UUID} ${mount_dir} ext4 defaults 0 0" >> /etc/fstab

    echoe "Mounting ${mount_dir}"
    mkdir -p ${mount_dir}
    mount ${mount_dir}

    echoe "Mounted ${mount_dir}"
}
    
# Reverts all devices without anything mounted to the Raw JBOD state.
configure_aacraid()
{
    echoe "Configuring the disks on this node for Hadoop."
    hash arcconf
    if [ $? == 0 ]
    then
        echoe "arcconf has been found, continuing."
    else
        echoe "arcconf has not been found, assuming drives are ready for use."
    fi

    #TODO This may need to change in the future.
    # The aacraid controller.
    controller=1
    # The channel of the devices in the RAID controller.
    channel=0

    # Get the states that are present and not RAW for 
    current_device=""
    current_state=""
    devs_to_clean=""
    while read -r output_line
    do
        if [[ ${output_line} =~ ^Device ]]
        then
            current_device=${output_line#Device #} 
    
        elif [[ ${output_line} =~ ^State ]]
        then
            current_state=${output_line#State[ ]*:}
        fi

        # When the state and device have been gathered determine if aacraid needs to do anything.
        if [[ ${current_device} != "" && ${current_state} != "" ]]
        then
            device_name=${device_keys[$current_device]}

            # If a mount wasn't found Determine if it needs to be uninitialized.
            if [[ ${drives_with_mounts[$current_device]} == "" ]]
            then
                if ! [[ ${output_line} =~ Raw ]]
                then
                    devs_to_clean=${devs_to_clean}" "${channel}" "${current_device}
                fi
            else
                echoe "Device ${current_device} (/dev/${device_name}) has a mount point present."
            fi

            # Reset the device/state    
            current_device=""
            current_state=""
        fi
    done < <(arcconf getconfig ${controller} PD | sed -n -e '/Device #/p'  -e '/^[ ]*State[ ]*/p')
    
    if [[ ${devs_to_clean} != "" ]]
    then
        arcconf UNINIT ${controller} ${devs_to_clean}
    fi

    echoe "The drives are now ready to be partitioned for Hadoop."
}

while getopts ${commands} opt
do
    case ${opt} in
        h) 
            usage; exit ${RET_INPUT_EXIT};;
        n)
            name_node_drive=${OPTARG}
            ;;
        d)
            data_node_drives=(${OPTARG})
            ;;
        a)
            aacraid_clear=1
            ;;
        *)
            echoe "Invaild option: ${opt}"
            usage
            exit ${RET_INPUT_EXIT}
            ;;
    esac
done

# Get the mounted disks:
declare -A device_map
declare -A mount_map
device_id=0
while read -r device mount
do
    dev=$(echo ${device} |  sed 's:^[^s]*\([^0-9]*\).*:\1:g')


    if [[ ${device_map[$dev]} = "" ]]
    then
        # This is a fix for the "aacraid: Host adapter reset request. SCSI hang ?" Message.
        echo 45 > /sys/block/${dev}/device/timeout
        device_map[${dev}]=${device_id} 
        device_keys[${device_id}]=${dev}
        ((device_id++))
    fi

    if [[ ${mount} != "" ]]
    then 
        drives_with_mounts[(( $device_id - 1 ))]=$dev
        mount_map[${dev}]=1
    fi
done < <(lsblk | awk '/sd/{print $1"  "$7}')

# =======================================================================
# Verify that the selected drives were available.
# This is not done inline for readability.
# =======================================================================

if [[ ${mount_map[${name_node_drive}]} != "" ]]
then
    echoe "/dev/${name_node_drive} already had something mounted on it!"
    echoe "Please select another drive!"
    echo ${name_node_drive}
    exit ${RET_NAME_MOUNTED}
fi

failed_devices=""
for device in ${data_node_drives}
do
    if [[ ${mount_map[${device}]} != "" ]]
    then 
       failed_devices=${failed_devices}" "${device} 
    fi
done

if [[ ${failed_devices} != "" ]]
then
    echoe "The following devices could not be used as a data node: ${failed_devices}"
    echoe "Something was already mounted on these devices!"
    echo ${failed_devices}
    exit ${RET_DATA_MOUNTED}
fi

# =======================================================================

# Configure the drives and be sure that they aren't initialized.
configure_aacraid 

# Preserve the old fstab before producing any of the nodes.
cp /etc/fstab /etc/fstab.old

# Partition the Name Node
partition_hadoop_drive "/dev/${name_node_drive}" /opt/IBM/ioala 
hadoopify /opt/IBM/ioala/ ioala
hadoopify /opt/IBM/ioala/hadoop hdfs 
hadoopify /opt/IBM/ioala/namenode hdfs 

chmod -R 755 /opt/IBM/ioala/
chown ioala:users /opt/IBM/ioala/

# Partition the Data Nodes
partitioned_drives=""
data_dir_num=1
for device in ${data_node_drives[@]}
do
    block_dev="/dev/${device}"
    
    partition_hadoop_drive ${block_dev} /data${data_dir_num}

    # Make the hadoop directories and change permissions.
    hadoopify /data${data_dir_num}/hadoop hdfs
    hadoopify /data${data_dir_num}/hadoop/hdfs hdfs
    hadoopify /data${data_dir_num}/hadoop/hdfs/nn hdfs
    hadoopify /data${data_dir_num}/hadoop/hdfs/snn hdfs
    hadoopify /data${data_dir_num}/hadoop/hdfs/dn hdfs
    
    hadoopify /data${data_dir_num}/hadoop/yarn/local yarn
    hadoopify /data${data_dir_num}/hadoop/yarn/log  yarn
    
    (( data_dir_num++ ))
done

# Hadoopify some files for logging.
hadoopify /var/log/hadoop/hdfs hdfs
hadoopify /var/run/hadoop/hdfs hdfs

hadoopify /var/log/hadoop/yarn yarn
hadoopify /var/run/hadoop/yarn yarn

hadoopify /var/log/hadoop/mapred mapred
hadoopify /var/run/hadoop/mapred mapred

# Check if any nfs mounts are present, Warn the user if it's found.
if [[ $(mount | grep -c nfs) != 0 ]]
then
    cat << EOF >&2 
    __________________________________
    WARNING : 
    
    An nfs mount was detected, before 
    installing Ambari/Hadoop disable 
    these mounts.
    __________________________________

EOF
    exit ${RET_NFS_MOUNT}
fi

exit ${RET_COMPLETE}
