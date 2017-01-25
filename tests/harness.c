/*
 * This file is part of clr-boot-manager.
 *
 * Copyright © 2016-2017 Intel Corporation
 *
 * clr-boot-manager is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE

#include <assert.h>
#include <check.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bootman.h"
#define _BOOTMAN_INTERNAL_
#include "bootman_private.h"
#undef _BOOTMAN_INTERNAL_
#include "files.h"
#include "nica/files.h"

#include "config.h"
#include "harness.h"

#define PLAYGROUND_ROOT TOP_BUILD_DIR "/tests/update_playground"

/**
 * 64-bit vs 32-bit test
 */
#if UINTPTR_MAX == 0xffffffffffffffff
#define EFI_STUB_SUFFIX "X64.EFI"
#define EFI_STUB_SUFFIX_L "x64.efi"
#else
#define EFI_STUB_SUFFIX "IA32.EFI"
#define EFI_STUB_SUFFIX_L "ia32.efi"
#endif

/**
 * i.e. $root/boot
 */
#define BOOT_FULL PLAYGROUND_ROOT "/" BOOT_DIRECTORY

#define EFI_START BOOT_FULL "/EFI"
/**
 * i.e. $dir/EFI/Boot/BOOTX64.EFI
 */
#define EFI_STUB_MAIN BOOT_FULL "/EFI/Boot/BOOT" EFI_STUB_SUFFIX

/**
 * Places that need to exist..
 */

/**
 * Systemd support
 */
#if defined(HAVE_SYSTEMD_BOOT)
#define ESP_BOOT_DIR EFI_START "/systemd"
#define ESP_BOOT_STUB ESP_BOOT_DIR "/systemd-boot" EFI_STUB_SUFFIX_L

#define BOOT_COPY_TARGET PLAYGROUND_ROOT "/usr/lib/systemd/boot/efi/systemd-boot" EFI_STUB_SUFFIX_L
#define BOOT_COPY_DIR PLAYGROUND_ROOT "/usr/lib/systemd/boot/efi"

/**
 * gummiboot support
 */
#elif defined(HAVE_GUMMIBOOT)
#define ESP_BOOT_DIR EFI_START "/gummiboot"
#define ESP_BOOT_STUB ESP_BOOT_DIR "/gummiboot" EFI_STUB_SUFFIX_L

#define BOOT_COPY_TARGET PLAYGROUND_ROOT "/usr/lib/gummiboot/gummiboot" EFI_STUB_SUFFIX_L
#define BOOT_COPY_DIR PLAYGROUND_ROOT "/usr/lib/gummiboot"

/**
 * goofiboot support
 */
#elif defined(HAVE_GOOFIBOOT)
#define ESP_BOOT_DIR EFI_START "/goofiboot"
#define ESP_BOOT_STUB ESP_BOOT_DIR "/goofiboot" EFI_STUB_SUFFIX_L
#define BOOT_COPY_TARGET PLAYGROUND_ROOT "/usr/lib/goofiboot/goofiboot" EFI_STUB_SUFFIX_L
#define BOOT_COPY_DIR PLAYGROUND_ROOT "/usr/lib/goofiboot"
#else
#error No known ESP loader
#endif

/* $moduledir/$i */
static const char *module_dirs[] = { "build", "source", "extra",   "kernel", "updates",
                                     "arch",  "crypto", "drivers", "fs",     "lib",
                                     "mm",    "net",    "sound" };
/* $moduledir/kernel/$i */
static const char *module_modules[] = { "arch/dummy.ko", "crypto/dummy.ko", "drivers/dummy.ko",
                                        "fs/dummy.ko",   "lib/dummy.ko",    "mm/dummy.ko",
                                        "net/dummy.ko",  "sound/dummy.ko" };

/**
 * Wrap nc_file_exists and spam to stderr
 */
__cbm_inline__ static inline bool noisy_file_exists(const char *p)
{
        bool b = nc_file_exists(p);
        if (b) {
                return b;
        }
        fprintf(stderr, "missing-file: %s does not exist\n", p);
        return b;
}

void confirm_bootloader(void)
{
        fail_if(!noisy_file_exists(EFI_STUB_MAIN), "Main EFI stub missing");
        fail_if(!noisy_file_exists(ESP_BOOT_DIR), "ESP target directory missing");
        fail_if(!noisy_file_exists(ESP_BOOT_STUB), "ESP target stub missing");
}

