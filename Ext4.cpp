/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#include "Ext4.h"
#include "VoldUtil.h"

static char MKEXT4FS_PATH[] = "/system/bin/make_ext4fs";
static char  FSCK_EXT4_PATH[] = "/system/bin/e2fsck";

int Ext4::check(const char *fsPath)
{
    int status;
	SLOGI("Ext4::check");

	if (access(FSCK_EXT4_PATH, X_OK)) {
        SLOGW("Skipping fs checks\n");
        return 0;
    }

    int rc = 0;

    const char *args[6];
    args[0] = FSCK_EXT4_PATH;
	args[1] = "-p";
	args[2] = "-v";
    args[3] = fsPath;
    args[4] = NULL;

    rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false,
            true);
	if( rc != 0 )
	{
		SLOGE("Filesystem check failed (unknown exit code %d)", rc);
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        SLOGE("Filesystem (ext4) check did not exit properly");
        errno = EIO;
        return -1;
    }

    if ((status == 0) || (status == 256) || (status == 1024)) {
        SLOGI("Filesystem (ext4) checked OK");
        return 0;
    } else {
        SLOGE("Check (ext4) failed (unknown exit code %d)", status);
        errno = EIO;
        return -1;
    }

	return rc;
}

int Ext4::doMount(const char *fsPath, const char *mountPoint,
                 bool ro, bool remount, bool executable,
                 int ownerUid, int ownerGid, int permMask, bool createLost)
{
	int rc = Ext4::doMount(fsPath, mountPoint, ro, remount, executable);
	if( rc == 0 ) {
		chmod(mountPoint, (~permMask)&0777);
		chown(mountPoint, ownerUid, ownerGid );
	}
	
	return rc;
}

int Ext4::doMount(const char *fsPath, const char *mountPoint, bool ro, bool remount,
        bool executable) {
    int rc;
    unsigned long flags;

    flags = MS_NOATIME | MS_NODEV | MS_NOSUID | MS_DIRSYNC;

    flags |= (executable ? 0 : MS_NOEXEC);
    flags |= (ro ? MS_RDONLY : 0);
    flags |= (remount ? MS_REMOUNT : 0);

    rc = mount(fsPath, mountPoint, "ext4", flags, NULL);

    if (rc && errno == EROFS) {
        SLOGE("%s appears to be a read only filesystem - retrying mount RO", fsPath);
        flags |= MS_RDONLY;
        rc = mount(fsPath, mountPoint, "ext4", flags, NULL);
    }

    return rc;
}

int Ext4::format(const char *fsPath, const char *mountpoint) {
    int fd;
    const char *args[5];
    int rc;
    int status;

    args[0] = MKEXT4FS_PATH;
    args[1] = "-J";
    args[2] = "-a";
    args[3] = mountpoint;
    args[4] = fsPath;
    rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false,
            true);
    if (rc != 0) {
        SLOGE("Filesystem (ext4) format failed due to logwrap error");
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        SLOGE("Filesystem (ext4) format did not exit properly");
        errno = EIO;
        return -1;
    }

    status = WEXITSTATUS(status);

    if (status == 0) {
        SLOGI("Filesystem (ext4) formatted OK");
        return 0;
    } else {
        SLOGE("Format (ext4) failed (unknown exit code %d)", status);
        errno = EIO;
        return -1;
    }
    return 0;
}
