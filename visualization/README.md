# NebulaStream statistics visualization

Prerequisites: Docker with Docker Compose and NebulaStream statistics CSV files under `systest-output/results/`.

Start:

```bash
docker compose -f visualization/docker-compose.yml up -d --build
```

Stop:

```bash
docker compose -f visualization/docker-compose.yml down
```

Grafana: http://localhost:3000 (login: `admin` / `admin`).

The exporter automatically tails the newest regular file under `systest-output/results` whose name starts with `internal-statistics`, and switches when a newer matching file appears. The stable repository workspace is mounted so recreating `systest-output` or `results` does not break the bind mount. Override it with `NES_WORKSPACE_DIR=/path/to/nebulastream`.

Check exporter metrics:

```bash
curl http://localhost:8000/metrics | grep '^nes_'
```

This stack is standalone and does not modify NebulaStream itself.
