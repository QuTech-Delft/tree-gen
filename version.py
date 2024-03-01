import os
import re


def get_version(verbose=False):
    """Extract version information from source code"""

    root_dir = os.getcwd()  # root of the repository
    inc_dir = os.path.join(root_dir, "src")  # C++ source directory
    matcher = re.compile('.*tree_gen_version\\{[\t ]+"(.*)"')
    version = None
    with open(os.path.join(inc_dir, "version.cpp"), "r") as f:
        for ln in f:
            m = matcher.match(ln)
            if m:
                version = m.group(1)
                break

    if verbose:
        print("get_version: %s" % version)

    return version
