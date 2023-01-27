# Assignment 3 &middot;

>

Directory for the `third` assignment of Networks Laboratory course (CS39006) offered in Spring semester 2023, Department of CSE, IIT Kharagpur.

## Getting started

Read the assignment problem statement from [Assignment_3.pdf](/assignment_3/Assignment_3.pdf)

- To start the servers

```shell
gcc servera.c -o servera && ./servera 20000
gcc serverb.c -o serverb && ./serverb 20001
```

- To start the load balancer

```shell
gcc lb.c -o lb && ./lb 3000 20000 20001
```

- To start the client and test the load balancer

```shell
gcc client.c -o client && ./client
```

## Solution

GCC version information-  

```shell
gcc (GCC) 12.1.1 20220730
Copyright (C) 2022 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
