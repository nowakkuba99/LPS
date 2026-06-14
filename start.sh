#!/bin/bash
# Start the full LPS stack (run from the project root)
set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"

echo "Building C++ compute binary..."
make -C "$ROOT" lps

echo "Starting PostgreSQL..."
docker compose -f "$ROOT/docker-compose.yml" up -d postgres

echo "Waiting for postgres to be healthy..."
until docker compose -f "$ROOT/docker-compose.yml" exec -T postgres pg_isready -U lps > /dev/null 2>&1; do
  sleep 1
done

echo "Starting Go server..."
cd "$ROOT/server"
LPS_BINARY="$ROOT/lps" go run . &
GO_PID=$!

echo ""
echo "Stack is up:"
echo "  API: http://localhost:9000/api/simulations"
echo "  DB:  postgres://lps:lps@localhost:5432/lps"
echo ""
echo "Press Ctrl+C to stop."

cleanup() {
  echo "Shutting down..."
  kill $GO_PID 2>/dev/null
  docker compose -f "$ROOT/docker-compose.yml" stop
}
trap cleanup INT TERM
wait $GO_PID
