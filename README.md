# Token Bucket Filter Traffic Shaper Simulator

## Description

This project implements a multi-threaded token bucket filter traffic shaper simulator. The system emulates packet and token arrivals, the operation of a token bucket filter, first-come-first-served queues, and packet transmission through servers.

## Features

- Token bucket filter with configurable capacity
- Two queues: Q1 (pre-filter) and Q2 (post-filter)
- Multi-threaded implementation using pthreads
- Two operation modes: Deterministic and Trace-driven
- Packet arrival and token arrival simulation
- Two-server transmission system

## System Architecture

[Packet Arrival] -> [Q1] -> [Token Bucket Filter] -> [Q2] -> [Servers (S1, S2)]
                              ^
                              |
                       [Token Arrival]

## Requirements

- C compiler (gcc recommended)
- POSIX threads library (pthreads)



## Parameters

- lambda: Packet arrival rate
- mu: Service rate of servers
- r: Token arrival rate
- B: Token bucket depth
- P: Number of tokens required per packet
- num_packets: Number of packets to simulate
- time: Total simulation time
- tsfile: Path to trace specification file

## Output

The program produces a detailed trace of events occurring during the simulation, including:

- Packet arrivals
- Token arrivals
- Packet movements between queues
- Server activities

## Implementation Details

- Separate threads for packet arrival, token arrival, and each server
- Mutex protection for shared resources (Q1, Q2, token bucket)
- Infinite capacity queues using My402List

