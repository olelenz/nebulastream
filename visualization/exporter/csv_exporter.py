import csv
import logging
import os
import time
from pathlib import Path

from prometheus_client import Counter, Gauge, start_http_server


STATISTICS_DIR = Path(os.environ.get("STATISTICS_DIR", "/statistics"))
POLL_SECONDS = float(
    os.environ.get("POLL_SECONDS", "0.25")
)

CPU_TIME_MS = Gauge("nes_worker_cpu_time_milliseconds", "Cumulative worker CPU time in milliseconds")
MEMORY_KB = Gauge("nes_worker_memory_usage_kilobytes", "Worker resident memory in kilobytes")
ACTIVE_QUERIES = Gauge("nes_worker_active_query_count", "Current active worker queries")
USED_BUFFERS = Gauge("nes_worker_buffer_used_count", "Current used worker buffers")
COMPILATION_TIME_MS = Gauge("nes_query_compilation_time_milliseconds", "Most recent query compilation time in milliseconds")

LAST_SEQUENCE = Gauge("nes_statistics_exporter_last_sequence", "Last processed statistics sequence number")
PARSE_ERRORS = Counter("nes_statistics_exporter_parse_errors_total", "Malformed statistics rows skipped")
FILE_SWITCHES = Counter("nes_statistics_exporter_file_switches_total", "Statistics file switches")
FILE_TIMESTAMP_MS = Gauge("nes_statistics_exporter_file_timestamp_milliseconds", "Selected file modification time in Unix milliseconds")

EVENTS = {
    "WorkerCpuTime": (CPU_TIME_MS, lambda value: value / 1000.0),
    "WorkerMemoryUsage": (MEMORY_KB, float),
    "WorkerActiveQueryCount": (ACTIVE_QUERIES, float),
    "WorkerBufferUsedCount": (USED_BUFFERS, float),
    "CompilationTime": (COMPILATION_TIME_MS, float),
}


def newest_statistics_file():
    try:
        files = [path for path in STATISTICS_DIR.iterdir() if path.is_file() and path.name.startswith("internal-statistics")]
        return max(files, key=lambda path: path.stat().st_mtime_ns, default=None)
    except OSError as error:
        logging.warning("Cannot scan %s: %s", STATISTICS_DIR, error)
        return None


def parse_row(text):
    stripped = text.strip()
    if not stripped or stripped.lstrip("\ufeff").startswith("seq,"):
        return None
    fields = next(csv.reader([stripped])) if "," in stripped else stripped.split()
    if fields and fields[0].strip().lstrip("\ufeff").lower() == "seq":
        return None
    if len(fields) != 5:
        raise ValueError("expected five fields")
    sequence = int(fields[0].strip())
    int(fields[1].strip())
    value = float(fields[3].strip())
    return sequence, value, fields[4].strip()


def consume(handle):
    while True:
        line_start = handle.tell()
        line = handle.readline()
        if not line:
            return
        if not line.endswith("\n"):
            handle.seek(line_start)
            return
        if line.endswith(",BufferAllocation\n") or line.rstrip().endswith((",BufferAllocation", " BufferAllocation")):
            continue
        try:
            parsed = parse_row(line)
            if parsed is None:
                continue
            sequence, value, event_type = parsed
            LAST_SEQUENCE.set(sequence)
            mapping = EVENTS.get(event_type)
            if mapping:
                metric, convert = mapping
                metric.set(convert(value))
        except (ValueError, csv.Error) as error:
            PARSE_ERRORS.inc()
            logging.warning("Skipping malformed row: %s (%s)", line.rstrip(), error)


def main():
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    start_http_server(8000)
    logging.info("Exporter listening on port 8000; watching %s", STATISTICS_DIR)
    selected = None
    selected_identity = None
    handle = None
    skip_initial_contents = newest_statistics_file() is not None

    while True:
        newest = newest_statistics_file()
        try:
            if newest is None:
                if handle:
                    handle.close()
                    handle = None
                selected = None
                selected_identity = None
            else:
                stat = newest.stat()
                identity = (stat.st_dev, stat.st_ino)
                changed = newest != selected or identity != selected_identity
                if changed:
                    if handle:
                        handle.close()
                        FILE_SWITCHES.inc()
                        logging.info("Switching statistics file: %s", newest.name)
                    else:
                        logging.info("Selected statistics file: %s", newest.name)
                    handle = newest.open("r", encoding="utf-8", newline="")
                    if skip_initial_contents:
                        handle.seek(0, os.SEEK_END)
                        skip_initial_contents = False
                        logging.info("Skipped existing contents of initial statistics file")
                    selected = newest
                    selected_identity = identity
                elif handle and stat.st_size < handle.tell():
                    logging.info("Statistics file truncated; resetting offset: %s", newest.name)
                    handle.seek(0)
                FILE_TIMESTAMP_MS.set(stat.st_mtime_ns / 1_000_000.0)
                if handle:
                    consume(handle)
        except OSError as error:
            logging.warning("Cannot read statistics file: %s", error)
            if handle:
                handle.close()
            handle = None
            selected = None
            selected_identity = None
        time.sleep(POLL_SECONDS)


if __name__ == "__main__":
    main()
