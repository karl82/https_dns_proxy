#!/usr/bin/env bash
set -euo pipefail

# Quick test for bootstrap DNS source binding feature
#
# When to use:
# - Quick verification during feature development
# - Testing bootstrap DNS feature specifically
# - Developing on macOS (proxy uses Linux-specific syscalls like accept4, MSG_MORE)
#
# Runtime: ~30 seconds

docker_bin="${DOCKER_BIN:-docker}"
if ! command -v "$docker_bin" >/dev/null 2>&1; then
  echo "docker not found; set DOCKER_BIN or install Docker." >&2
  exit 1
fi

net_a="hdp_net_a_$$"
net_b="hdp_net_b_$$"
container="hdp_bootstrap_test_$$"

cleanup() {
  "$docker_bin" rm -f "$container" >/dev/null 2>&1 || true
  "$docker_bin" network rm "$net_a" "$net_b" >/dev/null 2>&1 || true
}
trap cleanup EXIT

"$docker_bin" network create "$net_a" >/dev/null
"$docker_bin" network create "$net_b" >/dev/null

"$docker_bin" run -d --name "$container" --network "$net_a" \
  -v "$PWD":/src -w /src ubuntu:24.04 sleep infinity >/dev/null
"$docker_bin" network connect "$net_b" "$container"

"$docker_bin" exec "$container" bash -lc \
  "export DEBIAN_FRONTEND=noninteractive; \
   printf 'nameserver 1.1.1.1\n' > /etc/resolv.conf; \
   apt-get update; \
   apt-get install -y iproute2 cmake build-essential \
     libcurl4-openssl-dev libc-ares-dev libev-dev dnsutils"

eth0_ip="$("$docker_bin" exec "$container" bash -lc \
  \"ip -4 -br addr show eth0 | awk '{print \\$3}' | cut -d/ -f1\")"

if [[ -z "$eth0_ip" ]]; then
  echo "Failed to detect eth0 IP in container." >&2
  exit 1
fi

"$docker_bin" exec "$container" bash -lc \
  "cmake -S /src -B /tmp/build >/dev/null && cmake --build /tmp/build >/dev/null"

"$docker_bin" exec "$container" bash -lc \
  "printf 'nameserver 203.0.113.1\n' > /etc/resolv.conf; \
   (/tmp/build/https_dns_proxy -a $eth0_ip -p 53 -S $eth0_ip \
     -b 1.1.1.1,8.8.8.8 -r https://dns.google/dns-query -v -v -v \
     > /tmp/hdp.log 2>&1 &) && sleep 1"

"$docker_bin" exec "$container" bash -lc \
  "grep -q 'Received new DNS server IP' /tmp/hdp.log"

"$docker_bin" exec "$container" bash -lc \
  "grep -q \"Using source address: $eth0_ip\" /tmp/hdp.log"

dig_out="$("$docker_bin" exec "$container" bash -lc \
  \"dig @${eth0_ip} -p 53 example.com +short\")"

if [[ -z "$dig_out" ]]; then
  echo "dig returned no results; proxy may not be resolving." >&2
  "$docker_bin" exec "$container" bash -lc "tail -n 200 /tmp/hdp.log" || true
  exit 1
fi

echo "OK: bootstrap DNS and DoH resolution succeeded via https_dns_proxy."
