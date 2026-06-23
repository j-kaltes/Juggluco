import os
import subprocess

version = "10.6.6"
apk_name = "Juggluco-{}.apk".format(version)
download_folder = "build-data"
apk_path = os.path.join(download_folder, apk_name)
libs_path = os.path.join(download_folder, "libs-apk")

if not os.path.exists(download_folder):
    os.makedirs(download_folder)

if not os.path.exists(apk_path):
    subprocess.run(
        [
            'wget',
            'https://phoenixnap.dl.sourceforge.net/project/juggluco/{}?viasf=1'.format(apk_name),
            '-O',
            apk_path,
        ],
        check=True,
    )
if os.path.exists(libs_path):
    subprocess.run(['rm', '-rf', libs_path])
subprocess.run(
    [
        'unzip',
        apk_path,
        '-d',
        libs_path,
    ],
    check=True,
)

all_libs = {
    'main': ['libcalibrat2.so', 'libcalibrate.so'],
    'libre3': ['libcrl_dp.so', 'liblibre3extension.so', 'libinit.so'],
    'mobileSi': [
        'libnative-algorithm-jni-v113B.so',
        'libnative-encrypy-decrypt-v110.so',
        'libnative-struct2json.so',
        'libnative-algorithm-v1_1_3_B.so',
        'libnative-sensitivity-v110.so',
    ],
}
for arch in ['x86', 'x86_64', 'armeabi-v7a', 'arm64-v8a']:
    for kind, libs in all_libs.items():
        path = os.path.join('Common/src', kind, 'jniLibs', arch)
        if not os.path.exists(path):
            os.makedirs(path)
        for lib in libs:
            archive_path = os.path.abspath(os.path.join(libs_path, 'lib', arch, lib))
            if not os.path.exists(archive_path):
                print("No file `{}`, ignoring...".format(archive_path))
            lib_path = os.path.join(path, lib)
            if os.path.islink(lib_path):
                os.unlink(lib_path)
            print("Creating link `{}` -> `{}`".format(archive_path, lib_path))
            os.symlink(archive_path, lib_path)
