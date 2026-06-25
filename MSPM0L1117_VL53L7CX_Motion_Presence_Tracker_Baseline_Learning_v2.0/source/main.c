// MSPM0L1117 + VL53L7CX — BASELINE LEARNING v1 + SESSION JSON UART
// - 8x8 resolution
// - Adaptive per-pixel background learning
// - Supports wall / shelf / open-space background
// - 20-frame initial background learning
// - Foreground-vs-background connected blob detection
// - SPD-style 4-state session state machine
// - Stable presence hold
// - Zone classification + exclusive dwell times
// - first_entry_ms per zone
// - session_end JSON sent over UART to ESP32
//
// Baseline Learning v1 validated:
// - BG_INIT_FRAMES = 20
// - HUMAN_PIX_MIN_Z3 = 2
// - PRESENT_HOLD_FRAMES = 2
// - Stable stationary detection in Zone 1 / Zone 2 / Zone 3
// - Stable empty-room behavior after exit

#include "ti_msp_dl_config.h"
#include "vl53l7cx_api.h"
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
#define RES       VL53L7CX_RESOLUTION_8X8
#define GRID_N    64
#define GRID_W    8

// Baseline Learning v1 validated configuration
//#define FREQ_HZ              3
//#define INTEGRATION_TIME_MS  300

#define FREQ_HZ              10
#define INTEGRATION_TIME_MS  50

#define BG_INIT_FRAMES       20

/* Tracking window */
#define MIN_TRACK_MM  100
#define MAX_TRACK_MM  3500

/* Zone limits */
#define Z1_MAX  750
#define Z2_MAX  1500
#define Z3_MAX  3000

/* Foreground threshold */
#define FG_DELTA_MIN_MM      250
#define FG_NOISE_MULT        3
#define BG_NOISE_FLOOR_MM    30

/* Blob rules */
#define HUMAN_PIX_MIN        3
#define HUMAN_PIX_MIN_Z3     2
#define MAX_NEIGHBOR_DELTA   250
#define MAX_BLOB_SPAN_COLS   6
#define MAX_BLOB_SPAN_ROWS   8

/* Presence hold */
#define PRESENT_HOLD_FRAMES  2

/* Adaptive background */
#define BG_ADAPT_SHIFT       4       // 1/16 slow adaptation while empty
#define BG_CLEAR_MISSING_FRAMES 30   // clear learned bg if repeatedly invalid while empty

/* Session */
#define START_CONSEC_FRAMES  2   // kept for compatibility; FSM_CONFIRM_FRAMES is used by Phase 1 FSM
#define END_MISSING_MS       3000
#define MIN_SESSION_MS       1000

/* SPD-style Phase 1 state machine */
#define FSM_CONFIRM_FRAMES   1
#define FSM_LEAVE_FRAMES     2

#define TEST_MAX_RECORDS     1

/* Auto reboot test: 30 minutes */
#define AUTO_REBOOT_MS (30UL * 60UL * 1000UL)

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
    uint16_t best_dist;
    uint8_t best_idx;
    uint8_t zone;

    uint8_t min_row, max_row;
    uint8_t min_col, max_col;

    uint64_t mask;
} blob_t;

/* ---------------- GLOBAL STATE ---------------- */
static session_t ring[TEST_MAX_RECORDS];
static uint8_t ring_count = 0;
static uint32_t bootMs = 0;

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

/* Presence hold state */
static uint8_t missCount = 0;
static uint8_t presenceActive = 0;

/* SPD-style Phase 1 state machine */
typedef enum {
    FSM_IDLE = 0,
    FSM_POSSIBLE_PERSON,
    FSM_PERSON_CONFIRMED,
    FSM_LEAVING
} person_fsm_t;

static person_fsm_t personFsm = FSM_IDLE;
static uint8_t fsmPresentFrames = 0;
static uint8_t fsmAbsentFrames = 0;

/* Background model */
static uint8_t  bgReady = 0;
static uint8_t  bgLearnFrames = 0;
static uint8_t  bgCount[GRID_N];
static uint16_t bgMean[GRID_N];
static uint16_t bgMin[GRID_N];
static uint16_t bgMax[GRID_N];
static uint16_t bgNoise[GRID_N];
static uint8_t  bgValid[GRID_N];
static uint8_t  bgMissing[GRID_N];

