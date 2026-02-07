# LegalAI - Real-Time Legal Intelligence Platform

## Overview
LegalAI is a production-grade, real-time legal intelligence platform that demonstrates advanced infrastructure engineering principles through AI-powered legal services. Built on battle-tested distributed systems concepts from InfraChat, LegalAI focuses on **real-time collaboration**, **multi-region consistency**, **streaming AI responses**, and **massive-scale document processing**.

This project showcases expertise in:
- Real-time bidirectional streaming (gRPC)
- Multi-region distributed systems
- Event-driven architecture
- High-performance AI inference
- Production observability & reliability
- Chaos engineering & resilience

**Key Differentiator:** While most legal tech platforms use simple request-response APIs, LegalAI treats every interaction as a real-time stream, enabling collaborative editing, live AI assistance, and instant case updates across global teams.

---

## Architecture Philosophy

### Real-Time First Design
Every component in LegalAI is designed for real-time operation:

1. **Streaming AI Responses** - Token-by-token streaming, not batch responses
2. **Live Document Collaboration** - Google Docs-style real-time editing
3. **Instant Case Updates** - WebSocket/gRPC streams to all stakeholders
4. **Real-time Search** - As-you-type case law search with sub-100ms latency
5. **Live Metrics** - Real-time observability across all services

### Infrastructure-Centric Approach
- **Control Plane / Data Plane Separation** - Clear architectural boundaries
- **Regional Sharding** - Data locality for GDPR/CCPA compliance
- **Event-Driven** - Pub/Sub for loose coupling and scalability
- **Failure Isolation** - Regional failures don't cascade globally
- **Backpressure Management** - Slow clients don't impact system health

---

## High-Level Architecture

```
┌────────────────────────────────────────────────────────────┐
│           Global Load Balancer (Cloud Armor)               │
│        DDoS Protection, WAF, TLS Termination               │
└─────────────────────────┬──────────────────────────────────┘
                          │
                  ┌───────▼────────┐
                  │  API Gateway   │
                  │ Auth, Rate Limit│
                  │ (gRPC + HTTP)  │
                  └───────┬────────┘
                          │
      ┌───────────────────┼───────────────────┐
      │                   │                   │
┌─────▼──────┐    ┌──────▼──────┐    ┌──────▼──────┐
│ Web Client │    │ Mobile App  │    │ API Clients │
│ (Next.js)  │    │ (React Nat.)│    │   (gRPC)    │
└─────┬──────┘    └──────┬──────┘    └──────┬──────┘
      │                   │                   │
      └───────────────────┼───────────────────┘
                          │
          ┌───────────────┴───────────────┐
          │                               │
    ┌─────▼─────────┐           ┌────────▼─────────┐
    │   Region A    │           │    Region B      │
    │  (us-west1)   │           │  (europe-west1)  │
    └─────┬─────────┘           └────────┬─────────┘
          │                               │
    ┌─────▼──────────────────────────────▼─────┐
    │        Real-Time Services Layer           │
    ├───────────────────────────────────────────┤
    │ ┌─────────────────────────────────────┐   │
    │ │  AI Streaming Service               │   │
    │ │  - Token-by-token streaming         │   │
    │ │  - RAG pipeline (100ms p99)         │   │
    │ │  - Model routing & load balancing   │   │
    │ └─────────────────────────────────────┘   │
    │                                           │
    │ ┌─────────────────────────────────────┐   │
    │ │  Document Collaboration Service     │   │
    │ │  - Operational Transform (OT)       │   │
    │ │  - CRDT for conflict resolution     │   │
    │ │  - Real-time sync (WebSocket/gRPC)  │   │
    │ └─────────────────────────────────────┘   │
    │                                           │
    │ ┌─────────────────────────────────────┐   │
    │ │  Case Stream Service                │   │
    │ │  - Live case updates                │   │
    │ │  - Event sourcing                   │   │
    │ │  - CQRS pattern                     │   │
    │ └─────────────────────────────────────┘   │
    │                                           │
    │ ┌─────────────────────────────────────┐   │
    │ │  Search Stream Service              │   │
    │ │  - Real-time typeahead              │   │
    │ │  - Vector similarity streaming      │   │
    │ │  - Sub-100ms p99 latency            │   │
    │ └─────────────────────────────────────┘   │
    └───────────────────┬───────────────────────┘
                        │
              ┌─────────▼─────────┐
              │    EventBus       │
              │  (Pub/Sub)        │
              │  - Case updates   │
              │  - Doc changes    │
              │  - User actions   │
              └─────────┬─────────┘
                        │
        ┌───────────────┼───────────────┐
        │               │               │
  ┌─────▼─────┐  ┌─────▼─────┐  ┌─────▼─────┐
  │PostgreSQL │  │ Vector DB │  │  Redis    │
  │ (Cases)   │  │(Pinecone) │  │ (Cache)   │
  └───────────┘  └───────────┘  └───────────┘
```
