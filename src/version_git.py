# Генерирует в файл version.h - Git-URL для проекта и всех библиотек
# скопировать в корень проекта (рядом с platformio.ini)
# в platformio.ini необходимо добавить:
# extra_scripts = pre:version_git.py
import os
import os.path
import shutil
import time
import random
Import("env")
platform=env["PLATFORM"]
# BOARD можно задать конкретную строку. По умолчанию - берет board из platformio.ini
board=env["BOARD"]
src_dir=env["PROJECT_SRC_DIR"]
envBoard=env["PIOENV"]
pArgs=env["PROGRAM_ARGS"]
# print(env.Dump())

ver_file="src/version.h"
if os.path.isdir(src_dir):
    ver_file=src_dir+"/version.h";

cur_p = os.getcwd()
fileTmp=cur_p+"/version.tmp"

def GitInf(p, libName, fileTmp):
    res=""
    if os.path.exists(p + "/.git"):
        p="\""+p+"\""
        os.system("git -C " + p + " remote get-url origin --push > "+fileTmp)
        libName=libName.replace(' ','_')
        res="'"+libName+"':'"
        with open(fileTmp, 'r') as f:
            res+=f.read().replace('\r','').replace('\n','')
        res+='#'
        os.system("git -C " + p + " log -1 \"--format=format:%H'\" HEAD > "+fileTmp)
        with open(fileTmp, 'r') as f:
            res+=f.read().replace('\r','').replace('\n','')
    return res

def LibsGitInf(p, fileTmp):
    res=''
    items = os.listdir(p)
    for d in items:
        fd=p + '/' + d
        if os.path.isdir(fd):
            s=GitInf(fd, d, fileTmp)
            if s:
                res+=','+s;
            else:
                res+=LibsGitInf(fd,fileTmp)
    return res

def GetGitShortHash(projGitVer):
    ver=''
    n=-1
    for ch in projGitVer:
        if n>0 :
            ver += ch
            n -= 1
        else:
            if ch=='#' :
                n=8
    return ver


libsJson='"{';
projVer=GitInf(cur_p,"proj",fileTmp)
libsJson+=projVer
libsJson+=LibsGitInf(cur_p+"/lib",fileTmp)
libsJson+=LibsGitInf(cur_p+"/.pio/libdeps/"+envBoard,fileTmp)
libsJson+='}"'

os.remove(fileTmp)

buildTime=str(round(time.time()))
envBoardShort=envBoard

board='"'+board+'"'
if envBoard==board:
    envBoard=""
envBoard='"'+envBoard+'"'

print('#define MWOS_LIBS '+libsJson)
print('#define MWOS_BUILD_TIME '+buildTime)
print('#define MWOS_BOARD '+board)
print('#define MWOS_BOARD_FULL '+envBoard)

cid='0'
with open(ver_file, 'w') as f:
    f.write('\n\r#define MWOS_LIBS '+libsJson)
    f.write('\n\r#define MWOS_BUILD_TIME '+buildTime)
    f.write('\n\r#define MWOS_BOARD '+board)
    f.write('\n\r#define MWOS_BOARD_FULL '+envBoard)
    for pArg in pArgs:
        pArg = pArg.replace('\'','"',99)
        pArg = pArg.replace('^','\'',99)
        print('#define '+pArg)
        f.write('\n\r#define '+pArg)
        if pArg[:4]=='CID ':
            cid=pArg[4:]
    f.write('\n\r')

bDir=env['PROJECT_BUILD_DIR']
with open(bDir+'/'+envBoardShort+'/firmware.inf', 'w') as f:
    f.write(buildTime+';'+GetGitShortHash(projVer)+';'+envBoardShort+';'+cid)