/* ---------------- HELPERS ---------------- */
static inline bool ts_valid(uint8_t ts)
{
    return (ts == 5) || (ts == 6) || (ts == 9);
}

static inline uint8_t zone_from_dist(uint16_t d)
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

static inline int row_of(int idx) { return idx / GRID_W; }
static inline int col_of(int idx) { return idx % GRID_W; }

static inline bool pixel_has_valid_distance(const VL53L7CX_ResultsData *R, int i)
{
    if (R->nb_target_detected[i] <= 0) return false;
    if (!ts_valid(R->target_status[i])) return false;

    int d = R->distance_mm[i];
    if (d < MIN_TRACK_MM || d > MAX_TRACK_MM) return false;

    return true;
}

/* ---------------- RESET SESSION ---------------- */
static void reset_session(void)
{
    sessionActive = false;
    sessionStartMs = 0;
    sessionStartEpochMs = 0;
    lastSeenMs = 0;
    curZone = 255;
    startConsec = 0;

    presenceActive = 0;
    missCount = 0;

    personFsm = FSM_IDLE;
    fsmPresentFrames = 0;
    fsmAbsentFrames = 0;

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
        char json[420];

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

/* ---------------- BACKGROUND LEARNING ---------------- */
static void bg_reset(void)
{
    bgReady = 0;
    bgLearnFrames = 0;

    memset(bgCount, 0, sizeof(bgCount));
    memset(bgMean, 0, sizeof(bgMean));
    memset(bgMin, 0, sizeof(bgMin));
    memset(bgMax, 0, sizeof(bgMax));
    memset(bgNoise, 0, sizeof(bgNoise));
    memset(bgValid, 0, sizeof(bgValid));
    memset(bgMissing, 0, sizeof(bgMissing));
}

static void bg_initial_learn_frame(const VL53L7CX_ResultsData *R)
{
    for (int i = 0; i < GRID_N; i++) {
        if (!pixel_has_valid_distance(R, i))
            continue;

        uint16_t d = (uint16_t)R->distance_mm[i];

        if (bgCount[i] == 0) {
            bgMean[i] = d;
            bgMin[i] = d;
            bgMax[i] = d;
            bgCount[i] = 1;
        } else {
            uint32_t sum = ((uint32_t)bgMean[i] * bgCount[i]) + d;
            if (bgCount[i] < 255) bgCount[i]++;
            bgMean[i] = (uint16_t)(sum / bgCount[i]);

            if (d < bgMin[i]) bgMin[i] = d;
            if (d > bgMax[i]) bgMax[i] = d;
        }
    }

    if (bgLearnFrames < 255)
        bgLearnFrames++;

    if (bgLearnFrames >= BG_INIT_FRAMES) {
        for (int i = 0; i < GRID_N; i++) {
            if (bgCount[i] > 0) {
                bgValid[i] = 1;

                uint16_t n = (bgMax[i] >= bgMin[i]) ?
                             (uint16_t)(bgMax[i] - bgMin[i]) : 0;

                if (n < BG_NOISE_FLOOR_MM)
                    n = BG_NOISE_FLOOR_MM;

                bgNoise[i] = n;
            } else {
                bgValid[i] = 0;
                bgNoise[i] = BG_NOISE_FLOOR_MM;
            }
        }

        bgReady = 1;
        UartJson_Println("BG_READY");
    }
}

static bool bg_pixel_is_foreground(const VL53L7CX_ResultsData *R, int i)
{
    if (!pixel_has_valid_distance(R, i))
        return false;

    uint16_t d = (uint16_t)R->distance_mm[i];

    // Backgroundless pixel:
    // If this pixel never had a valid background target, any valid object
    // inside tracking range is foreground.
    if (!bgValid[i])
        return true;

    // Person/object must be closer than learned background.
    if (d >= bgMean[i])
        return false;

    uint16_t delta = (uint16_t)(bgMean[i] - d);

    uint16_t thr = FG_DELTA_MIN_MM;
    uint16_t noiseThr = (uint16_t)(bgNoise[i] * FG_NOISE_MULT);

    if (noiseThr > thr)
        thr = noiseThr;

    return (delta >= thr);
}

static void bg_build_foreground_mask(const VL53L7CX_ResultsData *R,
                                     bool fg[GRID_N],
                                     uint16_t dist[GRID_N],
                                     uint64_t *mask)
{
    *mask = 0ULL;

    for (int i = 0; i < GRID_N; i++) {
        fg[i] = false;
        dist[i] = 0;

        if (!bg_pixel_is_foreground(R, i))
            continue;

        fg[i] = true;
        dist[i] = (uint16_t)R->distance_mm[i];
        *mask |= (1ULL << i);
    }
}

static void bg_adapt_when_empty(const VL53L7CX_ResultsData *R)
{
    for (int i = 0; i < GRID_N; i++) {
        if (pixel_has_valid_distance(R, i)) {
            uint16_t d = (uint16_t)R->distance_mm[i];

            if (bgValid[i]) {
                // Slow IIR adaptation: bg = bg + (d-bg)/16
                int32_t old = (int32_t)bgMean[i];
                int32_t diff = (int32_t)d - old;
                bgMean[i] = (uint16_t)(old + (diff >> BG_ADAPT_SHIFT));

                uint16_t absdiff = (diff < 0) ? (uint16_t)(-diff) : (uint16_t)diff;
                bgNoise[i] = (uint16_t)((((uint32_t)bgNoise[i] * 15U) + absdiff) / 16U);

                if (bgNoise[i] < BG_NOISE_FLOOR_MM)
                    bgNoise[i] = BG_NOISE_FLOOR_MM;
            } else {
                // Empty scene now has a stable target here; initialize background.
                bgValid[i] = 1;
                bgMean[i] = d;
                bgNoise[i] = BG_NOISE_FLOOR_MM;
            }

            bgMissing[i] = 0;
        } else {
            // If background disappeared for many empty frames, convert pixel to
            // backgroundless/open-space.
            if (bgValid[i]) {
                if (bgMissing[i] < 255)
                    bgMissing[i]++;

                if (bgMissing[i] >= BG_CLEAR_MISSING_FRAMES) {
                    bgValid[i] = 0;
                    bgMissing[i] = 0;
                    bgNoise[i] = BG_NOISE_FLOOR_MM;
                }
            }
        }
    }
}

/* ---------------- FOREGROUND CONNECTED COMPONENT BLOB EXTRACTION ---------------- */
static blob_t extract_best_blob_from_fg(const bool fg[GRID_N],
                                        const uint16_t dist[GRID_N])
{
    bool visited[GRID_N];
    memset(visited, 0, sizeof(visited));

    blob_t best;
    memset(&best, 0, sizeof(best));
    best.found = false;
    best.best_dist = 65535;
    best.zone = 255;
    best.best_idx = 255;

    for (int seed = 0; seed < GRID_N; seed++) {
        if (!fg[seed] || visited[seed]) continue;

        blob_t b;
        memset(&b, 0, sizeof(b));
        b.found = false;
        b.pix_count = 0;
        b.best_dist = 65535;
        b.best_idx = 255;
        b.zone = 255;
        b.min_row = 255;
        b.min_col = 255;
        b.max_row = 0;
        b.max_col = 0;
        b.mask = 0ULL;

        uint8_t queue[GRID_N];
        uint8_t qh = 0, qt = 0;

        visited[seed] = true;
        queue[qt++] = (uint8_t)seed;

        while (qh < qt) {
            int cur = queue[qh++];

            int cr = row_of(cur);
            int cc = col_of(cur);
            uint16_t cd = dist[cur];

            b.pix_count++;
            b.mask |= (1ULL << cur);

            if (cd < b.best_dist) {
                b.best_dist = cd;
                b.best_idx = (uint8_t)cur;
            }

            if ((uint8_t)cr < b.min_row) b.min_row = (uint8_t)cr;
            if ((uint8_t)cr > b.max_row) b.max_row = (uint8_t)cr;
            if ((uint8_t)cc < b.min_col) b.min_col = (uint8_t)cc;
            if ((uint8_t)cc > b.max_col) b.max_col = (uint8_t)cc;

            for (int j = 0; j < GRID_N; j++) {
                if (!fg[j] || visited[j]) continue;

                int jr = row_of(j);
                int jc = col_of(j);

                if (abs_i(jr - cr) <= 1 && abs_i(jc - cc) <= 1) {
                    if (abs_i((int)dist[j] - (int)cd) <= MAX_NEIGHBOR_DELTA) {
                        visited[j] = true;
                        queue[qt++] = (uint8_t)j;
                    }
                }
            }
        }

        uint8_t span_rows = (uint8_t)(b.max_row - b.min_row + 1);
        uint8_t span_cols = (uint8_t)(b.max_col - b.min_col + 1);

        b.zone = zone_from_dist(b.best_dist);

        if (b.zone >= 3) continue;
        if (span_cols > MAX_BLOB_SPAN_COLS) continue;
        if (span_rows > MAX_BLOB_SPAN_ROWS) continue;

        uint8_t minPix = (b.zone == 2) ? HUMAN_PIX_MIN_Z3 : HUMAN_PIX_MIN;

        if (b.pix_count < minPix) continue;

        b.found = true;

        if (!best.found) {
            best = b;
        } else {
            if (b.pix_count > best.pix_count) {
                best = b;
            } else if (b.pix_count == best.pix_count && b.best_dist < best.best_dist) {
                best = b;
            }
        }
    }

    return best;
}

/* ---------------- MAIN ---------------- */
int main(void)
{
    SYSCFG_DL_init();

    UartJson_Println("JSON_BOOT_BASELINE_LEARNING_V1_SPD_NO_MOTION");

    SysTick->LOAD = (CPUCLK_FREQ / 1000U) - 1U;
    SysTick->VAL  = 0;
    SysTick->CTRL =
        SysTick_CTRL_CLKSOURCE_Msk |
        SysTick_CTRL_TICKINT_Msk   |
        SysTick_CTRL_ENABLE_Msk;

    bootMs = millis();

    VL_I2C_Init();
    VL_WaitMs(10);
    VL_LowPower(false);
    VL_WaitMs(10);
    VL_I2C_ResetPulse();
    VL_WaitMs(100);

    VL53L7CX_Configuration sensor;
    memset(&sensor, 0, sizeof(sensor));
    sensor.platform.address = 0x29;

    if (vl53l7cx_init(&sensor) != 0) {
        UartJson_Println("ERR_INIT");
        while (1) { }
    }

    vl53l7cx_set_resolution(&sensor, RES);
    vl53l7cx_set_ranging_frequency_hz(&sensor, FREQ_HZ);
    vl53l7cx_set_integration_time_ms(&sensor, INTEGRATION_TIME_MS);

    vl53l7cx_start_ranging(&sensor);

    reset_session();
    bg_reset();

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

        uint32_t now = millis();

        if ((now - bootMs) >= AUTO_REBOOT_MS) {
            UartJson_Println("DEBUG: AUTO_REBOOT_5MIN");
            VL_WaitMs(100);
            NVIC_SystemReset();
        }

        // ----------------------------------------------------
        // INITIAL BACKGROUND LEARNING PHASE
        // Learn every frame. Do not run presence/session logic
        // before BG_READY. This prevents false "present" frames
        // from pausing learning in difficult 2m/open-space scenes.
        // ----------------------------------------------------
        if (!bgReady) {
            bg_initial_learn_frame(&R);
            lastFrameMs = now;
            continue;
        }

        if (lastFrameMs == 0)
            lastFrameMs = now;

        bool fg[GRID_N];
        uint16_t fgDist[GRID_N];
        uint64_t fgMask = 0ULL;

        bg_build_foreground_mask(&R, fg, fgDist, &fgMask);

        blob_t blob = extract_best_blob_from_fg(fg, fgDist);

        bool rawPresent = blob.found;
        uint8_t z = blob.zone;

        // Baseline Learning v1 presence hold:
        // bridges short 1-2 frame dropouts while a person is stationary.
        bool present = false;

        if (rawPresent) {
            present = true;
            presenceActive = 1;
            missCount = 0;
        } else {
            if (presenceActive) {
                if (missCount < PRESENT_HOLD_FRAMES)
                    missCount++;

                present = (missCount < PRESENT_HOLD_FRAMES) ? true : false;

                if (!present) {
                    presenceActive = 0;
                } else {
                    if (curZone < 3)
                        z = curZone;
                }
            } else {
                present = false;
            }
        }

        /* ----------------------------------------------------
         * SPD-style Phase 1 state machine
         *
         * IDLE:
         *   No active session. Wait for a valid zone detection.
         *
         * POSSIBLE_PERSON:
         *   Candidate person. Requires FSM_CONFIRM_FRAMES valid
         *   present frames before starting a real session.
         *   This rejects 1-frame ghosts and unstable far-zone spikes.
         *
         * PERSON_CONFIRMED:
         *   Real session active. Accumulate exclusive zone dwell.
         *
         * LEAVING:
         *   Person disappeared. Wait FSM_LEAVE_FRAMES absent frames
         *   before closing and sending JSON. If person returns, continue
         *   the same session.
         *
         * Phase 1 intentionally does NOT use motion_ok to start sessions.
         * Presence comes from baseline-learning foreground blobs.
         * ---------------------------------------------------- */

        switch (personFsm) {
        case FSM_IDLE:
            sessionActive = false;
            startConsec = 0;

            if (present && z < 3) {
                personFsm = FSM_POSSIBLE_PERSON;
                fsmPresentFrames = 1;
                fsmAbsentFrames = 0;
            }
            break;

        case FSM_POSSIBLE_PERSON:
            if (present && z < 3) {
                if (fsmPresentFrames < 255)
                    fsmPresentFrames++;

                if (fsmPresentFrames >= FSM_CONFIRM_FRAMES) {
                    // Confirm real person and start session now.
                    personFsm = FSM_PERSON_CONFIRMED;
                    sessionActive = true;
                    sessionStartMs = now;
                    sessionStartEpochMs = current_epoch_ms();
                    lastSeenMs = now;
                    curZone = 255;
                    startConsec = START_CONSEC_FRAMES;

                    for (int i = 0; i < 3; i++) {
                        zoneEntered[i] = 0;
                        zoneFirstEntryMs[i] = 0;
                        zoneDwellMs[i] = 0;
                    }
                }
            } else {
                // Candidate disappeared before confirmation -> reject.
                personFsm = FSM_IDLE;
                fsmPresentFrames = 0;
                fsmAbsentFrames = 0;
            }
            break;

        case FSM_PERSON_CONFIRMED:
            if (present) {
                lastSeenMs = now;
                fsmAbsentFrames = 0;
            } else {
                personFsm = FSM_LEAVING;
                fsmAbsentFrames = 1;
            }
            break;

        case FSM_LEAVING:
            if (present) {
                // Person came back before timeout -> same session continues.
                personFsm = FSM_PERSON_CONFIRMED;
                lastSeenMs = now;
                fsmAbsentFrames = 0;
            } else {
                if (fsmAbsentFrames < 255)
                    fsmAbsentFrames++;

                if (fsmAbsentFrames >= FSM_LEAVE_FRAMES ||
                    (now - lastSeenMs) > END_MISSING_MS) {

                    uint32_t dur = now - sessionStartMs;

                    if (sessionActive && dur >= MIN_SESSION_MS) {
                        store_session();
                    }

                    reset_session();
                }
            }
            break;

        default:
            reset_session();
            break;
        }

        // Accumulate dwell only while the FSM has confirmed a real person.
        if (personFsm == FSM_PERSON_CONFIRMED && sessionActive && present && z < 3) {
            if (curZone != z) {
                curZone = z;

                if (!zoneEntered[z]) {
                    zoneEntered[z] = 1;
                    zoneFirstEntryMs[z] = now - sessionStartMs;
                }
            }

            zoneDwellMs[z] += (now - lastFrameMs);
        }

        // Slow background adaptation only when the FSM sees no person.
        // This keeps shelves/walls/drift updated, but prevents stationary
        // confirmed persons from being absorbed into the background.
        if (personFsm == FSM_IDLE && !present) {
            bg_adapt_when_empty(&R);
        }

        lastFrameMs = now;
    }
}
