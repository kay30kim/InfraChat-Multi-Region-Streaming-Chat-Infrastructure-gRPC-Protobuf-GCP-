# InfraChat

## Overview
InfraChat is a multi-region, real-time chat infrastructure designed to demonstrate production-grade backend and infrastructure engineering principles. The system enables terminal-based clients to communicate over gRPC bidirectional streaming using Protocol Buffers, with a strong focus on scalability, reliability, and observability.

This project intentionally models real-world infrastructure concerns such as long-lived connections, backpressure handling, authentication and routing separation, regional sharding, and failure-aware design. It is built to be reproducible locally via containers and deployable to Google Cloud Platform (GCP) for multi-region operation and operational testing.

## Architecture

InfraChat follows a **control-plane / data-plane separation** and **event-driven messaging** architecture.

Clients first authenticate through a dedicated **Gatekeeper** service, then obtain routing information from a **Switchboard**, which directs them to the appropriate regional **ChatShard** based on channel ownership, health, and availability.

Each ChatShard maintains gRPC streaming connections with clients and performs asynchronous message delivery with other ChatShards and services through an **EventBus**. The EventBus handles cross-region message fanout, buffering, and backpressure management to protect the system from slow consumers.

Channels are assigned a home region and shard using deterministic hashing, enabling clear failure isolation and predictable routing behavior.

Observability (metrics, logs, and tracing) is treated as a first-class concern to support debugging, load testing, and failure analysis across regions.

## High-Level Architecture (ASCII)

```
                ┌──────────────────────────────┐
                │  Global Load Balancer (GCP)  │
                │         (TLS Termination)     │
                └───────────────┬──────────────┘
                                │
                        ┌───────▼────────┐
                        │   Gatekeeper   │
                        │  (Auth/Token)  │
                        └───────┬────────┘
                                │
                        ┌───────▼────────┐
                        │  Switchboard   │
                        │ (Routing/LB)   │
                        └───────┬────────┘
                                │
          ┌─────────────────────┴──────────────────────┐
          │                                            │
    ┌─────▼──────┐                            ┌───────▼──────┐
    │  Region A  │                            │   Region B   │
    │ (us-west1) │                            │(asia-ne1)    │
    └─────┬──────┘                            └───────┬──────┘
          │                                           │
    ┌─────▼──────────┐                      ┌────────▼────────┐
    │ ChatShard A1   │◄─────────┐    ┌─────►│  ChatShard B1   │
    │ gRPC Streaming │          │    │      │ gRPC Streaming  │
    └─────┬──────────┘          │    │      └────────┬────────┘
          │                     │    │               │
    ┌─────▼──────────┐          │    │      ┌────────▼────────┐
    │ ChatShard A2   │          │    │      │  ChatShard B2   │
    │ gRPC Streaming │          │    │      │ gRPC Streaming  │
    └─────┬──────────┘          │    │      └────────┬────────┘
          │                     │    │               │
          └──────────┬──────────┘    └───────────────┘
                     │                        │
              ┌──────▼────────┐        ┌─────▼─────────┐
              │  EventBus A   │◄──────►│  EventBus B   │
              │ (Pub/Sub)     │        │  (Pub/Sub)    │
              └──────┬────────┘        └─────┬─────────┘
                     │                        │
              ┌──────▼────────┐        ┌─────▼─────────┐
              │  Presence A   │        │  Presence B   │
              │  (Heartbeat)  │        │  (Heartbeat)  │
              └──────┬────────┘        └─────┬─────────┘
                     │                        │
              ┌──────▼──────────────┐  ┌─────▼──────────────┐
              │ Metadata Storage A  │  │ Metadata Storage B │
              │  (Firestore/SQL)    │  │  (Firestore/SQL)   │
              └─────────────────────┘  └────────────────────┘
```

## Core Components

### 1. Gatekeeper (Authentication Service)
- Handles client authentication
- Issues JWT tokens
- Rate limiting
- Security policy enforcement

**Technologies:**
- C++ gRPC server
- JWT library
- Redis (token blacklist)

### 2. Switchboard (Routing Service)
- Manages channel → ChatShard mapping
- Health check-based load balancing
- Shard selection via consistent hashing
- Failure detection and re-routing

**Technologies:**
- C++ gRPC server
- Consistent hashing algorithm
- Health check probes

