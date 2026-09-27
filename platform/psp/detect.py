import os
import sys
from typing import TYPE_CHECKING

from methods import print_error
from platform_methods import validate_arch

if TYPE_CHECKING:
    from SCons.Script.SConscript import SConsEnvironment


def get_name():
    return "PSP"


def get_pspdev():
    return os.environ.get("PSPDEV", os.path.expanduser("~/pspdev"))


def can_build():
    return os.path.exists(os.path.join(get_pspdev(), "bin", "psp-g++"))


def get_tools(env: "SConsEnvironment"):
    return ["cc", "c++", "ar", "link", "textfile"]


def get_opts():
    return []


def get_doc_classes():
    return []


def get_doc_path():
    return "doc_classes"


def get_flags():
    return {
        "arch": "mips32",
        "target": "template_release",
        "threads": False,
        "modules_enabled_by_default": False,
        "disable_physics_2d": True,
        "disable_physics_3d": True,
        "disable_navigation_2d": True,
        "disable_navigation_3d": True,
        "disable_xr": True,
        "deprecated": False,
        "minizip": False,
        "brotli": False,
        "vulkan": False,
        "opengl3": False,
        "disable_exceptions": True,
        "optimize": "size",
        # EBOOT oyun klasörünü (--path) ve ileride game.pck'yi (--main-pack) argümanla alır.
        "disable_path_overrides": False,
        "accesskit": False,
        "sdl": False,
    }


def configure(env: "SConsEnvironment"):
    validate_arch(env["arch"], get_name(), ["mips32"])
    if env.dev_build or env["target"] != "template_release":
        print_error("PSP: only target=template_release is supported.")
        sys.exit(255)

    pspdev = get_pspdev()
    psp_sdk = os.path.join(pspdev, "psp", "sdk")
    bin_dir = os.path.join(pspdev, "bin")

    env["CC"] = os.path.join(bin_dir, "psp-gcc")
    env["CXX"] = os.path.join(bin_dir, "psp-g++")
    env["AR"] = os.path.join(bin_dir, "psp-ar")
    env["RANLIB"] = os.path.join(bin_dir, "psp-ranlib")
    env["LINK"] = os.path.join(bin_dir, "psp-g++")
    env["PROGSUFFIX"] = ".elf"
    env["ENV"]["PATH"] = bin_dir + os.pathsep + env["ENV"]["PATH"]

    env.Prepend(CPPPATH=["#platform/psp"])
    # pspdev newlib'de olmayan POSIX başlıkları için başarısız dönen yedekler (dlfcn.h, poll.h).
    env.Append(CPPPATH=["#platform/psp/posix_shim"])
    env.Append(CPPPATH=[os.path.join(psp_sdk, "include"), os.path.join(pspdev, "psp", "include")])
    env.Append(CPPDEFINES=["PSP_ENABLED", "UNIX_ENABLED", "UNIX_SOCKET_UNAVAILABLE", "_PSP_FW_VERSION=600", "NO_SAFE_CAST"])
    # GCC MIPS hedefi "mips" makrosunu (=1) tanımlar; alan/değişken adlarıyla çakışır.
    # int32_t/uint32_t'yi int/unsigned int yap (bkz. psp_stdint_fix.h).
    env.Append(CCFLAGS=["-include", env.Dir("#platform/psp").abspath + "/psp_stdint_fix.h"])
    env.Append(CCFLAGS=["-Umips", "-G0", "-ffunction-sections", "-fdata-sections"])
    # --gc-sections PSP modül bölümlerini (NID tabloları) silmesin diye KEEP'li linker betiği.
    env.Append(LINKFLAGS=["-G0", "-Wl,--gc-sections", "-T", env.File("#platform/psp/psp_gc.ld").abspath])
    env.Append(LIBPATH=[os.path.join(psp_sdk, "lib")])
    # libc/libm/libcglue/pspuser/psprtc/pspnet* derleyici specs'inden (*lib) doğru sırayla gelir.
    # Burada tekrar -lc vermek newlib'in chdir/getcwd/strtol'unu libcglue'dan önce bağlar (cwd boş kalır).
    env.Append(LIBS=["pspgum", "pspgu", "pspge", "pspdisplay", "pspctrl", "psppower", "pspdebug", "atomic"])