bool confirm_bootloader_match(void)
{
        if (!cbm_files_match(BOOT_COPY_TARGET, EFI_STUB_MAIN)) {
                fprintf(stderr, "EFI_STUB_MAIN doesn't match the source\n");
                return false;
        }
        if (!cbm_files_match(BOOT_COPY_TARGET, ESP_BOOT_STUB)) {
                fprintf(stderr, "ESP_BOOT_STUB(vendor) doesn't match the source\n");
                return false;
        }
        return true;
}

bool set_kernel_default(PlaygroundKernel *kernel)
{
        if (!kernel) {
                return false;
        }
        autofree(char) *link_source = NULL;
        autofree(char) *link_target = NULL;

        if (asprintf(&link_source,
                     "%s.%s.%s-%d",
                     KERNEL_NAMESPACE,
                     kernel->ktype,
                     kernel->version,
                     kernel->release) < 0) {
                return false;
        }

        /* i.e. default-kvm */
        if (asprintf(&link_target,
                     "%s/%s/default-%s",
                     PLAYGROUND_ROOT,
                     KERNEL_DIRECTORY,
                     kernel->ktype) < 0) {
                return false;
        }

        /* Purge old link */
        if (nc_file_exists(link_target) && unlink(link_target) < 0) {
                fprintf(stderr, "Cannot remove default symlink: %s\n", strerror(errno));
                return false;
        }

        /* Set new link */
        if (symlink(link_source, link_target) < 0) {
                fprintf(stderr, "Failed to create default-%s symlink\n", kernel->ktype);
                return false;
        }
        return true;
}

bool set_kernel_booted(PlaygroundKernel *kernel, bool did_boot)
{
        /*  /var/lib/kernel/k_booted_4.4.0-120.lts  */
        if (!kernel) {
                return false;
        }
        autofree(char) *p = NULL;

        if (asprintf(&p,
                     "%s/var/lib/kernel/k_booted_%s-%d.%s",
                     PLAYGROUND_ROOT,
                     kernel->version,
                     kernel->release,
                     kernel->ktype) < 0) {
                return false;
        }

        if (!did_boot) {
                if (nc_file_exists(p) && unlink(p) < 0) {
                        fprintf(stderr, "Cannot unlink %s: %s\n", p, strerror(errno));
                        return false;
                }
                return true;
        }
        if (!file_set_text((const char *)p, "clr-boot-manager file\n")) {
                fprintf(stderr, "Cannot write k_boot file %s: %s\n", p, strerror(errno));
                return false;
        }
        return true;
}

bool push_kernel_update(PlaygroundKernel *kernel)
{
        autofree(char) *kfile = NULL;
        autofree(char) *cmdfile = NULL;
        autofree(char) *conffile = NULL;
        autofree(char) *link_source = NULL;
        autofree(char) *link_target = NULL;
        autofree(char) *initrd_file = NULL;

        /* $root/$kerneldir/$prefix.native.4.2.1-137 */
        if (asprintf(&kfile,
                     "%s/%s/%s.%s.%s-%d",
                     PLAYGROUND_ROOT,
                     KERNEL_DIRECTORY,
                     KERNEL_NAMESPACE,
                     kernel->ktype,
                     kernel->version,
                     kernel->release) < 0) {
                return false;
        }

        /* $root/$kerneldir/initrd-$prefix.native.4.2.1-137 */
        if (asprintf(&initrd_file,
                     "%s/%s/initrd-%s.%s.%s-%d",
                     PLAYGROUND_ROOT,
                     KERNEL_DIRECTORY,
                     KERNEL_NAMESPACE,
                     kernel->ktype,
                     kernel->version,
                     kernel->release) < 0) {
                return false;
        }

        /* $root/$kerneldir/cmdline-$version-$release.$type */
        if (asprintf(&cmdfile,
                     "%s/%s/cmdline-%s-%d.%s",
                     PLAYGROUND_ROOT,
                     KERNEL_DIRECTORY,
                     kernel->version,
                     kernel->release,
                     kernel->ktype) < 0) {
                return false;
        }
        /* $root/$kerneldir/config-$version-$release.$type */
        if (asprintf(&conffile,
                     "%s/%s/config-%s-%d.%s",
                     PLAYGROUND_ROOT,
                     KERNEL_DIRECTORY,
                     kernel->version,
                     kernel->release,
                     kernel->ktype) < 0) {
                return false;
        }

        /* Write the "kernel blob" */
        if (!file_set_text((const char *)kfile, (char *)kernel->version)) {
                return false;
        }
        /* Write the "cmdline file" */
        if (!file_set_text((const char *)cmdfile, (char *)kernel->version)) {
                return false;
        }
        /* Write the "config file" */
        if (!file_set_text((const char *)conffile, (char *)kernel->version)) {
                return false;
        }
        /* Write the initrd file */
        if (!file_set_text((const char *)initrd_file, (char *)kernel->version)) {
                return false;
        }

        /* Create all the dirs .. */
        for (size_t i = 0; i < ARRAY_SIZE(module_dirs); i++) {
                const char *p = module_dirs[i];
                autofree(char) *t = NULL;

                /* $root/$moduledir/$version-$rel/$p */
                if (asprintf(&t,
                             "%s/%s/%s-%d/%s",
                             PLAYGROUND_ROOT,
                             KERNEL_MODULES_DIRECTORY,
                             kernel->version,
                             kernel->release,
                             p) < 0) {
                        return false;
                }
                if (!nc_mkdir_p(t, 00755)) {
                        fprintf(stderr, "Failed to mkdir: %s %s\n", p, strerror(errno));
                        return false;
                }
        }
        /* Create all the .ko's .. */
        for (size_t i = 0; i < ARRAY_SIZE(module_modules); i++) {
                const char *p = module_modules[i];
                autofree(char) *t = NULL;

                /* $root/$moduledir/$version-$rel/$p */
                if (asprintf(&t,
                             "%s/%s/%s-%d/%s",
                             PLAYGROUND_ROOT,
                             KERNEL_MODULES_DIRECTORY,
                             kernel->version,
                             kernel->release,
                             p) < 0) {
                        return false;
                }
                if (!file_set_text((const char *)t, (char *)kernel->version)) {
                        fprintf(stderr, "Failed to touch: %s %s\n", t, strerror(errno));
                        return false;
                }
        }
        return true;
}

