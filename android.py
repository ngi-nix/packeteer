#!/usr/bin/env python3

# This file is part of packeteer.
#
# Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
#
# Copyright (c) 2020 Jens Finkhaeuser.
#
# This software is licensed under the terms of the GNU GPLv3 for personal,
# educational and non-profit use. For all other uses, alternative license
# options are available. Please contact the copyright holder for additional
# information, stating your intended usage.
#
# You can find the full text of the GPLv3 in the COPYING file in this code
# distribution.
#
# This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

def get_host_properties(sdk, abi, platform):
    import os.path
    ar = platform['binaries']['ar']
    prefix, name = os.path.split(ar)
    cputype, _, system, _ = name.split('-')

    host_machine = {
        'endian': 'little',
    }
    if cputype == 'arm':
        host_machine['cpu'] = 'armv7a'
        host_machine['cpu_family'] = 'arm'
    else:
        host_machine['cpu'] = cputype
        host_machine['cpu_family'] = cputype

    host_machine['system'] = '%s%s' % (system, sdk['ndk_meta']['platforms']['max'])

    platform['host_machine'] = host_machine
    return prefix, platform


def get_sdk_info():
    # SDK deetails
    import os
    for key in ('ANDROID_SDK_ROOT', 'ANDROID_HOME'):
        sdk_root = os.environ.get(key, None)
    assert sdk_root is not None, ('Need to set ANDROID_SDK_ROOT environment '
            'variable; see '
            'https://developer.android.com/studio/command-line/variables'
            ' for details.')
    assert os.path.isdir(sdk_root), ('ANDROID_SDK_ROOT does not indicate a '
            'directory!')

    sdk_bin = os.path.join(sdk_root, 'tools', 'bin')
    assert os.path.isdir(sdk_bin), 'No tools/bin directory in ANDROID_SDK_ROOT'
    print('SDK root:', sdk_root)
    print('SDK tools:', sdk_bin)

    sdk = {
        'root': sdk_root,
        'tools': sdk_bin,
    }

    # Try to find ndk/<version> paths
    import glob, os.path, semver

    def _find_ndk_which(root):
        cmd = os.path.join(root, 'ndk-which')
        if os.path.isfile(cmd):
            return cmd
        return None

    # Try ndk/<version> first
    entries = glob.glob(os.path.join(sdk['root'], 'ndk', '*'))

    latest = None
    latest_ver = '0.0.0'
    ndk_which = None
    for entry in entries:
        if not os.path.isdir(entry):
            continue

        prefix, ver = os.path.split(entry)
        if semver.compare(ver, latest_ver) > 0:
            ret = _find_ndk_which(entry)
            if ret is not None:
                latest_ver = ver
                latest = entry
                ndk_which = ret

    # If that didn't yield anything, try ndk-bundle
    if latest is None:
        bundle = os.path.join(sdk['root'], 'ndk-bundle')
        if os.path.isdir(bundle):
            ret = _find_ndk_which(bundle)
            if ret is not None:
                latest = bundle
                ndk_which = ret

    assert latest is not None, 'No NDK detected!'
    print('NDK root:', latest)
    sdk['ndk_root'] = latest
    sdk['ndk_which'] = ndk_which

    # Detect NDK information from meta directory
    entries = glob.glob(os.path.join(sdk['ndk_root'], 'meta', '*.json'))
    import json
    sdk['ndk_meta'] = {}
    for entry in entries:
        prefix, name = os.path.split(entry)
        key, ext = os.path.splitext(name)

        with open(entry) as h:
            sdk['ndk_meta'][key] = json.loads(h.read())

    # Find sysroot
    from pathlib import Path
    entries = list(Path(os.path.join(sdk['ndk_root'], 'toolchains')).rglob('sysroot'))
    assert len(entries) > 0, 'No sysroot detected!'
    sysroot = str(entries[0])
    sdk['sysroot'] = sysroot

    # Populate all platform information
    import subprocess
    TOOLS = ['ar', 'ld', 'ranlib', 'strip']
    COMPILERS = {
        'cc': 'clang',
        'cpp': 'clang',
        'cxx': 'clang++',
    }
    platforms = {}
    for abi in sdk['ndk_meta']['abis'].keys():
        # Platform tools
        platform = { 'binaries': {} }
        for tool in TOOLS:
            ret = subprocess.run([sdk['ndk_which'], '--abi', abi, tool], stdout=subprocess.PIPE)
            toolpath = ret.stdout.decode('utf-8').strip()
            platform['binaries'][tool] = toolpath

        # Fill in host properties
        tool_prefix, platform = get_host_properties(sdk, abi, platform)

        # Platform properties
        platform['properties'] = {
            'vendor': 'linux',
            'sys_root': sdk['sysroot'],
            'target_dir': abi,
        }

        # Add compilers
        compiler_prefix = '%s-linux-%s' % (platform['host_machine']['cpu'],
                platform['host_machine']['system'])
        for name, filepart in COMPILERS.items():
            fullpath = os.path.join(tool_prefix, '%s-%s' % (compiler_prefix, filepart))
            assert os.path.isfile(fullpath), 'Tool not found!A'
            platform['binaries'][name] = fullpath

        # Host machine properties
        platforms[abi] = platform
    assert len(platforms) > 0, 'No platforms detected!'
    sdk['platforms'] = platforms
    print('Have configuration for platforms:', ', '.join(platforms.keys()))

    return sdk



def main(targets):
    sdk = get_sdk_info()

    # Ok, with the SDK platforms, write cross-files.
    if len(targets) <= 0:
        targets = list(sdk['platforms'].keys())

    import os.path
    for target in targets:
        # Create cross-file with platform name.
        cross_file = os.path.abspath('android-%s.txt' % (target,))

        # Generate config
        with open(cross_file, 'w') as fh:
            for section, data in sdk['platforms'][target].items():
                fh.write('[%s]\n' % (section,))
                for key, value in data.items():
                    fh.write("%s = '%s'\n" % (key, value))
                fh.write('\n')
        print('Cross-file written:', cross_file)


if __name__ == '__main__':
    import sys
    main(sys.argv[1:])
