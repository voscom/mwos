# Генерирует в файл version.h - Git-хеш версии проекта GIT_REVISION_PROJ и Git-хеш версии ос GIT_REVISION_MWOS
# скопировать в корень проекта
# в platformio.ini необходимо добавить:
# extra_scripts = pre:version_git.py
import os
import os.path
import shutil
import time
Import("env")
platform=env["PLATFORM"]
# BOARD можно задать конкретную строку. По умолчанию - берет board из platformio.ini
board=env["BOARD"]

if os.path.isdir("lib/mwos"):
    os.system("git -C ./lib/mwos log -1 \"--format=format:#define GIT_REVISION_MWOS \\\"%H\\\"%n\" HEAD > version.h")

    def is_can_copy_file(fname,notExtList):
        for notExt in notExtList:
            if fname.find(notExt)>-1:
                return 0
        return 1

    def copy_dir(subdir,notExtList):
        board_dir="./lib/mwos/examples/"+board+"/"+subdir
        subdir="."+subdir
        if not os.path.exists(board_dir):
            os.mkdir(board_dir)
        from os import listdir
        from os.path import isfile, join
        onlyfiles = [f for f in listdir(subdir) if isfile(join(subdir, f))]
        for fname in onlyfiles:
            if is_can_copy_file(fname,notExtList)==1:
                dst_file=board_dir+"/"+fname
                print(dst_file)
                if os.path.exists(dst_file):
                    os.remove(dst_file)
                shutil.copy(subdir+"/"+fname, board_dir)
    copy_dir("",[".h",".txt",".git"])
    # copy_dir("/src",[".bk"])

else:
    os.system("git -C ./.pio/libdeps/"+platform+"/mwos log -1 \"--format=format:#define GIT_REVISION_MWOS \\\"%H\\\"%n\" HEAD > version.h")

if os.path.isdir(".git"):
    os.system("git log -1 \"--format=format:#define GIT_REVISION_PROJ \\\"%H\\\"%n\" HEAD >> version.h")
else:
    s='#define GIT_REVISION_PROJ "t'+str(time.time())+'"'
    with open('version.h', 'a') as f:
        f.write(s)

env.Append(CPPDEFINES=[
    ("BOARD", env.StringifyMacro(board)),
])
