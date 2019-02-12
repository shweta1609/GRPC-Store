# CS 6210: Distributed Service using grpc - Project 3

# Description

In this project, we have implemented a store server that receives a product query from customers and retrieves the bids from different vendor servers and replies back to the customer with the consolidated response.

STORE.CC:

This file contains both the server and client logic for the store.

1. Run method starts the "server" for the store on the given IP and port.
2. It creates a channel to retrieve the vendor stub for each vendor in the vendor_addresses.txt file.
3. It then spawns over any new client (customer) request that it receives and associates them with a thread in the threadpool.
4. For each customer request, it will create an RPC call and send the BidQuery to all of the vendor servers (here it acts as a client).
5. It will then wait for vendors to send the response containing the price for the products sent in the query.
6. Now for each vendor's response, server will add product info to the response that needs to be sent to the client(customer).
7. Once all the vendors have responded back, we sent the consolidated reply contaning product bids from all the vendors to the customer.

THREADPOOL.H:

This file handles the client requests by assigning a thread from the threadpool and maintaining the synchronization.
1. When store receives a request from client, it adds the job to the threadpool by calling addToQueue method.
2. If there are any threads available, it will assign one to the job sent by the store to handle the query.
3. Once the job is done, it will notify one of the thread that are waiting in the threadpool to start executing.
4. Here we are also handling graceful termination and abrupt termination of the threads that are running.

# How to run it

1. Make the files in the source folder
`sudo make all`
This will create executables for all the c++ and protobuf files.

2. Run the store.cc to start the server. It can accept upto two command line parameters, one for specifying the IP address and port number on which the store server will be running, another one for specifying max number of threads we want to run
`./store 0.0.0.0:50056 5`

3. Run the vendors and test files to test the bid response.


# Setup
Cloned this repository:
`git clone https://github.gatech.edu/abijlani3/cs6210Project3`

# Setting up the dependencies
1. [sudo] apt-get install build-essential autoconf libtool
2. Created a new directory somewhere where you will pull code from github to install grpc and protobuf.
     Then: `cd  $C210Project3`
2. git clone --recursive `-b $(curl -L http://grpc.io/release)` https://github.com/grpc/grpc
3. cd  grpc/third_party/protobuf
4. sudo apt-get install autoconf automake libtool curl make g++ unzip
5. ./autogen.sh 
6. ./configure
7. sudo make
8. make check 
9. sudo make install
10. sudo ldconfig
11. cd ../../
12. make
13. sudo make install 

# References used
For GRPC:
1. https://grpc.io/docs/tutorials/async/helloasync-cpp.html
2. The code from examples in the grpc package
3. The tutorial and resources available on Canvas in Files>Project 3

For threadpool:
1. https://www.youtube.com/watch?v=eWTGtp3HXiw
