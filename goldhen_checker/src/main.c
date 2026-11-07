// goldhen_checker v2.0 — PS4 GoldHEN version checker + auto-downloader
// Code and design: (c) Hamza Aichaoui (from scratch, 2026)
//
// At boot (GoldHEN payload autorun):
//   1. reads installed version from /data/GoldHEN/payloads/goldhen.bin
//   2. fetches latest version from the user's GitHub repo (raw latest.txt)
//   3. equal      -> "You are on the latest GoldHEN version (x.y.z) - by hamza"
//   4. newer      -> downloads goldhen.bin from the repo, backs up the old one,
//                    swaps it in, then asks to restart the console
//   5. disk changed by hand (FTP) between boots -> activate-on-restart notice
//
// It never sends anything anywhere; it only GETs two small files.
//
// Known credit appears as "by hamza" on every screen message.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define CREDIT "hamza"

#define GOLDHEN_BIN   "/data/GoldHEN/payloads/goldhen.bin"
#define GOLDHEN_BAK   "/data/GoldHEN/payloads/goldhen.bin.bak"
#define GOLDHEN_VER   "/data/GoldHEN/payloads/goldhen.ver"
#define LOG_PATH      "/data/goldhen_checker.log"
#define STATE_DIR     "/data/goldhen_checker"
#define STATE_PATH    "/data/goldhen_checker/active_version.txt"
#define LATEST_LOCAL  "/data/goldhen_checker/latest.txt"
#define YIELD_PATH    STATE_DIR "/latest.txt"
#define DL_TMP        STATE_DIR "/goldhen_new.bin"

// user's repo — where the manual uploads live
#define URL_LATEST "https://raw.githubusercontent.com/hamzaaichaoui8/gch/main/latest.txt"
#define URL_BINARY "https://raw.githubusercontent.com/hamzaaichaoui8/gch/main/goldhen.bin"

// Internal sysmodule ids (PS4): libSceNetCtl / libSceHttp / libSceSsl / libSceNet
#define ORBIS_SYSMODULE_INTERNAL_NETCTL 0x80000009u
#define ORBIS_SYSMODULE_INTERNAL_HTTP   0x8000000Au
#define ORBIS_SYSMODULE_INTERNAL_SSL    0x8000000Bu
#define ORBIS_SYSMODULE_INTERNAL_NET    0x8000001Cu

#define ORBIS_METHOD_GET        0        // integer method table (GET = 0)
#define ORBIS_HTTP_VERSION_1_1  2        // 1_0 = 1, 1_1 = 2 (SCE HTTP family)
#define HTTP_UA "goldhen_checker/2.0 by hamza"

#define NET_POOL (16 * 1024)
#define SSL_POOL (512 * 1024)
#define HTTP_POOL (512 * 1024)
#define IO_CHUNK (64 * 1024)

struct OrbisNotificationRequest
{
	uint32_t unused0;
	uint32_t incomplete;
	uint32_t useIconImageUri;
	uint32_t targetId;
	char     iconImageUri[1024];
	char     message[1024];
};

// --- prototypes (SDK headers declare these without types here) ---
int sceKernelSendNotificationRequest(int, struct OrbisNotificationRequest *, int, int);
int sceKernelSleep(unsigned int);
int sceKernelMkdir(const char *, unsigned int);
int sceSysmoduleLoadModuleInternal(unsigned int);
int sceNetInit(void);
int sceNetPoolCreate(const char *, int, int);
int sceNetPoolDestroy(int);
int sceSslInit(int);
void sceSslTerm(void);
int sceHttpInit(int, int, uint64_t);
void sceHttpTerm(int);
int sceHttpCreateTemplate(int, const char *, int, int);
int sceHttpsSetSslCallback(int, void *, void *);
int sceHttpCreateConnectionWithURL(int, const char *, int);
int sceHttpCreateRequestWithURL(int, int, const char *, uint64_t);
int sceHttpSendRequest(int, const void *, uint64_t);
int sceHttpGetStatusCode(int, int *);
int sceHttpReadData(int, void *, uint64_t);
int sceHttpDeleteRequest(int);
int sceHttpDeleteConnection(int);
int sceHttpDeleteTemplate(int);

static FILE *g_log = 0;

static void logf(const char *fmt, ...)
{
	va_list ap;
	if (!g_log)
		return;
	va_start(ap, fmt);
	vfprintf(g_log, fmt, ap);
	va_end(ap);
	fputs("\n", g_log);
}

static void notify(const char *msg)
{
	struct OrbisNotificationRequest req;
	memset(&req, 0, sizeof(req));
	strncpy(req.message, msg, sizeof(req.message) - 1);
	sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
	logf("notify: %s", msg);
}

// --- version string helpers -------------------------------------------------

// find "GoldHEN v..." inside a (decrypted) goldhen build — kept for possible
// future builds that ship the version in plaintext
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

// ponytail: plain numeric-list compare ("2.4b18.13" -> 2,4,18,13).
static void ver_tuple(const char *s, int *out, int max)
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
}