bool push_bootloader_update(int revision)
{
        autofree(char) *text = NULL;

        if (asprintf(&text, "faux-bootloader-revision: %d\n", revision) < 0) {
                return false;
        }

        if (!nc_file_exists(BOOT_COPY_DIR)) {
                if (!nc_mkdir_p(BOOT_COPY_DIR, 00755)) {
                        fprintf(stderr, "Failed to mkdir %s: %s\n", BOOT_COPY_DIR, strerror(errno));
                        return false;
                }
        }
        if (!file_set_text(BOOT_COPY_TARGET, text)) {
                fprintf(stderr, "Failed to update bootloader: %s\n", strerror(errno));
                return false;
        }
        return true;
}

static int prepare_count = 0;

BootManager *prepare_playground(PlaygroundConfig *config)
{
        assert(config != NULL);

        BootManager *m = NULL;

        m = boot_manager_new();
        if (!m) {
                return NULL;
        }

        /* Purge last runs */
        if (nc_file_exists(PLAYGROUND_ROOT)) {
                if (!nc_rm_rf(PLAYGROUND_ROOT)) {
                        fprintf(stderr, "Failed to rm_rf: %s\n", strerror(errno));
                        return false;
                }
        }
        /* Now create fresh tree */
        if (!nc_mkdir_p(PLAYGROUND_ROOT, 00755)) {
                fprintf(stderr, "Cannot create playground root: %s\n", strerror(errno));
                return false;
        }

        if (!nc_mkdir_p(PLAYGROUND_ROOT "/var/lib/kernel", 00755)) {
                fprintf(stderr, "Cannot set up the booted dir: %s\n", strerror(errno));
                return false;
        }

        /* Construct /etc directory for os-release */
        if (!nc_mkdir_p(PLAYGROUND_ROOT "/" SYSCONFDIR, 00755)) {
                goto fail;
        }

        if (!file_set_text(PLAYGROUND_ROOT "/" SYSCONFDIR "/os-release",
                           "PRETTY_NAME=\"clr-boot-manager testing\"\n")) {
                goto fail;
        }

        /* Construct kernel config directory */
        if (!nc_mkdir_p(PLAYGROUND_ROOT "/" KERNEL_CONF_DIRECTORY, 00755)) {
                goto fail;
        }

        if (!boot_manager_set_prefix(m, PLAYGROUND_ROOT)) {
                goto fail;
        }

        if (m->sysconfig->root_device) {
                cbm_probe_free(m->sysconfig->root_device);
        }
        m->sysconfig->root_device = calloc(1, sizeof(struct CbmDeviceProbe));
        if (!m->sysconfig->root_device) {
                DECLARE_OOM();
                abort();
        }
        /* Create fake root device, and increase coverage */
        CbmDeviceProbe *root = m->sysconfig->root_device;
        if (prepare_count % 2 == 0) {
                *root = (CbmDeviceProbe){
                        .dev = 0,
                        .part_uuid = strdup("DUMMY_UUID"),
                        .uuid = strdup("UUID"),
                        .luks_uuid = strdup("fakeLuksUUID"),
                };
        } else {
                *root = (CbmDeviceProbe){
                        .dev = 0, .uuid = strdup("UUID"),
                };
        }
        ++prepare_count;

        /* Construct the root kernels directory */
        if (!nc_mkdir_p(PLAYGROUND_ROOT "/" KERNEL_DIRECTORY, 00755)) {
                goto fail;
        }
        /* Construct the root kernel modules directory */
        if (!nc_mkdir_p(PLAYGROUND_ROOT "/" KERNEL_MODULES_DIRECTORY, 00755)) {
                goto fail;
        }

        if (!nc_mkdir_p(PLAYGROUND_ROOT "/" BOOT_DIRECTORY, 00755)) {
                goto fail;
        }

        /* Copy the bootloader bits into the tree */
        if (!push_bootloader_update(0)) {
                goto fail;
        }

        /* Insert all intitial kernels into the root */
        for (size_t i = 0; i < config->n_kernels; i++) {
                PlaygroundKernel *k = &(config->initial_kernels[i]);
                if (!push_kernel_update(k)) {
                        goto fail;
                }
                /* Not default so skip */
                if (!k->default_for_type) {
                        continue;
                }
                if (!set_kernel_default(k)) {
                        goto fail;
                }
        }

        boot_manager_set_can_mount(m, false);
        boot_manager_set_image_mode(m, false);
        if (config->uts_name && !boot_manager_set_uname(m, config->uts_name)) {
                fprintf(stderr, "Cannot set given uname of %s\n", config->uts_name);
                goto fail;
        }

        return m;
fail:
        boot_manager_free(m);
        return NULL;
}