### 3. ChatShard (Chat Processing Service)
- Maintains gRPC streaming connections with clients
- Receives messages and publishes to EventBus
- Subscribes from EventBus and fans out to clients
- Backpressure management (handles slow clients)

**Technologies:**
- C++ gRPC bidirectional streaming
- Thread pool for concurrent connections
- EventBus client (Pub/Sub)

### 4. EventBus (Message Broker)
- Asynchronous message delivery
- Intra-region / cross-region message routing
- Message buffering and retry
- At-least-once delivery guarantee

**Technology Options:**
- **Google Cloud Pub/Sub** (multi-region)
- **Apache Kafka** (self-hosted)
- **Redis Streams** (lightweight)

### 5. Presence Service
- Tracks user online/offline status
- Heartbeat monitoring
- Timeout-based connection cleanup

**Technologies:**
- Redis (TTL-based state storage)
- C++ background worker

### 6. Metadata Storage
- Channel information (name, owner, home region)
- User profiles
- Message history (optional)

**Technologies:**
- **Firestore** (NoSQL, multi-region replication)
- **Cloud SQL** (relational data)

---

## Message Flow Examples

### Example 1: Same-Region Chat

**Scenario:** Alice and Bob are both connected to ChatShard A1 in Region A, chatting in "#general" channel

```
1. Alice types message: "Hello!"
   
2. Alice client → ChatShard A1 (gRPC stream)
   Message { channel: "#general", content: "Hello!", user: "alice" }

3. ChatShard A1 → EventBus A (publish)
   Topic: "channel.general"
   Message: { content: "Hello!", user: "alice", timestamp: 123456 }

4. EventBus A → ChatShard A1 (subscribe)
   (Same shard receives its own published message)

5. ChatShard A1 → Bob client (gRPC stream)
   (Filters out message for Alice - no echo to sender)

Result: Bob receives "alice: Hello!"
```

**Characteristics:**
- Latency: ~10-50ms
- Processed entirely within EventBus
- Minimal network hops

---

### Example 2: Cross-Region Chat

**Scenario:** Alice in Region A (ChatShard A1), Charlie in Region B (ChatShard B1), both in "#general" channel

```
1. Alice types message: "Hi from US!"

2. Alice client → ChatShard A1 (gRPC)

3. ChatShard A1 → EventBus A (publish)
   Topic: "channel.general"

4. EventBus A → EventBus B (cross-region replication)
   Google Pub/Sub automatically replicates messages

5. EventBus B → ChatShard B1 (subscribe)
   Topic: "channel.general"

6. ChatShard B1 → Charlie client (gRPC)

Result: Charlie receives "alice: Hi from US!" (in Region B)
```

**Characteristics:**
- Latency: ~100-300ms (intercontinental)
- EventBus handles cross-region replication
- ChatShards use identical logic regardless of region

---

### Example 3: Backpressure Handling

**Scenario:** Dave's network is slow and can't receive messages (connected to ChatShard A2)

```
1. 100 users rapidly send messages to "#popular" channel

2. EventBus A → ChatShard A2 (1000 messages/sec)

3. ChatShard A2 attempts to send to Dave
   → Dave's client responds slowly (network issue)

4. ChatShard A2 internal buffer grows
   → Exceeds threshold (e.g., 1000 pending messages)

5. ChatShard A2 activates backpressure:
   Option A: NACK to EventBus → delayed redelivery
   Option B: Force disconnect Dave + require reconnection
   Option C: Drop old messages + send warning

6. After Dave reconnects:
   → ChatShard A2 sends "some messages lost" notification
   → Resume receiving from latest messages

Result: Slow client doesn't block entire system
```

**Characteristics:**
- Slow clients don't block the entire system
- Each ChatShard independently manages backpressure
- EventBus remains unaffected

---

### Example 4: ChatShard Failure Recovery

**Scenario:** ChatShard A1 suddenly crashes

```
1. ChatShard A1 crashes
   → Alice, Bob disconnected
   → EventBus A is healthy (messages stored in buffer)

2. Kubernetes restarts ChatShard A1
   (or creates new Pod)

3. Switchboard detects health check failure
   → Routes new clients to ChatShard A2

4. Alice, Bob attempt to reconnect:
   → Gatekeeper re-authenticates
   → Switchboard → "Use ChatShard A2"
   → Successfully connect to ChatShard A2

5. ChatShard A2 → EventBus A resumes subscription
   → Receives buffered messages
   → Delivers to Alice, Bob

Result: Recovery after ~5-10 seconds downtime
```

