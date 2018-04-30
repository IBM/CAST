# CORAL System Software

## Primary Software Components
### Cluster System Manager (CSM)
CSM ...

### Burst Buffer

The Burst Buffer provides an asynchronous, low jitter, file transfer service between GPFS and local SSDs.

- [Burst Buffer](@ref bboverview)

### Function Shipping

## Supporting Libraries

### FlightLog
Provides a very lightweight in-memory logging mechanism.  Also provides a decoder tool that can externally read the flightlog(s) and generate human-readable messages.

- [Flightlog](@ref flightlog)

### Transport

Provides data transportation services between processes on either RDMA or Socket protocols.  Contains facilities to serialize/deserialize structured data.

- [Transport](@ref transport)