int kernel_installed_files_count(BootManager *manager, PlaygroundKernel *kernel)
{
        assert(manager);
        assert(kernel);

        autofree(char) *conf_file = NULL;
        autofree(char) *kernel_blob = NULL;
        autofree(char) *initrd_file = NULL;
        const char *vendor = NULL;
        int file_count = 0;

        vendor = boot_manager_get_vendor_prefix(manager);

        if (asprintf(&kernel_blob,
                     "%s/%s.%s.%s-%d",
                     BOOT_FULL,
                     KERNEL_NAMESPACE,
                     kernel->ktype,
                     kernel->version,
                     kernel->release) < 0) {
                abort();
        }
        if (asprintf(&initrd_file,
                     "%s/initrd-%s.%s.%s-%d",
                     BOOT_FULL,
                     KERNEL_NAMESPACE,
                     kernel->ktype,
                     kernel->version,
                     kernel->release) < 0) {
                abort();
        }
        if (asprintf(&conf_file,
                     "%s/loader/entries/%s-%s-%s-%d.conf",
                     BOOT_FULL,
                     vendor,
                     kernel->ktype,
                     kernel->version,
                     kernel->release) < 0) {
                abort();
        }

        if (nc_file_exists(kernel_blob)) {
                ++file_count;
        }
        if (nc_file_exists(conf_file)) {
                ++file_count;
        }
        if (nc_file_exists(initrd_file)) {
                ++file_count;
        }
        return file_count;
}

bool confirm_kernel_installed(BootManager *manager, PlaygroundKernel *kernel)
{
        return kernel_installed_files_count(manager, kernel) == 3;
}

bool confirm_kernel_uninstalled(BootManager *manager, PlaygroundKernel *kernel)
{
        return kernel_installed_files_count(manager, kernel) == 0;
}

bool create_timeout_conf(void)
{
        autofree(char) *timeout_conf = NULL;
        if (asprintf(&timeout_conf, "%s/%s/timeout", PLAYGROUND_ROOT, KERNEL_CONF_DIRECTORY) < 0) {
                return false;
        }
        if (!file_set_text((const char *)timeout_conf, (char *)"5")) {
                fprintf(stderr, "Failed to touch: %s %s\n", timeout_conf, strerror(errno));
                return false;
        }
        return true;
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */
