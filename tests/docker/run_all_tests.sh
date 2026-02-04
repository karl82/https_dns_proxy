#!/usr/bin/env bash
set -euo pipefail

# Docker-based test runner for https_dns_proxy
# This script builds a test image and runs all tests in a proper Linux environment.
# Uses Docker's default bridge network which has NAT/masquerading properly configured
# for source address binding to work with external DNS servers.
#
# When to use:
# - Full regression testing before commits/PRs
# - CI/CD pipelines
# - Developing on macOS (proxy uses Linux-specific syscalls like accept4, MSG_MORE)
#
# Runtime: ~2-3 minutes

docker_bin="${DOCKER_BIN:-docker}"
if ! command -v "$docker_bin" >/dev/null 2>&1; then
  echo "docker not found; set DOCKER_BIN or install Docker." >&2
  exit 1
fi

# Image and container names
image="https_dns_proxy_test:latest"
container="hdp_test_container_$$"

cleanup() {
  echo ""
  echo "==> Cleaning up..."
  "$docker_bin" rm -f "$container" >/dev/null 2>&1 || true
}
trap cleanup EXIT

echo "==> Building Docker test image..."
"$docker_bin" build -t "$image" -f tests/docker/Dockerfile . -q

echo "==> Creating container on default bridge network..."
# Use default bridge network (--network=bridge or no --network flag)
# The default bridge has proper NAT/masquerading for source address binding
"$docker_bin" run -d --name "$container" \
  --dns 1.1.1.1 --dns 8.8.8.8 \
  -v "$PWD":/src -w /src "$image" sleep infinity >/dev/null

# Get eth0 IP address
eth0_ip=$("$docker_bin" exec "$container" bash -c \
  "ip -4 -br addr show eth0 | awk '{print \$3}' | cut -d/ -f1")

if [[ -z "$eth0_ip" ]]; then
  echo "Failed to detect eth0 IP in container." >&2
  exit 1
fi

echo "Container eth0 IP: $eth0_ip"

echo ""
echo "==> Building project..."
"$docker_bin" exec "$container" bash -c \
  "cmake -S /src -B /src/build && cmake --build /src/build" 2>&1 | \
  grep -E "(Building|Linking|Built target)" || true

echo ""
echo "==> Running Robot Framework functional tests..."
"$docker_bin" exec "$container" bash -c \
  "cd /src/tests/robot && python3 -m robot.run --loglevel WARN functional_tests.robot"

exit_code=$?

echo ""
if [ $exit_code -eq 0 ]; then
  echo "✅ All tests passed!"
else
  echo "⚠️  Some tests failed (exit code: $exit_code)"
  echo "Check the Robot Framework report at tests/robot/report.html"
fi

exit $exit_code
