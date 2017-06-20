#
# Copyright (C) 2007-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
PKG_NAME:=wWpf
PKG_VERSION:=0.1
PKG_RELEASE:=1

PKG_MAINTAINER:=gauss
PKG_LICENSE:=GPL-2.0
OUTPUT_DIR=/home/edward/workdir/output


include $(INCLUDE_DIR)/package.mk


define Package/wWpf
  SECTION:=net
  CATEGORY:=Network
  TITLE:= wW platform
  DEPENDS:=+libjson-c +libpcre
endef

define Package/wWpf/preinst
#!/bin/sh
opkg remove wWpf
return 0
endef

define Package/wWpf/postinst
#!/bin/sh
/etc/init.d/wWpf enable
return 0
endef

define Package/wWpf/prerm
#!/bin/sh
return 0
endef

define Package/wWpf/postrm
#!/bin/sh
return 0
endef

define Build/Compile
	rm -rf $(PKG_BUILD_DIR)
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) -Rf ./src/* $(PKG_BUILD_DIR)/
	$(MAKE) -C $(PKG_BUILD_DIR) "CC=$(TARGET_CC)" "STRIP=$(RSTRIP)"	
endef

define Package/wWpf/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wWpf $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/wWpf $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,wWpf))
