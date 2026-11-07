#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// goldhen_checker v1.0 — PS4 GoldHEN version checker
// Code and design: (c) Hamza Aichaoui (from scratch, 2026)
// Built with OpenOrbis (docker: openorbisofficial/toolchain), deployed to
// /data/payloads/ and auto-loaded by GoldHEN at boot.
//
// NOTE: struct offsets for OrbisNotificationRequest must match the target
// firmware headers; verify on first build against OpenORBIS latest.

#define CREDIT "Hamza Aichaoui"

#define GOLDHEN_BIN   "/data/GoldHEN/payloads/goldhen.bin"
#define LOG_PATH      "/data/goldhen_checker.log"
#define LATEST_PATH   "/data/goldhen_checker/latest.txt"

struct OrbisNotificationRequest
{
	uint32_t unused0;
	uint32_t incomplete;
	uint32_t useIconImageUri;
	uint32_t targetId;
	char iconImageUri[1024];
	char message[1024];
};

int sceKernelSendNotificationRequest(int, struct OrbisNotificationRequest *, int, int);
int sceKernelSleep(unsigned int);

static void notify(const char *msg)
{
	struct OrbisNotificationRequest req;
	memset(&req, 0, sizeof(req));
	strncpy(req.message, msg, sizeof(req.message) - 1);
	sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
}

// ponytail: plain numeric-list compare ("2.4b18.13" -> 2,4,18,13).
// Good enough for GoldHEN tags; switch to semver lib if tags ever change shape.
static int ver_tuple(const char *s, int *out, int max)
{
	int n = 0;
	while (*s && n < max)
	{
		if (*s >= '0' && *s <= '9')
		{
			out[n++] = strtol(s, (char **)&s, 10);
			s++; // skip . or b
		}
		else
		{
			s++;
		}
	}
	return n;
}

static int ver_newer_or_equal(const char *latest, const char *local)
{
	int a[8] = {0}, b[8] = {0};
	ver_tuple(latest, a, 8);
	ver_tuple(local, b, 8);
	for (int i = 0; i < 8; i++)
	{
		if (a[i] != b[i])
			return a[i] > b[i];
	}
	return 1; // equal counts as "not outdated"
}

// scan goldhen.bin for "GoldHEN vX.Y..." and copy up to n chars
static int scan_installed_version(const char *path, char *out, int n)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return -1;
	static char buf[512 * 1024];
	size_t len = fread(buf, 1, sizeof(buf) - 1, f);
	fclose(f);
	buf[len] = 0;
	const char *tag = "GoldHEN v";
	char *p = strstr(buf, tag);
	if (!p)
		return -2;
	p += strlen(tag);
	int i = 0;
	while (p[i] && (p[i] == 'b' || (p[i] >= '0' && p[i] <= '9') || p[i] == '.') && i < n - 1)
	{
		out[i] = p[i];
		i++;
	}
	out[i] = 0;
	return i;
}

int main(void)
{
	char local[32] = "";
	char latest[32] = "";
	char msg[300];
	FILE *log = fopen(LOG_PATH, "w");
	if (log)
		setvbuf(log, NULL, _IONBF, 0);

	int r = scan_installed_version(GOLDHEN_BIN, local, sizeof(local));
	if (r < 0 || local[0] == 0)
	{
		snprintf(msg, sizeof(msg), "goldhen_checker by %s: no goldhen.bin (%d)", CREDIT, r);
		notify(msg);
		if (log)
			fputs(msg, log);
		return 1;
	}

	int outdated = -1; // unknown until we get a "latest"
	FILE *lf = fopen(LATEST_PATH, "rb");
	if (lf)
	{
		if (fgets(latest, sizeof(latest), lf))
		{
			char *nl = strpbrk(latest, "\r\n");
			if (nl)
				*nl = 0;
			outdated = ver_newer_or_equal(latest, local) ? 0 : 1;
		}
		fclose(lf);
	}

	if (outdated == 1)
		snprintf(msg, sizeof(msg), "GoldHEN %s - update available: %s - checker by %s", local, latest, CREDIT);
	else if (outdated == 0)
		snprintf(msg, sizeof(msg), "GoldHEN %s up to date - checker by %s", local, CREDIT);
	else
		snprintf(msg, sizeof(msg), "GoldHEN %s installed - checker by %s", local, CREDIT);

	notify(msg);
	if (log)
		fprintf(log, "installed=%s latest=%s outdated=%d\n", local, latest, outdated);
	if (log)
		fclose(log);
	return 0;
}
