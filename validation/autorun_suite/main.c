#include "nxgl.h"
#include "generated/autorun_version.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <hal/xbox.h>
#include <nxdk/mount.h>
#include <windows.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define AUTORUN_STRINGIFY_INNER(value) #value
#define AUTORUN_STRINGIFY(value) AUTORUN_STRINGIFY_INNER(value)
#define AUTORUN_SUITE_NAME "NXGL autorun suite v" AUTORUN_STRINGIFY(AUTORUN_VERSION)

#define AUTORUN_PROBE(name, fn) int fn(void);
#include "generated/probe_table.inc"
#undef AUTORUN_PROBE

typedef int (*ProbeMain)(void);

typedef struct ProbeCase {
    const char *name;
    ProbeMain main_fn;
} ProbeCase;

static const ProbeCase probes[] = {
#define AUTORUN_PROBE(name, fn) { name, fn },
#include "generated/probe_table.inc"
#undef AUTORUN_PROBE
};

static jmp_buf probe_exit_jmp;
static const char *report_path;
static bool report_append;
static const char *current_probe;
static int current_passes;
static int current_failures;
static int total_passes;
static int total_failures;
static int failed_probes;
static bool probe_running;

static bool ensure_e_drive(void)
{
    if (nxIsDriveMounted('E')) {
        return true;
    }

    return nxMountDrive('E', "\\Device\\Harddisk0\\Partition1\\");
}

static bool select_report_path(void)
{
    FILE *fp;

    ensure_e_drive();
    fp = fopen(AUTORUN_REPORT_E_PATH, "w");
    if (fp != NULL) {
        fclose(fp);
        report_path = AUTORUN_REPORT_E_PATH;
        report_append = true;
        return true;
    }

    fp = fopen(AUTORUN_REPORT_D_PATH, "w");
    if (fp != NULL) {
        fclose(fp);
        report_path = AUTORUN_REPORT_D_PATH;
        report_append = true;
        return true;
    }

    report_path = NULL;
    report_append = false;
    return false;
}

static void write_report_line(const char *line)
{
    FILE *fp;

    debugPrint("%s\n", line);
    if (report_path != NULL) {
        fp = fopen(report_path, report_append ? "a" : "w");
        if (fp != NULL) {
            fprintf(fp, "%s\n", line);
            fflush(fp);
            fclose(fp);
            report_append = true;
        }
    }
}

static bool starts_with(const char *text, const char *prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static void record_probe_output_line(const char *line)
{
    char prefixed[768];

    if (starts_with(line, "PASS:")) {
        ++current_passes;
        ++total_passes;
    } else if (starts_with(line, "FAIL:")) {
        ++current_failures;
        ++total_failures;
    }

    snprintf(prefixed, sizeof(prefixed), "[%s] %s", current_probe != NULL ? current_probe : "startup", line);
    write_report_line(prefixed);
}

void nxgl_autorun_debug_print(const char *fmt, ...)
{
    char buffer[768];
    char line[768];
    size_t pos = 0;
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        if (buffer[i] == '\r') {
            continue;
        }
        if (buffer[i] == '\n') {
            line[pos] = '\0';
            if (pos > 0) {
                record_probe_output_line(line);
            }
            pos = 0;
            continue;
        }
        if (pos + 1 < sizeof(line)) {
            line[pos++] = buffer[i];
        }
    }

    if (pos > 0) {
        line[pos] = '\0';
        record_probe_output_line(line);
    }
}

void nxgl_autorun_sleep(DWORD milliseconds)
{
    (void)milliseconds;
    if (probe_running) {
        longjmp(probe_exit_jmp, 1);
    }
}

static void run_probe(const ProbeCase *probe)
{
    char line[256];
    int rc = 0;

    current_probe = probe->name;
    current_passes = 0;
    current_failures = 0;

    snprintf(line, sizeof(line), "BEGIN %s", current_probe);
    write_report_line(line);

    if (setjmp(probe_exit_jmp) == 0) {
        probe_running = true;
        rc = probe->main_fn();
        probe_running = false;
        snprintf(line, sizeof(line), "[%s] returned %d before visual loop", current_probe, rc);
        write_report_line(line);
    } else {
        probe_running = false;
        snprintf(line, sizeof(line), "[%s] visual loop reached; continuing", current_probe);
        write_report_line(line);
    }

    nxglShutdown();

    if (current_failures > 0 || rc != 0) {
        ++failed_probes;
    }

    snprintf(line,
             sizeof(line),
             "END %s: pass=%d fail=%d rc=%d",
             current_probe,
             current_passes,
             current_failures,
             rc);
    write_report_line(line);
    write_report_line("");
}

int main(void)
{
    char line[256];
    const int probe_count = (int)(sizeof(probes) / sizeof(probes[0]));

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    select_report_path();

    write_report_line(AUTORUN_SUITE_NAME);
    snprintf(line, sizeof(line), "Build: %s", AUTORUN_BUILD_LABEL);
    write_report_line(line);
    write_report_line("Probe 28 expectation: line primitive accepted");
    snprintf(line,
             sizeof(line),
             "Report target: %s",
             report_path != NULL ? report_path : "unavailable (debug output only)");
    write_report_line(line);
    snprintf(line, sizeof(line), "Probe count: %d", probe_count);
    write_report_line(line);
    write_report_line("");

    for (int i = 0; i < probe_count; ++i) {
        run_probe(&probes[i]);
    }

    snprintf(line,
             sizeof(line),
             "SUMMARY probes=%d failed_probes=%d passes=%d failures=%d",
             probe_count,
             failed_probes,
             total_passes,
             total_failures);
    write_report_line(line);
    write_report_line("DONE");

    Sleep(1500);
    XLaunchXBE(NULL);

    return 0;
}