// latest >= local ?
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

// --- state file ---------------------------------------------------

static void save_state(const char *ver)
{
	sceKernelMkdir(STATE_DIR, 0777);
	FILE *sf = fopen(STATE_PATH, "w");
	if (sf)
	{
		fprintf(sf, "%s\n", ver);
		fclose(sf);
	}
}

// write the marker that records which goldhen.bin is deployed
static void save_installed_ver(const char *ver)
{
	sceKernelMkdir("/data/GoldHEN", 0777);
	sceKernelMkdir("/data/GoldHEN/payloads", 0777);
	FILE *f = fopen(GOLDHEN_VER, "w");
	if (f)
	{
		fprintf(f, "%s\n", ver);
		fclose(f);
	}
}

static int load_installed_ver(char *out, int n)
{
	FILE *f = fopen(GOLDHEN_VER, "rb");
	if (!f || !fgets(out, n, f))
	{
		if (f)
			fclose(f);
		return -1;
	}
	fclose(f);
	char *nl = strpbrk(out, "\r\n");
	if (nl)
		*nl = 0;
	return out[0] ? 0 : -2;
}

static int load_state(char *out, int n)
{
	FILE *sf = fopen(STATE_PATH, "rb");
	if (!sf || !fgets(out, n, sf))
	{
		if (sf)
			fclose(sf);
		return -1;
	}
	fclose(sf);
	char *nl = strpbrk(out, "\r\n");
	if (nl)
		*nl = 0;
	return 0;
}

// --- HTTP -----------------------------------------------------------

static int net_ready = 0;

static void net_begin(void)
{
	if (net_ready)
		return;
	logf("net init...");
	sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET);
	sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP);
	sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL);
	sceNetInit();
	int pool = sceNetPoolCreate("gcheckPool", NET_POOL, 0);
	if (pool < 0)
	{
		logf("pool create failed 0x%08X", pool);
		return;
	}
	int ssl = sceSslInit(SSL_POOL);
	if (ssl < 0)
	{
		logf("ssl init failed 0x%08X", ssl);
		sceNetPoolDestroy(pool);
		return;
	}
	int http = sceHttpInit(pool, ssl, HTTP_POOL);
	if (http < 0)
	{
		logf("http init failed 0x%08X", http);
		sceSslTerm();
		sceNetPoolDestroy(pool);
		return;
	}
	net_ready = 1;
	logf("net ready");
}

// ssl verify skip (accept everything; self-signed/consoles etc)
static int ssl_cb(int id, unsigned int err, void *const *certs, int n, void *arg)
{
	return 1;
}

// download url -> dst path; returns 0 on success, <0 on failure
static int http_download(const char *url, const char *dst)
{
	int ret;
	int tpl = 0;
	int conn = 0;
	int req = 0;
	int ok = -1;
	int status = 0;

	if (!net_ready)
		return -10;

	tpl = sceHttpCreateTemplate(1, HTTP_UA, ORBIS_HTTP_VERSION_1_1, 1);
	if (tpl < 0)
	{
		logf("template failed 0x%08X", tpl);
		return -11;
	}
	sceHttpsSetSslCallback(tpl, (void *)ssl_cb, NULL); // accept all certs

	conn = sceHttpCreateConnectionWithURL(tpl, url, 1);
	if (conn < 0)
	{
		logf("conn failed 0x%08X", conn);
		goto out;
	}
	req = sceHttpCreateRequestWithURL(conn, ORBIS_METHOD_GET, url, 0);
	if (req < 0)
	{
		logf("req failed 0x%08X", req);
		goto out;
	}
	ret = sceHttpSendRequest(req, NULL, 0);
	if (ret < 0)
	{
		logf("send failed 0x%08X", ret);
		goto out;
	}
	if (sceHttpGetStatusCode(req, &status) == 0 && status != 200)
	{
		logf("HTTP %d for %s", status, url);
		goto out;
	}

	{
		FILE *fd = fopen(dst, "wb");
		char *buf = malloc(IO_CHUNK);
		if (fd && buf)
		{
			int read;
			while ((read = (int)sceHttpReadData(req, buf, IO_CHUNK)) > 0)
			{
				if (fwrite(buf, 1, (unsigned int)read, fd) != (unsigned int)read)
				{
					ok = -20;
					break;
				}
			}
			if (read == 0)
				ok = 0;
			free(buf);
			fclose(fd);
		}
	}

out:
	if (req)
		sceHttpDeleteRequest(req);
	if (conn)
		sceHttpDeleteConnection(conn);
	if (tpl)
		sceHttpDeleteTemplate(tpl);
	logf("download %s ret=%d", url, ok);
	return ok;
}

