FD650_MODULE_VERSION = 1.0
FD650_SITE = package/fd650/src
FD650_SITE_METHOD = local

define KERNEL_MODULE_BUILD_CMDS
    $(MAKE) -C '$(@D)' LINUX_DIR='$(LINUX_DIR)' CC='$(TARGET_CC)' LD='$(TARGET_LD)' modules
endef

$(eval $(kernel-module))
$(eval $(generic-package))

