/******************************************************************************
 *
 * MSPM0L1117 + VL53L7CX Motion and Presence Tracker
 *
 * Description:
 * Presence and Motion detection, session analytics using the VL53L7CX
 * Time-of-Flight sensor.
 *
 * Features:
 * - 4x4 ranging
 * - Motion-gated detection
 * - Foreground extraction
 * - Blob clustering
 * - Zone tracking
 * - Session generation
 * - Dwell time measurement
 * - UART JSON output
 * - Ring buffer storage
 * - Time synchronization
 *
 * Platform:
 * - MSPM0L1117
 * - VL53L7CX
 *
 ******************************************************************************/
#include "ti_msp_dl_config.h"
#include "vl53l7cx_api.h"
#include "vl53l7cx_plugin_motion_indicator.h"
#include "vl53l7cx_i2c_if.h"
#include "vl53l7cx_uart_if.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* ---------------- SysTick millis() ---------------- */
static volatile uint32_t g_ms = 0;
void SysTick_Handler(void) { g_ms++; }
static inline uint32_t millis(void) { return g_ms; }

/* ---------------- TIME SYNC ---------------- */
static bool     g_time_valid = false;
static uint64_t g_epoch_at_sync_ms = 0;
static uint32_t g_millis_at_sync = 0;

static uint64_t current_epoch_ms(void)
{
    if (!g_time_valid) return 0ULL;
    return g_epoch_at_sync_ms + (uint64_t)(millis() - g_millis_at_sync);
}

/* ---------------- UART RX ---------------- */
static char uart_rx_buf[64];
static uint8_t uart_rx_idx = 0;

static uint64_t parse_u64(const char *s)
{
    return strtoull(s, NULL, 10);
}

/* ---------------- CONFIG ---------------- */
#define RES     VL53L7CX_RESOLUTION_4X4
#define FREQ_HZ 2

/* Tracking window */
#define MIN_TRACK_MM  250
#define MAX_TRACK_MM  3000

/* Zone limits */
#define Z1_MAX  750
#define Z2_MAX  1500
#define Z3_MAX  2800

/* Blob / human candidate rules */
#define HUMAN_PIX_MIN        2
#define HUMAN_PIX_MIN_Z3     2
#define MAX_NEIGHBOR_DELTA   350
#define MAX_BLOB_SPAN_COLS   3
#define MAX_BLOB_SPAN_ROWS   4

/* Motion gating */
#define MOTION_MIN_MM        400
#define MOTION_MAX_MM        1500
#define MOTION_START_THR     120
#define MOTION_ZONES_MIN     1

/* Session / anti-noise */
#define START_CONSEC_FRAMES  2
#define END_MISSING_MS       3000
#define MIN_SESSION_MS       1000

#define TEST_MAX_RECORDS 1

/* ---------------- SESSION STRUCT ---------------- */
typedef struct {
    uint64_t start_epoch_ms;
    uint64_t end_epoch_ms;
    uint32_t total_ms;
    uint8_t final_zone;

    uint8_t entered[3];
    uint32_t first_entry[3];
    uint32_t dwell[3];
} session_t;

/* ---------------- BLOB TYPE ---------------- */
typedef struct {
    bool found;
    uint8_t pix_count;
    int best_dist;
    uint8_t zone;
    uint8_t min_row, max_row, min_col, max_col;
} blob_t;

/* ---------------- RING BUFFER ---------------- */
static session_t ring[TEST_MAX_RECORDS];
static uint8_t ring_count = 0;

/* ---------------- SESSION STATE ---------------- */
static bool sessionActive = false;
static uint32_t sessionStartMs = 0;
static uint64_t sessionStartEpochMs = 0;
static uint32_t lastSeenMs = 0;
static uint32_t lastFrameMs = 0;
static uint8_t curZone = 255;
static uint8_t startConsec = 0;

static uint8_t zoneEntered[3] = {0};
static uint32_t zoneFirstEntryMs[3] = {0};
static uint32_t zoneDwellMs[3] = {0};

/* ---------------- MOTION CONFIG ---------------- */
static VL53L7CX_Motion_Configuration motion_cfg;

/* ---------------- HELPERS ---------------- */
static inline bool ts_valid(uint8_t ts)
{
    return (ts == 5) || (ts == 9);
}

static inline uint8_t zone_from_dist(int d)
{
    if (d < Z1_MAX) return 0;
    if (d < Z2_MAX) return 1;
    if (d < Z3_MAX) return 2;
    return 255;
}

static inline int abs_i(int x)
{
    return (x < 0) ? -x : x;
}

static inline int row_of(int idx) { return idx / 4; }
static inline int col_of(int idx) { return idx % 4; }

/* ---------------- RESET ---------------- */
static void reset_session(void)
{
    sessionActive = false;
    sessionStartMs = 0;
    sessionStartEpochMs = 0;
    lastSeenMs = 0;
    curZone = 255;
    startConsec = 0;

    for (int i = 0; i < 3; i++) {
        zoneEntered[i] = 0;
        zoneFirstEntryMs[i] = 0;
        zoneDwellMs[i] = 0;
    }
}

/* ---------------- SEND BUFFER ---------------- */
static void send_buffer(void)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "BUF_BEGIN,%d", ring_count);
    UartJson_Println(buf);

    for (int i = 0; i < ring_count; i++) {
        session_t *s = &ring[i];
        char json[360];

        snprintf(json, sizeof(json),
            "{\"event\":\"session_end\","
            "\"start_epoch_ms\":%llu,"
            "\"end_epoch_ms\":%llu,"
            "\"total_ms\":%u,"
            "\"final_zone\":%u,"
            "\"zones\":["
            "{\"idx\":1,\"entered\":%d,\"first_entry_ms\":%u,\"dwell_ms\":%u},"
            "{\"idx\":2,\"entered\":%d,\"first_entry_ms\":%u,\"dwell_ms\":%u},"
            "{\"idx\":3,\"entered\":%d,\"first_entry_ms\":%u,\"dwell_ms\":%u}"
            "],\"conversionEvent\":0}",
            (unsigned long long)s->start_epoch_ms,
            (unsigned long long)s->end_epoch_ms,
            s->total_ms,
            s->final_zone,
            s->entered[0], s->first_entry[0], s->dwell[0],
            s->entered[1], s->first_entry[1], s->dwell[1],
            s->entered[2], s->first_entry[2], s->dwell[2]
        );

        UartJson_Println(json);
    }

    UartJson_Println("BUF_END");
    ring_count = 0;
}

/* ---------------- STORE SESSION ---------------- */
static void store_session(void)
{
    if (ring_count >= TEST_MAX_RECORDS) return;

    session_t *s = &ring[ring_count];
    uint64_t end_epoch = current_epoch_ms();

    s->start_epoch_ms = sessionStartEpochMs;
    s->end_epoch_ms   = end_epoch;
    s->total_ms       = zoneDwellMs[0] + zoneDwellMs[1] + zoneDwellMs[2];
    s->final_zone     = (curZone < 3) ? (curZone + 1) : 0;

    for (int i = 0; i < 3; i++) {
        s->entered[i]     = zoneEntered[i];
        s->first_entry[i] = zoneFirstEntryMs[i];
        s->dwell[i]       = zoneDwellMs[i];
    }

    ring_count++;

    if (ring_count >= TEST_MAX_RECORDS) {
        send_buffer();
    } else {
        char buf[32];
        snprintf(buf, sizeof(buf), "BUF_COUNT,%d", ring_count);
        UartJson_Println(buf);
    }
}

/* ---------------- UART COMMAND ---------------- */
static void handle_uart_command(const char *cmd)
{
    if (!cmd || !cmd[0]) return;

    if ((cmd[0] == 'T') && (cmd[1] == ',')) {
        uint64_t epoch = parse_u64(&cmd[2]);

        if (epoch > 1000000000ULL) {
            g_epoch_at_sync_ms = epoch * 1000ULL;
            g_millis_at_sync   = millis();
            g_time_valid       = true;
            UartJson_Println("TIME_SYNC_OK");
        }
        return;
    }

    if (strcmp(cmd, "GET") == 0) {
        send_buffer();
    }
}

