# ═══════════════════════════════════════════════════════════════
# Stage 1: Build the C++ Binary
# ═══════════════════════════════════════════════════════════════
FROM alpine:3.19 AS builder

RUN apk add --no-cache g++ make

WORKDIR /build

COPY Makefile ./
COPY src/ src/

RUN CXX=g++ make

# ═══════════════════════════════════════════════════════════════
# Stage 2: Ultra-Lightweight Production Runtime (< 15MB)
# ═══════════════════════════════════════════════════════════════
FROM alpine:3.19

RUN apk add --no-cache libstdc++

WORKDIR /app

# Copy compiled executable and frontend files
COPY --from=builder /build/gc_engine ./
COPY frontend/ frontend/
COPY Memory_Compaction_and_GC_Engine_Documentation.pdf ./

# Cloud platforms (Render, Railway, Fly.io, Cloud Run) inject $PORT
ENV PORT=8080
EXPOSE 8080

CMD ["./gc_engine"]
