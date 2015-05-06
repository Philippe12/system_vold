#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/wait.h>

#include <linux/kdev_t.h>

#define LOG_TAG "Vold"

#include <cutils/log.h>
#include <cutils/properties.h>

#include <logwrap/logwrap.h>

#include "Exfat.h"
#include "VoldUtil.h"

static char FSCK_EXFAT_PATH[] = "/system/bin/fsck.exfat";
static char MK_EXFAT_PATH[] = "/system/bin/mkfs.exfat";
static char MOUNT_EXFAT_PATH[] = "/system/bin/mount.exfat";

extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);

int Exfat::check(const char *fsPath)
{
    int status;
	SLOGI("Exfat::check");

	if (access(FSCK_EXFAT_PATH, X_OK)) {
        SLOGW("Skipping fs checks\n");
        return 0;
    }

    int rc = 0;

    const char *args[5];
    args[0] = FSCK_EXFAT_PATH;
    args[1] = fsPath;
    args[2] = NULL;

    rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false,
            true);
	if( rc != 0 )
	{
		SLOGE("Filesystem check failed (unknown exit code %d)", rc);
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        SLOGE("Filesystem (exfat) check did not exit properly");
        errno = EIO;
        return -1;
    }

    if (status == 0) {
        SLOGI("Filesystem (exfat) checked OK");
        return 0;
    } else {
        SLOGE("Check (exfat) failed (unknown exit code %d)", status);
        errno = EIO;
        return -1;
    }

	return rc;
}

int Exfat::doMount(const char *fsPath, const char *mountPoint,
                       bool ro, bool remount, bool executable,
                       int ownerUid, int ownerGid, int permMask, bool createLost)
{
	int rc;
    const char *args[11];
    char mountData[255];
    int status;

    SLOGE("Exfat::doMount");
    sprintf(mountData,
            "locale=utf8,uid=%d,gid=%d,fmask=%o,dmask=%o,noatime,nodiratime",
            ownerUid, ownerGid, permMask, permMask);

    args[0] = MOUNT_EXFAT_PATH;
    args[1] = fsPath;
    args[2] = mountPoint;
    args[3] = "-o";
    args[4] = mountData;
    args[5] = NULL;
    rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false,
            true);
	if( rc !=0 )
	{
		SLOGE("Exfat::doMount error :", strerror(errno));
        errno = EIO;
        return -1;
	}

    if (!WIFEXITED(status)) {
        SLOGE("Filesystem (ext4) format did not exit properly");
        errno = EIO;
        return -1;
    }

    if (status == 0) {
        SLOGI("Filesystem (exfat) Mounted OK");
        return 0;
    } else {
        SLOGE("Mount (exfat) failed (unknown exit code %d)", status);
        errno = EIO;
        return -1;
    }

	return rc;
}

int Exfat::format(const char *fsPath, unsigned int numSectors)
{
	SLOGW("donnot support exfat format");
	return 0;
}