/* ---------------- UART POLL ---------------- */
static void poll_uart_commands(void)
{
    uint8_t c;

    while (UartJson_ReadByte(&c)) {
        if (c == '\n' || c == '\r') {
            uart_rx_buf[uart_rx_idx] = 0;

            if (uart_rx_idx > 0) {
                handle_uart_command(uart_rx_buf);
            }

            uart_rx_idx = 0;
        } else {
            if (uart_rx_idx < sizeof(uart_rx_buf) - 1) {
                uart_rx_buf[uart_rx_idx++] = c;
            }
        }
    }
}

/* ---------------- BLOB EXTRACTION ---------------- */
static blob_t extract_human_blob(const VL53L7CX_ResultsData *R)
{
    blob_t b;
    b.found = false;
    b.pix_count = 0;
    b.best_dist = 99999;
    b.zone = 255;
    b.min_row = 255;
    b.max_row = 0;
    b.min_col = 255;
    b.max_col = 0;

    bool valid[16];
    int dist[16];

    for (int i = 0; i < 16; i++) {
        valid[i] = false;
        dist[i] = 0;

        if (R->nb_target_detected[i] <= 0) continue;
        if (!ts_valid(R->target_status[i])) continue;

        {
            int d = R->distance_mm[i];
            if (d < MIN_TRACK_MM || d > MAX_TRACK_MM) continue;
            valid[i] = true;
            dist[i] = d;
        }
    }

    {
        int seed = -1;
        int seedDist = 99999;

        for (int i = 0; i < 16; i++) {
            if (valid[i] && dist[i] < seedDist) {
                seedDist = dist[i];
                seed = i;
            }
        }

        if (seed < 0) return b;

        {
            bool in_blob[16] = {0};
            uint8_t queue[16];
            uint8_t qh = 0, qt = 0;

            in_blob[seed] = true;
            queue[qt++] = (uint8_t)seed;

            while (qh < qt) {
                int cur = queue[qh++];
                int cr = row_of(cur);
                int cc = col_of(cur);
                int cd = dist[cur];

                if (cd < b.best_dist) b.best_dist = cd;
                b.pix_count++;

                if ((uint8_t)cr < b.min_row) b.min_row = (uint8_t)cr;
                if ((uint8_t)cr > b.max_row) b.max_row = (uint8_t)cr;
                if ((uint8_t)cc < b.min_col) b.min_col = (uint8_t)cc;
                if ((uint8_t)cc > b.max_col) b.max_col = (uint8_t)cc;

                for (int j = 0; j < 16; j++) {
                    if (!valid[j] || in_blob[j]) continue;

                    {
                        int jr = row_of(j);
                        int jc = col_of(j);

                        if (abs_i(jr - cr) <= 1 && abs_i(jc - cc) <= 1) {
                            if (abs_i(dist[j] - cd) <= MAX_NEIGHBOR_DELTA) {
                                in_blob[j] = true;
                                queue[qt++] = (uint8_t)j;
                            }
                        }
                    }
                }
            }
        }
    }

    {
        uint8_t span_rows = (uint8_t)(b.max_row - b.min_row + 1);
        uint8_t span_cols = (uint8_t)(b.max_col - b.min_col + 1);
        uint8_t z = zone_from_dist(b.best_dist);
        bool enoughPix = false;

        if (z == 2) enoughPix = (b.pix_count >= HUMAN_PIX_MIN_Z3);
        else        enoughPix = (b.pix_count >= HUMAN_PIX_MIN);

        if (!enoughPix) return (blob_t){0};
        if (span_cols > MAX_BLOB_SPAN_COLS) return (blob_t){0};
        if (span_rows > MAX_BLOB_SPAN_ROWS) return (blob_t){0};
        if (z >= 3) return (blob_t){0};

        b.found = true;
        b.zone = z;
    }

    return b;
}

