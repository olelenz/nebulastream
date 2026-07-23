CREATE WORKER "localhost:8080" SET ('localhost:9090' AS DATA);

-- long running generator query
CREATE LOGICAL SOURCE endless(ts UINT64);
CREATE PHYSICAL SOURCE FOR endless TYPE Generator SET(
       'ALL' as `SOURCE`.STOP_GENERATOR_WHEN_SEQUENCE_FINISHES,
       'CSV' as INPUT_FORMATTER.`TYPE`,
       10000000 AS `SOURCE`.MAX_RUNTIME_MS,
       "localhost:8080" AS `SOURCE`.`HOST`,
       'emit_rate 1' AS `SOURCE`.GENERATOR_RATE_CONFIG,
       1 AS `SOURCE`.SEED,
       'SEQUENCE UINT64 0 10000000 1' AS `SOURCE`.GENERATOR_SCHEMA);

CREATE SINK endlessPrintSink(endless.ts UINT64) TYPE Print SET('CSV' as `SINK`.OUTPUT_FORMAT, "localhost:8080" AS `SINK`.`HOST`);
SHOW QUERIES;

SELECT TS FROM ENDLESS INTO endlessPrintSink SET ('long-running-query' AS `QUERY`.`ID`);

-- query the internal statistics buffer
CREATE LOGICAL SOURCE stats_logical(
    seq UINT64,
    ts UINT64,
    queryId VARSIZED,
    metricValue UINT64,
    eventType VARSIZED
);

CREATE PHYSICAL SOURCE FOR stats_logical TYPE Stats SET(
       1000 as `SOURCE`.POLL_INTERVAL_MS,
       100000 AS `SOURCE`.MAX_RUNTIME_MS,
       "localhost:8080" AS `SOURCE`.`HOST`,
       'CSV' as INPUT_FORMATTER.`TYPE`);

CREATE SINK statsPrintSink(stats_logical.seq UINT64, stats_logical.ts UINT64, stats_logical.queryId VARSIZED, stats_logical.metricValue UINT64, stats_logical.eventType VARSIZED) TYPE Print SET('CSV' as `SINK`.OUTPUT_FORMAT, "localhost:8080" AS `SINK`.`HOST`);

SELECT * FROM stats_logical INTO statsPrintSink SET ('stats-query' AS `QUERY`.`ID`);
SHOW QUERIES;
