/*
 * Host tests for Filter's mailbox handling. Each test runs in a forked
 * child inside its own temporary directory, so exit(), crashes, and
 * AddressSanitizer reports all count as failures without stopping the run.
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "amiga_stubs.h"
#include "filter.h"
#include "user.h"

extern char CFGName[255], MBoxName[255], Home[255], Default[255], DefDir[255];

static char dir[PATH_MAX];

/* ---- helpers ---------------------------------------------------------- */

static void fail(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "    ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

static void write_file(const char *name, const char *content)
{
	FILE *f = fopen(name, "w");

	if (f == NULL) fail("cannot write %s", name);
	fputs(content, f);
	fclose(f);
}

/* Returns a malloc'd copy of the file, or NULL if it does not exist. */
static char *read_file(const char *name)
{
	FILE *f = fopen(name, "r");
	char *buf;
	long n;

	if (f == NULL) return NULL;
	fseek(f, 0, SEEK_END);
	n = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = malloc(n + 1);
	n = (long)fread(buf, 1, n, f);
	buf[n] = 0;
	fclose(f);
	return buf;
}

static void expect_file(const char *name, const char *want)
{
	char *got = read_file(name);

	if (got == NULL) fail("%s: missing, want:\n%s", name, want);
	if (strcmp(got, want) != 0)
		fail("%s: content mismatch\n--- got (%zu bytes) ---\n%s\n--- want (%zu bytes) ---\n%s",
		     name, strlen(got), got, strlen(want), want);
	free(got);
}

static void expect_absent_or_empty(const char *name)
{
	char *got = read_file(name);

	if (got != NULL && got[0] != 0) fail("%s: expected empty, has:\n%s", name, got);
	free(got);
}

static int file_contains(const char *name, const char *needle)
{
	char *got = read_file(name);
	int found = got != NULL && strstr(got, needle) != NULL;

	free(got);
	return found;
}

/* Point Filter's globals at the test directory. */
static void setup(void)
{
	snprintf(Home, sizeof Home, "%s/", dir);
	snprintf(MBoxName, sizeof MBoxName, "%s/inbox", dir);
	snprintf(CFGName, sizeof CFGName, "%s/.filterrc", dir);
	snprintf(DefDir, sizeof DefDir, "%s/mail/", dir);
	snprintf(Default, sizeof Default, "%s/mail/default", dir);
	mkdir("mail", 0700);
	setenv("HOME", dir, 1);
}

/* ---- sample messages -------------------------------------------------- */

#define MSG_A \
	"From alice@example.com Mon Jan  1 00:00:00 1996\n" \
	"From: alice@example.com\n" \
	"To: me@example.com\n" \
	"Subject: hello\n" \
	"\n" \
	"body a\n" \
	"\n"

#define MSG_B \
	"From bob@example.com Tue Jan  2 00:00:00 1996\n" \
	"From: bob@example.com\n" \
	"To: me@example.com\n" \
	"Subject: b5 news\n" \
	"\n" \
	"body b\n" \
	"\n"

#define MSG_NEWS \
	"From carol@example.com Wed Jan  3 00:00:00 1996\n" \
	"From: carol@example.com\n" \
	"Newsgroups: comp.sys.amiga.misc\n" \
	"Subject: crosspost\n" \
	"\n" \
	"body n\n" \
	"\n"

#define MSG_LATE \
	"From dave@example.com Thu Jan  4 00:00:00 1996\n" \
	"From: dave@example.com\n" \
	"Subject: late\n" \
	"\n" \
	"body late\n" \
	"\n"

/* ---- tests ------------------------------------------------------------ */

/* Messages are routed by rule, copied byte for byte, and the inbox is emptied. */
static void test_routes_messages_exactly(void)
{
	write_file(".filterrc", "Subject: b5#?\nb5\n");
	write_file("inbox", MSG_A MSG_B);
	FilterMail();
	expect_file("mail/default", MSG_A);
	expect_file("mail/b5", MSG_B);
	expect_absent_or_empty("inbox");
}

/* A message without Newsgroups: must not inherit the previous message's. */
static void test_newsgroups_not_carried_over(void)
{
	write_file(".filterrc", "Newsgroups: comp.sys.amiga#?\nnews\n");
	write_file("inbox", MSG_NEWS MSG_A);
	FilterMail();
	expect_file("mail/news", MSG_NEWS);
	expect_file("mail/default", MSG_A);
}

/*
 * A header longer than its 256-byte field must be cut off cleanly. If the
 * field is left unterminated, From: runs on into the next field ("None"),
 * and this rule matches text the message never contained.
 */
static void test_long_header_truncated_cleanly(void)
{
	char msg[1024], from[400];

	memset(from, 'x', 300);
	from[300] = 0;
	snprintf(msg, sizeof msg,
		 "From x@example.com Mon Jan  1 00:00:00 1996\nFrom: %s\nSubject: s\n\nbody\n\n", from);
	write_file(".filterrc", "#?xNone\nwrong\n");
	write_file("inbox", msg);
	FilterMail();
	expect_file("mail/default", msg);
}

/* Whole-line comments in .filterrc, as in the sample config. */
static void test_config_comment_lines(void)
{
	write_file(".filterrc", "; comment\n;\nSubject: b5#?\nb5\n");
	write_file("inbox", MSG_A MSG_B);
	FilterMail();
	expect_file("mail/b5", MSG_B);
}

/* The config allows 255-character patterns; they must parse. */
static void test_long_pattern(void)
{
	char cfg[600];

	memset(cfg, 0, sizeof cfg);
	strcpy(cfg, "Subject: b5");
	while (strlen(cfg) < 250) strcat(cfg, "#?");
	strcat(cfg, "\nb5\n");
	write_file(".filterrc", cfg);
	write_file("inbox", MSG_A MSG_B);
	FilterMail();
	expect_file("mail/b5", MSG_B);
}

/* Long mail directory plus long folder name must not overflow the path buffer. */
static void test_long_destination_path(void)
{
	char cfg[300], name[120], path[400];

	memset(name, 'f', 100);
	name[100] = 0;
	snprintf(cfg, sizeof cfg, "Subject: b5#?\n%s\n", name);
	write_file(".filterrc", cfg);
	write_file("inbox", MSG_A MSG_B);
	FilterMail();
	snprintf(path, sizeof path, "mail/%s", name);
	expect_file(path, MSG_B);
}

/* "a,b" delivers to both folders. */
static void test_multiple_destinations(void)
{
	write_file(".filterrc", "Subject: b5#?\none,two\n");
	write_file("inbox", MSG_A MSG_B);
	FilterMail();
	expect_file("mail/one", MSG_B);
	expect_file("mail/two", MSG_B);
}

/* COMMAND: hands the command a file holding exactly the message. */
static void test_command_gets_message(void)
{
	write_file(".filterrc", "Subject: b5#?\nCOMMAND:sh -c 'cp \"$0\" got'\n");
	write_file("inbox", MSG_A MSG_B);
	FilterMail();
	expect_file("got", MSG_B);
}

/* A COMMAND: action near the 299-character limit must fit the command buffer. */
static void test_long_command_action(void)
{
	char cfg[400], pad[300];

	memset(pad, 'x', 250);
	pad[250] = 0;
	snprintf(cfg, sizeof cfg, "Subject: b5#?\nCOMMAND:sh -c 'cp \"$0\" got && : %s'\n", pad);
	write_file(".filterrc", cfg);
	write_file("inbox", MSG_A MSG_B);
	FilterMail();
	expect_file("got", MSG_B);
}

/* DELETE drops the message; others still go to the default folder. */
static void test_delete_action(void)
{
	write_file(".filterrc", "Subject: b5#?\nDELETE\n");
	write_file("inbox", MSG_A MSG_B);
	FilterMail();
	expect_file("mail/default", MSG_A);
	if (access("mail/DELETE", F_OK) == 0) fail("DELETE treated as a folder");
}

/*
 * Mail delivered while Filter is still working through the mailbox must
 * survive. The COMMAND action appends a message to the inbox while the last
 * message is being handled, the way a mail program might.
 */
static void test_mail_arriving_during_filtering_is_kept(void)
{
	write_file("late", MSG_LATE);
	write_file(".filterrc", "Subject: b5#?\nCOMMAND:sh -c 'cat late >> inbox'\n");
	write_file("inbox", MSG_A MSG_B);
	FilterMail();
	if (!file_contains("inbox", "body late") && !file_contains("mail/default", "body late"))
		fail("message delivered during filtering was lost");
}

/* A work file left by a crash is filtered on the next run. */
static void test_leftover_work_file_processed(void)
{
	write_file(".filterrc", "Subject: b5#?\nb5\n");
	write_file("inbox.filter", MSG_A);
	write_file("inbox", MSG_B);
	FilterMail();
	expect_file("mail/default", MSG_A);
	expect_file("mail/b5", MSG_B);
	if (access("inbox.filter", F_OK) == 0) fail("work file not removed");
}

/* An (almost) empty mailbox is left alone, and not left open. */
static void test_small_mailbox_closed(void)
{
	char link[PATH_MAX], target[PATH_MAX], inbox[PATH_MAX];
	struct dirent *d;
	DIR *fds;
	ssize_t n;

	write_file(".filterrc", "Subject: b5#?\nb5\n");
	write_file("inbox", "\n");
	FilterMail();
	snprintf(inbox, sizeof inbox, "%s/inbox", dir);
	fds = opendir("/proc/self/fd");
	while ((d = readdir(fds)) != NULL) {
		snprintf(link, sizeof link, "/proc/self/fd/%s", d->d_name);
		n = readlink(link, target, sizeof target - 1);
		if (n < 0) continue;
		target[n] = 0;
		if (strcmp(target, inbox) == 0) fail("mailbox left open");
	}
	closedir(fds);
}

/* A missing config file returns NULL without leaking (checked by LSan). */
static void test_missing_config_no_leak(void)
{
	if (LoadConfigList("no-such-file") != NULL) fail("expected NULL");
}

static void test_save_config_returns_true(void)
{
	struct ConfigList c;

	memset(&c, 0, sizeof c);
	strcpy(c.Pattern, "Subject: x");
	strcpy(c.Action, "x");
	if (SaveConfigList("saved", &c) != TRUE) fail("SaveConfigList did not return TRUE");
	expect_file("saved", "Subject: x\nx\n");
}

/* GetEnvHomeDir must not free() the string getenv() returns. */
static void test_home_dir_from_env(void)
{
	char home[256], want[PATH_MAX];

	GetEnvHomeDir(home);
	snprintf(want, sizeof want, "%s/", dir);
	if (strcmp(home, want) != 0) fail("got %s, want %s", home, want);
}

/* A long .signature line must not overflow AddSig's line buffer. */
static void test_signature_long_line(void)
{
	MsgIndexType msg;
	char sig[300];

	InitMsgIndexType(&msg);
	msg.Body = AddBodyAfter(NULL, "body");
	memset(sig, 's', 200);
	strcpy(sig + 200, "\n");
	write_file(".signature", sig);
	if (!AddSig(&msg, "me")) fail("AddSig failed");
	while (msg.Body != NULL) {
		BodyType *next = msg.Body->next;
		free(msg.Body);
		msg.Body = next;
	}
}

/* ---- runner ----------------------------------------------------------- */

struct test { const char *name; void (*fn)(void); };

#define T(f) { #f, f }
static const struct test tests[] = {
	T(test_routes_messages_exactly),
	T(test_newsgroups_not_carried_over),
	T(test_long_header_truncated_cleanly),
	T(test_config_comment_lines),
	T(test_long_pattern),
	T(test_long_destination_path),
	T(test_multiple_destinations),
	T(test_command_gets_message),
	T(test_long_command_action),
	T(test_delete_action),
	T(test_mail_arriving_during_filtering_is_kept),
	T(test_leftover_work_file_processed),
	T(test_small_mailbox_closed),
	T(test_missing_config_no_leak),
	T(test_save_config_returns_true),
	T(test_home_dir_from_env),
	T(test_signature_long_line),
};

int main(int argc, char **argv)
{
	size_t i;
	int failed = 0, run = 0;
	const char *tmpdir = getenv("TMPDIR");

	if (tmpdir == NULL || *tmpdir == 0) tmpdir = "/tmp";

	for (i = 0; i < sizeof tests / sizeof tests[0]; i++) {
		pid_t pid;
		int status;

		if (argc > 1 && strstr(tests[i].name, argv[1]) == NULL) continue;
		run++;
		snprintf(dir, sizeof dir, "%s/filter-test-XXXXXX", tmpdir);
		if (mkdtemp(dir) == NULL) {
			perror("mkdtemp");
			return 2;
		}
		fflush(NULL);
		pid = fork();
		if (pid == 0) {
			if (chdir(dir) != 0) {
				perror("chdir");
				_exit(2);
			}
			setup();
			tests[i].fn();
			exit(0);
		}
		waitpid(pid, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			char cmd[PATH_MAX + 16];

			printf("PASS %s\n", tests[i].name);
			snprintf(cmd, sizeof cmd, "rm -rf '%s'", dir);
			if (system(cmd) != 0) fprintf(stderr, "could not remove %s\n", dir);
		} else {
			printf("FAIL %s (files kept in %s)\n", tests[i].name, dir);
			failed++;
		}
	}
	printf("%d/%d passed\n", run - failed, run);
	return failed != 0;
}
