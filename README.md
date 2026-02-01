# InfraChat

## Overview

**InfraChat** is a multi-region, real-time chat infrastructure designed to demonstrate production-grade backend and infrastructure engineering principles.  
The system enables terminal-based clients to communicate over **gRPC bidirectional streaming** using **Protocol Buffers**, with a strong focus on scalability, reliability, and observability rather than UI features.

This project intentionally models real-world infrastructure concerns such as long-lived connections, backpressure handling, authentication and routing separation, regional sharding, and failure-aware design.  
It is built to be reproducible locally via containers and deployable to **Google Cloud Platform (GCP)** for multi-region operation and operational testing.

---

## Architecture

InfraChat follows a **control-plane / data-plane separation**.

Clients first authenticate through a dedicated **Gatekeeper** service, then obtain routing information from a **Switchboard**, which directs them to the appropriate regional **ChatShard** based on channel ownership, health, and availability.

Each ChatShard maintains **gRPC streaming connections** with clients and performs fanout with explicit backpressure control to protect the system from slow consumers.  
Channels are assigned a **home region** and **shard** using deterministic hashing, enabling clear failure isolation and predictable routing behavior.

Observability (metrics, logs, and tracing) is treated as a first-class concern to support debugging, load testing, and failure analysis across regions.

---

## High-Level Architecture (ASCII)

                ┌──────────────────────────────┐
                │     Global Load Balancer      │
                │        (GCP / TLS)            │
                └───────────────┬──────────────┘
                                │
                        ┌───────▼───────┐
                        │  Gatekeeper   │
                        │ Auth / Token  │
                        └───────┬───────┘
                                │
                        ┌───────▼───────┐
                        │  Switchboard  │
                        │ Routing / LB  │
                        └───────┬───────┘
                                │
          ┌─────────────────────┴─────────────────────┐
          │                                           │
  ┌───────▼────────┐                        ┌────────▼────────┐
  │   Region A      │                        │    Region B      │
  │   (us-west1)    │                        │ (na-northeast1)  │
  └───────┬────────┘                        └────────┬─────────┘
          │                                           │
 ┌────────▼────────┐                      ┌──────────▼─────────┐
 │  ChatShard A1   │                      │   ChatShard B1      │
 │ gRPC Streaming  │                      │  gRPC Streaming     │
 │ Fanout + BP     │                      │  Fanout + BP        │
 └───────┬────────┘                      └──────────┬─────────┘
         │                                           │
  ┌──────▼──────┐                           ┌───────▼───────┐
  │  Presence   │                           │   Presence    │
  │ Heartbeats  │                           │  Heartbeats   │
  └──────┬──────┘                           └───────┬───────┘
         │                                           │
  ┌──────▼────────────┐                    ┌────────▼───────────┐
  │  Metadata Storage │                    │  Metadata Storage  │
  │ (Firestore / SQL) │                    │ (Firestore / SQL)  │
  └───────────────────┘                    └────────────────────┘
