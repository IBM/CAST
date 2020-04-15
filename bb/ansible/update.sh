#!/bin/bash 
export ANSIBLE_INVENTORY_CACHE_PLUGIN=jsonfile
export ANSIBLE_INVENTORY_CACHE=True
#default forks is 5, override
FORKS=50
FQPATH=$(readlink -fm $0)
echo $FQPATH
PLAYDIR=$(dirname $FQPATH)
echo $PLAYDIR
cd $PLAYDIR
sudo ansible-playbook -i hosts  update.yml -f $FORKS