/* ---------------- MOTION GATE ---------------- */
static uint8_t motion_gate_pass(const VL53L7CX_ResultsData *R)
{
    uint8_t hitZones = 0;

    for (int i = 0; i < 16; i++) {
        if (R->motion_indicator.motion[i] >= MOTION_START_THR) {
            hitZones++;
        }
    }

    return (hitZones >= MOTION_ZONES_MIN) ? 1U : 0U;
}

/* ---------------- MAIN ---------------- */
int main(void)
{
    SYSCFG_DL_init();

    UartJson_Println("JSON_BOOT_TEST");

    SysTick->LOAD = (CPUCLK_FREQ / 1000U) - 1U;
    SysTick->VAL  = 0;
    SysTick->CTRL =
        SysTick_CTRL_CLKSOURCE_Msk |
        SysTick_CTRL_TICKINT_Msk   |
        SysTick_CTRL_ENABLE_Msk;

    VL_I2C_Init();
    VL_WaitMs(10);
    VL_LowPower(false);
    VL_WaitMs(10);
    VL_I2C_ResetPulse();
    VL_WaitMs(100);

    {
        VL53L7CX_Configuration sensor;
        memset(&sensor, 0, sizeof(sensor));
        sensor.platform.address = 0x29;

        if (vl53l7cx_init(&sensor) != 0) {
            UartJson_Println("ERR_INIT");
            while (1) { }
        }

        vl53l7cx_set_resolution(&sensor, RES);
        vl53l7cx_set_ranging_frequency_hz(&sensor, FREQ_HZ);

        memset(&motion_cfg, 0, sizeof(motion_cfg));
        vl53l7cx_motion_indicator_init(&sensor, &motion_cfg, RES);
        vl53l7cx_motion_indicator_set_resolution(&sensor, &motion_cfg, RES);
        vl53l7cx_motion_indicator_set_distance_motion(&sensor, &motion_cfg, MOTION_MIN_MM, MOTION_MAX_MM);

        vl53l7cx_start_ranging(&sensor);

        reset_session();

        while (1)
        {
            VL53L7CX_ResultsData R;
            uint8_t ready = 0;

            poll_uart_commands();

            while (vl53l7cx_check_data_ready(&sensor, &ready) || !ready) {
                VL_WaitMs(1);
                poll_uart_commands();
            }

            if (vl53l7cx_get_ranging_data(&sensor, &R) != 0)
                continue;

            {
                uint32_t now = millis();
                blob_t blob;
                bool present;
                uint8_t z;
                uint8_t motion_ok;

                if (lastFrameMs == 0) lastFrameMs = now;

                blob = extract_human_blob(&R);
                present = blob.found;
                z = blob.zone;
                motion_ok = motion_gate_pass(&R);

                if (present) {
                    if (!sessionActive) {
                        if (motion_ok) {
                            lastSeenMs = now;

                            if (startConsec < 255) startConsec++;

                            if (startConsec >= START_CONSEC_FRAMES) {
                                sessionActive = true;
                                sessionStartMs = now;
                                sessionStartEpochMs = current_epoch_ms();
                                curZone = 255;

                                for (int i = 0; i < 3; i++) {
                                    zoneEntered[i] = 0;
                                    zoneFirstEntryMs[i] = 0;
                                    zoneDwellMs[i] = 0;
                                }
                            }
                        } else {
                            startConsec = 0;
                        }
                    } else {
                        lastSeenMs = now;
                        startConsec = START_CONSEC_FRAMES;
                    }

                    if (sessionActive && z < 3) {
                        if (curZone != z) {
                            curZone = z;
                            if (!zoneEntered[z]) {
                                zoneEntered[z] = 1;
                                zoneFirstEntryMs[z] = now - sessionStartMs;
                            }
                        }

                        zoneDwellMs[z] += (now - lastFrameMs);
                    }
                } else {
                    startConsec = 0;

                    if (sessionActive) {
                        if ((now - lastSeenMs) > END_MISSING_MS) {
                            uint32_t dur = now - sessionStartMs;

                            if (dur >= MIN_SESSION_MS) {
                                store_session();
                            }

                            reset_session();
                        }
                    }
                }

                lastFrameMs = now;
            }
        }
    }
}