Group members:
    Tianyuan Liu
    Konstantin Evchenko

Project desciption:
    This is the fourth MP project for CS425.

Installation on Edu VMs:
    1. clone repo to VM01
    2. install cmake, zeromq-devel, boost-devel, protobuf-devel (>=2.6.1), gtest-devel on VM01
    3. ssh to VM01 (suppose in source root) run following command:
        cd build
        cmake ../
        make
    4. on VM01, cd to build
        ./deploy.sh 

    NOTE: need to change default introducer ip and candidate member ip

Run:
    1. On VM01 ./daemon -i
    2. On other VMs, ./daemon

       