**Characteristics:**
- EventBus buffers messages (prevents loss)
- Stateless ChatShard → can reconnect anywhere
- Switchboard automatically re-routes

---

### Example 5: New Channel Creation and Subscription

**Scenario:** Alice creates new channel "#team-alpha"

```
1. Alice → ChatShard A1: CreateChannel("#team-alpha")

2. ChatShard A1 → Metadata Storage (Firestore):
   INSERT { channel: "#team-alpha", owner: "alice", region: "A" }

3. Consistent Hashing calculation:
   hash("#team-alpha") % num_regions = Region A
   → Set home region

4. ChatShard A1 → EventBus A:
   Subscribe to topic "channel.team-alpha"

5. Bob joins "#team-alpha":
   Bob → Switchboard: "Where should I go?"
   Switchboard → Metadata Storage query
   → "Region A, ChatShard A1"
   Bob → Connects to ChatShard A1

6. Bob sends message:
   Bob → ChatShard A1 → EventBus A → ChatShard A1 → Alice

Result: New channel created and multi-user communication enabled
```

---

## Technology Stack

### Language & Framework
- **C++17** (core services)
- **gRPC** (RPC framework)
- **Protocol Buffers** (serialization)

### Infrastructure & Cloud
- **GKE** (Google Kubernetes Engine)
- **Google Cloud Pub/Sub** (EventBus)
- **Firestore** (metadata storage)
- **Cloud Load Balancer** (global traffic)

### Concurrency
- **C++ Thread Pool**
- **Asynchronous I/O**
- **Mutexes & Condition Variables**

### Observability
- **Prometheus** (metrics)
- **Cloud Logging** (structured logs)
- **OpenTelemetry** (distributed tracing)

### Containerization
- **Docker** (containerization)
- **Kubernetes** (orchestration)

---

## Deployment

### Local Development
```bash
# Run entire stack with Docker Compose
docker-compose up

# Components:
# - Gatekeeper (port 8001)
# - Switchboard (port 8002)
# - ChatShard x2 (port 8003, 8004)
# - Redis Pub/Sub (EventBus)
# - Redis (Presence)
```

### GCP Multi-Region Deployment
```bash
# Deploy to Region A (us-west1)
kubectl apply -f k8s/region-a/

# Deploy to Region B (asia-northeast1)
kubectl apply -f k8s/region-b/

# Configure Global Load Balancer
gcloud compute url-maps create infrachat-lb \
  --default-service=backend-service
```

---

## Key Design Principles

1. **Clear Failure Isolation** - Each region operates independently
2. **Backpressure First** - Slow clients don't block the system
3. **EventBus-Centric** - Loose coupling, scalable
4. **Built-in Observability** - Metrics/logs/tracing at every layer
5. **Deterministic Routing** - Predictable via consistent hashing
6. **Graceful Degradation** - Service continues during partial failures

---

## Why EventBus?

### Without EventBus (Original Design):
```
ChatShard A1 ──(direct connection)──> ChatShard A2
             ──(direct connection)──> ChatShard B1
```
- N² connection complexity
- Full reconfiguration when adding/removing ChatShards
- One shard's problem affects others

### With EventBus:
```
ChatShard A1 ──> EventBus <── ChatShard A2
                    ↕
                EventBus <── ChatShard B1
```
- Loose coupling
- Dynamic scaling
- Message buffering + retry
- Automated cross-region replication

---

## Getting Started

1. **Prerequisites**
   - C++17 compiler
   - Docker & Docker Compose
   - gRPC & Protobuf libraries
   - GCP account (for production deployment)

2. **Clone & Build**
   ```bash
   git clone https://github.com/yourname/infrachat
   cd infrachat
   mkdir build && cd build
   cmake ..
   make -j8
   ```

3. **Run Locally**
   ```bash
   docker-compose up
   ./client --server=localhost:8001
   ```

4. **Deploy to GCP**
   - Create GKE clusters
   - Configure Pub/Sub topics
   - Apply Kubernetes manifests

---

## Future Enhancements

- [ ] End-to-end encryption
- [ ] Message persistence (history)
- [ ] Voice/Video streaming
- [ ] WebSocket gateway (for web clients)
- [ ] GraphQL API
- [ ] AI moderation

---

## License
MIT

## Contact
questions@infrachat.dev
