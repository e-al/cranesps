#!/bin/bash

echo "Input username:"
read USERNAME
echo "Input password for $USERNAME"
read -s PASSWORD
HOST_BASE=fa15-cs425-g10-0
HOST_FINISH=.cs.illinois.edu

echo "create local crane root"
rm -rf /home/$USERNAME/crane
mkdir -p /home/$USERNAME/crane/{logs,files,tmp/code}
cp daemon/daemon /home/$USERNAME/crane/
cp log.conf /home/$USERNAME/crane/log.conf
echo "Done creating local crane root"

echo "distribute crane root to other machines"
for i in `seq 2 7`
do
HOST=$USERNAME@$HOST_BASE$i$HOST_FINISH
sshpass -p $PASSWORD ssh -t $HOST bash -c "'
#sudo yum install zeromq
#sudo yum install boost
#sudo yum install protobuf
rm -rf /home/$USERNAME/crane
mkdir crane
'"
sshpass -p $PASSWORD scp -r /home/$USERNAME/crane/* $HOST:/home/$USERNAME/crane
done
echo "Done distributing crane"

