SUMMARY = "bitbake-layers recipe"
DESCRIPTION = "Recipe created by bitbake-layers"
LICENSE = "MIT"

LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

#SRC_URI = "git://github.com/ninhnn2/licheepi_nano_gpio;protocol=https;branch=master"

inherit cmake
inherit deploy

# Modify these as desired
#PV = "0.1+git${SRCPV}"
#SRCREV = "a9cc086d9050419777df2b2f602b6e7040c63cfd"

#S = "${WORKDIR}/git"


SRC_URI = "file://fagpio.c"

python do_configure () {
    # Specify any needed configure commands here
    bb.plain("***********************************************");
    bb.plain("*                                             *");
    bb.plain("*  Example recipe Nguyen Nhut Ninh configure   *");
    bb.plain("*                                             *");
    bb.plain("***********************************************");
    bb.plain("*                                             *");
}

python do_build() {
    bb.plain("***********************************************");
    bb.plain("*                                             *");
    bb.plain("*  Example recipe created by bitbake-layers   *");
    bb.plain("*                                             *");
    bb.plain("***********************************************");
}

python do_compile() {
    bb.plain("***********************************************");
    bb.plain("*                                             *");
    bb.plain("*  Example recipe Nguyen Nhut Ninh compile  *");
    bb.plain("*                                             *");
    bb.plain("***********************************************");
    
}

python do_install() {
    bb.plain("***********************************************");
    bb.plain("*                                             *");
    bb.plain("*  Example recipe Nguyen Nhut Ninh install  *");
    bb.plain("*                                             *");
    bb.plain("***********************************************");
}
