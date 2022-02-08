# Multithreaded Airline Check-In Simulator

This is a task scheduler which includes a single queue and 5 clerks. The queue will be sorted based on priority (business = 1 being higher than economy = 0) of customers followed by their arrival time. 
Developed for CSC 360 - Operating Systems Fall 2021 - Rewritten in Go

## Installation

Download the files and type 'make' into the command line.

## Usage

Type ./ACS <input_file> to start the program.

## File Format

The <input_file> has a simple format that MUST be followed. The first line contains the total number of customers that will be simulated. After that:
1. The first character specifies the unique ID of customers.
2. A colon(:) immediately follows the unique number of the customer.
3. Immediately following is an integer equal to either 1 (indicating the customer belongs to business class) or 040
(indicating the customer belongs to economy class).
4. A comma(,) immediately follows the previous number.
5. Immediately following is an integer that indicates the arrival time of the customer.
6. A comma(,) immediately follows the previous number.
7. Immediately following is an integer that indicates the service time of the customer.
8. A newline (\n) ends a line.
Do not worry about checking a false input file.

Sample Input:
8
1:0,2,60
2:0,4,70
3:0,5,50
4:1,7,30
5:1,7,40
6:1,8,50
7:0,10,30
8:0,11,50
