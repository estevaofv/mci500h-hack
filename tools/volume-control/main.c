#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <linux/soundcard.h>

static void ioctl_check(int fd, int d, void *p)
{
	int res = ioctl(fd, d, p);

	if (res < 0) {
		fprintf(stderr, "IOCTL failed: %d\n", res);
		exit(1);
	}
}

static void status(int fd)
{
	int v = 2;

	ioctl_check(fd, SOUND_MIXER_PRIVATE2, &v);
	printf("Mute status: %d\n", v);

	ioctl_check(fd, SOUND_MIXER_READ_VOLUME, &v);
	printf("Volume: Left: %02x, Right: %02x\n", v & 0xff, (v >> 8) & 0xff);
}

static void setvol(int fd, unsigned long vol_in)
{
	unsigned long vol = vol_in & 0xffff;

	printf("Setting volume to Left %02lx, Right %02lx\n",
			vol & 0xff, (vol >> 8) & 0xff);
	ioctl_check(fd, SOUND_MIXER_WRITE_VOLUME, &vol);
}

static void mute(int fd)
{
	int v = 1;

	printf("Muting sound\n");
	ioctl_check(fd, SOUND_MIXER_PRIVATE2, &v);
}

static void unmute(int fd)
{
	int v = 0; /* See drivers/sound/pnx0106.c */

	printf("Unmuting sound\n");
	ioctl_check(fd, SOUND_MIXER_PRIVATE2, &v);
}

static void saO3_dac(int fd)
{
	int v = 1;

	printf("Connecting SAO3 to DAC\n");
	ioctl_check(fd, SOUND_MIXER_PRIVATE3, &v);
}

static void saO3_epics(int fd)
{
	int v = 0;

	printf("Connecting SAO3 to EPICS\n");
	ioctl_check(fd, SOUND_MIXER_PRIVATE3, &v);
}

static void sync_playback(int fd, unsigned long tsc_in)
{
	uint64_t v = tsc_in;

	printf("Setting up sync playback to %llux\n", v);
	ioctl_check(fd, SNDCTL_DSP_POST, &v);
}


static void usage(void)
{
	printf("Usage: audio-test <command> [options]\n\n"
			"Where <command> can be\n"
			"  status     Display status\n"
			"  mute       Mute the sound\n"
			"  unmute     Unmute the sound\n"
			"  saO3-dac   Connect SAO3 to DAC\n"
			"  saO3-epics Connect SAO3 to EPICS\n"
			"  sync <tsc> Setup sync playback\n"
			"  vol <vol>  Set the volume (e.g., 0x1020: right 0x10, left 0x20)\n"
			);
}

int main(int argc, const char *argv[])
{
	const char *command = argv[1];
	int mixer_fd, dsp_fd;

	if (argc < 2)
		usage();

	mixer_fd = open("/dev/mixer", O_RDONLY);
	if (mixer_fd < 0) {
		fprintf(stderr, "Can't open mixer device\n");
		return -1;
	}

	dsp_fd = open("/dev/dsp", O_RDONLY);
	if (dsp_fd < 0) {
		fprintf(stderr, "Can't open dsp device\n");
		return -1;
	}

	if (strcmp(command, "status") == 0)
		status(mixer_fd);
	if (strcmp(command, "mute") == 0)
		mute(mixer_fd);
	if (strcmp(command, "unmute") == 0)
		unmute(mixer_fd);
	if (strcmp(command, "saO3-dac") == 0)
		saO3_dac(mixer_fd);
	if (strcmp(command, "saO3-epics") == 0)
		saO3_epics(mixer_fd);
	if (strcmp(command, "sync") == 0) {
		const char *volstr = argv[2];
		char *endp;
		unsigned long tsc;

		if (argc < 3)
			usage();
		tsc = strtoul(volstr, &endp, 0);
		if (endp == volstr)
			usage();

		sync_playback(dsp_fd, tsc);
	}
	if (strcmp(command, "vol") == 0) {
		const char *volstr = argv[2];
		char *endp;
		unsigned long vol;

		if (argc < 3)
			usage();
		vol = strtoul(volstr, &endp, 0);
		if (endp == volstr)
			usage();

		setvol(mixer_fd, vol);
	}

	close(dsp_fd);
	close(mixer_fd);

	return 0;
}