// copy patch (src -> dst) with stdio, 64KB chunks
static int copy_file(const char *src, const char *dst)
{
	FILE *a = fopen(src, "rb");
	FILE *b = fopen(dst, "wb");
	char *buf = malloc(IO_CHUNK);
	int ok = -1;
	if (buf && a && b)
	{
		size_t r;
		while ((r = fread(buf, 1, IO_CHUNK, a)) > 0)
		{
			if (fwrite(buf, 1, r, b) != r)
				break;
		}
		ok = (r == 0) ? 0 : -1;
	}
	free(buf);
	if (b)
		fclose(b);
	if (a)
		fclose(a);
	return ok;
}

// --- main -----------------------------------------------------------

int main(void)
{
	char local[32] = {0};
	char latest[32] = {0};
	char saved[32] = {0};
	char msg[300];
	sceKernelMkdir(STATE_DIR, 0777);
	g_log = fopen(LOG_PATH, "w");
	if (g_log)
		setvbuf(g_log, NULL, _IONBF, 0);
	logf("goldhen_checker v2.0 by hamza");

	// installed version comes from the marker file next to goldhen.bin
	// (goldhen.bin is encrypted — it carries no readable version string)
	int r = load_installed_ver(local, sizeof(local));
	if (r != 0)
	{
		snprintf(msg, sizeof(msg),
			"goldhen_checker by %s: upload goldhen.ver next to goldhen.bin", CREDIT);
		notify(msg);
		return 1;
	}
	logf("installed=%s", local);

	if (load_state(saved, sizeof(saved)) == 0 && saved[0] && strcmp(saved, local) != 0)
	{
		snprintf(msg, sizeof(msg),
			"GoldHEN %s ready: RESTART console to activate it (was %s) - by %s",
			local, saved, CREDIT);
		notify(msg);
		save_state(local); // stop repeating; a fully-active boot clears it
		return 0;
	}

	// ---- check latest over the network ----
	net_begin();
	int have_latest = 0;
	if (net_ready)
	{
		if (http_download(URL_LATEST, YIELD_PATH) == 0)
		{
			FILE *lf = fopen(YIELD_PATH, "rb");
			if (lf && fgets(latest, sizeof(latest), lf))
			{
				char *nl = strpbrk(latest, "\r\n");
				if (nl)
					*nl = 0;
				have_latest = (latest[0] != 0);
				logf("remote latest=%s", latest);
			}
			if (lf)
				fclose(lf);
		}
	}
	if (!have_latest)
	{
		// fall back to the local copy if there is one
		FILE *lf = fopen(LATEST_LOCAL, "rb");
		if (lf && fgets(latest, sizeof(latest), lf))
		{
			char *nl = strpbrk(latest, "\r\n");
			if (nl)
				*nl = 0;
			have_latest = (latest[0] != 0);
		}
		if (lf)
			fclose(lf);
	}

	if (!have_latest)
	{
		snprintf(msg, sizeof(msg), "GoldHEN %s installed (check offline) - by %s", local, CREDIT);
		notify(msg);
		save_state(local);
		return 0;
	}

	// ---- on the newest version? ----
	if (strcmp(latest, local) == 0 || !ver_newer_or_equal(latest, local))
	{
		snprintf(msg, sizeof(msg),
			"You are on the latest GoldHEN version (%s) - by %s", local, CREDIT);
		notify(msg);
		logf("up to date (%s)", local);
		save_state(local);
		return 0;
	}

	// ---- update available: download + swap ----
	snprintf(msg, sizeof(msg),
		"GoldHEN %s -> %s available: downloading... - by %s", local, latest, CREDIT);
	notify(msg);

	if (http_download(URL_BINARY, DL_TMP) != 0)
	{
		snprintf(msg, sizeof(msg), "GoldHEN update download failed - by %s", CREDIT);
		notify(msg);
		save_state(local);
		return 1;
	}

	// sanity: a real goldhen.bin build is never tiny
	{
		FILE *chk = fopen(DL_TMP, "rb");
		long sz = 0;
		if (chk)
		{
			fseek(chk, 0, SEEK_END);
			sz = ftell(chk);
			fclose(chk);
		}
		if (sz < 100 * 1024)
		{
			snprintf(msg, sizeof(msg),
				"GoldHEN %s unchanged: downloaded file invalid (%ld bytes) - by %s",
				local, sz, CREDIT);
			notify(msg);
			remove(DL_TMP);
			save_state(local);
			return 1;
		}
	}

	copy_file(GOLDHEN_BIN, GOLDHEN_BAK);          // keep a backup of the old one
	if (copy_file(DL_TMP, GOLDHEN_BIN) != 0)
	{
		copy_file(GOLDHEN_BAK, GOLDHEN_BIN);      // roll back
		snprintf(msg, sizeof(msg), "GoldHEN update failed, kept %s - by %s", local, CREDIT);
		notify(msg);
		save_state(local);
		return 1;
	}
	remove(DL_TMP);

	snprintf(msg, sizeof(msg),
		"GoldHEN %s installed - RESTART console to activate it - by %s", latest, CREDIT);
	notify(msg);
	save_installed_ver(latest);
	save_state(latest);
	logf("update done: %s -> %s", local, latest);
	return 0;
}
