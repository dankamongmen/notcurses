#include "ncart.h"
#include <stdlib.h>
#include <strings.h>
// Output of "neofetch-ripper /usr/bin/neofetch"
// Generated on Mon 30 Aug 2021 04:06:25 AM UTC
// Copyright Dylan Araps under an MIT License
// The MIT License (MIT)
//
// Copyright (c) 2015-2021 Dylan Araps
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Found get_distro_ascii at line 5286
// Logo #0: AIX...
static const char AIX[] = "           `:+ssssossossss+-`\n        .oys///oyhddddhyo///sy+.\n      /yo:+hNNNNNNNNNNNNNNNNh+:oy/\n    :h/:yNNNNNNNNNNNNNNNNNNNNNNy-+h:\n  `ys.yNNNNNNNNNNNNNNNNNNNNNNNNNNy.ys\n `h+-mNNNNNNNNNNNNNNNNNNNNNNNNNNNNm-oh\n h+-NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN.oy\n/d`mNNNNNNN/::mNNNd::m+:/dNNNo::dNNNd`m:\nh//NNNNNNN: . .NNNh  mNo  od. -dNNNNN:+y\nN.sNNNNNN+ -N/ -NNh  mNNd.   sNNNNNNNo-m\nN.sNNNNNs  +oo  /Nh  mNNs` ` /mNNNNNNo-m\nh//NNNNh  ossss` +h  md- .hm/ `sNNNNN:+y\n:d`mNNN+/yNNNNNd//y//h//oNNNNy//sNNNd`m-\n yo-NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNm.ss\n `h+-mNNNNNNNNNNNNNNNNNNNNNNNNNNNNm-oy\n   sy.yNNNNNNNNNNNNNNNNNNNNNNNNNNs.yo\n    :h+-yNNNNNNNNNNNNNNNNNNNNNNs-oh-\n      :ys:/yNNNNNNNNNNNNNNNmy/:sy:\n        .+ys///osyhhhhys+///sy+.\n            -/osssossossso/-\n";
// AIX: 21 lines, done at line 5314.
// Logo #1: Hash...
static const char Hash[] = "\n\n      +   ######   +\n    ###   ######   ###\n  #####   ######   #####\n ######   ######   ######\n\n####### '\"###### '\"########\n#######   ######   ########\n#######   ######   ########\n\n ###### '\"###### '\"######\n  #####   ######   #####\n    ###   ######   ###\n      ~   ######   ~\n\n";
// Hash: 17 lines, done at line 5336.
// Logo #2: alpine_small...
static const char alpine_small[] = "   /\\\\ /\\\\\n  // \\\\  \\\\\n //   \\\\  \\\\\n///    \\\\  \\\\\n//      \\\\  \\\\\n         \\\\\n";
// alpine_small: 7 lines, done at line 5348.
// Logo #3: Alpine...
static const char Alpine[] = "       .hddddddddddddddddddddddh.\n      :dddddddddddddddddddddddddd:\n     /dddddddddddddddddddddddddddd/\n    +dddddddddddddddddddddddddddddd+\n  `sdddddddddddddddddddddddddddddddds`\n `ydddddddddddd++hdddddddddddddddddddy`\n.hddddddddddd+`  `+ddddh:-sdddddddddddh.\nhdddddddddd+`      `+y:    .sddddddddddh\nddddddddh+`   `//`   `.`     -sddddddddd\nddddddh+`   `/hddh/`   `:s-    -sddddddd\nddddh+`   `/+/dddddh/`   `+s-    -sddddd\nddd+`   `/o` :dddddddh/`   `oy-    .yddd\nhdddyo+ohddyosdddddddddho+oydddy++ohdddh\n.hddddddddddddddddddddddddddddddddddddh.\n `yddddddddddddddddddddddddddddddddddy`\n  `sdddddddddddddddddddddddddddddddds`\n    +dddddddddddddddddddddddddddddd+\n     /dddddddddddddddddddddddddddd/\n      :dddddddddddddddddddddddddd:\n       .hddddddddddddddddddddddh.\n";
// Alpine: 21 lines, done at line 5374.
// Logo #4: Alter...
static const char Alter[] = "                      %,\n                    ^WWWw\n                   'wwwwww\n                  !wwwwwwww\n                 #`wwwwwwwww\n                @wwwwwwwwwwww\n               wwwwwwwwwwwwwww\n              wwwwwwwwwwwwwwwww\n             wwwwwwwwwwwwwwwwwww\n            wwwwwwwwwwwwwwwwwwww,\n           w~1i.wwwwwwwwwwwwwwwww,\n         3~:~1lli.wwwwwwwwwwwwwwww.\n        :~~:~?ttttzwwwwwwwwwwwwwwww\n       #<~:~~~~?llllltO-.wwwwwwwwwww\n      #~:~~:~:~~?ltlltlttO-.wwwwwwwww\n     @~:~~:~:~:~~(zttlltltlOda.wwwwwww\n    @~:~~: ~:~~:~:(zltlltlO    a,wwwwww\n   8~~:~~:~~~~:~~~~_1ltltu          ,www\n  5~~:~~:~~:~~:~~:~~~_1ltq             N,,\n g~:~~:~~~:~~:~~:~:~~~~1q                N,\n";
// Alter: 21 lines, done at line 5400.
// Logo #5: Amazon...
static const char Amazon[] = "             `-/oydNNdyo:.`\n      `.:+shmMMMMMMMMMMMMMMmhs+:.`\n    -+hNNMMMMMMMMMMMMMMMMMMMMMMNNho-\n.``      -/+shmNNMMMMMMNNmhs+/-      ``.\ndNmhs+:.       `.:/oo/:.`       .:+shmNd\ndMMMMMMMNdhs+:..        ..:+shdNMMMMMMMd\ndMMMMMMMMMMMMMMNds    odNMMMMMMMMMMMMMMd\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\n.:+ydNMMMMMMMMMMMh    yMMMMMMMMMMMNdy+:.\n     `.:+shNMMMMMh    yMMMMMNhs+:``\n            `-+shy    shs+:`\n";
// Amazon: 20 lines, done at line 5425.
// Logo #6: Anarchy...
static const char Anarchy[] = "                         ..\n                        ..\n                      :..\n                    :+++.\n              .:::+++++++::.\n          .:+######++++######+:.\n       .+#########+++++##########:.\n     .+##########+++++++##+#########+.\n    +###########+++++++++############:\n   +##########++++++#++++#+###########+\n  +###########+++++###++++#+###########+\n :##########+#++++####++++#+############:\n ###########+++++#####+++++#+###++######+\n.##########++++++#####++++++++++++#######.\n.##########+++++++++++++++++++###########.\n #####++++++++++++++###++++++++#########+\n :###++++++++++#########+++++++#########:\n  +######+++++##########++++++++#######+\n   +####+++++###########+++++++++#####+\n    :##++++++############++++++++++##:\n     .++++++#############+++++++++++.\n      :++++###############+++++++::\n     .++. .:+##############+++++++..\n     .:.      ..::++++++::..:+++++.\n     .                       .:+++.\n                                .::\n                                   ..\n                                    ..\n";
// Anarchy: 29 lines, done at line 5459.
// Logo #7: android_small...
static const char android_small[] = "  ;,           ,;\n   ';,.-----.,;'\n  ,'           ',\n /    O     O    \\\\\n|                 |\n'-----------------'\n";
// android_small: 7 lines, done at line 5471.
// Logo #8: Android...
static const char Android[] = "         -o          o-\n          +hydNNNNdyh+\n        +mMMMMMMMMMMMMm+\n      `dMMm:NMMMMMMN:mMMd`\n      hMMMMMMMMMMMMMMMMMMh\n  ..  yyyyyyyyyyyyyyyyyyyy  ..\n.mMMm`MMMMMMMMMMMMMMMMMMMM`mMMm.\n:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:\n:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:\n:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:\n:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:\n-MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM-\n +yy+ MMMMMMMMMMMMMMMMMMMM +yy+\n      mMMMMMMMMMMMMMMMMMMm\n      `/++MMMMh++hMMMM++/`\n          MMMMo  oMMMM\n          MMMMo  oMMMM\n          oNMm-  -mMNs\n";
// Android: 19 lines, done at line 5495.
// Logo #9: Antergos...
static const char Antergos[] = "              `.-/::/-``\n            .-/osssssssso/.\n           :osyysssssssyyys+-\n        `.+yyyysssssssssyyyyy+.\n       `/syyyyyssssssssssyyyyys-`\n      `/yhyyyyysss++ssosyyyyhhy/`\n     .ohhhyyyyso++/+oso+syy+shhhho.\n    .shhhhysoo++//+sss+++yyy+shhhhs.\n   -yhhhhs+++++++ossso+++yyys+ohhddy:\n  -yddhhyo+++++osyyss++++yyyyooyhdddy-\n .yddddhso++osyyyyys+++++yyhhsoshddddy`\n`odddddhyosyhyyyyyy++++++yhhhyosddddddo\n.dmdddddhhhhhhhyyyo+++++shhhhhohddddmmh.\nddmmdddddhhhhhhhso++++++yhhhhhhdddddmmdy\ndmmmdddddddhhhyso++++++shhhhhddddddmmmmh\n-dmmmdddddddhhyso++++oshhhhdddddddmmmmd-\n.smmmmddddddddhhhhhhhhhdddddddddmmmms.\n   `+ydmmmdddddddddddddddddddmmmmdy/.\n      `.:+ooyyddddddddddddyyso+:.`\n";
// Antergos: 20 lines, done at line 5520.
// Logo #10: antiX...
static const char antiX[] = "\n                    \\\n         , - ~ ^ ~ - \\        /\n     , '              \\ ' ,  /\n   ,                   \\   '/\n  ,                     \\  / ,\n ,___,                   \\/   ,\n /   |   _  _  _|_ o     /\\   ,\n|,   |  / |/ |  |  |    /  \\  ,\n \\,_/\\_/  |  |_/|_/|_/_/    \\,\n   ,                  /     ,\\\n     ,               /  , '   \\\n      ' - , _ _ _ ,  '\n";
// antiX: 14 lines, done at line 5539.
// Logo #11: AOSC OS/Retro...
static const char AOSC_OS_Retro[] = "          .........\n     ...................\n   .....................################\n ..............     ....################\n..............       ...################\n.............         ..****************\n............     .     .****************\n...........     ...     ................\n..........     .....     ...............\n.........     .......     ...\n .......                   .\n  .....      .........    ...........\n  ....      .......       ...........\n  ...      .......        ...........\n  ................        ***********\n  ................        ###########\n  ****************\n  ################\n";
// AOSC_OS_Retro: 19 lines, done at line 5563.
// Logo #12: AOSC OS...
static const char AOSC_OS[] = "             .:+syhhhhys+:.\n         .ohNMMMMMMMMMMMMMMNho.\n      `+mMMMMMMMMMMmdmNMMMMMMMMm+`\n     +NMMMMMMMMMMMM/   `./smMMMMMN+\n   .mMMMMMMMMMMMMMMo        -yMMMMMm.\n  :NMMMMMMMMMMMMMMMs          .hMMMMN:\n .NMMMMhmMMMMMMMMMMm+/-         oMMMMN.\n dMMMMs  ./ymMMMMMMMMMMNy.       sMMMMd\n-MMMMN`      oMMMMMMMMMMMN:      `NMMMM-\n/MMMMh       NMMMMMMMMMMMMm       hMMMM/\n/MMMMh       NMMMMMMMMMMMMm       hMMMM/\n-MMMMN`      :MMMMMMMMMMMMy.     `NMMMM-\n dMMMMs       .yNMMMMMMMMMMMNy/. sMMMMd\n .NMMMMo         -/+sMMMMMMMMMMMmMMMMN.\n  :NMMMMh.          .MMMMMMMMMMMMMMMN:\n   .mMMMMMy-         NMMMMMMMMMMMMMm.\n     +NMMMMMms/.`    mMMMMMMMMMMMN+\n      `+mMMMMMMMMNmddMMMMMMMMMMm+`\n         .ohNMMMMMMMMMMMMMMNho.\n             .:+syhhhhys+:.\n";
// AOSC_OS: 21 lines, done at line 5589.
// Logo #13: Apricity...
static const char Apricity[] = "                                    ./o-\n          ``...``              `:. -/:\n     `-+ymNMMMMMNmho-`      :sdNNm/\n   `+dMMMMMMMMMMMMMMMmo` sh:.:::-\n  /mMMMMMMMMMMMMMMMMMMMm/`sNd/\n oMMMMMMMMMMMMMMMMMMMMMMMs -`\n:MMMMMMMMMMMMMMMMMMMMMMMMM/\nNMMMMMMMMMMMMMMMMMMMMMMMMMd\nMMMMMMMmdmMMMMMMMMMMMMMMMMd\nMMMMMMy` .mMMMMMMMMMMMmho:`\nMMMMMMNo/sMMMMMMMNdy+-.`-/\nMMMMMMMMMMMMNdy+:.`.:ohmm:\nMMMMMMMmhs+-.`.:+ymNMMMy.\nMMMMMM/`.-/ohmNMMMMMMy-\nMMMMMMNmNNMMMMMMMMmo.\nMMMMMMMMMMMMMMMms:`\nMMMMMMMMMMNds/.\ndhhyys+/-`\n";
// Apricity: 19 lines, done at line 5613.
// Logo #14: arcolinux_small...
static const char arcolinux_small[] = "          A\n         ooo\n        ooooo\n       ooooooo\n      ooooooooo\n     ooooo ooooo\n    ooooo   ooooo\n   ooooo     ooooo\n  ooooo  <oooooooo>\n ooooo      <oooooo>\nooooo          <oooo>\n";
// arcolinux_small: 12 lines, done at line 5630.
// Logo #15: ArcoLinux...
static const char ArcoLinux[] = "                    /-\n                   ooo:\n                  yoooo/\n                 yooooooo\n                yooooooooo\n               yooooooooooo\n             .yooooooooooooo\n            .oooooooooooooooo\n           .oooooooarcoooooooo\n          .ooooooooo-oooooooooo\n         .ooooooooo-  oooooooooo\n        :ooooooooo.    :ooooooooo\n       :ooooooooo.      :ooooooooo\n      :oooarcooo         .oooarcooo\n     :ooooooooy           .ooooooooo\n    :ooooooooo   /ooooooooooooooooooo\n   :ooooooooo      .-ooooooooooooooooo.\n  ooooooooo-             -ooooooooooooo.\n ooooooooo-                 .-oooooooooo.\nooooooooo.                     -ooooooooo\n";
// ArcoLinux: 21 lines, done at line 5656.
// Logo #16: arch_small...
static const char arch_small[] = "      /\\\\\n     /  \\\\\n    /\\\\   \\\\\n   /      \\\\\n  /   ,,   \\\\\n /   |  |  -\\\\\n/_-''    ''-_\\\\\n";
// arch_small: 8 lines, done at line 5669.
// Logo #17: arch_old...
static const char arch_old[] = "             __\n         _=(SDGJT=_\n       _GTDJHGGFCVS)\n      ,GTDJGGDTDFBGX0\n     JDJDIJHRORVFSBSVL-=+=,_\n    IJFDUFHJNXIXCDXDSV,  \"DEBL\n   [LKDSDJTDU=OUSCSBFLD.   '?ZWX,\n  ,LMDSDSWH'     `DCBOSI     DRDS],\n  SDDFDFH'         !YEWD,   )HDROD\n !KMDOCG            &GSU|\\_GFHRGO\\'\n HKLSGP'           __\\TKM0\\GHRBV)'\nJSNRVW'       __+MNAEC\\IOI,\\BN'\nHELK['    __,=OFFXCBGHC\\FD)\n?KGHE \\_-#DASDFLSV='    'EF\n'EHTI                    !H\n `0F'                    '!\n";
// arch_old: 17 lines, done at line 5691.
// Logo #18: ArchBox...
static const char ArchBox[] = "              ...:+oh/:::..\n         ..-/oshhhhhh`   `::::-.\n     .:/ohhhhhhhhhhhh`        `-::::.\n .+shhhhhhhhhhhhhhhhh`             `.::-.\n /`-:+shhhhhhhhhhhhhh`            .-/+shh\n /      .:/ohhhhhhhhh`       .:/ohhhhhhhh\n /           `-:+shhh`  ..:+shhhhhhhhhhhh\n /                 .:ohhhhhhhhhhhhhhhhhhh\n /                  `hhhhhhhhhhhhhhhhhhhh\n /                  `hhhhhhhhhhhhhhhhhhhh\n /                  `hhhhhhhhhhhhhhhhhhhh\n /                  `hhhhhhhhhhhhhhhhhhhh\n /      .+o+        `hhhhhhhhhhhhhhhhhhhh\n /     -hhhhh       `hhhhhhhhhhhhhhhhhhhh\n /     ohhhhho      `hhhhhhhhhhhhhhhhhhhh\n /:::+`hhhhoos`     `hhhhhhhhhhhhhhhhhs+`\n    `--/:`   /:     `hhhhhhhhhhhho/-\n             -/:.   `hhhhhhs+:-`\n                ::::/ho/-`\n";
// ArchBox: 20 lines, done at line 5716.
// Logo #19: ARCHlabs...
static const char ARCHlabs[] = "                     'c'\n                    'kKk,\n                   .dKKKx.\n                  .oKXKXKd.\n                 .l0XXXXKKo.\n                 c0KXXXXKX0l.\n                :0XKKOxxOKX0l.\n               :OXKOc. .c0XX0l.\n              :OK0o. ...'dKKX0l.\n             :OX0c  ;xOx''dKXX0l.\n            :0KKo..o0XXKd'.lKXX0l.\n           c0XKd..oKXXXXKd..oKKX0l.\n         .c0XKk;.l0K0OO0XKd..oKXXKo.\n        .l0XXXk:,dKx,.'l0XKo..kXXXKo.\n       .o0XXXX0d,:x;   .oKKx'.dXKXXKd.\n      .oKXXXXKK0c.;.    :00c'cOXXXXXKd.\n     .dKXXXXXXXXk,.     cKx''xKXXXXXXKx'\n    'xKXXXXK0kdl:.     .ok; .cdk0KKXXXKx'\n   'xKK0koc,..         'c,     ..,cok0KKk,\n  ,xko:'.             ..            .':okx;\n .,'.                                   .',.\n";
// ARCHlabs: 22 lines, done at line 5743.
// Logo #20: ArchStrike...
static const char ArchStrike[] = "                   *   \n                  **.\n                 ****\n                ******\n                *******\n              ** *******\n             **** *******\n            ****_____***/*\n           ***/*******//***\n          **/********///*/**\n         **/*******////***/**\n        **/****//////.,****/**\n       ***/*****/////////**/***\n      ****/****    /////***/****\n     ******/***  ////   **/******\n    ********/* ///      */********\n  ,******     // ______ /    ******,\n";
// ArchStrike: 18 lines, done at line 5766.
// Logo #21: XFerience...
static const char XFerience[] = "           ``--:::::::-.`\n        .-/+++ooooooooo+++:-`\n     `-/+oooooooooooooooooo++:.\n    -/+oooooo/+ooooooooo+/ooo++:`\n  `/+oo++oo.   .+oooooo+.-: +:-o+-\n `/+o/.  -o.    :oooooo+ ```:.+oo+-\n`:+oo-    -/`   :oooooo+ .`-`+oooo/.\n.+ooo+.    .`   `://///+-+..oooooo+:`\n-+ooo:`                ``.-+oooooo+/`\n-+oo/`                       :+oooo/.\n.+oo:            ..-/. .      -+oo+/`\n`/++-         -:::++::/.      -+oo+-\n ./o:          `:///+-     `./ooo+:`\n  .++-         `` /-`   -:/+oooo+:`\n   .:+/:``          `-:ooooooo++-\n     ./+o+//:...../+oooooooo++:`\n       `:/++ooooooooooooo++/-`\n          `.-//++++++//:-.`\n               ``````\n";
// XFerience: 20 lines, done at line 5791.
// Logo #22: ArchMerge...
static const char ArchMerge[] = "                    y:\n                  sMN-\n                 +MMMm`\n                /MMMMMd`\n               :NMMMMMMy\n              -NMMMMMMMMs\n             .NMMMMMMMMMM+\n            .mMMMMMMMMMMMM+\n            oNMMMMMMMMMMMMM+\n          `+:-+NMMMMMMMMMMMM+\n          .sNMNhNMMMMMMMMMMMM/\n        `hho/sNMMMMMMMMMMMMMMM/\n       `.`omMMmMMMMMMMMMMMMMMMM+\n      .mMNdshMMMMd+::oNMMMMMMMMMo\n     .mMMMMMMMMM+     `yMMMMMMMMMs\n    .NMMMMMMMMM/        yMMMMMMMMMy\n   -NMMMMMMMMMh         `mNMMMMMMMMd`\n  /NMMMNds+:.`             `-/oymMMMm.\n +Mmy/.                          `:smN:\n/+.                                  -o.\n";
// ArchMerge: 21 lines, done at line 5817.
// Logo #23: Arch...
static const char Arch[] = "                   -`\n                  .o+`\n                 `ooo/\n                `+oooo:\n               `+oooooo:\n               -+oooooo+:\n             `/:-:++oooo+:\n            `/++++/+++++++:\n           `/++++++++++++++:\n          `/+++ooooooooooooo/`\n         ./ooosssso++osssssso+`\n        .oossssso-````/ossssss+`\n       -osssssso.      :ssssssso.\n      :osssssss/        osssso+++.\n     /ossssssss/        +ssssooo/-\n   `/ossssso+/:-        -:/+osssso+-\n  `+sso+:-`                 `.-/+oso:\n `++:.                           `-/+/\n .`                                 `/\n";
// Arch: 20 lines, done at line 5842.
// Logo #24: artix_small...
static const char artix_small[] = "      /\\\\\n     /  \\\\\n    /`'.,\\\\\n   /     ',\n  /      ,`\\\\\n /   ,.'`.  \\\\\n/.,'`     `'.\\\\\n";
// artix_small: 8 lines, done at line 5855.
// Logo #25: Artix...
static const char Artix[] = "                   '\n                  'o'\n                 'ooo'\n                'ooxoo'\n               'ooxxxoo'\n              'oookkxxoo'\n             'oiioxkkxxoo'\n            ':;:iiiioxxxoo'\n               `'.;::ioxxoo'\n          '-.      `':;jiooo'\n         'oooio-..     `'i:io'\n        'ooooxxxxoio:,.   `'-;'\n       'ooooxxxxxkkxoooIi:-.  `'\n      'ooooxxxxxkkkkxoiiiiiji'\n     'ooooxxxxxkxxoiiii:'`     .i'\n    'ooooxxxxxoi:::'`       .;ioxo'\n   'ooooxooi::'`         .:iiixkxxo'\n  'ooooi:'`                `'';ioxxo'\n 'i:'`                          '':io'\n'`                                   `'\n";
// Artix: 21 lines, done at line 5881.
// Logo #26: Arya...
static const char Arya[] = "                `oyyy/-yyyyyy+\n               -syyyy/-yyyyyy+\n              .syyyyy/-yyyyyy+\n              :yyyyyy/-yyyyyy+\n           `/ :yyyyyy/-yyyyyy+\n          .+s :yyyyyy/-yyyyyy+\n         .oys :yyyyyy/-yyyyyy+\n        -oyys :yyyyyy/-yyyyyy+\n       :syyys :yyyyyy/-yyyyyy+\n      /syyyys :yyyyyy/-yyyyyy+\n     +yyyyyys :yyyyyy/-yyyyyy+\n   .oyyyyyyo. :yyyyyy/-yyyyyy+ ---------\n  .syyyyyy+`  :yyyyyy/-yyyyy+-+syyyyyyyy\n -syyyyyy/    :yyyyyy/-yyys:.syyyyyyyyyy\n:syyyyyy/     :yyyyyy/-yyo.:syyyyyyyyyyy\n";
// Arya: 16 lines, done at line 5902.
// Logo #27: Bedrock...
static const char Bedrock[] = "--------------------------------------\n--------------------------------------\n--------------------------------------\n---\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\-----------------------\n----\\\\\\\\\\\\      \\\\\\\\\\\\----------------------\n-----\\\\\\\\\\\\      \\\\\\\\\\\\---------------------\n------\\\\\\\\\\\\      \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\------\n-------\\\\\\\\\\\\                    \\\\\\\\\\\\-----\n--------\\\\\\\\\\\\                    \\\\\\\\\\\\----\n---------\\\\\\\\\\\\        ______      \\\\\\\\\\\\---\n----------\\\\\\\\\\\\                   ///---\n-----------\\\\\\\\\\\\                 ///----\n------------\\\\\\\\\\\\               ///-----\n-------------\\\\\\\\\\\\////////////////------\n--------------------------------------\n--------------------------------------\n--------------------------------------\n";
// Bedrock: 18 lines, done at line 5925.
// Logo #28: Bitrig...
static const char Bitrig[] = "   `hMMMMN+\n   -MMo-dMd`\n   oMN- oMN`\n   yMd  /NM:\n  .mMmyyhMMs\n  :NMMMhsmMh\n  +MNhNNoyMm-\n  hMd.-hMNMN:\n  mMmsssmMMMo\n .MMdyyhNMMMd\n oMN.`/dMddMN`\n yMm/hNm+./MM/\n.dMMMmo.``.NMo\n:NMMMNmmmmmMMh\n/MN/-------oNN:\nhMd.       .dMh\nsm/         /ms\n";
// Bitrig: 18 lines, done at line 5948.
// Logo #29: BlackArch...
static const char BlackArch[] = "                   00\n                   11\n                  ====\n                  .//\n                 `o//:\n                `+o//o:\n               `+oo//oo:\n               -+oo//oo+:\n             `/:-:+//ooo+:\n            `/+++++//+++++:\n           `/++++++//++++++:\n          `/+++oooo//ooooooo/`\n         ./ooosssso//osssssso+`\n        .oossssso-`//`/ossssss+`\n       -osssssso.  //  :ssssssso.\n      :osssssss/   //   osssso+++.\n     /ossssssss/   //   +ssssooo/-\n   `/ossssso+/:-   //   -:/+osssso+-\n  `+sso+:-`        //       `.-/+oso:\n `++:.             //            `-/+/\n .`                /                `/\n";
// BlackArch: 22 lines, done at line 5975.
// Logo #30: BLAG...
static const char BLAG[] = "             d\n            ,MK:\n            xMMMX:\n           .NMMMMMX;\n           lMMMMMMMM0clodkO0KXWW:\n           KMMMMMMMMMMMMMMMMMMX'\n      .;d0NMMMMMMMMMMMMMMMMMMK.\n .;dONMMMMMMMMMMMMMMMMMMMMMMx\n'dKMMMMMMMMMMMMMMMMMMMMMMMMl\n   .:xKWMMMMMMMMMMMMMMMMMMM0.\n       .:xNMMMMMMMMMMMMMMMMMK.\n          lMMMMMMMMMMMMMMMMMMK.\n          ,MMMMMMMMWkOXWMMMMMM0\n          .NMMMMMNd.     `':ldko\n           OMMMK:\n           oWk,\n           ;:\n";
// BLAG: 18 lines, done at line 5998.
// Logo #31: BlankOn...
static const char BlankOn[] = "        `./ohdNMMMMNmho+.`        .+oo:`\n      -smMMMMMMMMMMMMMMMMmy-`    `yyyyy+\n   `:dMMMMMMMMMMMMMMMMMMMMMMd/`  `yyyyys\n  .hMMMMMMMNmhso/++symNMMMMMMMh- `yyyyys\n -mMMMMMMms-`         -omMMMMMMN-.yyyyys\n.mMMMMMMy.              .yMMMMMMm:yyyyys\nsMMMMMMy                 `sMMMMMMhyyyyys\nNMMMMMN:                  .NMMMMMNyyyyys\nMMMMMMm.                   NMMMMMNyyyyys\nhMMMMMM+                  /MMMMMMNyyyyys\n:NMMMMMN:                :mMMMMMM+yyyyys\n oMMMMMMNs-            .sNMMMMMMs.yyyyys\n  +MMMMMMMNho:.`  `.:ohNMMMMMMNo `yyyyys\n   -hMMMMMMMMNNNmmNNNMMMMMMMMh-  `yyyyys\n     :yNMMMMMMMMMMMMMMMMMMNy:`   `yyyyys\n       .:sdNMMMMMMMMMMNds/.      `yyyyyo\n           `.:/++++/:.`           :oys+.\n";
// BlankOn: 18 lines, done at line 6021.
// Logo #32: BlueLight...
static const char BlueLight[] = "              oMMNMMMMMMMMMMMMMMMMMMMMMM\n              oMMMMMMMMMMMMMMMMMMMMMMMMM\n              oMMMMMMMMMMMMMMMMMMMMMMMMM\n              oMMMMMMMMMMMMMMMMMMMMMMMMM\n              -+++++++++++++++++++++++mM\n             ```````````````````````..dM\n           ```````````````````````....dM\n         ```````````````````````......dM\n       ```````````````````````........dM\n     ```````````````````````..........dM\n   ```````````````````````............dM\n.::::::::::::::::::::::-..............dM\n `-+yyyyyyyyyyyyyyyyyyyo............+mMM\n     -+yyyyyyyyyyyyyyyyo..........+mMMMM\n        ./syyyyyyyyyyyyo........+mMMMMMM\n           ./oyyyyyyyyyo......+mMMMMMMMM\n              omdyyyyyyo....+mMMMMMMMMMM\n              oMMMmdhyyo..+mMMMMMMMMMMMM\n              oNNNNNNmdsomMMMMMMMMMMMMMM\n";
// BlueLight: 20 lines, done at line 6046.
// Logo #33: bonsai...
static const char bonsai[] = "   ,####,\n   #######,  ,#####,\n   #####',#  '######\n    ''###'';,,,'###'\n          ,;  ''''\n         ;;;   ,#####,\n        ;;;'  ,,;;;###\n        ';;;;'''####'\n         ;;;\n      ,.;;';'',,,\n     '     '\n #\n #                        O\n ##, ,##,',##, ,##  ,#,   ,\n # # #  # #''# #,,  # #   #\n '#' '##' #  #  ,,# '##;, #\n";
// bonsai: 17 lines, done at line 6068.
// Logo #34: BSD...
static const char BSD[] = "             ,        ,\n            /(        )`\n            \\ \\___   / |\n            /- _  `-/  '\n           (/\\/ \\ \\   /\\\n           / /   | `    \\\n           O O   ) /    |\n           `-^--'`<     '\n          (_.)  _  )   /\n           `.___/`    /\n             `-----' /\n<----.     __ / __   \\\n<----|====O)))==) \\) /====|\n<----'    `--' `.__,' \\\n             |        |\n              \\       /       /\\\n         ______( (_  / \\______/\n       ,'  ,-----'   |\n       `--{__________)\n";
// BSD: 20 lines, done at line 6093.
// Logo #35: BunsenLabs...
static const char BunsenLabs[] = "        `++\n      -yMMs\n    `yMMMMN`\n   -NMMMMMMm.\n  :MMMMMMMMMN-\n .NMMMMMMMMMMM/\n yMMMMMMMMMMMMM/\n`MMMMMMNMMMMMMMN.\n-MMMMN+ /mMMMMMMy\n-MMMm`   `dMMMMMM\n`MMN.     .NMMMMM.\n hMy       yMMMMM`\n -Mo       +MMMMN\n  /o       +MMMMs\n           +MMMN`\n           hMMM:\n          `NMM/\n          +MN:\n          mh.\n         -/\n";
// BunsenLabs: 21 lines, done at line 6119.
// Logo #36: Calculate...
static const char Calculate[] = "                              ......\n                           ,,+++++++,.\n                         .,,,....,,,+**+,,.\n                       ............,++++,,,\n                      ...............\n                    ......,,,........\n                  .....+*#####+,,,*+.\n              .....,*###############,..,,,,,,..\n           ......,*#################*..,,,,,..,,,..\n         .,,....*####################+***+,,,,...,++,\n       .,,..,..*#####################*,\n     ,+,.+*..*#######################.\n   ,+,,+*+..,########################*\n.,++++++.  ..+##**###################+\n.....      ..+##***#################*.\n           .,.*#*****##############*.\n           ..,,*********#####****+.\n     .,++*****+++*****************+++++,.\n      ,++++++**+++++***********+++++++++,\n     .,,,,++++,..  .,,,,,.....,+++,.,,\n";
// Calculate: 21 lines, done at line 6145.
// Logo #37: Carbs...
static const char Carbs[] = "             ..........\n          ..,;:ccccccc:;'..\n       ..,clllc:;;;;;:cllc,.\n      .,cllc,...     ..';;'.\n     .;lol;..           ..\n    .,lol;.\n    .coo:.\n   .'lol,.\n   .,lol,.\n   .,lol,.\n    'col;.\n    .:ooc'.\n    .'col:.\n     .'cllc'..          .''.\n      ..:lolc,'.......',cll,.\n        ..;cllllccccclllc;'.\n          ...',;;;;;;,,...\n                .....\n";
// Carbs: 19 lines, done at line 6168.
// Logo #38: centos_small...
static const char centos_small[] = " ____^____\n |\\\\  |  /|\n | \\\\ | / |\n<---- ---->\n | / | \\\\ |\n |/__|__\\\\|\n     v\n";
// centos_small: 8 lines, done at line 6181.
// Logo #39: CentOS...
static const char CentOS[] = "                 ..\n               .PLTJ.\n              <><><><>\n     KKSSV' 4KKK LJ KKKL.'VSSKK\n     KKV' 4KKKKK LJ KKKKAL 'VKK\n     V' ' 'VKKKK LJ KKKKV' ' 'V\n     .4MA.' 'VKK LJ KKV' '.4Mb.\n   . KKKKKA.' 'V LJ V' '.4KKKKK .\n .4D KKKKKKKA.'' LJ ''.4KKKKKKK FA.\n<QDD ++++++++++++  ++++++++++++ GFD>\n 'VD KKKKKKKK'.. LJ ..'KKKKKKKK FV\n   ' VKKKKK'. .4 LJ K. .'KKKKKV '\n      'VK'. .4KK LJ KKA. .'KV'\n     A. . .4KKKK LJ KKKKA. . .4\n     KKA. 'KKKKK LJ KKKKK' .4KK\n     KKSSA. VKKK LJ KKKV .4SSKK\n              <><><><>\n               'MKKM'\n                 ''\n";
// CentOS: 20 lines, done at line 6206.
// Logo #40: Chakra...
static const char Chakra[] = "     _ _ _        \"kkkkkkkk.\n   ,kkkkkkkk.,    'kkkkkkkkk,\n   ,kkkkkkkkkkkk., 'kkkkkkkkk.\n  ,kkkkkkkkkkkkkkkk,'kkkkkkkk,\n ,kkkkkkkkkkkkkkkkkkk'kkkkkkk.\n  \"''\"''',;::,,\"''kkk''kkkkk;   __\n      ,kkkkkkkkkk, \"k''kkkkk' ,kkkk\n    ,kkkkkkk' ., ' .: 'kkkk',kkkkkk\n  ,kkkkkkkk'.k'   ,  ,kkkk;kkkkkkkkk\n ,kkkkkkkk';kk 'k  \"'k',kkkkkkkkkkkk\n.kkkkkkkkk.kkkk.'kkkkkkkkkkkkkkkkkk'\n;kkkkkkkk''kkkkkk;'kkkkkkkkkkkkk''\n'kkkkkkk; 'kkkkkkkk.,\"\"''\"''\"\"\n  ''kkkk;  'kkkkkkkkkk.,\n     ';'    'kkkkkkkkkkkk.,\n             ';kkkkkkkkkk'\n               ';kkkkkk'\n                  \"''\"\n";
// Chakra: 19 lines, done at line 6230.
// Logo #41: ChaletOS...
static const char ChaletOS[] = "             `.//+osso+/:``\n         `/sdNNmhyssssydmNNdo:`\n       :hNmy+-`          .-+hNNs-\n     /mMh/`       `+:`       `+dMd:\n   .hMd-        -sNNMNo.  /yyy  /mMs`\n  -NM+       `/dMd/--omNh::dMM   `yMd`\n .NN+      .sNNs:/dMNy:/hNmo/s     yMd`\n hMs    `/hNd+-smMMMMMMd+:omNy-    `dMo\n:NM.  .omMy:/hNMMMMMMMMMMNy:/hMd+`  :Md`\n/Md` `sm+.omMMMMMMMMMMMMMMMMd/-sm+  .MN:\n/Md`      MMMMMMMMMMMMMMMMMMMN      .MN:\n:NN.      MMMMMMm....--NMMMMMN      -Mm.\n`dMo      MMMMMMd      mMMMMMN      hMs\n -MN:     MMMMMMd      mMMMMMN     oMm`\n  :NM:    MMMMMMd      mMMMMMN    +Mm-\n   -mMy.  mmmmmmh      dmmmmmh  -hMh.\n     oNNs-                    :yMm/\n      .+mMdo:`            `:smMd/`\n         -ohNNmhsoo++osshmNNh+.\n            `./+syyhhyys+:``\n";
// ChaletOS: 21 lines, done at line 6256.
// Logo #42: Chapeau...
static const char Chapeau[] = "               .-/-.\n            ////////.\n          ////////y+//.\n        ////////mMN/////.\n      ////////mMN+////////.\n    ////////////////////////.\n  /////////+shhddhyo+////////.\n ////////ymMNmdhhdmNNdo///////.\n///////+mMms////////hNMh///////.\n///////NMm+//////////sMMh///////\n//////oMMNmmmmmmmmmmmmMMm///////\n//////+MMmssssssssssssss+///////\n`//////yMMy////////////////////\n `//////smMNhso++oydNm////////\n  `///////ohmNMMMNNdy+///////\n    `//////////++//////////\n       `////////////////.\n           -////////-\n";
// Chapeau: 19 lines, done at line 6280.
// Logo #43: Chrom...
static const char Chrom[] = "            .,:loool:,.\n        .,coooooooooooooc,.\n     .,lllllllllllllllllllll,.\n    ;ccccccccccccccccccccccccc;\n  'ccccccccccccccccccccccccccccc.\n ,ooc::::::::okO0000OOkkkkkkkkkkk:\n.ooool;;;;:xK0kxxxxxk0XK0000000000.\n:oooool;,;OKdddddddddddKX000000000d\nlllllool;lNdllllllllllldNK000000000\nllllllllloMdcccccccccccoWK000000000\n;cllllllllXXc:::::::::c0X000000000d\n.ccccllllllONkc;,,,;cxKK0000000000.\n .cccccclllllxOOOOOOkxO0000000000;\n  .:cccccccclllllllloO0000000OOO,\n    ,:ccccccccclllcd0000OOOOOOl.\n      '::cccccccccdOOOOOOOkx:.\n        ..,::ccccxOOOkkko;.\n            ..,:dOkxl:.\n";
// Chrom: 19 lines, done at line 6304.
// Logo #44: cleanjaro_small...
static const char cleanjaro_small[] = "█████ ██████████\n█████ ██████████\n█████\n█████\n█████\n████████████████\n████████████████\n";
// cleanjaro_small: 8 lines, done at line 6317.
// Logo #45: Cleanjaro...
static const char Cleanjaro[] = "███████▌ ████████████████\n███████▌ ████████████████\n███████▌ ████████████████\n███████▌\n███████▌\n███████▌\n███████▌\n███████▌\n█████████████████████████\n█████████████████████████\n█████████████████████████\n▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀\n";
// Cleanjaro: 13 lines, done at line 6335.
// Logo #46: ClearOS...
static const char ClearOS[] = "             `.--::::::--.`\n         .-:////////////////:-.\n      `-////////////////////////-`\n     -////////////////////////////-\n   `//////////////-..-//////////////`\n  ./////////////:      ://///////////.\n `//////:..-////:      :////-..-//////`\n ://////`    -///:.``.:///-`    ://///:\n`///////:.     -////////-`    `:///////`\n.//:--////:.     -////-`    `:////--://.\n./:    .////:.     --`    `:////-    :/.\n`//-`    .////:.        `:////-    `-//`\n :///-`    .////:.    `:////-    `-///:\n `/////-`    -///:    :///-    `-/////`\n  `//////-   `///:    :///`   .//////`\n   `:////:   `///:    :///`   -////:`\n     .://:   `///:    :///`   -//:.\n       .::   `///:    :///`   -:.\n             `///:    :///`\n              `...    ...`\n";
// ClearOS: 21 lines, done at line 6361.
// Logo #47: Clear_Linux...
static const char Clear_Linux[] = "          BBB\n       BBBBBBBBB\n     BBBBBBBBBBBBBBB\n   BBBBBBBBBBBBBBBBBBBB\n   BBBBBBBBBBB         BBB\n  BBBBBBBBYYYYY\n  BBBBBBBBYYYYYY\n  BBBBBBBBYYYYYYY\n  BBBBBBBBBYYYYYW\n GGBBBBBBBYYYYYWWW\n GGGBBBBBBBYYWWWWWWWW\n GGGGGGBBBBBBWWWWWWWW\n GGGGGGGGBBBBWWWWWWWW\nGGGGGGGGGGGBBBWWWWWWW\nGGGGGGGGGGGGGBWWWWWW\nGGGGGGGGWWWWWWWWWWW\nGGWWWWWWWWWWWWWWWW\n WWWWWWWWWWWWWWWW\n      WWWWWWWWWW\n          WWW\n";
// Clear_Linux: 21 lines, done at line 6387.
// Logo #48: Clover...
static const char Clover[] = "               `omo``omo`\n             `oNMMMNNMMMNo`\n           `oNMMMMMMMMMMMMNo`\n          oNMMMMMMMMMMMMMMMMNo\n          `sNMMMMMMMMMMMMMMNs`\n     `omo`  `sNMMMMMMMMMMNs`  `omo`\n   `oNMMMNo`  `sNMMMMMMNs`  `oNMMMNo`\n `oNMMMMMMMNo`  `oNMMNs`  `oNMMMMMMMNo`\noNMMMMMMMMMMMNo`  `sy`  `oNMMMMMMMMMMMNo\n`sNMMMMMMMMMMMMNo.oNNs.oNMMMMMMMMMMMMNs`\n`oNMMMMMMMMMMMMNs.oNNs.oNMMMMMMMMMMMMNo`\noNMMMMMMMMMMMNs`  `sy`  `oNMMMMMMMMMMMNo\n `oNMMMMMMMNs`  `oNMMNo`  `oNMMMMMMMNs`\n   `oNMMMNs`  `sNMMMMMMNs`  `oNMMMNs`\n     `oNs`  `sNMMMMMMMMMMNs`  `oNs`\n          `sNMMMMMMMMMMMMMMNs`\n          +NMMMMMMMMMMMMMMMMNo\n           `oNMMMMMMMMMMMMNo`\n             `oNMMMNNMMMNs`\n               `omo``oNs`\n";
// Clover: 21 lines, done at line 6413.
// Logo #49: Condres...
static const char Condres[] = "syyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy+.+.\n`oyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy+:++.\n/o+oyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy/oo++.\n/y+syyyyyyyyyyyyyyyyyyyyyyyyyyyyy+ooo++.\n/hy+oyyyhhhhhhhhhhhhhhyyyyyyyyy+oo+++++.\n/hhh+shhhhhdddddhhhhhhhyyyyyyy+oo++++++.\n/hhdd+oddddddddddddhhhhhyyyys+oo+++++++.\n/hhddd+odmmmdddddddhhhhyyyy+ooo++++++++.\n/hhdddmoodmmmdddddhhhhhyyy+oooo++++++++.\n/hdddmmms/dmdddddhhhhyyys+oooo+++++++++.\n/hddddmmmy/hdddhhhhyyyyo+oooo++++++++++:\n/hhdddmmmmy:yhhhhyyyyy++oooo+++++++++++:\n/hhddddddddy-syyyyyys+ooooo++++++++++++:\n/hhhddddddddy-+yyyy+/ooooo+++++++++++++:\n/hhhhhdddddhhy./yo:+oooooo+++++++++++++/\n/hhhhhhhhhhhhhy:-.+sooooo+++++++++++///:\n:sssssssssssso++`:/:--------.````````\n";
// Condres: 18 lines, done at line 6436.
// Logo #50: Container_Linux...
static const char Container_Linux[] = "                .....\n          .';:cccccccc:;'.\n        ':ccccclclllllllllcc:.\n     .;cccccccclllllllllllllllc,\n    ;clllcccccllllllllllllllllllc,\n  .cllclcccccllllllllllllllllllllc:\n  ccclclcccccllllkWMMNKkllllllllllc:\n :ccclclcccclllloWMMMMMMWOlllllllllc,\n.ccllllllcccclllOMMMMMMMMM0lllllllllc\n.lllllclccccllllKMMMMMMMMMMollllllllc.\n.lllllllccccclllKMMMMMMMMN0lllllllllc.\n.cclllllcccclllldxkkxxdollllllllllclc\n :cccllllllcccclllccllllcclccccccccc;\n .ccclllllllcccccccclllccccclccccccc\n  .cllllllllllclcccclccclccllllcllc\n    :cllllllllccclcllllllllllllcc;\n     .cccccccccccccclcccccccccc:.\n       .;cccclccccccllllllccc,.\n          .';ccccclllccc:;..\n                .....\n";
// Container_Linux: 21 lines, done at line 6462.
// Logo #51: CRUX...
static const char CRUX[] = "         odddd\n      oddxkkkxxdoo\n     ddcoddxxxdoool\n     xdclodod  olol\n     xoc  xdd  olol\n     xdc  k00Okdlol\n     xxdkOKKKOkdldd\n     xdcoxOkdlodldd\n     ddc:clllloooodo\n   odxxddxkO000kxooxdo\n  oxddx0NMMMMMMWW0odkkxo\n oooxd0WMMMMMMMMMW0odxkx\ndocldkXWMMMMMMMWWNOdolco\nxxdxkxxOKNWMMWN0xdoxo::c\nxOkkO0ooodOWWWXkdodOxc:l\ndkkkxkkkOKXNNNX0Oxxxc:cd\n odxxdxxllodddooxxdc:ldo\n   lodddolcccccoxxoloo\n";
// CRUX: 19 lines, done at line 6499.
// Logo #52: Cucumber...
static const char Cucumber[] = "           `.-://++++++//:-.`\n        `:/+//::--------:://+/:`\n      -++/:----..........----:/++-\n    .++:---...........-......---:++.\n   /+:---....-::/:/--//:::-....---:+/\n `++:--.....:---::/--/::---:.....--:++`\n /+:--.....--.--::::-/::--.--.....--:+/\n-o:--.......-:::://--/:::::-.......--:o-\n/+:--...-:-::---:::..:::---:--:-...--:+/\no/:-...-:.:.-/:::......::/:.--.:-...-:/o\no/--...::-:/::/:-......-::::::-/-...-:/o\n/+:--..-/:/:::--:::..:::--::////-..--:+/\n-o:--...----::/:::/--/:::::-----...--:o-\n /+:--....://:::.:/--/:.::://:....--:+/\n `++:--...-:::.--.:..:.--.:/:-...--:++`\n   /+:---....----:-..-:----....---:+/\n    .++:---..................---:++.\n      -/+/:----..........----:/+/-\n        `:/+//::--------:::/+/:`\n           `.-://++++++//:-.`\n";
// Cucumber: 21 lines, done at line 6525.
// Logo #53: dahlia...
static const char dahlia[] = "\n                  .#.\n                *%@@@%*\n        .,,,,,(&@@@@@@@&/,,,,,.\n       ,#@@@@@@@@@@@@@@@@@@@@@#.\n       ,#@@@@@@@&#///#&@@@@@@@#.\n     ,/%&@@@@@%/,    .,(%@@@@@&#/.\n   *#&@@@@@@#,.         .*#@@@@@@&#,\n .&@@@@@@@@@(            .(@@@@@@@@@&&.\n#@@@@@@@@@@(               )@@@@@@@@@@@#\n °@@@@@@@@@@(            .(@@@@@@@@@@@°\n   *%@@@@@@@(.           ,#@@@@@@@%*\n     ,(&@@@@@@%*.     ./%@@@@@@%(,\n       ,#@@@@@@@&(***(&@@@@@@@#.\n       ,#@@@@@@@@@@@@@@@@@@@@@#.\n        ,*****#&@@@@@@@&(*****,\n               ,/%@@@%/.\n                  ,#,\n";
// dahlia: 19 lines, done at line 6549.
// Logo #54: debian_small...
static const char debian_small[] = "  _____\n /  __ \\\\\n|  /    |\n|  \\\\___-\n-_\n  --_\n";
// debian_small: 7 lines, done at line 6561.
// Logo #55: Debian...
static const char Debian[] = "       _,met$$$$$gg.\n    ,g$$$$$$$$$$$$$$$P.\n  ,g$$P\"     \"\"\"Y$$.\".\n ,$$P'              `$$$.\n',$$P       ,ggs.     `$$b:\n`d$$'     ,$P\"'   .    $$$\n $$P      d$'     ,    $$P\n $$:      $$.   -    ,d$$'\n $$;      Y$b._   _,d$P'\n Y$$.    `.`\"Y$$$$P\"'\n `$$b      \"-.__\n  `Y$$\n   `Y$$.\n     `$$b.\n       `Y$$b.\n          `\"Y$b._\n              `\"\"\"\n";
// Debian: 18 lines, done at line 6584.
// Logo #56: Deepin...
static const char Deepin[] = "             ............\n         .';;;;;.       .,;,.\n      .,;;;;;;;.       ';;;;;;;.\n    .;::::::::'     .,::;;,''''',.\n   ,'.::::::::    .;;'.          ';\n  ;'  'cccccc,   ,' :: '..        .:\n ,,    :ccccc.  ;: .c, '' :.       ,;\n.l.     cllll' ., .lc  :; .l'       l.\n.c       :lllc  ;cl:  .l' .ll.      :'\n.l        'looc. .   ,o:  'oo'      c,\n.o.         .:ool::coc'  .ooo'      o.\n ::            .....   .;dddo      ;c\n  l:...            .';lddddo.     ,o\n   lxxxxxdoolllodxxxxxxxxxc      :l\n    ,dxxxxxxxxxxxxxxxxxxl.     'o,\n      ,dkkkkkkkkkkkkko;.    .;o;\n        .;okkkkkdl;.    .,cl:.\n            .,:cccccccc:,.\n";
// Deepin: 19 lines, done at line 6608.
// Logo #57: DesaOS...
static const char DesaOS[] = "███████████████████████\n███████████████████████\n███████████████████████\n███████████████████████\n████████               ███████\n████████               ███████\n████████               ███████\n████████               ███████\n████████               ███████\n████████               ███████\n████████               ███████\n██████████████████████████████\n██████████████████████████████\n████████████████████████\n████████████████████████\n████████████████████████\n";
// DesaOS: 17 lines, done at line 6630.
// Logo #58: Devuan...
static const char Devuan[] = "   ..,,;;;::;,..\n           `':ddd;:,.\n                 `'dPPd:,.\n                     `:b$$b`.\n                        'P$$$d`\n                         .$$$$$`\n                         ;$$$$$P\n                      .:P$$$$$$`\n                  .,:b$$$$$$$;'\n             .,:dP$$$$$$$$b:'\n      .,:;db$$$$$$$$$$Pd'`\n ,db$$$$$$$$$$$$$$b:'`\n:$$$$$$$$$$$$b:'`\n `$$$$$bd:''`\n   `'''`\n";
// Devuan: 16 lines, done at line 6651.
// Logo #59: DracOS...
static const char DracOS[] = "       `-:/-\n          -os:\n            -os/`\n              :sy+-`\n               `/yyyy+.\n                 `+yyyyo-\n                   `/yyyys:\n`:osssoooo++-        +yyyyyy/`\n   ./yyyyyyo         yo`:syyyy+.\n      -oyyy+         +-   :yyyyyo-\n        `:sy:        `.    `/yyyyys:\n           ./o/.`           .oyyso+oo:`\n              :+oo+//::::///:-.`     `.`\n";
// DracOS: 14 lines, done at line 6670.
// Logo #60: DarkOs...
static const char DarkOs[] = "\n⠀⠀⠀⠀  ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⠢⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣶⠋⡆⢹⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡆⢀⣤⢛⠛⣠⣿⠀⡏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣶⣿⠟⣡⠊⣠⣾⣿⠃⣠⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣴⣯⣿⠀⠊⣤⣿⣿⣿⠃⣴⣧⣄⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣤⣶⣿⣿⡟⣠⣶⣿⣿⣿⢋⣤⠿⠛⠉⢁⣭⣽⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n  ⠀⠀⠀⠀⠀⠀ ⠀⣠⠖⡭⢉⣿⣯⣿⣯⣿⣿⣿⣟⣧⠛⢉⣤⣶⣾⣿⣿⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⣴⣫⠓⢱⣯⣿⢿⠋⠛⢛⠟⠯⠶⢟⣿⣯⣿⣿⣿⣿⣿⣿⣦⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⢀⡮⢁⣴⣿⣿⣿⠖⣠⠐⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠉⠉⠛⠛⠛⢿⣶⣄⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⢀⣤⣷⣿⣿⠿⢛⣭⠒⠉⠀⠀⠀⣀⣀⣄⣤⣤⣴⣶⣶⣶⣿⣿⣿⣿⣿⠿⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⢀⣶⠏⠟⠝⠉⢀⣤⣿⣿⣶⣾⣿⣿⣿⣿⣿⣿⣟⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⢴⣯⣤⣶⣿⣿⣿⣿⣿⡿⣿⣯⠉⠉⠉⠉⠀⠀⠀⠈⣿⡀⣟⣿⣿⢿⣿⣿⣿⣿⣿⣦⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠉⠛⣿⣧⠀⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⠃⣿⣿⣯⣿⣦⡀⠀⠉⠻⣿⣦⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠉⢿⣮⣦⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⣿⠀⣯⠉⠉⠛⢿⣿⣷⣄⠀⠈⢻⣆⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠢⠀⠀⠀⠀⠀⠀⠀⢀⢡⠃⣾⣿⣿⣦⠀⠀⠀⠙⢿⣿⣤⠀⠙⣄⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⢋⡟⢠⣿⣿⣿⠋⢿⣄⠀⠀⠀⠈⡄⠙⣶⣈⡄⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠐⠚⢲⣿⠀⣾⣿⣿⠁⠀⠀⠉⢷⡀⠀⠀⣇⠀⠀⠈⠻⡀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢢⣀⣿⡏⠀⣿⡿⠀⠀⠀⠀⠀⠀⠙⣦⠀⢧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠿⣧⣾⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⣮⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠙⠛⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n\n";
// DarkOs: 23 lines, done at line 6698.
// Logo #61: Itc...
static const char Itc[] = "....................-==============+...\n....................-==============:...\n...:===========-....-==============:...\n...-===========:....-==============-...\n....*==========+........-::********-...\n....*===========+.:*====**==*+-.-......\n....:============*+-..--:+**====*---...\n......::--........................::...\n..+-:+-.+::*:+::+:-++::++-.:-.*.:++:++.\n..:-:-++++:-::--:+::-::.:++-++:++--:-:.    ⠀⠀⠀⠀⠀\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n";
// Itc: 12 lines, done at line 6715.
// Logo #62: dragonfly_old...
static const char dragonfly_old[] = "                        .-.\n                  ()I()\n             \"==.__:-:__.==\"\n            \"==.__/~|~\\__.==\"\n            \"==._(  Y  )_.==\"\n .-'~~\"\"~=--...,__\\/|\\/__,...--=~\"\"~~'-.\n(               ..=\\\\=/=..               )\n `'-.        ,.-\"`;/=\\\\;\"-.,_        .-'`\n     `~\"-=-~` .-~` |=| `~-. `~-=-\"~`\n          .-~`    /|=|\\    `~-.\n       .~`       / |=| \\       `~.\n   .-~`        .'  |=|  `.        `~-.\n (`     _,.-=\"`    |=|    `\"=-.,_     `)\n  `~\"~\"`           |=|           `\"~\"~`\n                   /=\\\\\n                   \\\\=/\n                    ^\n";
// dragonfly_old: 18 lines, done at line 6738.
// Logo #63: dragonfly_small...
static const char dragonfly_small[] = "   ,_,\n('-_|_-')\n >--|--<\n(_-'|'-_)\n    |\n    |\n    |\n";
// dragonfly_small: 8 lines, done at line 6751.
// Logo #64: DragonFly...
static const char DragonFly[] = ",--,           |           ,--,\n|   `-,       ,^,       ,-'   |\n `,    `-,   (/ \\)   ,-'    ,'\n   `-,    `-,/   \\,-'    ,-'\n      `------(   )------'\n  ,----------(   )----------,\n |        _,-(   )-,_        |\n  `-,__,-'   \\   /   `-,__,-'\n              | |\n              | |\n              | |\n              | |\n              | |\n              | |\n              `|'\n";
// DragonFly: 16 lines, done at line 6772.
// Logo #65: Drauger...
static const char Drauger[] = "                  -``-\n                `:+``+:`\n               `/++``++/.\n              .++/.  ./++.\n             :++/`    `/++:\n           `/++:        :++/`\n          ./+/-          -/+/.\n         -++/.            ./++-\n        :++:`              `:++:\n      `/++-                  -++/`\n     ./++.                    ./+/.\n    -++/`                      `/++-\n   :++:`                        `:++:\n `/++-                            -++/`\n.:-.`..............................`.-:.\n`.-/++++++++++++++++++++++++++++++++/-.`\n";
// Drauger: 17 lines, done at line 6794.
// Logo #66: elementary_small...
static const char elementary_small[] = "  _______\n / ____  \\\\\n/  |  /  /\\\\\n|__\\\\ /  / |\n\\\\   /__/  /\n \\\\_______/\n";
// elementary_small: 7 lines, done at line 6806.
// Logo #67: Elementary...
static const char Elementary[] = "         eeeeeeeeeeeeeeeee\n      eeeeeeeeeeeeeeeeeeeeeee\n    eeeee  eeeeeeeeeeee   eeeee\n  eeee   eeeee       eee     eeee\n eeee   eeee          eee     eeee\neee    eee            eee       eee\neee   eee            eee        eee\nee    eee           eeee       eeee\nee    eee         eeeee      eeeeee\nee    eee       eeeee      eeeee ee\neee   eeee   eeeeee      eeeee  eee\neee    eeeeeeeeee     eeeeee    eee\n eeeeeeeeeeeeeeeeeeeeeeee    eeeee\n  eeeeeeee eeeeeeeeeeee      eeee\n    eeeee                 eeeee\n      eeeeeee         eeeeeee\n         eeeeeeeeeeeeeeeee\n";
// Elementary: 18 lines, done at line 6829.
// Logo #68: EndeavourOS...
static const char EndeavourOS[] = "                     ./o.\n                   ./sssso-\n                 `:osssssss+-\n               `:+sssssssssso/.\n             `-/ossssssssssssso/.\n           `-/+sssssssssssssssso+:`\n         `-:/+sssssssssssssssssso+/.\n       `.://osssssssssssssssssssso++-\n      .://+ssssssssssssssssssssssso++:\n    .:///ossssssssssssssssssssssssso++:\n  `:////ssssssssssssssssssssssssssso+++.\n`-////+ssssssssssssssssssssssssssso++++-\n `..-+oosssssssssssssssssssssssso+++++/`\n   ./++++++++++++++++++++++++++++++/:.\n  `:::::::::::::::::::::::::------``\n";
// EndeavourOS: 16 lines, done at line 6850.
// Logo #69: Endless...
static const char Endless[] = "           `:+yhmNMMMMNmhy+:`\n        -odMMNhso//////oshNMMdo-\n      /dMMh+.              .+hMMd/\n    /mMNo`                    `oNMm:\n  `yMMo`                        `oMMy`\n `dMN-                            -NMd`\n hMN.                              .NMh\n/MM/                  -os`          /MM/\ndMm    `smNmmhs/- `:sNMd+   ``       mMd\nMMy    oMd--:+yMMMMMNo.:ohmMMMNy`    yMM\nMMy    -NNyyhmMNh+oNMMMMMy:.  dMo    yMM\ndMm     `/++/-``/yNNh+/sdNMNddMm-    mMd\n/MM/          `dNy:       `-::-     /MM/\n hMN.                              .NMh\n `dMN-                            -NMd`\n  `yMMo`                        `oMMy`\n    /mMNo`                    `oNMm/\n      /dMMh+.              .+hMMd/\n        -odMMNhso//////oshNMMdo-\n           `:+yhmNMMMMNmhy+:`\n";
// Endless: 21 lines, done at line 6876.
// Logo #70: EuroLinux...
static const char EuroLinux[] = "                __\n         -wwwWWWWWWWWWwww-\n        -WWWWWWWWWWWWWWWWWWw-\n          \\WWWWWWWWWWWWWWWWWWW-\n  _Ww      `WWWWWWWWWWWWWWWWWWWw\n -WEWww                -WWWWWWWWW-\n_WWUWWWW-                _WWWWWWWW\n_WWRWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW-\nwWWOWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW\nWWWLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWw\nWWWIWWWWWWWWWWWWWWWWWWWWWWWWWWWWww-\nwWWNWWWWw\n WWUWWWWWWw\n wWXWWWWWWWWww\n   wWWWWWWWWWWWWWWWw\n    wWWWWWWWWWWWWWWWw\n       WWWWWWWWWWWWWw\n           wWWWWWWWw\n";
// EuroLinux: 19 lines, done at line 6900.
// Logo #71: Exherbo...
static const char Exherbo[] = " ,\nOXo.\nNXdX0:    .cok0KXNNXXK0ko:.\nKX  '0XdKMMK;.xMMMk, .0MMMMMXx;  ...\n'NO..xWkMMx   kMMM    cMMMMMX,NMWOxOXd.\n  cNMk  NK    .oXM.   OMMMMO. 0MMNo  kW.\n  lMc   o:       .,   .oKNk;   ;NMMWlxW'\n ;Mc    ..   .,,'    .0Mg;WMN'dWMMMMMMO\n XX        ,WMMMMW.  cMcfliWMKlo.   .kMk\n.Mo        .WMGDMW.   XMWO0MMk        oMl\n,M:         ,XMMWx::,''oOK0x;          NM.\n'Ml      ,kNKOxxxxxkkO0XXKOd:.         oMk\n NK    .0Nxc:::::::::::::::fkKNk,      .MW\n ,Mo  .NXc::qXWXb::::::::::oo::lNK.    .MW\n  ;Wo oMd:::oNMNP::::::::oWMMMx:c0M;   lMO\n   'NO;W0c:::::::::::::::dMMMMO::lMk  .WM'\n     xWONXdc::::::::::::::oOOo::lXN. ,WMd\n      'KWWNXXK0Okxxo,:::::::,lkKNo  xMMO\n        :XMNxl,';:lodxkOO000Oxc. .oWMMo\n          'dXMMXkl;,.        .,o0MMNo'\n             ':d0XWMMMMWNNNNMMMNOl'\n                   ':okKXWNKkl'\n";
// Exherbo: 23 lines, done at line 6928.
// Logo #72: fedora_small...
static const char fedora_small[] = "      _____\n     /   __)\\\\\n     |  /  \\\\ \\\\\n  ___|  |__/ /\n / (_    _)_/\n/ /  |  |\n\\\\ \\\\__/  |\n \\\\(_____/\n";
// fedora_small: 9 lines, done at line 6942.
// Logo #73: RFRemix...
static const char RFRemix[] = "          /:-------------:\\\\\n       :-------------------::\n     :-----------/shhOHbmp---:\\\\\n   /-----------omMMMNNNMMD  ---:\n  :-----------sMMMMNMNMP.    ---:\n :-----------:MMMdP-------    ---\\\\\n,------------:MMMd--------    ---:\n:------------:MMMd-------    .---:\n:----    oNMMMMMMMMMNho     .----:\n:--     .+shhhMMMmhhy++   .------/\n:-    -------:MMMd--------------:\n:-   --------/MMMd-------------;\n:-    ------/hMMMy------------:\n:-- :dMNdhhdNMMNo------------;\n:---:sdNMMMMNds:------------:\n:------:://:-------------::\n:---------------------://\n";
// RFRemix: 18 lines, done at line 6965.
// Logo #74: Feren...
static const char Feren[] = " `----------`\n :+ooooooooo+.\n-o+oooooooooo+-\n..`/+++++++++++/...`````````````````\n   .++++++++++++++++++++++++++/////-\n    ++++++++++++++++++++++++++++++++//:`\n    -++++++++++++++++++++++++++++++/-`\n     ++++++++++++++++++++++++++++:.\n     -++++++++++++++++++++++++/.\n      +++++++++++++++++++++/-`\n      -++++++++++++++++++//-`\n        .:+++++++++++++//////-\n           .:++++++++//////////-\n             `-++++++---:::://///.\n           `.:///+++.             `\n          `.........\n";
// Feren: 17 lines, done at line 6987.
// Logo #75: freebsd_small...
static const char freebsd_small[] = "/\\\\,-'''''-,/\\\\\n\\\\_)       (_/\n|           |\n|           |\n ;         ;\n  '-_____-'\n";
// freebsd_small: 7 lines, done at line 6999.
// Logo #76: FreeMiNT...
static const char FreeMiNT[] = "          ##\n          ##         #########\n                    ####      ##\n            ####  ####        ##\n####        ####  ##        ##\n        ####    ####      ##  ##\n        ####  ####  ##  ##  ##\n            ####  ######\n        ######  ##  ##  ####\n      ####    ################\n    ####        ##  ####\n    ##            ####  ######\n    ##      ##    ####  ####\n    ##    ##  ##    ##  ##  ####\n      ####  ##          ##  ##\n";
// FreeMiNT: 16 lines, done at line 7045.
// Logo #77: Frugalware...
static const char Frugalware[] = "          `++/::-.`\n         /o+++++++++/::-.`\n        `o+++++++++++++++o++/::-.`\n        /+++++++++++++++++++++++oo++/:-.``\n       .o+ooooooooooooooooooosssssssso++oo++/:-`\n       ++osoooooooooooosssssssssssssyyo+++++++o:\n      -o+ssoooooooooooosssssssssssssyyo+++++++s`\n      o++ssoooooo++++++++++++++sssyyyyo++++++o:\n     :o++ssoooooo/-------------+syyyyyo+++++oo\n    `o+++ssoooooo/-----+++++ooosyyyyyyo++++os:\n    /o+++ssoooooo/-----ooooooosyyyyyyyo+oooss\n   .o++++ssooooos/------------syyyyyyhsosssy-\n   ++++++ssooooss/-----+++++ooyyhhhhhdssssso\n  -s+++++syssssss/-----yyhhhhhhhhhhhddssssy.\n  sooooooyhyyyyyh/-----hhhhhhhhhhhddddyssy+\n :yooooooyhyyyhhhyyyyyyhhhhhhhhhhdddddyssy`\n yoooooooyhyyhhhhhhhhhhhhhhhhhhhddddddysy/\n-ysooooooydhhhhhhhhhhhddddddddddddddddssy\n .-:/+osssyyyysyyyyyyyyyyyyyyyyyyyyyyssy:\n       ``.-/+oosysssssssssssssssssssssss\n               ``.:/+osyysssssssssssssh.\n                        `-:/+osyyssssyo\n                                .-:+++`\n";
// Frugalware: 24 lines, done at line 7074.
// Logo #78: Funtoo...
static const char Funtoo[] = "   .dKXXd                         .\n  :XXl;:.                      .OXo\n.'OXO''  .''''''''''''''''''''':XNd..'oco.lco,\nxXXXXXX, cXXXNNNXXXXNNXXXXXXXXNNNNKOOK; d0O .k\n  kXX  xXo  KNNN0  KNN.       'xXNo   :c; 'cc.\n  kXX  xNo  KNNN0  KNN. :xxxx. 'NNo\n  kXX  xNo  loooc  KNN. oNNNN. 'NNo\n  kXX  xN0:.       KNN' oNNNX' ,XNk\n  kXX  xNNXNNNNNNNNXNNNNNNNNXNNOxXNX0Xl\n  ...  ......................... .;cc;.\n";
// Funtoo: 11 lines, done at line 7090.
// Logo #79: GalliumOS...
static const char GalliumOS[] = "sooooooooooooooooooooooooooooooooooooo+:\nyyooooooooooooooooooooooooooooooooo+/:::\nyyysoooooooooooooooooooooooooooo+/::::::\nyyyyyoooooooooooooooooooooooo+/:::::::::\nyyyyyysoooooooooooooooooo++/::::::::::::\nyyyyyyysoooooooooooooo++/:::::::::::::::\nyyyyyyyyysoooooosydddys+/:::::::::::::::\nyyyyyyyyyysooosmMMMMMMMNd+::::::::::::::\nyyyyyyyyyyyyosMMMMMMMMMMMN/:::::::::::::\nyyyyyyyyyyyyydMMMMMMMMMMMMo//:::::::::::\nyyyyyyyyyyyyyhMMMMMMMMMMMm--//::::::::::\nyyyyyyyyyyyyyyhmMMMMMMMNy:..-://::::::::\nyyyyyyyyyyyyyyyyyhhyys+:......://:::::::\nyyyyyyyyyyyyyyys+:--...........-///:::::\nyyyyyyyyyyyys+:--................://::::\nyyyyyyyyyo+:-.....................-//:::\nyyyyyyo+:-..........................://:\nyyyo+:-..............................-//\no/:-...................................:\n";
// GalliumOS: 20 lines, done at line 7115.
// Logo #80: Garuda...
static const char Garuda[] = "                  __,,,,,,,_\n            _╓╗╣╫╠╠╠╠╠╠╠╠╠╠╠╠╠╕╗╗┐_\n         ╥╢╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╥,\n       ╗╠╠╠╠╠╠╠╝╜╜╜╜╝╢╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠┐\n      ╣╠╠╠╠╠╠╠╠╢╣╢╗╕ , `\"╘╠╠╠╠╠╠╠╠╠╠╠╠╠╠╔╥_\n    ╒╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╕╙╥╥╜   `\"╜╠╬╠╠╠╠╠╠╠╠╠╠╠╥,\n    ╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╗╥╥╥╥╗╗╬╠╠╠╠╠╠╠╝╙╠╠╣╠╠╠╠╢┐\n   ╣╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╥╬╣╠╠╠╠╠╠╠╠╗\n  ╒╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╗\n  ╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠\n  ╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╬     ```\"╜╝╢╠╠╡\n ╒╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╣,         ╘╠╪\n ╞╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╢┐        ╜\n `╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╗\n ,╬╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠\"╕\n ╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╗\n ╝^╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╝╣╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╡\n  ╔╜`╞┘╢╛╜ ╡╢╠\"╚╠╠╜╝┌╞╞\"╢╠╠╠╠╠╠╠╠╠╠╣╩╢╪\n     ╜╒\"   `╜    `      ╜╙╕  └╣╠╠╠╠╕ ╞╙╖\n                                ╠╠╠\n                                 ╜\n";
// Garuda: 22 lines, done at line 7142.
// Logo #81: gentoo_small...
static const char gentoo_small[] = " _-----_\n(       \\\\\n\\    0   \\\\\n \\        )\n /      _/\n(     _-\n\\____-\n";
// gentoo_small: 8 lines, done at line 7155.
// Logo #82: Gentoo...
static const char Gentoo[] = "         -/oyddmdhs+:.\n     -odNMMMMMMMMNNmhy+-`\n   -yNMMMMMMMMMMMNNNmmdhy+-\n `omMMMMMMMMMMMMNmdmmmmddhhy/`\n omMMMMMMMMMMMNhhyyyohmdddhhhdo`\n.ydMMMMMMMMMMdhs++so/smdddhhhhdm+`\n oyhdmNMMMMMMMNdyooydmddddhhhhyhNd.\n  :oyhhdNNMMMMMMMNNNmmdddhhhhhyymMh\n    .:+sydNMMMMMNNNmmmdddhhhhhhmMmy\n       /mMMMMMMNNNmmmdddhhhhhmMNhs:\n    `oNMMMMMMMNNNmmmddddhhdmMNhs+`\n  `sNMMMMMMMMNNNmmmdddddmNMmhs/.\n /NMMMMMMMMNNNNmmmdddmNMNdso:`\n+MMMMMMMNNNNNmmmmdmNMNdso/-\nyMMNNNNNNNmmmmmNNMmhs+/-`\n/hMMNNNNNNNNMNdhs++/-`\n`/ohdmmddhys+++/:.`\n  `-//////:--.\n";
// Gentoo: 19 lines, done at line 7179.
// Logo #83: Pentoo...
static const char Pentoo[] = "           `:oydNNMMMMNNdyo:`\n        :yNMMMMMMMMMMMMMMMMNy:\n      :dMMMMMMMMMMMMMMMMMMMMMMd:\n     oMMMMMMMho/-....-/ohMMMMMMMo\n    oMMMMMMy.            .yMMMMMMo\n   .MMMMMMo                oMMMMMM.\n   +MMMMMm                  mMMMMM+\n   oMMMMMh                  hMMMMMo\n //hMMMMMm//`          `////mMMMMMh//\nMMMMMMMMMMM/      /o/`  .smMMMMMMMMMMM\nMMMMMMMMMMm      `NMN:    .yMMMMMMMMMM\nMMMMMMMMMMMh:.              dMMMMMMMMM\nMMMMMMMMMMMMMy.            -NMMMMMMMMM\nMMMMMMMMMMMd:`           -yNMMMMMMMMMM\nMMMMMMMMMMh`          ./hNMMMMMMMMMMMM\nMMMMMMMMMMs        .:ymMMMMMMMMMMMMMMM\nMMMMMMMMMMNs:..-/ohNMMMMMMMMMMMMMMMMMM\nMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\nMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\n MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\n\n";
// Pentoo: 22 lines, done at line 7206.
// Logo #84: gNewSense...
static const char gNewSense[] = "                     ..,,,,..\n               .oocchhhhhhhhhhccoo.\n        .ochhlllllllc hhhhhh ollllllhhco.\n    ochlllllllllll hhhllllllhhh lllllllllllhco\n .cllllllllllllll hlllllo  +hllh llllllllllllllc.\nollllllllllhco''  hlllllo  +hllh  ``ochllllllllllo\nhllllllllc'       hllllllllllllh       `cllllllllh\nollllllh          +llllllllllll+          hllllllo\n `cllllh.           ohllllllho           .hllllc'\n    ochllc.            ++++            .cllhco\n       `+occooo+.                .+ooocco+'\n              `+oo++++      ++++oo+'\n";
// gNewSense: 13 lines, done at line 7224.
// Logo #85: GNOME...
static const char GNOME[] = "                               ,@@@@@@@@,\n                 @@@@@@      @@@@@@@@@@@@\n        ,@@.    @@@@@@@    *@@@@@@@@@@@@\n       @@@@@%   @@@@@@(    @@@@@@@@@@@&\n       @@@@@@    @@@@*     @@@@@@@@@#\n@@@@*   @@@@,              *@@@@@%\n@@@@@.\n @@@@#         @@@@@@@@@@@@@@@@\n         ,@@@@@@@@@@@@@@@@@@@@@@@,\n      ,@@@@@@@@@@@@@@@@@@@@@@@@@@&\n    .@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n    @@@@@@@@@@@@@@@@@@@@@@@@@@@\n   @@@@@@@@@@@@@@@@@@@@@@@@(\n   @@@@@@@@@@@@@@@@@@@@%\n    @@@@@@@@@@@@@@@@\n     @@@@@@@@@@@@*        @@@@@@@@/\n      &@@@@@@@@@@        @@@@@@@@@*\n        @@@@@@@@@@@,    @@@@@@@@@*\n          ,@@@@@@@@@@@@@@@@@@@@&\n              &@@@@@@@@@@@@@@\n                     ...\n";
// GNOME: 22 lines, done at line 7251.
// Logo #86: GNU...
static const char GNU[] = "    _-`````-,           ,- '- .\n  .'   .- - |          | - -.  `.\n /.'  /                     `.   \\\n:/   :      _...   ..._      ``   :\n::   :     /._ .`:'_.._\\.    ||   :\n::    `._ ./  ,`  :    \\ . _.''   .\n`:.      /   |  -.  \\-. \\\\_      /\n  \\:._ _/  .'   .@)  \\@) ` `\\ ,.'\n     _/,--'       .- .\\,-.`--`.\n       ,'/''     (( \\ `  )\n        /'/'  \\    `-'  (\n         '/''  `._,-----'\n          ''/'    .,---'\n           ''/'      ;:\n             ''/''  ''/\n               ''/''/''\n                 '/'/'\n                  `;\n";
// GNU: 19 lines, done at line 7275.
// Logo #87: GoboLinux...
static const char GoboLinux[] = "  _____       _\n / ____|     | |\n| |  __  ___ | |__   ___\n| | |_ |/ _ \\| '_ \\ / _ \\\n| |__| | (_) | |_) | (_) |\n \\_____|\\___/|_.__/ \\___/\n";
// GoboLinux: 7 lines, done at line 7287.
// Logo #88: Grombyang...
static const char Grombyang[] = "            eeeeeeeeeeee\n         eeeeeeeeeeeeeeeee\n      eeeeeeeeeeeeeeeeeeeeeee\n    eeeee       .o+       eeee\n  eeee         `ooo/         eeee\n eeee         `+oooo:         eeee\neee          `+oooooo:          eee\neee          -+oooooo+:         eee\nee         `/:oooooooo+:         ee\nee        `/+   +++    +:        ee\nee              +o+\\             ee\neee             +o+\\            eee\neee        //  \\\\ooo/  \\\\\\        eee\n eee      //++++oooo++++\\\\\\     eee\n  eeee    ::::++oooo+:::::   eeee\n    eeeee   Grombyang OS   eeee\n      eeeeeeeeeeeeeeeeeeeeeee\n         eeeeeeeeeeeeeeeee\n";
// Grombyang: 19 lines, done at line 7311.
// Logo #89: guix_small...
static const char guix_small[] = "|.__          __.|\n|__ \\\\        / __|\n   \\\\ \\\\      / /\n    \\\\ \\\\    / /\n     \\\\ \\\\  / /\n      \\\\ \\\\/ /\n       \\\\__/\n";
// guix_small: 8 lines, done at line 7324.
// Logo #90: Guix...
static const char Guix[] = " ..                             `.\n `--..```..`           `..```..--`\n   .-:///-:::.       `-:::///:-.\n      ````.:::`     `:::.````\n           -//:`    -::-\n            ://:   -::-\n            `///- .:::`\n             -+++-:::.\n              :+/:::-\n              `-....`\n";
// Guix: 11 lines, done at line 7340.
// Logo #91: haiku_small...
static const char haiku_small[] = "       ,^,\n      /   \\\\\n*--_ ;     ; _--*\n\\\\   '\"     \"'   /\n '.           .'\n.-'\"         \"'-.\n '-.__.   .__.-'\n       |_|\n";
// haiku_small: 9 lines, done at line 7354.
// Logo #92: Haiku...
static const char Haiku[] = "          :dc'\n       'l:;','ck.    .;dc:.\n       co    ..k.  .;;   ':o.\n       co    ..k. ol      .0.\n       co    ..k. oc     ..0.\n       co    ..k. oc     ..0.\n.Ol,.  co ...''Oc;kkodxOdddOoc,.\n ';lxxlxOdxkxk0kdooolldlccc:clxd;\n     ..oOolllllccccccc:::::od;\n       cx:ooc:::::::;cooolcX.\n       cd.''cloxdoollc' ...0.\n       cd......k;.xl....  .0.\n       .::c;..cx;.xo..... .0.\n          '::c'...do..... .K,\n                  cd,.....:O,\n                    ':clod:'\n                        \n";
// Haiku: 18 lines, done at line 7377.
// Logo #93: Huayra...
static const char Huayra[] = "                     `\n            .       .       `\n       ``    -      .      .\n        `.`   -` `. -  `` .`\n          ..`-`-` + -  / .`     ```\n          .--.+--`+:- :/.` .-``.`\n            -+/so::h:.d-`./:`.`\n              :hNhyMomy:os-...-.  ````\n               .dhsshNmNhoo+:-``.```\n                `ohy:-NMds+::-.``\n            ````.hNN+`mMNho/:-....````\n       `````     `../dmNhoo+/:..``\n    ````            .dh++o/:....`\n.+s/`                `/s-.-.:.`` ````\n::`                    `::`..`\n                          .` `..\n                                ``\n";
// Huayra: 18 lines, done at line 7400.
// Logo #94: hyperbola_small...
static const char hyperbola_small[] = "    |`__.`/\n    \\____/\n    .--.\n   /    \\\\\n  /  ___ \\\\\n / .`   `.\\\\\n/.`      `.\\\\\n";
// hyperbola_small: 8 lines, done at line 7413.
// Logo #95: Hyperbola...
static const char Hyperbola[] = "                     WW\n                     KX              W\n                    WO0W          NX0O\n                    NOO0NW  WNXK0OOKW\n                    W0OOOOOOOOOOOOKN\n                     N0OOOOOOO0KXW\n                       WNXXXNW\n                 NXK00000KN\n             WNK0OOOOOOOOOO0W\n           NK0OOOOOOOOOOOOOO0W\n         X0OOOOOOO00KK00OOOOOK\n       X0OOOO0KNWW      WX0OO0W\n     X0OO0XNW              KOOW\n   N00KNW                   KOW\n NKXN                       W0W\nWW                           W\n";
// Hyperbola: 17 lines, done at line 7435.
// Logo #96: Ataraxia...
static const char Ataraxia[] = "               'l:\n        loooooo\n          loooo coooool\n looooooooooooooooooool\n  looooooooooooooooo\n         lool   cooo\n        coooooooloooooooo\n     clooooo  ;lood  cloooo\n  :loooocooo cloo      loooo\n loooo  :ooooool       loooo\nlooo    cooooo        cooooo\nlooooooooooooo      ;loooooo looooooc\nlooooooooo loo   cloooooool    looooc\n cooo       cooooooooooo       looolooooool\n            cooo:     coooooooooooooooooool\n                       loooooooooooolc:   loooc;\n                             cooo:    loooooooooooc\n                            ;oool         looooooo:\n                           coool          olc,\n                          looooc   ,,\n                        coooooc    loc\n                       :oooool,    coool:, looool:,\n                       looool:      ooooooooooooooo:\n                       cooolc        .ooooooooooool\n";
// Ataraxia: 25 lines, done at line 7465.
// Logo #97: Kali...
static const char Kali[] = "..............\n            ..,;:ccc,.\n          ......''';lxO.\n.....''''..........,:ld;\n           .';;;:::;,,.x,\n      ..'''.            0Xxoc:,.  ...\n  ....                ,ONkc;,;cokOdc',.\n .                   OMo           ':ddo.\n                    dMc               :OO;\n                    0M.                 .:o.\n                    ;Wd\n                     ;XO,\n                       ,d0Odlc;,..\n                           ..',;:cdOOd::,.\n                                    .:d;.':;.\n                                       'd,  .'\n                                         ;l   ..\n                                          .o\n                                            c\n                                            .'\n                                             .\n";
// Kali: 22 lines, done at line 7492.
// Logo #98: KaOS...
static const char KaOS[] = "                     ..\n  .....         ..OSSAAAAAAA..\n .KKKKSS.     .SSAAAAAAAAAAA.\n.KKKKKSO.    .SAAAAAAAAAA...\nKKKKKKS.   .OAAAAAAAA.\nKKKKKKS.  .OAAAAAA.\nKKKKKKS. .SSAA..\n.KKKKKS..OAAAAAAAAAAAA........\n DKKKKO.=AA=========A===AASSSO..\n  AKKKS.==========AASSSSAAAAAASS.\n  .=KKO..========ASS.....SSSSASSSS.\n    .KK.       .ASS..O.. =SSSSAOSS:\n     .OK.      .ASSSSSSSO...=A.SSA.\n       .K      ..SSSASSSS.. ..SSA.\n                 .SSS.AAKAKSSKA.\n                    .SSS....S..\n";
// KaOS: 17 lines, done at line 7514.
// Logo #99: KDE...
static const char KDE[] = "             `..---+/---..`\n         `---.``   ``   `.---.`\n      .--.`        ``        `-:-.\n    `:/:     `.----//----.`     :/-\n   .:.    `---`          `--.`    .:`\n  .:`   `--`                .:-    `:.\n `/    `:.      `.-::-.`      -:`   `/`\n /.    /.     `:++++++++:`     .:    .:\n`/    .:     `+++++++++++/      /`   `+`\n/+`   --     .++++++++++++`     :.   .+:\n`/    .:     `+++++++++++/      /`   `+`\n /`    /.     `:++++++++:`     .:    .:\n ./    `:.      `.:::-.`      -:`   `/`\n  .:`   `--`                .:-    `:.\n   .:.    `---`          `--.`    .:`\n    `:/:     `.----//----.`     :/-\n      .-:.`        ``        `-:-.\n         `---.``   ``   `.---.`\n             `..---+/---..`\n";
// KDE: 20 lines, done at line 7539.
// Logo #100: Kibojoe...
static const char Kibojoe[] = "                       ./+oooooo+/.\n           -/+ooooo+/:.`\n          `yyyo+++/++osss.\n         +NMNyssssssssssss.\n       .dMMMMNsssssssssssyNs`\n      +MMMMMMMmssssssssssshMNo`\n    `hMMMMMNNNMdsssssssssssdMMN/\n   .syyyssssssyNNmmmmdssssshMMMMd:\n  -NMmhyssssssssyhhhhyssyhmMMMMMMMy`\n -NMMMMMNNmdhyyyyyyyhdmNMMMMMMMMMMMN+\n`NMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMd.\nods+/:-----://+oyydmNMMMMMMMMMMMMMMMMMN-\n`                     .-:+osyhhdmmNNNmdo\n";
// Kibojoe: 14 lines, done at line 7558.
// Logo #101: Kogaion...
static const char Kogaion[] = "            ;;      ,;\n           ;;;     ,;;\n         ,;;;;     ;;;;\n      ,;;;;;;;;    ;;;;\n     ;;;;;;;;;;;   ;;;;;\n    ,;;;;;;;;;;;;  ';;;;;,\n    ;;;;;;;;;;;;;;, ';;;;;;;\n    ;;;;;;;;;;;;;;;;;, ';;;;;\n;    ';;;;;;;;;;;;;;;;;;, ;;;\n;;;,  ';;;;;;;;;;;;;;;;;;;,;;\n;;;;;,  ';;;;;;;;;;;;;;;;;;,\n;;;;;;;;,  ';;;;;;;;;;;;;;;;,\n;;;;;;;;;;;;, ';;;;;;;;;;;;;;\n';;;;;;;;;;;;; ';;;;;;;;;;;;;\n ';;;;;;;;;;;;;, ';;;;;;;;;;;\n  ';;;;;;;;;;;;;  ;;;;;;;;;;\n    ';;;;;;;;;;;; ;;;;;;;;\n        ';;;;;;;; ;;;;;;\n           ';;;;; ;;;;\n             ';;; ;;\n";
// Kogaion: 21 lines, done at line 7584.
// Logo #102: Korora...
static const char Korora[] = "                ____________\n             _add55555555554:\n           _w?'``````````')k:\n          _Z'`            ]k:\n          m(`             )k:\n     _.ss`m[`,            ]e:\n   .uY\"^``Xc`?Ss.         d(`\n  jF'`    `@.  `Sc      .jr`\n jr`       `?n_ `$;   _a2\"`\n.m:          `~M`1k`5?!``\n:#:             `)e```\n:m:             ,#'`\n:#:           .s2'`\n:m,________.aa7^`\n:#baaaaaaas!J'`\n ```````````\n";
// Korora: 17 lines, done at line 7606.
// Logo #103: KSLinux...
static const char KSLinux[] = " K   K U   U RRRR   ooo\n K  K  U   U R   R o   o\n KKK   U   U RRRR  o   o\n K  K  U   U R  R  o   o\n K   K  UUU  R   R  ooo\n\n  SSS   AAA  W   W  AAA\n S     A   A W   W A   A\n  SSS  AAAAA W W W AAAAA\n     S A   A WW WW A   A\n  SSS  A   A W   W A   A\n";
// KSLinux: 12 lines, done at line 7623.
// Logo #104: Kubuntu...
static const char Kubuntu[] = "           `.:/ossyyyysso/:.\n        .:oyyyyyyyyyyyyyyyyyyo:`\n      -oyyyyyyyodMMyyyyyyyysyyyyo-\n    -syyyyyyyyyydMMyoyyyydmMMyyyyys-\n   oyyysdMysyyyydMMMMMMMMMMMMMyyyyyyyo\n `oyyyydMMMMysyysoooooodMMMMyyyyyyyyyo`\n oyyyyyydMMMMyyyyyyyyyyyysdMMysssssyyyo\n-yyyyyyyydMysyyyyyyyyyyyyyysdMMMMMysyyy-\noyyyysoodMyyyyyyyyyyyyyyyyyyydMMMMysyyyo\nyyysdMMMMMyyyyyyyyyyyyyyyyyyysosyyyyyyyy\nyyysdMMMMMyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\noyyyyysosdyyyyyyyyyyyyyyyyyyydMMMMysyyyo\n-yyyyyyyydMysyyyyyyyyyyyyyysdMMMMMysyyy-\n oyyyyyydMMMysyyyyyyyyyyysdMMyoyyyoyyyo\n `oyyyydMMMysyyyoooooodMMMMyoyyyyyyyyo\n   oyyysyyoyyyysdMMMMMMMMMMMyyyyyyyyo\n    -syyyyyyyyydMMMysyyydMMMysyyyys-\n      -oyyyyyyydMMyyyyyyysosyyyyo-\n        ./oyyyyyyyyyyyyyyyyyyo/.\n           `.:/oosyyyysso/:.`\n";
// Kubuntu: 21 lines, done at line 7649.
// Logo #105: LEDE...
static const char LEDE[] = "     _________\n    /        /\\\n   /  LE    /  \\\n  /    DE  /    \\\n /________/  LE  \\\n \\        \\   DE /\n  \\    LE  \\    /\n   \\  DE    \\  /\n    \\________\\/\n";
// LEDE: 10 lines, done at line 7664.
// Logo #106: Linux...
static const char Linux[] = "        #####\n       #######\n       ##O#O##\n       #######\n     ###########\n    #############\n   ###############\n   ################\n  #################\n#####################\n#####################\n  #################\n";
// Linux: 13 lines, done at line 7682.
// Logo #107: linuxlite_small...
static const char linuxlite_small[] = "   /\\\\\n  /  \\\\\n / / /\n> / /\n\\\\ \\\\ \\\\\n \\\\_\\\\_\\\\\n    \\\\\n";
// linuxlite_small: 8 lines, done at line 7695.
// Logo #108: Linux_Lite...
static const char Linux_Lite[] = "          ,xXc\n      .l0MMMMMO\n   .kNMMMMMWMMMN,\n   KMMMMMMKMMMMMMo\n  'MMMMMMNKMMMMMM:\n  kMMMMMMOMMMMMMO\n .MMMMMMX0MMMMMW.\n oMMMMMMxWMMMMM:\n WMMMMMNkMMMMMO\n:MMMMMMOXMMMMW\n.0MMMMMxMMMMM;\n:;cKMMWxMMMMO\n'MMWMMXOMMMMl\n kMMMMKOMMMMMX:\n .WMMMMKOWMMM0c\n  lMMMMMWO0MNd:'\n   oollXMKXoxl;.\n     ':. .: .'\n              ..\n                .\n";
// Linux_Lite: 21 lines, done at line 7721.
// Logo #109: LMDE...
static const char LMDE[] = "         `.-::---..\n      .:++++ooooosssoo:.\n    .+o++::.      `.:oos+.\n   :oo:.`             -+oo:\n `+o/`    .::::::-.    .++-`\n`/s/    .yyyyyyyyyyo:   +o-`\n`so     .ss       ohyo` :s-:\n`s/     .ss  h  m  myy/ /s``\n`s:     `oo  s  m  Myy+-o:`\n`oo      :+sdoohyoydyso/.\n :o.      .:////////++:\n `/++        -:::::-\n  `++-\n   `/+-\n     .+/.\n       .:+-.\n          `--.``\n";
// LMDE: 18 lines, done at line 7744.
// Logo #110: Lubuntu...
static const char Lubuntu[] = "           `-mddhhhhhhhhhddmss`\n        ./mdhhhhhhhhhhhhhhhhhhhhhh.\n     :mdhhhhhhhhhhhhhhhhhhhhhhhhhhhm`\n   :ymhhhhhhhhhhhhhhhyyyyyyhhhhhhhhhy:\n  `odhyyyhhhhhhhhhy+-````./syhhhhhhhho`\n `hhy..:oyhhhhhhhy-`:osso/..:/++oosyyyh`\n dhhs   .-/syhhhhs`shhhhhhyyyyyyyyyyyyhs\n:hhhy`  yso/:+syhy/yhhhhhshhhhhhhhhhhhhh:\nhhhhho. +hhhys++oyyyhhhhh-yhhhhhhhhhhhhhs\nhhhhhhs-`/syhhhhyssyyhhhh:-yhhhhhhhhhhhhh\nhhhhhhs  `:/+ossyyhyyhhhhs -yhhhhhhhhhhhh\nhhhhhhy/ `syyyssyyyyhhhhhh: :yhhhhhhhhhhs\n:hhhhhhyo:-/osyhhhhhhhhhhho  ohhhhhhhhhh:\n sdhhhhhhhyyssyyhhhhhhhhhhh+  +hhhhhhhhs\n `shhhhhhhhhhhhhhhhhhhhhhy+` .yhhhhhhhh`\n  +sdhhhhhhhhhhhhhhhhhyo/. `/yhhhhhhhd`\n   `:shhhhhhhhhh+---..``.:+yyhhhhhhh:\n     `:mdhhhhhh/.syssyyyyhhhhhhhd:`\n        `+smdhhh+shhhhhhhhhhhhdm`\n           `sNmdddhhhhhhhddm-`\n";
// Lubuntu: 21 lines, done at line 7770.
// Logo #111: Lunar...
static const char Lunar[] = "`-.                                 `-.\n  -ohys/-`                    `:+shy/`\n     -omNNdyo/`          :+shmNNy/`\n                   -\n                 /mMmo\n                 hMMMN`\n                 .NMMs\n      -:+oooo+//: /MN. -///oooo+/-`\n     /:.`          /           `.:/`\n          __\n         |  |   _ _ ___ ___ ___\n         |  |__| | |   | .'|  _|\n         |_____|___|_|_|__,|_|\n";
// Lunar: 14 lines, done at line 7789.
// Logo #112: _small...
static const char _small[] = "       .:'\n    _ :'_\n .'`_`-'_``.\n:________.-'\n:_______:\n:_______:\n :_______`-;\n  `._.-._.'\n";
// _small: 9 lines, done at line 7803.
// Logo #113: Darwin...
static const char Darwin[] = "                    'c.\n                 ,xNMM.\n               .OMMMMo\n               OMMM0,\n     .;loddo:' loolloddol;.\n   cKMMMMMMMMMMNWMMMMMMMMMM0:\n .KMMMMMMMMMMMMMMMMMMMMMMMWd.\n XMMMMMMMMMMMMMMMMMMMMMMMX.\n;MMMMMMMMMMMMMMMMMMMMMMMM:\n:MMMMMMMMMMMMMMMMMMMMMMMM:\n.MMMMMMMMMMMMMMMMMMMMMMMMX.\n kMMMMMMMMMMMMMMMMMMMMMMMMWd.\n .XMMMMMMMMMMMMMMMMMMMMMMMMMMk\n  .XMMMMMMMMMMMMMMMMMMMMMMMMK.\n    kMMMMMMMMMMMMMMMMMMMMMMd\n     ;KMMMMMMMWXXWMMMMMMMk.\n       .cooc,.    .,coo:.\n";
// Darwin: 18 lines, done at line 7826.
// Logo #114: mageia_small...
static const char mageia_small[] = "   *\n    *\n   **\n /\\\\__/\\\\\n/      \\\\\n\\\\      /\n \\\\____/\n";
// mageia_small: 8 lines, done at line 7839.
// Logo #115: Mageia...
static const char Mageia[] = "        .°°.\n         °°   .°°.\n         .°°°. °°\n         .   .\n          °°° .°°°.\n      .°°°.   '___'\n     .'___'        .\n   :dkxc;'.  ..,cxkd;\n .dkk. kkkkkkkkkk .kkd.\n.dkk.  ';cloolc;.  .kkd\nckk.                .kk;\nxO:                  cOd\nxO:                  lOd\nlOO.                .OO:\n.k00.              .00x\n .k00;            ;00O.\n  .lO0Kc;,,,,,,;c0KOc.\n     ;d00KKKKKK00d;\n        .,KKKK,.\n";
// Mageia: 20 lines, done at line 7864.
// Logo #116: MagpieOS...
static const char MagpieOS[] = "        ;00000     :000Ol\n     .x00kk00:    O0kk00k;\n    l00:   :00.  o0k   :O0k.\n  .k0k.     xd$ddddk'    .d00;\n  k0k.      .dddddl       o00,\n o00.        ':cc:.        d0O\n.00l                       ,00.\nl00.                       d0x\nk0O                     .:k0o\nO0k                 ;dO0000d.\nk0O               .O0Oxxxxk00:\no00.              k0Oddddddocc\n'00l              x0Odddddo;..\n x00.             .x00kxxd:..\n .O0x               .:oxxxOkl.\n  .x0d                     ,xx,\n    .:o.          .xd       ckd\n       ..          dxl     .xx;\n                    :xxolldxd'\n                      ;oxdl.\n";
// MagpieOS: 21 lines, done at line 7890.
// Logo #117: Mandriva...
static const char Mandriva[] = "                        ``\n                       `-.\n      `               .---\n    -/               -::--`\n  `++    `----...```-:::::.\n `os.      .::::::::::::::-```     `  `\n +s+         .::::::::::::::::---...--`\n-ss:          `-::::::::::::::::-.``.``\n/ss-           .::::::::::::-.``   `\n+ss:          .::::::::::::-\n/sso         .::::::-::::::-\n.sss/       -:::-.`   .:::::\n /sss+.    ..`  `--`    .:::\n  -ossso+/:://+/-`        .:`\n    -/+ooo+/-.              `\n";
// Mandriva: 16 lines, done at line 7911.
// Logo #118: manjaro_small...
static const char manjaro_small[] = "||||||||| ||||\n||||||||| ||||\n||||      ||||\n|||| |||| ||||\n|||| |||| ||||\n|||| |||| ||||\n|||| |||| ||||\n";
// manjaro_small: 8 lines, done at line 7924.
// Logo #119: Manjaro...
static const char Manjaro[] = "██████████████████  ████████\n██████████████████  ████████\n██████████████████  ████████\n██████████████████  ████████\n████████            ████████\n████████  ████████  ████████\n████████  ████████  ████████\n████████  ████████  ████████\n████████  ████████  ████████\n████████  ████████  ████████\n████████  ████████  ████████\n████████  ████████  ████████\n████████  ████████  ████████\n████████  ████████  ████████\n";
// Manjaro: 15 lines, done at line 7944.
// Logo #120: Maui...
static const char Maui[] = "             `.-://////:--`\n         .:/oooooooooooooooo+:.\n      `:+ooooooooooooooooooooooo:`\n    `:oooooooooooooooooooooooooooo/`\n    ..```-oooooo/-`` `:oooooo+:.` `--\n  :.      +oo+-`       /ooo/`       -/\n -o.     `o+-          +o/`         -o:\n`oo`     ::`  :o/     `+.  .+o`     /oo.\n/o+      .  -+oo-     `   /oo/     `ooo/\n+o-        /ooo+`       .+ooo.     :ooo+\n++       .+oooo:       -oooo+     `oooo+\n:.      .oooooo`      :ooooo-     :oooo:\n`      .oooooo:      :ooooo+     `ooo+-`\n      .+oooooo`     -oooooo:     `o/-\n      +oooooo:     .ooooooo.\n     /ooooooo`     /ooooooo/       ..\n    `:oooooooo/:::/ooooooooo+:--:/:`\n      `:+oooooooooooooooooooooo+:`\n         .:+oooooooooooooooo+:.\n             `.-://////:-.`\n";
// Maui: 21 lines, done at line 7970.
// Logo #121: Mer...
static const char Mer[] = "                         dMs\n                         .-`\n                       `y`-o+`\n                        ``NMMy\n                      .--`:++.\n                    .hNNNNs\n                    /MMMMMN\n                    `ommmd/ +/\n                      ````  +/\n                     `:+sssso/-`\n  .-::. `-::-`     `smNMNmdmNMNd/      .://-`\n.ymNMNNdmNMMNm+`  -dMMh:.....+dMMs   `sNNMMNo\ndMN+::NMMy::hMM+  mMMo `ohhy/ `dMM+  yMMy::-\nMMm   yMM-  :MMs  NMN` `:::::--sMMh  dMM`\nMMm   yMM-  -MMs  mMM+ `ymmdsymMMMs  dMM`\nNNd   sNN-  -NNs  -mMNs-.--..:dMMh`  dNN\n---   .--`  `--.   .smMMmdddmMNdo`   .--\n                     ./ohddds+:`\n                     +h- `.:-.\n                     ./`.dMMMN+\n                        +MMMMMd\n                        `+dmmy-\n                      ``` .+`\n                     .dMNo-y.\n                     `hmm/\n                         .:`\n                         dMs\n";
// Mer: 28 lines, done at line 8003.
// Logo #122: Minix...
static const char Minix[] = "   -sdhyo+:-`                -/syymm:\n   sdyooymmNNy.     ``    .smNmmdysNd\n   odyoso+syNNmysoyhhdhsoomNmm+/osdm/\n    :hhy+-/syNNmddhddddddmNMNo:sdNd:\n     `smNNdNmmNmddddddddddmmmmmmmy`\n   `ohhhhdddddmmNNdmddNmNNmdddddmdh-\n   odNNNmdyo/:/-/hNddNy-`..-+ydNNNmd:\n `+mNho:`   smmd/ sNNh :dmms`   -+ymmo.\n-od/       -mmmmo -NN+ +mmmm-       yms:\n+sms -.`    :so:  .NN+  :os/     .-`mNh:\n.-hyh+:////-     -sNNd:`    .--://ohNs-\n `:hNNNNNNNMMd/sNMmhsdMMh/ymmNNNmmNNy/\n  -+sNNNNMMNNNsmNMo: :NNmymNNNNMMMms:\n    //oydNMMMMydMMNysNMMmsMMMMMNyo/`\n       ../-yNMMy--/::/-.sMMmos+.`\n           -+oyhNsooo+omy/```\n              `::ohdmds-`\n";
// Minix: 18 lines, done at line 8026.
// Logo #123: linuxmint_small...
static const char linuxmint_small[] = " ___________\n|_          \\\\\n  | | _____ |\n  | | | | | |\n  | | | | | |\n  | \\\\_____/ |\n  \\\\_________/\n";
// linuxmint_small: 8 lines, done at line 8039.
// Logo #124: mint_old...
static const char mint_old[] = "MMMMMMMMMMMMMMMMMMMMMMMMMmds+.\nMMm----::-://////////////oymNMd+`\nMMd      /++                -sNMd:\nMMNso/`  dMM    `.::-. .-::.` .hMN:\nddddMMh  dMM   :hNMNMNhNMNMNh: `NMm\n    NMm  dMM  .NMN/-+MMM+-/NMN` dMM\n    NMm  dMM  -MMm  `MMM   dMM. dMM\n    NMm  dMM  -MMm  `MMM   dMM. dMM\n    NMm  dMM  .mmd  `mmm   yMM. dMM\n    NMm  dMM`  ..`   ...   ydm. dMM\n    hMM- +MMd/-------...-:sdds  dMM\n    -NMm- :hNMNNNmdddddddddy/`  dMM\n     -dMNs-``-::::-------.``    dMM\n      `/dMNmy+/:-------------:/yMMM\n         ./ydNMMMMMMMMMMMMMMMMMMMMM\n            .MMMMMMMMMMMMMMMMMMM\n";
// mint_old: 17 lines, done at line 8061.
// Logo #125: mint...
static const char mint[] = "             ...-:::::-...\n          .-MMMMMMMMMMMMMMM-.\n      .-MMMM`..-:::::::-..`MMMM-.\n    .:MMMM.:MMMMMMMMMMMMMMM:.MMMM:.\n   -MMM-M---MMMMMMMMMMMMMMMMMMM.MMM-\n `:MMM:MM`  :MMMM:....::-...-MMMM:MMM:`\n :MMM:MMM`  :MM:`  ``    ``  `:MMM:MMM:\n.MMM.MMMM`  :MM.  -MM.  .MM-  `MMMM.MMM.\n:MMM:MMMM`  :MM.  -MM-  .MM:  `MMMM-MMM:\n:MMM:MMMM`  :MM.  -MM-  .MM:  `MMMM:MMM:\n:MMM:MMMM`  :MM.  -MM-  .MM:  `MMMM-MMM:\n.MMM.MMMM`  :MM:--:MM:--:MM:  `MMMM.MMM.\n :MMM:MMM-  `-MMMMMMMMMMMM-`  -MMM-MMM:\n  :MMM:MMM:`                `:MMM:MMM:\n   .MMM.MMMM:--------------:MMMM.MMM.\n     '-MMMM.-MMMMMMMMMMMMMMM-.MMMM-'\n       '.-MMMM``--:::::--``MMMM-.'\n            '-MMMMMMMMMMMMM-'\n               ``-:::::-``\n";
// mint: 20 lines, done at line 8086.
// Logo #126: mx_small...
static const char mx_small[] = "    \\\\\\\\  /\n     \\\\\\\\/\n      \\\\\\\\\n   /\\\\/ \\\\\\\\\n  /  \\\\  /\\\\\n /    \\\\/  \\\\\n/__________\\\\\n";
// mx_small: 8 lines, done at line 8100.
// Logo #127: MX...
static const char MX[] = "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNMMMMMMMMM\nMMMMMMMMMMNs..yMMMMMMMMMMMMMm: +NMMMMMMM\nMMMMMMMMMN+    :mMMMMMMMMMNo` -dMMMMMMMM\nMMMMMMMMMMMs.   `oNMMMMMMh- `sNMMMMMMMMM\nMMMMMMMMMMMMN/    -hMMMN+  :dMMMMMMMMMMM\nMMMMMMMMMMMMMMh-    +ms. .sMMMMMMMMMMMMM\nMMMMMMMMMMMMMMMN+`   `  +NMMMMMMMMMMMMMM\nMMMMMMMMMMMMMMNMMd:    .dMMMMMMMMMMMMMMM\nMMMMMMMMMMMMm/-hMd-     `sNMMMMMMMMMMMMM\nMMMMMMMMMMNo`   -` :h/    -dMMMMMMMMMMMM\nMMMMMMMMMd:       /NMMh-   `+NMMMMMMMMMM\nMMMMMMMNo`         :mMMN+`   `-hMMMMMMMM\nMMMMMMh.            `oNMMd:    `/mMMMMMM\nMMMMm/                -hMd-      `sNMMMM\nMMNs`                   -          :dMMM\nMm:                                 `oMM\nMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\n";
// MX: 18 lines, done at line 8123.
// Logo #128: Namib...
static const char Namib[] = "          .:+shysyhhhhysyhs+:.\n       -/yyys              syyy/-\n     -shy                      yhs-\n   -yhs                          shy-\n  +hy                              yh+\n +ds                                sd+\n/ys                  so              sy/\nsh                 smMMNdyo           hs\nyo               ymMMMMNNMMNho        oy\nN             ydMMMNNMMMMMMMMMmy       N\nN         shmMMMMNNMMMMMMMMMMMMMNy     N\nyo  ooshmNMMMNNNNMMMMMMMMMMMMMMMMMms  oy\nsd yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy ds\n/ys                                  sy/\n +ds                                sd+\n  +hy                              yh+\n   -yhs                          shy-\n     -shy                      yhs-\n       -/yyys              syyy/-\n          .:+shysyhyhhysyhs+:.\n";
// Namib: 21 lines, done at line 8149.
// Logo #129: Neptune...
static const char Neptune[] = "            ./+sydddddddys/-.\n        .+ymNNdyooo/:+oooymNNmy/`\n     `/hNNh/.`             `-+dNNy:`\n    /mMd/.          .++.:oy/   .+mMd-\n  `sMN/             oMMmdy+.     `oNNo\n `hMd.           `/ymy/.           :NMo\n oMN-          `/dMd:               /MM-\n`mMy          -dMN+`                 mMs\n.MMo         -NMM/                   yMs\n dMh         mMMMo:`                `NMo\n /MM/        /ymMMMm-               sMN.\n  +Mm:         .hMMd`              oMN/\n   +mNs.      `yNd/`             -dMm-\n    .yMNs:    `/.`            `/yNNo`\n      .odNNy+-`           .:ohNNd/.\n         -+ymNNmdyyyyyyydmNNmy+.\n             `-//sssssss//.\n";
// Neptune: 18 lines, done at line 8172.
// Logo #130: netbsd_small...
static const char netbsd_small[] = "\\\\\\\\\\`-______,----__\n \\\\\\\\        __,---\\`_\n  \\\\\\\\       \\`.____\n   \\\\\\\\-______,----\\`-\n    \\\\\\\\\n     \\\\\\\\\n      \\\\\\\\\n";
// netbsd_small: 8 lines, done at line 8185.
// Logo #131: NetBSD...
static const char NetBSD[] = "                     `-/oshdmNMNdhyo+:-`\ny/s+:-``    `.-:+oydNMMMMNhs/-``\n-m+NMMMMMMMMMMMMMMMMMMMNdhmNMMMmdhs+/-`\n -m+NMMMMMMMMMMMMMMMMMMMMmy+:`\n  -N/dMMMMMMMMMMMMMMMds:`\n   -N/hMMMMMMMMMmho:`\n    -N/-:/++/:.`\n     :M+\n      :Mo\n       :Ms\n        :Ms\n         :Ms\n          :Ms\n           :Ms\n            :Ms\n             :Ms\n              :Ms\n";
// NetBSD: 18 lines, done at line 8208.
// Logo #132: Netrunner...
static const char Netrunner[] = "           .:oydmMMMMMMmdyo:`\n        -smMMMMMMMMMMMMMMMMMMds-\n      +mMMMMMMMMMMMMMMMMMMMMMMMMd+\n    /mMMMMMMMMMMMMMMMMMMMMMMMMMMMMm/\n  `hMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMy`\n .mMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMd`\n dMMMMMMMMMMMMMMMMMMMMMMNdhmMMMMMMMMMMh\n+MMMMMMMMMMMMMNmhyo+/-.   -MMMMMMMMMMMM/\nmMMMMMMMMd+:.`           `mMMMMMMMMMMMMd\nMMMMMMMMMMMdy/.          yMMMMMMMMMMMMMM\nMMMMMMMMMMMMMMMNh+`     +MMMMMMMMMMMMMMM\nmMMMMMMMMMMMMMMMMMs    -NMMMMMMMMMMMMMMd\n+MMMMMMMMMMMMMMMMMN.  `mMMMMMMMMMMMMMMM/\n dMMMMMMMMMMMMMMMMMy  hMMMMMMMMMMMMMMMh\n `dMMMMMMMMMMMMMMMMM-+MMMMMMMMMMMMMMMd`\n  `hMMMMMMMMMMMMMMMMmMMMMMMMMMMMMMMMy\n    /mMMMMMMMMMMMMMMMMMMMMMMMMMMMMm:\n      +dMMMMMMMMMMMMMMMMMMMMMMMMd/\n        -odMMMMMMMMMMMMMMMMMMdo-\n           `:+ydmNMMMMNmhy+-`\n";
// Netrunner: 21 lines, done at line 8234.
// Logo #133: Nitrux...
static const char Nitrux[] = "`:/.\n`/yo\n`/yo\n`/yo      .+:.\n`/yo      .sys+:.`\n`/yo       `-/sys+:.`\n`/yo           ./sss+:.`\n`/yo              .:oss+:-`\n`/yo                 ./o///:-`\n`/yo              `.-:///////:`\n`/yo           `.://///++//-``\n`/yo       `.-:////++++/-`\n`/yo    `-://///++o+/-`\n`/yo `-/+o+++ooo+/-`\n`/s+:+oooossso/.`\n`//+sssssso:.\n`+syyyy+:`\n:+s+-\n";
// Nitrux: 19 lines, done at line 8258.
// Logo #134: nixos_small...
static const char nixos_small[] = "    \\\\\\\\  \\\\\\\\ //\n ==\\\\\\\\__\\\\\\\\/ //\n   //   \\\\\\\\//\n==//     //==\n //\\\\\\\\___//\n// /\\\\\\\\  \\\\\\\\==\n  // \\\\\\\\  \\\\\\\\\n";
// nixos_small: 8 lines, done at line 8271.
// Logo #135: NixOS...
static const char NixOS[] = "          ::::.    ':::::     ::::'\n          ':::::    ':::::.  ::::'\n            :::::     '::::.:::::\n      .......:::::..... ::::::::\n     ::::::::::::::::::. ::::::    ::::.\n    ::::::::::::::::::::: :::::.  .::::'\n           .....           ::::' :::::'\n          :::::            '::' :::::'\n ........:::::               ' :::::::::::.\n:::::::::::::                 :::::::::::::\n ::::::::::: ..              :::::\n     .::::: .:::            :::::\n    .:::::  :::::          '''''    .....\n    :::::   ':::::.  ......:::::::::::::'\n     :::     ::::::. ':::::::::::::::::'\n            .:::::::: '::::::::::\n           .::::''::::.     '::::.\n          .::::'   ::::.     '::::.\n         .::::      ::::      '::::.\n";
// NixOS: 20 lines, done at line 8296.
// Logo #136: Nurunner...
static const char Nurunner[] = "                  ,xc\n                ;00cxXl\n              ;K0,   .xNo.\n            :KO'       .lXx.\n          cXk.    ;xl     cXk.\n        cXk.    ;k:.,xo.    cXk.\n     .lXx.    :x::0MNl,dd.    :KO,\n   .xNx.    cx;:KMMMMMNo'dx.    ;KK;\n .dNl.    cd,cXMMMMMMMMMWd,ox'    'OK:\n;WK.    'K,.KMMMMMMMMMMMMMWc.Kx     lMO\n 'OK:    'dl'xWMMMMMMMMMM0::x:    'OK:\n   .kNo    .xo'xWMMMMMM0;:O:    ;KK;\n     .dXd.   .do,oNMMO;ck:    ;00,\n        oNd.   .dx,;'cO;    ;K0,\n          oNx.    okk;    ;K0,\n            lXx.        :KO'\n              cKk'    cXk.\n                ;00:lXx.\n                  ,kd.\n";
// Nurunner: 20 lines, done at line 8321.
// Logo #137: NuTyX...
static const char NuTyX[] = "                                      .\n                                    .\n                                 ...\n                               ...\n            ....     .........--.\n       ..-++-----....--++++++---.\n    .-++++++-.   .-++++++++++++-----..\n  .--...  .++..-+++--.....-++++++++++--..\n .     .-+-. .**-            ....  ..-+----..\n     .+++.  .*+.         +            -++-----.\n   .+++++-  ++.         .*+.     .....-+++-----.\n  -+++-++. .+.          .-+***++***++--++++.  .\n -+-. --   -.          -*- ......        ..--.\n.-. .+-    .          -+.\n.  .+-                +.\n   --                 --\n  -+----.              .-\n  -++-.+.                .\n .++. --\n  +.  ----.\n  .  .+. ..\n      -  .\n      .\n";
// NuTyX: 24 lines, done at line 8350.
// Logo #138: OBRevenge...
static const char OBRevenge[] = "   __   __\n     _@@@@   @@@g_\n   _@@@@@@   @@@@@@\n  _@@@@@@M   W@@@@@@_\n j@@@@P        ^W@@@@\n @@@@L____  _____Q@@@@\nQ@@@@@@@@@@j@@@@@@@@@@\n@@@@@    T@j@    T@@@@@\n@@@@@ ___Q@J@    _@@@@@\n@@@@@fMMM@@j@jggg@@@@@@\n@@@@@    j@j@^MW@P @@@@\nQ@@@@@ggg@@f@   @@@@@@L\n^@@@@WWMMP  ^    Q@@@@\n @@@@@_         _@@@@l\n  W@@@@@g_____g@@@@@P\n   @@@@@@@@@@@@@@@@l\n    ^W@@@@@@@@@@@P\n       ^TMMMMTll\n";
// OBRevenge: 19 lines, done at line 8374.
// Logo #139: openbsd_small...
static const char openbsd_small[] = "      _____\n    \\\\-     -/\n \\\\_/         \\\\\n |        O O |\n |_  <   )  3 )\n / \\\\         /\n    /-_____-\\\\\n";
// openbsd_small: 8 lines, done at line 8387.
// Logo #140: OpenBSD...
static const char OpenBSD[] = "                                     _\n                                    (_)\n              |    .\n          .   |L  /|   .          _\n      _ . |\\ _| \\--+._/| .       (_)\n     / ||\\| Y J  )   / |/| ./\n    J  |)'( |        ` F`.'/        _\n  -<|  F         __     .-<        (_)\n    | /       .-'. `.  /-. L___\n    J \\\\      <    \\  | | O\\\\|.-'  _\n  _J \\\\  .-    \\\\/ O | | \\\\  |F    (_)\n '-F  -<_.     \\\\   .-'  `-' L__\n__J  _   _.     >-'  )._.   |-'\n `-|.'   /_.          \\_|   F\n  /.-   .                _.<\n /'    /.'             .'  `\\\\\n  /L  /'   |/      _.-'-\\\\\n /'J       ___.---'\\|\n   |\\  .--' V  | `. `\n   |/`. `-.     `._)\n      / .-.\\\\\n      \\\\ (  `\\\\\n       `.\\\\\n";
// OpenBSD: 24 lines, done at line 8416.
// Logo #141: openEuler...
static const char openEuler[] = "\n                       (#####\n                     (((########  #####\n                    (((        ##########    __...__\n             ((((((((           #######    /((((((###\\\n           (((((((((((   .......           \\(((((####/\n          ((((((    ((((#########            *******\n    %((((((#          ((########\n /////(((((              ###\n/////(((((((#   (((&\n         (((((((((((((\n          ((((((((((((\n           (((((((((     ((((((###\n                       /((((((######\n                      //((((((######\n                       /((((((#####\n                        *********/\n";
// openEuler: 18 lines, done at line 8439.
// Logo #142: OpenIndiana...
static const char OpenIndiana[] = "                         .sy/\n                         .yh+\n\n           -+syyyo+-      /+.\n         +ddo/---/sdh/    ym-\n       `hm+        `sms   ym-```````.-.\n       sm+           sm/  ym-         +s\n       hm.           /mo  ym-         /h\n       omo           ym:  ym-       `os`\n        smo`       .ym+   ym-     .os-\n     ``  :ymy+///oyms-    ym-  .+s+.\n   ..`     `:+oo+/-`      -//oyo-\n -:`                   .:oys/.\n+-               `./oyys/.\nh+`      `.-:+oyyyo/-`\n`/ossssysso+/-.`\n";
// OpenIndiana: 17 lines, done at line 8461.
// Logo #143: openmamba...
static const char openmamba[] = "                 `````\n           .-/+ooooooooo+/:-`\n        ./ooooooooooooooooooo+:.\n      -+oooooooooooooooooooooooo+-\n    .+ooooooooo+/:---::/+ooooooooo+.\n   :oooooooo/-`          `-/oos´oooo.s´\n  :ooooooo/`                `sNdsooosNds\n -ooooooo-                   :dmyooo:dmy\n +oooooo:                      :oooooo-\n.ooooooo                        .://:`\n:oooooo+                        ./+o+:`\n-ooooooo`                      `oooooo+\n`ooooooo:                      /oooooo+\n -ooooooo:                    :ooooooo.\n  :ooooooo+.                .+ooooooo:\n   :oooooooo+-`          `-+oooooooo:\n    .+ooooooooo+/::::://oooooooooo+.\n      -+oooooooooooooooooooooooo+-\n        .:ooooooooooooooooooo+:.\n           `-:/ooooooooo+/:.`\n                 ``````\n";
// openmamba: 22 lines, done at line 8488.
// Logo #144: OpenMandriva...
static const char OpenMandriva[] = "                  ``````\n            `-:/+++++++//:-.`\n         .:+++oooo+/:.``   ``\n      `:+ooooooo+:.  `-:/++++++/:.`\n     -+oooooooo:` `-++o+/::::://+o+/-\n   `/ooooooooo-  -+oo/.`        `-/oo+.\n  `+ooooooooo.  :os/`              .+so:\n  +sssssssss/  :ss/                 `+ss-\n :ssssssssss`  sss`                  .sso\n ossssssssss  `yyo                    sys\n`sssssssssss` `yys                   `yys\n`sssssssssss:  +yy/                  +yy:\n oyyyyyyyyyys. `oyy/`              `+yy+\n :yyyyyyyyyyyo. `+yhs:.         `./shy/\n  oyyyyyyyyyyys:` .oyhys+:----/+syhy+. `\n  `syyyyyyyyyyyyo-` .:osyhhhhhyys+:``.:`\n   `oyyyyyyyyyyyyys+-`` `.----.```./oo.\n     /yhhhhhhhhhhhhhhyso+//://+osyhy/`\n      `/yhhhhhhhhhhhhhhhhhhhhhhhhy/`\n        `:oyhhhhhhhhhhhhhhhhhhyo:`\n            .:+syhhhhhhhhys+:-`\n                 ``....``\n";
// OpenMandriva: 23 lines, done at line 8516.
// Logo #145: OpenStage...
static const char OpenStage[] = "                 /(/\n              .(((((((,\n             /(((((((((/\n           .(((((/,/(((((,\n          *(((((*   ,(((((/\n          (((((*      .*/((\n         *((((/  (//(/*\n         /((((*  ((((((((((,\n      .  /((((*  (((((((((((((.\n     ((. *((((/        ,((((((((\n   ,(((/  (((((/     **   ,((((((*\n  /(((((. .(((((/   //(((*  *(((((/\n .(((((,    ((/   .(((((/.   .(((((,\n /((((*        ,(((((((/      ,(((((\n /(((((((((((((((((((/.  /(((((((((/\n /(((((((((((((((((,   /(((((((((((/\n     */(((((//*.      */((/(/(/*\n";
// OpenStage: 18 lines, done at line 8539.
// Logo #146: OpenWrt...
static const char OpenWrt[] = " _______\n|       |.-----.-----.-----.\n|   -   ||  _  |  -__|     |\n|_______||   __|_____|__|__|\n         |__|\n ________        __\n|  |  |  |.----.|  |_\n|  |  |  ||   _||   _|\n|________||__|  |____|\n";
// OpenWrt: 10 lines, done at line 8554.
// Logo #147: osmc...
static const char osmc[] = "            -+shdmNNNNmdhs+-\n        .+hMNho/:..``..:/ohNMh+.\n      :hMdo.                .odMh:\n    -dMy-                      -yMd-\n   sMd-                          -dMs\n  hMy       +.            .+       yMh\n yMy        dMs.        .sMd        yMy\n:Mm         dMNMs`    `sMNMd        `mM:\nyM+         dM//mNs``sNm//Md         +My\nmM-         dM:  +NNNN+  :Md         -Mm\nmM-         dM: `oNN+    :Md         -Mm\nyM+         dM/+NNo`     :Md         +My\n:Mm`        dMMNs`       :Md        `mM:\n yMy        dMs`         -ms        yMy\n  hMy       +.                     yMh\n   sMd-                          -dMs\n    -dMy-                      -yMd-\n      :hMdo.                .odMh:\n        .+hMNho/:..``..:/ohNMh+.\n            -+shdmNNNNmdhs+-\n";
// osmc: 21 lines, done at line 8580.
// Logo #148: Oracle...
static const char Oracle[] = "\n      `-/+++++++++++++++++/-.`\n   `/syyyyyyyyyyyyyyyyyyyyyyys/.\n  :yyyyo/-...............-/oyyyy/\n /yyys-                     .oyyy+\n.yyyy`                       `syyy-\n:yyyo                         /yyy/\n.yyyy`                       `syyy-\n /yyys.                     .oyyyo\n  /yyyyo:-...............-:oyyyy/`\n   `/syyyyyyyyyyyyyyyyyyyyyyys+.\n     `.:/+ooooooooooooooo+/:.`\n";
// Oracle: 13 lines, done at line 8598.
// Logo #149: OS Elbrus...
static const char OS_Elbrus[] = "   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄\n   ██▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀██\n   ██                       ██\n   ██   ███████   ███████   ██\n   ██   ██   ██   ██   ██   ██\n   ██   ██   ██   ██   ██   ██\n   ██   ██   ██   ██   ██   ██\n   ██   ██   ██   ██   ██   ██\n   ██   ██   ███████   ███████\n   ██   ██                  ██\n   ██   ██▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄██\n   ██   ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀██\n   ██                       ██\n   ███████████████████████████\n";
// OS_Elbrus: 15 lines, done at line 8618.
// Logo #150: PacBSD...
static const char PacBSD[] = "      :+sMs.\n  `:ddNMd-                         -o--`\n -sMMMMh:                          `+N+``\n yMMMMMs`     .....-/-...           `mNh/\n yMMMMMmh+-`:sdmmmmmmMmmmmddy+-``./ddNMMm\n yNMMNMMMMNdyyNNMMMMMMMMMMMMMMMhyshNmMMMm\n :yMMMMMMMMMNdooNMMMMMMMMMMMMMMMMNmy:mMMd\n  +MMMMMMMMMmy:sNMMMMMMMMMMMMMMMMMMMmshs-\n  :hNMMMMMMN+-+MMMMMMMMMMMMMMMMMMMMMMMs.\n .omysmNNhy/+yNMMMMMMMMMMNMMMMMMMMMNdNNy-\n /hMM:::::/hNMMMMMMMMMMMm/-yNMMMMMMN.mMNh`\n.hMMMMdhdMMMMMMMMMMMMMMmo  `sMMMMMMN mMMm-\n:dMMMMMMMMMMMMMMMMMMMMMdo+  oMMMMMMN`smMNo`\n/dMMMMMMMMMMMMMMMMMMMMMNd/` :yMMMMMN:-hMMM.\n:dMMMMMMMMMMMMMMMMMMMMMNh`  oMMMMMMNo/dMNN`\n:hMMMMMMMMMMMMMMMMMMMMMMNs--sMMMMMMMNNmy++`\n sNMMMMMMMMMMMMMMMMMMMMMMMmmNMMMMMMNho::o.\n :yMMMMMMMMMMMMMNho+sydNNNNNNNmysso/` -//\n  /dMMMMMMMMMMMMMs-  ````````..``\n   .oMMMMMMMMMMMMNs`               ./y:`\n     +dNMMNMMMMMMMmy`          ``./ys.\n      `/hMMMMMMMMMMMNo-``    `.+yy+-`\n        `-/hmNMNMMMMMMmmddddhhy/-`\n            `-+oooyMMMdsoo+/:.\n";
// PacBSD: 25 lines, done at line 8648.
// Logo #151: parabola_small...
static const char parabola_small[] = "  __ __ __  _\n.`_//_//_/ / `.\n          /  .`\n         / .`\n        /.`\n       /`\n";
// parabola_small: 7 lines, done at line 8660.
// Logo #152: Parabola...
static const char Parabola[] = "                          `.-.    `.\n                   `.`  `:++.   `-+o+.\n             `` `:+/. `:+/.   `-+oooo+\n        ``-::-.:+/. `:+/.   `-+oooooo+\n    `.-:///-  ..`   .-.   `-+oooooooo-\n `..-..`                 `+ooooooooo:\n``                        :oooooooo/\n                          `ooooooo:\n                          `oooooo:\n                          -oooo+.\n                          +ooo/`\n                         -ooo-\n                        `+o/.\n                        /+-\n                       //`\n                      -.\n";
// Parabola: 17 lines, done at line 8682.
// Logo #153: Pardus...
static const char Pardus[] = " .smNdy+-    `.:/osyyso+:.`    -+ydmNs.\n/Md- -/ymMdmNNdhso/::/oshdNNmdMmy/. :dM/\nmN.     oMdyy- -y          `-dMo     .Nm\n.mN+`  sMy hN+ -:             yMs  `+Nm.\n `yMMddMs.dy `+`               sMddMMy`\n   +MMMo  .`  .                 oMMM+\n   `NM/    `````.`    `.`````    +MN`\n   yM+   `.-:yhomy    ymohy:-.`   +My\n   yM:          yo    oy          :My\n   +Ms         .N`    `N.      +h sM+\n   `MN      -   -::::::-   : :o:+`NM`\n    yM/    sh   -dMMMMd-   ho  +y+My\n    .dNhsohMh-//: /mm/ ://-yMyoshNd`\n      `-ommNMm+:/. oo ./:+mMNmmo:`\n     `/o+.-somNh- :yy: -hNmos-.+o/`\n    ./` .s/`s+sMdd+``+ddMs+s`/s. `/.\n        : -y.  -hNmddmNy.  .y- :\n         -+       `..`       +-\n";
// Pardus: 19 lines, done at line 8706.
// Logo #154: Parrot...
static const char Parrot[] = "  `:oho/-`\n`mMMMMMMMMMMMNmmdhy-\n dMMMMMMMMMMMMMMMMMMs`\n +MMsohNMMMMMMMMMMMMMm/\n .My   .+dMMMMMMMMMMMMMh.\n  +       :NMMMMMMMMMMMMNo\n           `yMMMMMMMMMMMMMm:\n             /NMMMMMMMMMMMMMy`\n              .hMMMMMMMMMMMMMN+\n                  ``-NMMMMMMMMMd-\n                     /MMMMMMMMMMMs`\n                      mMMMMMMMsyNMN/\n                      +MMMMMMMo  :sNh.\n                      `NMMMMMMm     -o/\n                       oMMMMMMM.\n                       `NMMMMMM+\n                        +MMd/NMh\n                         mMm -mN`\n                         /MM  `h:\n                          dM`   .\n                          :M-\n                           d:\n                           -+\n                            -\n";
// Parrot: 25 lines, done at line 8736.
// Logo #155: Parsix...
static const char Parsix[] = "                 -/+/:.\n               .syssssys.\n       .--.    ssssssssso   ..--.\n     :++++++:  +ssssssss+ ./++/+++:\n    /+++++++++..yssooooy`-+///////o-\n    /++++++++++.+soooos::+////////+-\n     :+++++////o-oooooo-+/////////-\n      `-/++//++-.-----.-:+/////:-\n  -://::---:/:.--.````.--.:::---::::::.\n-/:::::::://:.:-`      `-:`:/:::::::--/-\n/::::::::::/---.        .-.-/://///::::/\n-/:::::::::/:`:-.      .-:`:///////////-\n `-::::--.-://.---....---`:+/:---::::-`\n       -/+///+o/-.----..:oo+++o+.\n     -+/////+++o:syyyyy.o+++++++++:\n    .+////+++++-+sssssy+.++++++++++\\\n    .+:/++++++..yssssssy-`+++++++++:\n     :/+++++-  +sssssssss  -++++++-\n       `--`    +sssssssso    `--`\n                +sssssy+`\n                 `.::-`\n";
// Parsix: 22 lines, done at line 8763.
// Logo #156: TrueOS...
static const char TrueOS[] = "                       ..\n                        s.\n                        +y\n                        yN\n                       -MN  `.\n                      :NMs `m\n                    .yMMm` `No\n            `-/+++sdMMMNs+-`+Ms\n        `:oo+-` .yMMMMy` `-+oNMh\n      -oo-     +NMMMM/       oMMh-\n    .s+` `    oMMMMM/     -  oMMMhy.\n   +s`- ::   :MMMMMd     -o `mMMMy`s+\n  y+  h .Ny+oNMMMMMN/    sh+NMMMMo  +y\n s+ .ds  -NMMMMMMMMMMNdhdNMMMMMMh`   +s\n-h .NM`   `hMMMMMMMMMMMMMMNMMNy:      h-\ny- hMN`     hMMmMMMMMMMMMNsdMNs.      -y\nm` mMMy`    oMMNoNMMMMMMo`  sMMMo     `m\nm` :NMMMdyydMMMMo+MdMMMs     sMMMd`   `m\nh-  `+ymMMMMMMMM--M+hMMN/    +MMMMy   -h\n:y     `.sMMMMM/ oMM+.yMMNddNMMMMMm   y:\n y:   `s  dMMN- .MMMM/ :MMMMMMMMMMh  :y\n `h:  `mdmMMM/  yMMMMs  sMMMMMMMMN- :h`\n   so  -NMMMN   /mmd+  `dMMMMMMMm- os\n    :y: `yMMM`       `+NMMMMMMNo`:y:\n      /s+`.omy      /NMMMMMNh/.+s:\n        .+oo:-.     /mdhs+::oo+.\n            -/o+++++++++++/-\n";
// TrueOS: 28 lines, done at line 8796.
// Logo #157: PCLinuxOS...
static const char PCLinuxOS[] = "            mhhhyyyyhhhdN\n        dyssyhhhhhhhhhhhssyhN\n     Nysyhhyo/:-.....-/oyhhhssd\n   Nsshhy+.              `/shhysm\n  dohhy/                    -shhsy\n dohhs`                       /hhys\nN+hho   +ssssss+-   .+syhys+   /hhsy\nohhh`   ymmo++hmm+`smmy/::+y`   shh+\n+hho    ymm-  /mmy+mms          :hhod\n/hh+    ymmhhdmmh.smm/          .hhsh\n+hhs    ymm+::-`  /mmy`    `    /hh+m\nyyhh-   ymm-       /dmdyosyd`  `yhh+\n ohhy`  ://`         -/+++/-   ohhom\n N+hhy-                      `shhoh\n   sshho.                  `+hhyom\n    dsyhhs/.            `:ohhhoy\n      dysyhhhso///://+syhhhssh\n         dhyssyhhhhhhyssyyhN\n              mddhdhdmN\n";
// PCLinuxOS: 20 lines, done at line 8821.
// Logo #158: Peppermint...
static const char Peppermint[] = "               PPPPPPPPPPPPPP\n           PPPPMMMMMMMPPPPPPPPPPP\n         PPPPMMMMMMMMMMPPPPPPPPMMPP\n       PPPPPPPPMMMMMMMPPPPPPPPMMMMMPP\n     PPPPPPPPPPPPMMMMMMPPPPPPPMMMMMMMPP\n    PPPPPPPPPPPPMMMMMMMPPPPMPMMMMMMMMMPP\n   PPMMMMPPPPPPPPPPMMMPPPPPMMMMMMMPMMPPPP\n   PMMMMMMMMMMPPPPPPMMPPPPPMMMMMMPPPPPPPP\n  PMMMMMMMMMMMMPPPPPMMPPMPMMPMMPPPPPPPPPPP\n  PMMMMMMMMMMMMMMMMPPMPMMMPPPPPPPPPPPPPPPP\n  PMMMPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPMMMMMP\n  PPPPPPPPPPPPPPPPMMMPMPMMMMMMMMMMMMMMMMPP\n  PPPPPPPPPPPMMPMMPPPPMMPPPPPMMMMMMMMMMMPP\n   PPPPPPPPMMMMMMPPPPPMMPPPPPPMMMMMMMMMPP\n   PPPPMMPMMMMMMMPPPPPPMMPPPPPPPPPPMMMMPP\n    PPMMMMMMMMMPMPPPPMMMMMMPPPPPPPPPPPPP\n     PPMMMMMMMPPPPPPPMMMMMMPPPPPPPPPPPP\n       PPMMMMPPPPPPPPPMMMMMMMPPPPPPPP\n         PPMMPPPPPPPPMMMMMMMMMMPPPP\n           PPPPPPPPPPMMMMMMMMPPPP\n               PPPPPPPPPPPPPP\n";
// Peppermint: 22 lines, done at line 8848.
// Logo #159: pop_os_small...
static const char pop_os_small[] = "______\n\\\\   _ \\\\        __\n \\\\ \\\\ \\\\ \\\\      / /\n  \\\\ \\\\_\\\\ \\\\    / /\n   \\\\  ___\\\\  /_/\n    \\\\ \\\\    _\n   __\\\\_\\\\__(_)_\n  (___________)`\n";
// pop_os_small: 9 lines, done at line 8862.
// Logo #160: pop_os...
static const char pop_os[] = "             /////////////\n         /////////////////////\n      ///////*767////////////////\n    //////7676767676*//////////////\n   /////76767//7676767//////////////\n  /////767676///*76767///////////////\n ///////767676///76767.///7676*///////\n/////////767676//76767///767676////////\n//////////76767676767////76767/////////\n///////////76767676//////7676//////////\n////////////,7676,///////767///////////\n/////////////*7676///////76////////////\n///////////////7676////////////////////\n ///////////////7676///767////////////\n  //////////////////////'////////////\n   //////.7676767676767676767,//////\n    /////767676767676767676767/////\n      ///////////////////////////\n         /////////////////////\n             /////////////\n";
// pop_os: 21 lines, done at line 8888.
// Logo #161: Porteus...
static const char Porteus[] = "             `.-:::-.`\n         -+ydmNNNNNNNmdy+-\n      .+dNmdhs+//////+shdmdo.\n    .smmy+-`             ./sdy:\n  `omdo.    `.-/+osssso+/-` `+dy.\n `yms.   `:shmNmdhsoo++osyyo-``oh.\n hm/   .odNmds/.`    ``.....:::-+s\n/m:  `+dNmy:`   `./oyhhhhyyooo++so\nys  `yNmy-    .+hmmho:-.`     ```\ns:  yNm+`   .smNd+.\n`` /Nm:    +dNd+`\n   yN+   `smNy.\n   dm    oNNy`\n   hy   -mNm.\n   +y   oNNo\n   `y`  sNN:\n    `:  +NN:\n     `  .mNo\n         /mm`\n          /my`\n           .sy`\n             .+:\n                `\n";
// Porteus: 24 lines, done at line 8917.
// Logo #162: postmarketos_small...
static const char postmarketos_small[] = "        /\\\\\n       /  \\\\\n      /    \\\\\n      \\\\__   \\\\\n    /\\\\__ \\\\  _\\\\\n   /   /  \\\\/ __\n  /   / ____/  \\\\\n /    \\\\ \\\\       \\\\\n/_____/ /________\\\\\n";
// postmarketos_small: 10 lines, done at line 8932.
// Logo #163: PostMarketOS...
static const char PostMarketOS[] = "                 /\\\\\n                /  \\\\\n               /    \\\\\n              /      \\\\\n             /        \\\\\n            /          \\\\\n            \\\\           \\\\\n          /\\\\ \\\\____       \\\\\n         /  \\\\____ \\\\       \\\\\n        /       /  \\\\       \\\\\n       /       /    \\\\    ___\\\\\n      /       /      \\\\  / ____\n     /       /        \\\\/ /    \\\\\n    /       / __________/      \\\\\n   /        \\\\ \\\\                 \\\\\n  /          \\\\ \\\\                 \\\\\n /           / /                  \\\\\n/___________/ /____________________\\\\\n";
// PostMarketOS: 19 lines, done at line 8956.
// Logo #164: Proxmox...
static const char Proxmox[] = "         .://:`              `://:.\n       `hMMMMMMd/          /dMMMMMMh`\n        `sMMMMMMMd:      :mMMMMMMMs`\n`-/+oo+/:`.yMMMMMMMh-  -hMMMMMMMy.`:/+oo+/-`\n`:oooooooo/`-hMMMMMMMyyMMMMMMMh-`/oooooooo:`\n  `/oooooooo:`:mMMMMMMMMMMMMm:`:oooooooo/`\n    ./ooooooo+- +NMMMMMMMMN+ -+ooooooo/.\n      .+ooooooo+-`oNMMMMNo`-+ooooooo+.\n        -+ooooooo/.`sMMs`./ooooooo+-\n          :oooooooo/`..`/oooooooo:\n          :oooooooo/`..`/oooooooo:\n        -+ooooooo/.`sMMs`./ooooooo+-\n      .+ooooooo+-`oNMMMMNo`-+ooooooo+.\n    ./ooooooo+- +NMMMMMMMMN+ -+ooooooo/.\n  `/oooooooo:`:mMMMMMMMMMMMMm:`:oooooooo/`\n`:oooooooo/`-hMMMMMMMyyMMMMMMMh-`/oooooooo:`\n`-/+oo+/:`.yMMMMMMMh-  -hMMMMMMMy.`:/+oo+/-`\n        `sMMMMMMMm:      :dMMMMMMMs`\n       `hMMMMMMd/          /dMMMMMMh`\n         `://:`              `://:`\n";
// Proxmox: 21 lines, done at line 8982.
// Logo #165: Precise Puppy...
static const char Precise_Puppy[] = "           `-/osyyyysosyhhhhhyys+-\n  -ohmNNmh+/hMMMMMMMMNNNNd+dMMMMNM+\n yMMMMNNmmddo/NMMMNNNNNNNNNo+NNNNNy\n.NNNNNNmmmddds:MMNNNNNNNNNNNh:mNNN/\n-NNNdyyyhdmmmd`dNNNNNmmmmNNmdd/os/\n.Nm+shddyooo+/smNNNNmmmmNh.   :mmd.\n NNNNy:`   ./hmmmmmmmNNNN:     hNMh\n NMN-    -++- +NNNNNNNNNNm+..-sMMMM-\n.MMo    oNNNNo hNNNNNNNNmhdNNNMMMMM+\n.MMs    /NNNN/ dNmhs+:-`  yMMMMMMMM+\n mMM+     .. `sNN+.      hMMMMhhMMM-\n +MMMmo:...:sNMMMMMms:` hMMMMm.hMMy\n  yMMMMMMMMMMMNdMMMMMM::/+o+//dMMd`\n   sMMMMMMMMMMN+:oyyo:sMMMNNMMMNy`\n    :mMMMMMMMMMMMmddNMMMMMMMMmh/\n      /dMMMMMMMMMMMMMMMMMMNdy/`\n        .+hNMMMMMMMMMNmdhs/.\n            .:/+ooo+/:-.\n";
// Precise_Puppy: 19 lines, done at line 9006.
// Logo #166: pureos_small...
static const char pureos_small[] = " _____________\n|  _________  |\n| |         | |\n| |         | |\n| |_________| |\n|_____________|\n";
// pureos_small: 7 lines, done at line 9018.
// Logo #167: PureOS...
static const char PureOS[] = "dmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmd\ndNm//////////////////////////////////mNd\ndNd                                  dNd\ndNd                                  dNd\ndNd                                  dNd\ndNd                                  dNd\ndNd                                  dNd\ndNd                                  dNd\ndNd                                  dNd\ndNd                                  dNd\ndNm//////////////////////////////////mNd\ndmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmd\n";
// PureOS: 13 lines, done at line 9036.
// Logo #168: Qubes...
static const char Qubes[] = "               `..--..`\n            `.----------.`\n        `..----------------..`\n     `.------------------------.``\n `..-------------....-------------..`\n.::----------..``    ``..----------:+:\n:////:----..`            `..---:/ossso\n:///////:`                  `/osssssso\n:///////:                    /ssssssso\n:///////:                    /ssssssso\n:///////:                    /ssssssso\n:///////:                    /ssssssso\n:///////:                    /ssssssso\n:////////-`                .:sssssssso\n:///////////-.`        `-/osssssssssso\n`//////////////:-```.:+ssssssssssssso-\n  .-://////////////sssssssssssssso/-`\n     `.:///////////sssssssssssssso:.\n         .-:///////ssssssssssssssssss/`\n            `.:////ssss+/+ssssssssssss.\n                `--//-    `-/osssso/.\n";
// Qubes: 22 lines, done at line 9063.
// Logo #169: Radix...
static const char Radix[] = "                .:oyhdmNo\n             `/yhyoosdms`\n            -o+/ohmmho-\n           ..`.:/:-`\n     `.--:::-.``\n  .+ydNMMMMMMNmhs:`\n`omMMMMMMMMMMMMMMNh-\noNMMMNmddhhyyhhhddmy.\nmMMMMNmmddhhysoo+/:-`\nyMMMMMMMMMMMMMMMMNNh.\n-dmmmmmNNMMMMMMMMMMs`\n -+oossyhmMMMMMMMMd-\n `sNMMMMMMMMMMMMMm:\n  `yMMMMMMNmdhhhh:\n   `sNMMMMMNmmho.\n    `+mMMMMMMMy.\n      .yNMMMm+`\n       `:yd+.\n";
// Radix: 19 lines, done at line 9087.
// Logo #170: Raspbian_small...
static const char Raspbian_small[] = "   .~~.   .~~.\n  '. \\\\ ' ' / .'\n   .~ .~~~..~.\n  : .~.'~'.~. :\n ~ (   ) (   ) ~\n( : '~'.~.'~' : )\n ~ .~ (   ) ~. ~\n  (  : '~' :  )\n   '~ .~~~. ~'\n       '~'\n";
// Raspbian_small: 11 lines, done at line 9103.
// Logo #171: Raspbian...
static const char Raspbian[] = "  `.::///+:/-.        --///+//-:``\n `+oooooooooooo:   `+oooooooooooo:\n  /oooo++//ooooo:  ooooo+//+ooooo.\n  `+ooooooo:-:oo-  +o+::/ooooooo:\n   `:oooooooo+``    `.oooooooo+-\n     `:++ooo/.        :+ooo+/.`\n        ...`  `.----.` ``..\n     .::::-``:::::::::.`-:::-`\n    -:::-`   .:::::::-`  `-:::-\n   `::.  `.--.`  `` `.---.``.::`\n       .::::::::`  -::::::::` `\n .::` .:::::::::- `::::::::::``::.\n-:::` ::::::::::.  ::::::::::.`:::-\n::::  -::::::::.   `-::::::::  ::::\n-::-   .-:::-.``....``.-::-.   -::-\n .. ``       .::::::::.     `..`..\n   -:::-`   -::::::::::`  .:::::`\n   :::::::` -::::::::::` :::::::.\n   .:::::::  -::::::::. ::::::::\n    `-:::::`   ..--.`   ::::::.\n      `...`  `...--..`  `...`\n            .::::::::::\n             `.-::::-`\n";
// Raspbian: 24 lines, done at line 9132.
// Logo #172: Reborn...
static const char Reborn[] = "\n        mMMMMMMMMM  MMMMMMMMMm\n       NM                    MN\n      MM  dddddddd  dddddddd  MN\n     mM  dd                dd  MM\n        dd  hhhhhh   hhhhh  dd\n   mM      hh            hh      Mm\n  NM  hd       mMMMMMMd       dh  MN\n NM  dd  hh   mMMMMMMMMm   hh  dd  MN\nNM  dd  hh   mMMMMMMMMMMm   hh  dd  MN\n NM  dd  hh   mMMMMMMMMm   hh  dd  MN\n  NM  hd       mMMMMMMm       dh  MN\n   mM      hh            hh      Mm\n        dd  hhhhhh  hhhhhh  dd\n     MM  dd                dd  MM\n      MM  dddddddd  dddddddd  MN\n       NM                    MN\n        mMMMMMMMMM  MMMMMMMMMm\n";
// Reborn: 19 lines, done at line 9156.
// Logo #173: Redstar...
static const char Redstar[] = "                    ..\n                  .oK0l\n                 :0KKKKd.\n               .xKO0KKKKd\n              ,Od' .d0000l\n             .c;.   .'''...           ..'.\n.,:cloddxxxkkkkOOOOkkkkkkkkxxxxxxxxxkkkx:\n;kOOOOOOOkxOkc'...',;;;;,,,'',;;:cllc:,.\n .okkkkd,.lko  .......',;:cllc:;,,'''''.\n   .cdo. :xd' cd:.  ..';'',,,'',,;;;,'.\n      . .ddl.;doooc'..;oc;'..';::;,'.\n        coo;.oooolllllllcccc:'.  .\n       .ool''lllllccccccc:::::;.\n       ;lll. .':cccc:::::::;;;;'\n       :lcc:'',..';::::;;;;;;;,,.\n       :cccc::::;...';;;;;,,,,,,.\n       ,::::::;;;,'.  ..',,,,'''.\n        ........          ......\n";
// Redstar: 19 lines, done at line 9180.
// Logo #174: Redcore...
static const char Redcore[] = "                 RRRRRRRRR\n               RRRRRRRRRRRRR\n        RRRRRRRRRR      RRRRR\n   RRRRRRRRRRRRRRRRRRRRRRRRRRR\n RRRRRRR  RRR         RRR RRRRRRRR\nRRRRR    RR                 RRRRRRRRR\nRRRR    RR     RRRRRRRR      RR RRRRRR\nRRRR   R    RRRRRRRRRRRRRR   RR   RRRRR\nRRRR   R  RRRRRRRRRRRRRRRRRR  R   RRRRR\nRRRR     RRRRRRRRRRRRRRRRRRR  R   RRRR\n RRR     RRRRRRRRRRRRRRRRRRRR R   RRRR\n  RRR    RRRRRRRRRRRRRRRRRRRR    RRRR\n    RR   RRRRRRRRRRRRRRRRRRR    RRR\n     RR   RRRRRRRRRRRRRRRRR    RRR\n       RR   RRRRRRRRRRRRRR   RR\n         R       RRRR      RR\n";
// Redcore: 17 lines, done at line 9202.
// Logo #175: rhel_old...
static const char rhel_old[] = "             `.-..........`\n            `////////::.`-/.\n            -: ....-////////.\n            //:-::///////////`\n     `--::: `-://////////////:\n     //////-    ``.-:///////// .`\n     `://////:-.`    :///////::///:`\n       .-/////////:---/////////////:\n          .-://////////////////////.\n         yMN+`.-::///////////////-`\n      .-`:NMMNMs`  `..-------..`\n       MN+/mMMMMMhoooyysshsss\nMMM    MMMMMMMMMMMMMMyyddMMM+\n MMMM   MMMMMMMMMMMMMNdyNMMh`     hyhMMM\n  MMMMMMMMMMMMMMMMyoNNNMMM+.   MMMMMMMM\n   MMNMMMNNMMMMMNM+ mhsMNyyyyMNMMMMsMM\n";
// rhel_old: 17 lines, done at line 9224.
// Logo #176: rhel...
static const char rhel[] = "           .MMM..:MMMMMMM\n          MMMMMMMMMMMMMMMMMM\n          MMMMMMMMMMMMMMMMMMMM.\n         MMMMMMMMMMMMMMMMMMMMMM\n        ,MMMMMMMMMMMMMMMMMMMMMM:\n        MMMMMMMMMMMMMMMMMMMMMMMM\n  .MMMM'  MMMMMMMMMMMMMMMMMMMMMM\n MMMMMM    `MMMMMMMMMMMMMMMMMMMM.\nMMMMMMMM      MMMMMMMMMMMMMMMMMM .\nMMMMMMMMM.       `MMMMMMMMMMMMM' MM.\nMMMMMMMMMMM.                     MMMM\n`MMMMMMMMMMMMM.                 ,MMMMM.\n `MMMMMMMMMMMMMMMMM.          ,MMMMMMMM.\n    MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\n      MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM:\n         MMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\n            `MMMMMMMMMMMMMMMMMMMMMMMM:\n                ``MMMMMMMMMMMMMMMMM'\n";
// rhel: 19 lines, done at line 9248.
// Logo #177: Refracted_Devuan...
static const char Refracted_Devuan[] = "                             A\n                            VW\n                           VVW\\\\\n                         .yWWW\\\\\n ,;,,u,;yy;;v;uyyyyyyy  ,WWWWW^\n    *WWWWWWWWWWWWWWWW/  $VWWWWw      ,\n        ^*%WWWWWWVWWX  $WWWW**    ,yy\n        ,    \"**WWW/' **'   ,yy/WWW*`\n       &WWWWwy    `*`  <,ywWW%VWWW*\n     yWWWWWWWWWW*    .,   \"**WW%W\n   ,&WWWWWM*\"`  ,y/  &WWWww   ^*\n  XWWX*^   ,yWWWW09 .WWWWWWWWwy,\n *`        &WWWWWM  WWWWWWWWWWWWWww,\n           (WWWWW` /#####WWW***********\n           ^WWWW\n            VWW\n            Wh.\n            V/\n";
// Refracted_Devuan: 19 lines, done at line 9272.
// Logo #178: Regata...
static const char Regata[] = "            ddhso+++++osydd\n        dho/.`hh.:/+/:.hhh`:+yd\n      do-hhhhhh/sssssss+`hhhhh./yd\n    h/`hhhhhhh-sssssssss:hhhhhhhh-yd\n  do`hhhhhhhhh`ossssssso.hhhhhhhhhh/d\n d/hhhhhhhhhhhh`/ossso/.hhhhhhhhhhhh.h\n /hhhhhhhhhhhh`-/osyso/-`hhhhhhhhhhhh.h\nshh-/ooo+-hhh:syyso+osyys/`hhh`+oo`hhh/\nh`ohhhhhhho`+yyo.hhhhh.+yyo`.sssssss.h`h\ns:hhhhhhhhhoyys`hhhhhhh.oyy/ossssssso-hs\ns.yhhhhhhhy/yys`hhhhhhh.oyy/ossssssso-hs\nhh./syyys+. +yy+.hhhhh.+yyo`.ossssso/h`h\nshhh``.`hhh`/syyso++oyys/`hhh`+++-`hh:h\nd/hhhhhhhhhhhh`-/osyso+-`hhhhhhhhhhhh.h\n d/hhhhhhhhhhhh`/ossso/.hhhhhhhhhhhh.h\n  do`hhhhhhhhh`ossssssso.hhhhhhhhhh:h\n    h/`hhhhhhh-sssssssss:hhhhhhhh-yd\n      h+.hhhhhh+sssssss+hhhhhh`/yd\n        dho:.hhh.:+++/.hhh`-+yd\n            ddhso+++++osyhd\n";
// Regata: 21 lines, done at line 9298.
// Logo #179: Regolith...
static const char Regolith[] = "\n                 ``....```\n            `.:/++++++/::-.`\n          -/+++++++:.`\n        -++++++++:`\n      `/++++++++-\n     `/++++++++.                    -/+/\n     /++++++++/             ``   .:+++:.\n    -+++++++++/          ./++++:+++/-`\n    :+++++++++/         `+++++++/-`\n    :++++++++++`      .-/+++++++`\n   `:++++++++++/``.-/++++:-:::-`      `\n `:+++++++++++++++++/:.`            ./`\n:++/-:+++++++++/:-..              -/+.\n+++++++++/::-...:/+++/-..````..-/+++.\n`......``.::/+++++++++++++++++++++/.\n         -/+++++++++++++++++++++/.\n           .:/+++++++++++++++/-`\n              `.-:://////:-.\n";
// Regolith: 20 lines, done at line 9323.
// Logo #180: Rosa...
static const char Rosa[] = "           ROSAROSAROSAROSAR\n        ROSA               AROS\n      ROS   SAROSAROSAROSAR   AROS\n    RO   ROSAROSAROSAROSAROSAR   RO\n  ARO  AROSAROSAROSARO      AROS  ROS\n ARO  ROSAROS         OSAR   ROSA  ROS\n RO  AROSA   ROSAROSAROSA    ROSAR  RO\nRO  ROSAR  ROSAROSAROSAR  R  ROSARO  RO\nRO  ROSA  AROSAROSAROSA  AR  ROSARO  AR\nRO AROS  ROSAROSAROSA   ROS  AROSARO AR\nRO AROS  ROSAROSARO   ROSARO  ROSARO AR\nRO  ROS  AROSAROS   ROSAROSA AROSAR  AR\nRO  ROSA  ROS     ROSAROSAR  ROSARO  RO\n RO  ROS     AROSAROSAROSA  ROSARO  AR\n ARO  ROSA   ROSAROSAROS   AROSAR  ARO\n  ARO  OROSA      R      ROSAROS  ROS\n    RO   AROSAROS   AROSAROSAR   RO\n     AROS   AROSAROSAROSARO   AROS\n        ROSA               SARO\n           ROSAROSAROSAROSAR\n";
// Rosa: 21 lines, done at line 9349.
// Logo #181: sabotage...
static const char sabotage[] = " .|'''.|      |     '||''|.    ..|''||\n ||..  '     |||     ||   ||  .|'    ||\n  ''|||.    |  ||    ||'''|.  ||      ||\n.     '||  .''''|.   ||    || '|.     ||\n|'....|'  .|.  .||. .||...|'   ''|...|'\n\n|''||''|     |      ..|'''.|  '||''''|\n   ||       |||    .|'     '   ||  .\n   ||      |  ||   ||    ....  ||''|\n   ||     .''''|.  '|.    ||   ||\n  .||.   .|.  .||.  ''|...'|  .||.....|\n";
// sabotage: 12 lines, done at line 9366.
// Logo #182: Sabayon...
static const char Sabayon[] = "            ...........\n         ..             ..\n      ..                   ..\n    ..           o           ..\n  ..            :W'            ..\n ..             .d.             ..\n:.             .KNO              .:\n:.             cNNN.             .:\n:              dXXX,              :\n:   .          dXXX,       .cd,   :\n:   'kc ..     dKKK.    ,ll;:'    :\n:     .xkkxc;..dkkkc',cxkkl       :\n:.     .,cdddddddddddddo:.       .:\n ..         :lllllll:           ..\n   ..         ',,,,,          ..\n     ..                     ..\n        ..               ..\n          ...............\n";
// Sabayon: 19 lines, done at line 9390.
// Logo #183: Sailfish...
static const char Sailfish[] = "                 _a@b\n              _#b (b\n            _@@   @_         _,\n          _#^@ _#*^^*gg,aa@^^\n          #- @@^  _a@^^\n          @_  *g#b\n          ^@_   ^@_\n            ^@_   @\n             @(b (b\n            #b(b#^\n          _@_#@^\n       _a@a*^\n   ,a@*^\n";
// Sailfish: 14 lines, done at line 9409.
// Logo #184: SalentOS...
static const char SalentOS[] = "                 ``..``\n        .-:+oshdNMMMMMMNdhyo+:-.`\n  -oydmMMMMMMMMMMMMMMMMMMMMMMMMMMNdhs/\n +hdddmNMMMMMMMMMMMMMMMMMMMMMMMMNmdddh+`\n`MMMMMNmdddddmMMMMMMMMMMMMmdddddmNMMMMM-\n mMMMMMMMMMMMNddddhyyhhdddNMMMMMMMMMMMM`\n dMMMMMMMMMMMMMMMMMooMMMMMMMMMMMMMMMMMN`\n yMMMMMMMMMMMMMMMMMhhMMMMMMMMMMMMMMMMMd\n +MMMMMMMMMMMMMMMMMhhMMMMMMMMMMMMMMMMMy\n :MMMMMMMMMMMMMMMMMhhMMMMMMMMMMMMMMMMMo\n .MMMMMMMMMMMMMMMMMhhMMMMMMMMMMMMMMMMM/\n `NMMMMMMMMMMMMMMMMhhMMMMMMMMMMMMMMMMM-\n  mMMMMMMMMMMMMMMMMhhMMMMMMMMMMMMMMMMN`\n  hMMMMMMMMMMMMMMMMhhMMMMMMMMMMMMMMMMm\n  /MMMMMMMMMMMMMMMMhhMMMMMMMMMMMMMMMMy\n   .+hMMMMMMMMMMMMMhhMMMMMMMMMMMMMms:\n      `:smMMMMMMMMMhhMMMMMMMMMNh+.\n          .+hMMMMMMhhMMMMMMdo:\n             `:smMMyyMMNy/`\n                 .- `:.\n";
// SalentOS: 21 lines, done at line 9435.
// Logo #185: Scientific...
static const char Scientific[] = "                 =/;;/-\n                +:    //\n               /;      /;\n              -X        H.\n.//;;;:;;-,   X=        :+   .-;:=;:;#;.\nM-       ,=;;;#:,      ,:#;;:=,       ,@\n:#           :#.=/++++/=.$=           #=\n ,#;         #/:+/;,,/++:+/         ;+.\n   ,+/.    ,;@+,        ,#H;,    ,/+,\n      ;+;;/= @.  .H##X   -X :///+;\n      ;+=;;;.@,  .XM@$.  =X.//;=#/.\n   ,;:      :@#=        =$H:     .+#-\n ,#=         #;-///==///-//         =#,\n;+           :#-;;;:;;;;-X-           +:\n@-      .-;;;;M-        =M/;;;-.      -X\n :;;::;;-.    #-        :+    ,-;;-;:==\n              ,X        H.\n               ;/      #=\n                //    +;\n                 '////'\n";
// Scientific: 21 lines, done at line 9461.
// Logo #186: Septor...
static const char Septor[] = "ssssssssssssssssssssssssssssssssssssssss\nssssssssssssssssssssssssssssssssssssssss\nssssssssssssssssssssssssssssssssssssssss\nssssssssssssssssssssssssssssssssssssssss\nssssssssss;okOOOOOOOOOOOOOOko;ssssssssss\nsssssssssoNWWWWWWWWWWWWWWWWWWNosssssssss\nssssssss:WWWWWWWWWWWWWWWWWWWWWW:ssssssss\nsssssssslWWWWWksssssssssslddddd:ssssssss\nsssssssscWWWWWNKKKKKKKKKKKKOx:ssssssssss\nyysssssssOWWWWWWWWWWWWWWWWWWWWxsssssssyy\nyyyyyyyyyy:kKNNNNNNNNNNNNWWWWWW:yyyyyyyy\nyyyyyyyysccccc;yyyyyyyyyykWWWWW:yyyyyyyy\nyyyyyyyy:WWWWWWNNNNNNNNNNWWWWWW;yyyyyyyy\nyyyyyyyy.dWWWWWWWWWWWWWWWWWWWNdyyyyyyyyy\nyyyyyyyyyysdO0KKKKKKKKKKKK0Od;yyyyyyyyyy\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\n";
// Septor: 21 lines, done at line 9487.
// Logo #187: Serene...
static const char Serene[] = "              __---''''''---__\n          .                      .\n        :                          :\n      -                       _______----_-\n     s               __----'''     __----\n __h_            _-'           _-'     h\n '-._''--.._    ;           _-'         y\n  :  ''-._  '-._/        _-'             :\n  y       ':_       _--''                y\n  m    .--'' '-._.;'                     m\n  m   :        :                         m\n  y    '.._     '-__                     y\n  :        '--._    '''----___           :\n   y            '--._         ''-- _    y\n    h                '--._          :  h\n     s                  __';         vs\n      -         __..--''             -\n        :_..--''                   :\n          .                     _ .\n            `''---______---''-``\n";
// Serene: 21 lines, done at line 9513.
// Logo #188: SharkLinux...
static const char SharkLinux[] = "                              `:shd/\n                          `:yNMMMMs\n                       `-smMMMMMMN.\n                     .+dNMMMMMMMMs\n                   .smNNMMMMMMMMm`\n                 .sNNNNNNNMMMMMM/\n               `omNNNNNNNMMMMMMm\n              /dNNNNNNNNMMMMMMM+\n            .yNNNNNNNNNMMMMMMMN`\n           +mNNNNNNNNNMMMMMMMMh\n         .hNNNNNNNNNNMMMMMMMMMs\n        +mMNNNNNNNNMMMMMMMMMMMs\n      .hNMMNNNNMMMMMMMMMMMMMMMd\n    .oNNNNNNNNNNMMMMMMMMMMMMMMMo\n `:+syyssoo++++ooooossssssssssso:\n";
// SharkLinux: 16 lines, done at line 9534.
// Logo #189: Siduction...
static const char Siduction[] = "                _aass,\n               jQh: =$w\n               QWmwawQW\n               )$QQQQ@(   ..\n         _a_a.   ~??^  syDY?Sa,\n       _mW>-<$c       jWmi  imm.\n       ]QQwayQE       4QQmgwmQQ`\n        ?WWQWP'       -9QQQQQ@'._aas,\n _a%is.        .adYYs,. -\"?!` aQB*~^3$c\n_Qh;.nm       .QWc. {QL      ]QQp;..vmQ/\n\"QQmmQ@       -QQQggmQP      ]QQWmggmQQ(\n -???\"         \"$WQQQY`  __,  ?QQQQQQW!\n        _yZ!?q,   -   .yWY!!Sw, \"???^\n       .QQa_=qQ       mQm>..vmm\n        $QQWQQP       $QQQgmQQ@\n         \"???\"   _aa, -9WWQQWY`\n               _mB>~)$a  -~~\n               mQms_vmQ.\n               ]WQQQQQP\n                -?T??\"\n";
// Siduction: 21 lines, done at line 9560.
// Logo #190: slackware_small...
static const char slackware_small[] = "   ________\n  /  ______|\n  | |______\n  \\\\______  \\\\\n   ______| |\n| |________/\n|____________\n";
// slackware_small: 8 lines, done at line 9573.
// Logo #191: Slackware...
static const char Slackware[] = "                  :::::::\n            :::::::::::::::::::\n         :::::::::::::::::::::::::\n       ::::::::cllcccccllllllll::::::\n    :::::::::lc               dc:::::::\n   ::::::::cl   clllccllll    oc:::::::::\n  :::::::::o   lc::::::::co   oc::::::::::\n ::::::::::o    cccclc:::::clcc::::::::::::\n :::::::::::lc        cclccclc:::::::::::::\n::::::::::::::lcclcc          lc::::::::::::\n::::::::::cclcc:::::lccclc     oc:::::::::::\n::::::::::o    l::::::::::l    lc:::::::::::\n :::::cll:o     clcllcccll     o:::::::::::\n :::::occ:o                  clc:::::::::::\n  ::::ocl:ccslclccclclccclclc:::::::::::::\n   :::oclcccccccccccccllllllllllllll:::::\n    ::lcc1lcccccccccccccccccccccccco::::\n      ::::::::::::::::::::::::::::::::\n        ::::::::::::::::::::::::::::\n           ::::::::::::::::::::::\n                ::::::::::::\n";
// Slackware: 22 lines, done at line 9600.
// Logo #192: SliTaz...
static const char SliTaz[] = "        @    @(               @\n      @@   @@                  @    @/\n     @@   @@                   @@   @@\n    @@  %@@                     @@   @@\n   @@  %@@@       @@@@@.       @@@@  @@\n  @@@    @@@@    @@@@@@@    &@@@    @@@\n   @@@@@@@ %@@@@@@@@@@@@ &@@@% @@@@@@@/\n       ,@@@@@@@@@@@@@@@@@@@@@@@@@\n  .@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@/\n@@@@@@.  @@@@@@@@@@@@@@@@@@@@@  /@@@@@@\n@@    @@@@@  @@@@@@@@@@@@,  @@@@@   @@@\n@@ @@@@.    @@@@@@@@@@@@@%    #@@@@ @@.\n@@ ,@@      @@@@@@@@@@@@@      @@@  @@\n@   @@.     @@@@@@@@@@@@@     @@@  *@\n@    @@     @@@@@@@@@@@@      @@   @\n      @      @@@@@@@@@.     #@\n       @      ,@@@@@       @\n";
// SliTaz: 18 lines, done at line 9623.
// Logo #193: SmartOS...
static const char SmartOS[] = "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\nyyyys             oyyyyyyyyyyyyyyyy\nyyyys  yyyyyyyyy  oyyyyyyyyyyyyyyyy\nyyyys  yyyyyyyyy  oyyyyyyyyyyyyyyyy\nyyyys  yyyyyyyyy  oyyyyyyyyyyyyyyyy\nyyyys  yyyyyyyyy  oyyyyyyyyyyyyyyyy\nyyyys  yyyyyyyyyyyyyyyyyyyyyyyyyyyy\nyyyyy                         syyyy\nyyyyyyyyyyyyyyyyyyyyyyyyyyyy  syyyy\nyyyyyyyyyyyyyyyy  syyyyyyyyy  syyyy\nyyyyyyyyyyyyyyyy  oyyyyyyyyy  syyyy\nyyyyyyyyyyyyyyyy  oyyyyyyyyy  syyyy\nyyyyyyyyyyyyyyyy  syyyyyyyyy  syyyy\nyyyyyyyyyyyyyyyy              yyyyy\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\n";
// SmartOS: 18 lines, done at line 9646.
// Logo #194: Solus...
static const char Solus[] = "            -```````````\n          `-+/------------.`\n       .---:mNo---------------.\n     .-----yMMMy:---------------.\n   `------oMMMMMm/----------------`\n  .------/MMMMMMMN+----------------.\n .------/NMMMMMMMMm-+/--------------.\n`------/NMMMMMMMMMN-:mh/-------------`\n.-----/NMMMMMMMMMMM:-+MMd//oso/:-----.\n-----/NMMMMMMMMMMMM+--mMMMh::smMmyo:--\n----+NMMMMMMMMMMMMMo--yMMMMNo-:yMMMMd/.\n.--oMMMMMMMMMMMMMMMy--yMMMMMMh:-yMMMy-`\n`-sMMMMMMMMMMMMMMMMh--dMMMMMMMd:/Ny+y.\n`-/+osyhhdmmNNMMMMMm-/MMMMMMMmh+/ohm+\n  .------------:://+-/++++++oshddys:\n   -hhhhyyyyyyyyyyyhhhhddddhysssso-\n    `:ossssssyysssssssssssssssso:`\n      `:+ssssssssssssssssssss+-\n         `-/+ssssssssssso+/-`\n              `.-----..`\n";
// Solus: 21 lines, done at line 9672.
// Logo #195: Source_Mage...
static const char Source_Mage[] = "       :ymNMNho.\n.+sdmNMMMMMMMMMMy`\n.-::/yMMMMMMMMMMMm-\n      sMMMMMMMMMMMm/\n     /NMMMMMMMMMMMMMm:\n    .MMMMMMMMMMMMMMMMM:\n    `MMMMMMMMMMMMMMMMMN.\n     NMMMMMMMMMMMMMMMMMd\n     mMMMMMMMMMMMMMMMMMMo\n     hhMMMMMMMMMMMMMMMMMM.\n     .`/MMMMMMMMMMMMMMMMMs\n        :mMMMMMMMMMMMMMMMN`\n         `sMMMMMMMMMMMMMMM+\n           /NMMMMMMMMMMMMMN`\n             oMMMMMMMMMMMMM+\n          ./sd.-hMMMMMMMMmmN`\n      ./+oyyyh- `MMMMMMMMMmNh\n                 sMMMMMMMMMmmo\n                 `NMMMMMMMMMd:\n                  -dMMMMMMMMMo\n                    -shmNMMms.\n";
// Source_Mage: 22 lines, done at line 9699.
// Logo #196: Sparky...
static const char Sparky[] = "\n           .            `-:-`\n          .o`       .-///-`\n         `oo`    .:/++:.\n         os+`  -/+++:` ``.........```\n        /ys+`./+++/-.-::::::----......``\n       `syyo`++o+--::::-::/+++/-``\n       -yyy+.+o+`:/:-:sdmmmmmmmmdy+-`\n::-`   :yyy/-oo.-+/`ymho++++++oyhdmdy/`\n`/yy+-`.syyo`+o..o--h..osyhhddhs+//osyy/`\n  -ydhs+-oyy/.+o.-: ` `  :/::+ydhy+```-os-\n   .sdddy::syo--/:.     `.:dy+-ohhho    ./:\n     :yddds/:+oo+//:-`- /+ +hy+.shhy:     ``\n      `:ydmmdysooooooo-.ss`/yss--oyyo\n        `./ossyyyyo+:-/oo:.osso- .oys\n       ``..-------::////.-oooo/   :so\n    `...----::::::::--.`/oooo:    .o:\n           ```````     ++o+:`     `:`\n                     ./+/-`        `\n                   `-:-.\n                   ``\n";
// Sparky: 22 lines, done at line 9726.
// Logo #197: Star...
static const char Star[] = "                   ./\n                  `yy-\n                 `y.`y`\n    ``           s-  .y            `\n    +h//:..`    +/    /o    ``..:/so\n     /o``.-::/:/+      o/://::-.`+o`\n      :s`     `.        .`     `s/\n       .y.                    .s-\n        `y-                  :s`\n      .-//.                  /+:.\n   .:/:.                       .:/:.\n-+o:.                             .:+:.\n-///++///:::`              .-::::///+so-\n       ``..o/              d-....```\n           s.     `/.      d\n           h    .+o-+o-    h.\n           h  -o/`   `/o:  s:\n          -s/o:`       `:o/+/\n          /s-             -yo\n";
// Star: 20 lines, done at line 9751.
// Logo #198: SteamOS...
static const char SteamOS[] = "              .,,,,.\n        .,'onNMMMMMNNnn',.\n     .'oNMANKMMMMMMMMMMMNNn'.\n   .'ANMMMMMMMXKNNWWWPFFWNNMNn.\n  ;NNMMMMMMMMMMNWW'' ,.., 'WMMM,\n ;NMMMMV+##+VNWWW' .+;'':+, 'WMW,\n,VNNWP+######+WW,  +:    :+, +MMM,\n'+#############,   +.    ,+' +NMMM\n  '*#########*'     '*,,*' .+NMMMM.\n     `'*###*'          ,.,;###+WNM,\n         .,;;,      .;##########+W\n,',.         ';  ,+##############'\n '###+. :,. .,; ,###############'\n  '####.. `'' .,###############'\n    '#####+++################'\n      '*##################*'\n         ''*##########*''\n              ''''''\n";
// SteamOS: 19 lines, done at line 9775.
// Logo #199: solaris_small...
static const char solaris_small[] = "       .   .;   .\n   .   :;  ::  ;:   .\n   .;. ..      .. .;.\n..  ..             ..  ..\n .;,                 ,;.\n";
// solaris_small: 6 lines, done at line 9786.
// Logo #200: Solaris...
static const char Solaris[] = "                 `-     `\n          `--    `+-    .:\n           .+:  `++:  -/+-     .\n    `.::`  -++/``:::`./+/  `.-/.\n      `++/-`.`          ` /++:`\n  ``   ./:`                .: `..`.-\n``./+/:-                     -+++:-\n    -/+`                      :.\n";
// Solaris: 9 lines, done at line 9800.
// Logo #201: openSUSE_Leap...
static const char openSUSE_Leap[] = "                 `-++:`\n               ./oooooo/-\n            `:oooooooooooo:.\n          -+oooooooooooooooo+-`\n       ./oooooooooooooooooooooo/-\n      :oooooooooooooooooooooooooo:\n    `  `-+oooooooooooooooooooo/-   `\n `:oo/-   .:ooooooooooooooo+:`  `-+oo/.\n`/oooooo:.   -/oooooooooo/.   ./oooooo/.\n  `:+ooooo+-`  `:+oooo+-   `:oooooo+:`\n     .:oooooo/.   .::`   -+oooooo/.\n        -/oooooo:.    ./oooooo+-\n          `:+ooooo+-:+oooooo:`\n             ./oooooooooo/.\n                -/oooo+:`\n                  `:/.\n";
// openSUSE_Leap: 17 lines, done at line 9822.
// Logo #202: t2...
static const char t2[] = "\nTTTTTTTTTT\n    tt   222\n    tt  2   2\n    tt     2\n    tt    2\n    tt  22222\n";
// t2: 8 lines, done at line 9835.
// Logo #203: openSUSE_Tumbleweed...
static const char openSUSE_Tumbleweed[] = "                                     ......\n     .,cdxxxoc,.               .:kKMMMNWMMMNk:.\n    cKMMN0OOOKWMMXo. ;        ;0MWk:.      .:OMMk.\n  ;WMK;.       .lKMMNM,     :NMK,             .OMW;\n cMW;            'WMMMN   ,XMK,                 oMM'\n.MMc               ..;l. xMN:                    KM0\n'MM.                   'NMO                      oMM\n.MM,                 .kMMl                       xMN\n KM0               .kMM0. .dl:,..               .WMd\n .XM0.           ,OMMK,    OMMMK.              .XMK\n   oWMO:.    .;xNMMk,       NNNMKl.          .xWMx\n     :ONMMNXMMMKx;          .  ,xNMWKkxllox0NMWk,\n         .....                    .:dOOXXKOxl,\n";
// openSUSE_Tumbleweed: 14 lines, done at line 9854.
// Logo #204: suse_small...
static const char suse_small[] = "  _______\n__|   __ \\\\\n     / .\\\\ \\\\\n     \\\\__/ |\n   _______|\n   \\\\_______\n__________/\n";
// suse_small: 8 lines, done at line 9867.
// Logo #205: SUSE...
static const char SUSE[] = "           .;ldkO0000Okdl;.\n       .;d00xl:^''''''^:ok00d;.\n     .d00l'                'o00d.\n   .d0Kd'  Okxol:;,.          :O0d.\n  .OKKKK0kOKKKKKKKKKKOxo:,      lKO.\n ,0KKKKKKKKKKKKKKKK0P^,,,^dx:    ;00,\n.OKKKKKKKKKKKKKKKKk'.oOPPb.'0k.   cKO.\n:KKKKKKKKKKKKKKKKK: kKx..dd lKd   'OK:\ndKKKKKKKKKKKOx0KKKd ^0KKKO' kKKc   dKd\ndKKKKKKKKKKKK;.;oOKx,..^..;kKKK0.  dKd\n:KKKKKKKKKKKK0o;...^cdxxOK0O/^^'  .0K:\n kKKKKKKKKKKKKKKK0x;,,......,;od  lKk\n '0KKKKKKKKKKKKKKKKKKKKK00KKOo^  c00'\n  'kKKKOxddxkOO00000Okxoc;''   .dKk'\n    l0Ko.                    .c00l'\n     'l0Kk:.              .;xK0l'\n        'lkK0xl:;,,,,;:ldO0kl'\n            '^:ldxkkkkxdl:^'\n";
// SUSE: 19 lines, done at line 9891.
// Logo #206: SwagArch...
static const char SwagArch[] = "        .;ldkOKXXNNNNXXK0Oxoc,.\n   ,lkXMMNK0OkkxkkOKWMMMMMMMMMM;\n 'K0xo  ..,;:c:.     `'lKMMMMM0\n     .lONMMMMMM'         `lNMk'\n    ;WMMMMMMMMMO.              ....::...\n    OMMMMMMMMMMMMKl.       .,;;;;;ccccccc,\n    `0MMMMMMMMMMMMMM0:         .. .ccccccc.\n      'kWMMMMMMMMMMMMMNo.   .,:'  .ccccccc.\n        `c0MMMMMMMMMMMMMN,,:c;    :cccccc:\n ckl.      `lXMMMMMMMMMXocccc:.. ;ccccccc.\ndMMMMXd,     `OMMMMMMWkccc;:''` ,ccccccc:\nXMMMMMMMWKkxxOWMMMMMNoccc;     .cccccccc.\n `':ldxO0KXXXXXK0Okdocccc.     :cccccccc.\n                    :ccc:'     `cccccccc:,\n                                   ''\n";
// SwagArch: 16 lines, done at line 9912.
// Logo #207: Tails...
static const char Tails[] = "      ``\n  ./yhNh\nsyy/Nshh         `:o/\nN:dsNshh  █   `ohNMMd\nN-/+Nshh      `yMMMMd\nN-yhMshh       yMMMMd\nN-s:hshh  █    yMMMMd so//.\nN-oyNsyh       yMMMMd d  Mms.\nN:hohhhd:.     yMMMMd  syMMM+\nNsyh+-..+y+-   yMMMMd   :mMM+\n+hy-      -ss/`yMMMM     `+d+\n  :sy/.     ./yNMMMMm      ``\n    .+ys- `:+hNMMMMMMy/`\n      `hNmmMMMMMMMMMMMMdo.\n       dMMMMMMMMMMMMMMMMMNh:\n       +hMMMMMMMMMMMMMMMMMmy.\n         -oNMMMMMMMMMMmy+.`\n           `:yNMMMds/.`\n              .//`\n";
// Tails: 20 lines, done at line 9937.
// Logo #208: Trisquel...
static const char Trisquel[] = "                         ▄▄▄▄▄▄\n                      ▄█████████▄\n      ▄▄▄▄▄▄         ████▀   ▀████\n   ▄██████████▄     ████▀   ▄▄ ▀███\n ▄███▀▀   ▀▀████     ███▄   ▄█   ███\n▄███   ▄▄▄   ████▄    ▀██████   ▄███\n███   █▀▀██▄  █████▄     ▀▀   ▄████\n▀███      ███  ███████▄▄  ▄▄██████\n ▀███▄   ▄███  █████████████████▀\n  ▀█████████    ██████████▀▀▀\n    ▀▀███▀▀     ██████▀▀\n               ██████▀   ▄▄▄▄\n              █████▀   ████████\n              █████   ███▀  ▀███\n               ████▄   ██▄▄▄  ███\n                █████▄   ▀▀  ▄██\n                  ██████▄▄▄████\n                     ▀▀█████▀▀\n";
// Trisquel: 19 lines, done at line 9961.
// Logo #209: Ubuntu-Cinnamon...
static const char Ubuntu_Cinnamon[] = "            .-:/++oooo++/:-.\n        `:/oooooooooooooooooo/-`\n      -/oooooooooooooooooooo+ooo/-\n    .+oooooooooooooooooo+/-`.ooooo+.\n   :oooooooooooo+//:://++:. .ooooooo:\n  /oooooooooo+o:`.----.``./+/oooooooo/\n /ooooooooo+. +ooooooooo+:``/ooooooooo/\n.ooooooooo: .+ooooooooooooo- -ooooooooo.\n/oooooo/o+ .ooooooo:`+oo+ooo- :oooooooo/\nooo+:. .o: :ooooo:` .+/. ./o+:/ooooooooo\noooo/-`.o: :ooo/` `/+.     ./.:ooooooooo\n/oooooo+o+``++. `:+-          /oooooooo/\n.ooooooooo/``  -+:`          :ooooooooo.\n /ooooooooo+--+/`          .+ooooooooo/\n  /ooooooooooo+.`      `.:++:oooooooo/\n   :oooooooooooooo++++oo+-` .ooooooo:\n    .+ooooooooooooooooooo+:..ooooo+.\n      -/oooooooooooooooooooooooo/-\n        `-/oooooooooooooooooo/:`\n            .-:/++oooo++/:-.\n";
// Ubuntu_Cinnamon: 21 lines, done at line 9987.
// Logo #210: Ubuntu-Budgie...
static const char Ubuntu_Budgie[] = "           ./oydmMMMMMMmdyo/.\n        :smMMMMMMMMMMMhs+:++yhs:\n     `omMMMMMMMMMMMN+`        `odo`\n    /NMMMMMMMMMMMMN-            `sN/\n  `hMMMMmhhmMMMMMMh               sMh`\n .mMmo-     /yMMMMm`              `MMm.\n mN/       yMMMMMMMd-              MMMm\noN-        oMMMMMMMMMms+//+o+:    :MMMMo\nm/          +NMMMMMMMMMMMMMMMMm. :NMMMMm\nM`           .NMMMMMMMMMMMMMMMNodMMMMMMM\nM-            sMMMMMMMMMMMMMMMMMMMMMMMMM\nmm`           mMMMMMMMMMNdhhdNMMMMMMMMMm\noMm/        .dMMMMMMMMh:      :dMMMMMMMo\n mMMNyo/:/sdMMMMMMMMM+          sMMMMMm\n .mMMMMMMMMMMMMMMMMMs           `NMMMm.\n  `hMMMMMMMMMMM.oo+.            `MMMh`\n    /NMMMMMMMMMo                sMN/\n     `omMMMMMMMMy.            :dmo`\n        :smMMMMMMMh+-`   `.:ohs:\n           ./oydmMMMMMMdhyo/.\n";
// Ubuntu_Budgie: 21 lines, done at line 10013.
// Logo #211: Ubuntu-GNOME...
static const char Ubuntu_GNOME[] = "          ./o.\n        .oooooooo\n      .oooo```soooo\n    .oooo`     `soooo\n   .ooo`   .o.   `\\/ooo.\n   :ooo   :oooo.   `\\/ooo.\n    sooo    `ooooo    \\/oooo\n     \\/ooo    `soooo    `ooooo\n      `soooo    `\\/ooo    `soooo\n./oo    `\\/ooo    `/oooo.   `/ooo\n`\\/ooo.   `/oooo.   `/oooo.   ``\n  `\\/ooo.    /oooo     /ooo`\n     `ooooo    ``    .oooo\n       `soooo.     .oooo`\n         `\\/oooooooooo`\n            ``\\/oo``\n";
// Ubuntu_GNOME: 17 lines, done at line 10035.
// Logo #212: Ubuntu-MATE...
static const char Ubuntu_MATE[] = "           `:+shmNNMMNNmhs+:`\n        .odMMMMMMMMMMMMMMMMMMdo.\n      /dMMMMMMMMMMMMMMMmMMMMMMMMd/\n    :mMMMMMMMMMMMMNNNNM/`/yNMMMMMMm:\n  `yMMMMMMMMMms:..-::oM:    -omMMMMMy`\n `dMMMMMMMMy-.odNMMMMMM:    -odMMMMMMd`\n hMMMMMMMm-.hMMy/....+M:`/yNm+mMMMMMMMh\n/MMMMNmMN-:NMy`-yNMMMMMmNyyMN:`dMMMMMMM/\nhMMMMm -odMMh`sMMMMMMMMMMs sMN..MMMMMMMh\nNMMMMm    `/yNMMMMMMMMMMMM: MM+ mMMMMMMN\nNMMMMm    `/yNMMMMMMMMMMMM: MM+ mMMMMMMN\nhMMMMm -odMMh sMMMMMMMMMMs oMN..MMMMMMMh\n/MMMMNNMN-:NMy`-yNMMMMMNNsyMN:`dMMMMMMM/\n hMMMMMMMm-.hMMy/....+M:.+hNd+mMMMMMMMh\n `dMMMMMMMMy-.odNMMMMMM:    :smMMMMMMd`\n   yMMMMMMMMMms/..-::oM:    .+dMMMMMy\n    :mMMMMMMMMMMMMNNNNM: :smMMMMMMm:\n      /dMMMMMMMMMMMMMMMdNMMMMMMMd/\n        .odMMMMMMMMMMMMMMMMMMdo.\n           `:+shmNNMMNNmhs+:`\n";
// Ubuntu_MATE: 21 lines, done at line 10061.
// Logo #213: ubuntu_old...
static const char ubuntu_old[] = "                         ./+o+-\n                 yyyyy- -yyyyyy+\n              ://+//////-yyyyyyo\n          .++ .:/++++++/-.+sss/`\n        .:++o:  /++++++++/:--:/-\n       o:+o+:++.`..```.-/oo+++++/\n      .:+o:+o/.          `+sssoo+/\n .++/+:+oo+o:`             /sssooo.\n/+++//+:`oo+o               /::--:.\n+/+o+++`o++o               ++////.\n .++.o+++oo+:`             /dddhhh.\n      .+.o+oo:.          `oddhhhh+\n       +.++o+o``-````.:ohdhhhhh+\n        `:o+++ `ohhhhhhhhyo++os:\n          .o:`.syhhhhhhh/.oo++o`\n              /osyyyyyyo++ooo+++/\n                  ````` +oo+++o:\n                         `oo++.\n";
// ubuntu_old: 19 lines, done at line 10085.
// Logo #214: Ubuntu-Studio...
static const char Ubuntu_Studio[] = "              ..-::::::-.`\n         `.:+++++++++++ooo++:.`\n       ./+++++++++++++sMMMNdyo+/.\n     .++++++++++++++++oyhmMMMMms++.\n   `/+++++++++osyhddddhys+osdMMMh++/`\n  `+++++++++ydMMMMNNNMMMMNds+oyyo++++`\n  +++++++++dMMNhso++++oydNMMmo++++++++`\n :+odmy+++ooysoohmNMMNmyoohMMNs+++++++:\n ++dMMm+oNMd++yMMMmhhmMMNs+yMMNo+++++++\n`++NMMy+hMMd+oMMMs++++sMMN++NMMs+++++++.\n`++NMMy+hMMd+oMMMo++++sMMN++mMMs+++++++.\n ++dMMd+oNMm++yMMNdhhdMMMs+yMMNo+++++++\n :+odmy++oo+ss+ohNMMMMmho+yMMMs+++++++:\n  +++++++++hMMmhs+ooo+oshNMMms++++++++\n  `++++++++oymMMMMNmmNMMMMmy+oys+++++`\n   `/+++++++++oyhdmmmmdhso+sdMMMs++/\n     ./+++++++++++++++oyhdNMMMms++.\n       ./+++++++++++++hMMMNdyo+/.\n         `.:+++++++++++sso++:.\n              ..-::::::-..\n";
// Ubuntu_Studio: 21 lines, done at line 10111.
// Logo #215: ubuntu_small...
static const char ubuntu_small[] = "         _\n     ---(_)\n _/  ---  \\\\\n(_) |   |\n  \\\\  --- _/\n     ---(_)\n";
// ubuntu_small: 7 lines, done at line 10123.
// Logo #216: i3buntu...
static const char i3buntu[] = "            .-/+oossssoo+/-.\n        `:+ssssssssssssssssss+:`\n      -+ssssssssssssssssssyyssss+-\n    .ossssssssssssssssssdMMMNysssso.\n   /ssssssssssshdmmNNmmyNMMMMhssssss/\n  +ssssssssshmydMMMMMMMNddddyssssssss+\n /sssssssshNMMMyhhyyyyhmNMMMNhssssssss/\n.ssssssssdMMMNhsssssssssshNMMMdssssssss.\n+sssshhhyNMMNyssssssssssssyNMMMysssssss+\nossyNMMMNyMMhsssssssssssssshmmmhssssssso\nossyNMMMNyMMhsssssssssssssshmmmhssssssso\n+sssshhhyNMMNyssssssssssssyNMMMysssssss+\n.ssssssssdMMMNhsssssssssshNMMMdssssssss.\n /sssssssshNMMMyhhyyyyhdNMMMNhssssssss/\n  +sssssssssdmydMMMMMMMMddddyssssssss+\n   /ssssssssssshdmNNNNmyNMMMMhssssss/\n    .ossssssssssssssssssdMMMNysssso.\n      -+sssssssssssssssssyyyssss+-\n        `:+ssssssssssssssssss+:`\n            .-/+oossssoo+/-.\n";
// i3buntu: 21 lines, done at line 10149.
// Logo #217: Venom...
static const char Venom[] = "   :::::::          :::::::\n   mMMMMMMm        dMMMMMMm\n   /MMMMMMMo      +MMMMMMM/\n    yMMMMMMN      mMMMMMMy\n     NMMMMMMs    oMMMMMMm\n     +MMMMMMN:   NMMMMMM+\n      hMMMMMMy  sMMMMMMy\n      :NMMMMMM::NMMMMMN:\n       oMMMMMMyyMMMMMM+\n        dMMMMMMMMMMMMh\n        /MMMMMMMMMMMN:\n         sMMMMMMMMMMo\n          mMMMMMMMMd\n          +MMMMMMMN:\n            ::::::\n";
// Venom: 16 lines, done at line 10170.
// Logo #218: void_small...
static const char void_small[] = "    _______\n _ \\\\______ -\n| \\\\  ___  \\\\ |\n| | /   \\ | |\n| | \\___/ | |\n| \\\\______ \\\\_|\n -_______\\\\\n";
// void_small: 8 lines, done at line 10183.
// Logo #219: Void...
static const char Void[] = "                __.;=====;.__\n            _.=+==++=++=+=+===;.\n             -=+++=+===+=+=+++++=_\n        .     -=:``     `--==+=++==.\n       _vi,    `            --+=++++:\n      .uvnvi.       _._       -==+==+.\n     .vvnvnI`    .;==|==;.     :|=||=|.\n+QmQQmpvvnv; _yYsyQQWUUQQQm #QmQ#:QQQWUV$QQm.\n -QQWQWpvvowZ?.wQQQE==<QWWQ/QWQW.QQWW(: jQWQE\n  -$QQQQmmU'  jQQQ@+=<QWQQ)mQQQ.mQQQC+;jWQQ@'\n   -$WQ8YnI:   QWQQwgQQWV`mWQQ.jQWQQgyyWW@!\n     -1vvnvv.     `~+++`        ++|+++\n      +vnvnnv,                 `-|===\n       +vnvnvns.           .      :=-\n        -Invnvvnsi..___..=sv=.     `\n          +Invnvnvnnnnnnnnvvnn;.\n            ~|Invnvnvvnvvvnnv}+`\n               -~|{*l}*|~\n";
// Void: 19 lines, done at line 10207.
// Logo #220: Obarun...
static const char Obarun[] = "                    ,;::::;\n                ;cooolc;,\n             ,coool;\n           ,loool,\n          loooo;\n        :ooool\n       cooooc            ,:ccc;\n      looooc           :oooooool\n     cooooo          ;oooooooooo,\n    :ooooo;         :ooooooooooo\n    oooooo          oooooooooooc\n   :oooooo         :ooooooooool\n   loooooo         ;oooooooool\n   looooooc        .coooooooc\n   cooooooo:           ,;co;\n   ,ooooooool;       ,:loc\n    cooooooooooooloooooc\n     ;ooooooooooooool;\n       ;looooooolc;\n";
// Obarun: 20 lines, done at line 10232.
// Logo #221: windows8...
static const char windows8[] = "                                ..,\n                    ....,,:;+ccllll\n      ...,,+:;  cllllllllllllllllll\n,cclllllllllll  lllllllllllllllllll\nllllllllllllll  lllllllllllllllllll\nllllllllllllll  lllllllllllllllllll\nllllllllllllll  lllllllllllllllllll\nllllllllllllll  lllllllllllllllllll\nllllllllllllll  lllllllllllllllllll\n\nllllllllllllll  lllllllllllllllllll\nllllllllllllll  lllllllllllllllllll\nllllllllllllll  lllllllllllllllllll\nllllllllllllll  lllllllllllllllllll\nllllllllllllll  lllllllllllllllllll\n`'ccllllllllll  lllllllllllllllllll\n       `' \\\\*::  :ccllllllllllllllll\n                       ````''*::cll\n                                 ``\n";
// windows8: 20 lines, done at line 10258.
// Logo #222: Windows...
static const char Windows[] = "        ,.=:!!t3Z3z.,\n       :tt:::tt333EE3\n       Et:::ztt33EEEL @Ee.,      ..,\n      ;tt:::tt333EE7 ;EEEEEEttttt33#\n     :Et:::zt333EEQ. $EEEEEttttt33QL\n     it::::tt333EEF @EEEEEEttttt33F\n    ;3=*^```\"*4EEV :EEEEEEttttt33@.\n    ,.=::::!t=., ` @EEEEEEtttz33QF\n   ;::::::::zt33)   \"4EEEtttji3P*\n  :t::::::::tt33.:Z3z..  `` ,..g.\n  i::::::::zt33F AEEEtttt::::ztF\n ;:::::::::t33V ;EEEttttt::::t3\n E::::::::zt33L @EEEtttt::::z3F\n{3=*^```\"*4E3) ;EEEtttt:::::tZ`\n             ` :EEEEtttt::::z7\n                 \"VEzjt:;;z>*`\n";
// Windows: 17 lines, done at line 10280.
// Logo #223: Xubuntu...
static const char Xubuntu[] = "           `-/osyhddddhyso/-`\n        .+yddddddddddddddddddy+.\n      :yddddddddddddddddddddddddy:\n    -yddddddddddddddddddddhdddddddy-\n   odddddddddddyshdddddddh`dddd+ydddo\n `yddddddhshdd-   ydddddd+`ddh.:dddddy`\n sddddddy   /d.   :dddddd-:dy`-ddddddds\n:ddddddds    /+   .dddddd`yy`:ddddddddd:\nsdddddddd`    .    .-:/+ssdyodddddddddds\nddddddddy                  `:ohddddddddd\ndddddddd.                      +dddddddd\nsddddddy                        ydddddds\n:dddddd+                      .oddddddd:\n sdddddo                   ./ydddddddds\n `yddddd.              `:ohddddddddddy`\n   oddddh/`      `.:+shdddddddddddddo\n    -ydddddhyssyhdddddddddddddddddy-\n      :yddddddddddddddddddddddddy:\n        .+yddddddddddddddddddy+.\n           `-/osyhddddhyso/-`\n";
// Xubuntu: 21 lines, done at line 10306.
// Logo #224: IRIX...
static const char IRIX[] = "           ./ohmNd/  +dNmho/-\n     `:+ydNMMMMMMMM.-MMMMMMMMMdyo:.\n   `hMMMMMMNhs/sMMM-:MMM+/shNMMMMMMh`\n   -NMMMMMmo-` /MMM-/MMM- `-omMMMMMN.\n `.`-+hNMMMMMNhyMMM-/MMMshmMMMMMmy+...`\n+mMNds:-:sdNMMMMMMMyyMMMMMMMNdo:.:sdMMm+\ndMMMMMMmy+.-/ymNMMMMMMMMNmy/-.+hmMMMMMMd\noMMMMmMMMMNds:.+MMMmmMMN/.-odNMMMMmMMMM+\n.MMMM-/ymMMMMMmNMMy..hMMNmMMMMMmy/-MMMM.\n hMMM/ `/dMMMMMMMN////NMMMMMMMd/. /MMMh\n /MMMdhmMMMmyyMMMMMMMMMMMMhymMMMmhdMMM:\n `mMMMMNho//sdMMMMM//NMMMMms//ohNMMMMd\n  `/so/:+ymMMMNMMMM` mMMMMMMMmh+::+o/`\n     `yNMMNho-yMMMM` NMMMm.+hNMMNh`\n     -MMMMd:  oMMMM. NMMMh  :hMMMM-\n      -yNMMMmooMMMM- NMMMyomMMMNy-\n        .omMMMMMMMM-`NMMMMMMMmo.\n          `:hMMMMMM. NMMMMMh/`\n             .odNm+  /dNms.\n";
// IRIX: 20 lines, done at line 10330.
// Logo #225: Zorin...
static const char Zorin[] = "        `osssssssssssssssssssso`\n       .osssssssssssssssssssssso.\n      .+oooooooooooooooooooooooo+.\n\n\n  `::::::::::::::::::::::.         .:`\n `+ssssssssssssssssss+:.`     `.:+ssso`\n.ossssssssssssssso/.       `-+ossssssso.\nssssssssssssso/-`      `-/osssssssssssss\n.ossssssso/-`      .-/ossssssssssssssso.\n `+sss+:.      `.:+ssssssssssssssssss+`\n  `:.         .::::::::::::::::::::::`\n\n\n      .+oooooooooooooooooooooooo+.\n       -osssssssssssssssssssssso-\n        `osssssssssssssssssssso`\n";
// Zorin: 18 lines, done at line 10352.
// Logo #226: BSD...
// Logo #227: Darwin...
// Logo #228: GNU...
// Logo #229: Linux...
// Logo #230: SambaBOX...
static const char SambaBOX[] = "\n                    #\n               *////#####\n           /////////#########(\n      .((((((/////    ,####(#(((((\n  /#######(((*             (#(((((((((.\n//((#(#(#,        ((##(        ,((((((//\n//////        #(##########(       //////\n//////    ((#(#(#(#(##########(/////////\n/////(    (((((((#########(##((((((/////\n/(((#(                             ((((/\n####(#                             ((###\n#########(((/////////(((((((((,    (#(#(\n########(   /////////(((((((*      #####\n####///,        *////(((         (((((((\n.///////////                .//(((((((((\n     ///////////,       *(/////((((*\n         ,/(((((((((##########/.\n             .((((((#######\n                  ((##*\n";
// SambaBOX: 21 lines, done at line 10469.
// Logo #231: SunOS...
static const char SunOS[] = "                 `-     `\n          `--    `+-    .:\n           .+:  `++:  -/+-     .\n    `.::`  -++/``:::`./+/  `.-/.\n      `++/-`.`          ` /++:`\n  ``   ./:`                .: `..`.-\n``./+/:-                     -+++:-\n    -/+`                      :.\n";
// SunOS: 9 lines, done at line 10483.
// Logo #232: IRIX...
// Closed get_distro_ascii at line 10521

struct neofetch_art { const char* oskey; const char* ncart; };

const struct neofetch_art ncarts[] = {
  { "gentoo_small", gentoo_small },
  { "netbsd_small", netbsd_small },
  { "LEDE", LEDE },
  { "Tails", Tails },
  { "mageia_small", mageia_small },
  { "Devuan", Devuan },
  { "void_small", void_small },
  { "RFRemix", RFRemix },
  { "fedora_small", fedora_small },
  { "Ubuntu_Cinnamon", Ubuntu_Cinnamon },
  { "PostMarketOS", PostMarketOS },
  { "Peppermint", Peppermint },
  { "rhel_old", rhel_old },
  { "OpenStage", OpenStage },
  { "Clover", Clover },
  { "Condres", Condres },
  { "Trisquel", Trisquel },
  { "OBRevenge", OBRevenge },
  { "Feren", Feren },
  { "Kogaion", Kogaion },
  { "MX", MX },
  { "Porteus", Porteus },
  { "ARCHlabs", ARCHlabs },
  { "freebsd_small", freebsd_small },
  { "SunOS", SunOS },
  { "FreeMiNT", FreeMiNT },
  { "arcolinux_small", arcolinux_small },
  { "NetBSD", NetBSD },
  { "dragonfly_old", dragonfly_old },
  { "openmamba", openmamba },
  { "AOSC_OS", AOSC_OS },
  { "openSUSE_Tumbleweed", openSUSE_Tumbleweed },
  { "Hash", Hash },
  { "Slackware", Slackware },
  { "openbsd_small", openbsd_small },
  { "OpenBSD", OpenBSD },
  { "pop_os_small", pop_os_small },
  { "Netrunner", Netrunner },
  { "Sparky", Sparky },
  { "BSD", BSD },
  { "Apricity", Apricity },
  { "Calculate", Calculate },
  { "Elementary", Elementary },
  { "Container_Linux", Container_Linux },
  { "AOSC_OS_Retro", AOSC_OS_Retro },
  { "Anarchy", Anarchy },
  { "Itc", Itc },
  { "IRIX", IRIX },
  { "SambaBOX", SambaBOX },
  { "Parrot", Parrot },
  { "Arya", Arya },
  { "Chakra", Chakra },
  { "solaris_small", solaris_small },
  { "Frugalware", Frugalware },
  { "Radix", Radix },
  { "suse_small", suse_small },
  { "Scientific", Scientific },
  { "Pentoo", Pentoo },
  { "Android", Android },
  { "ArcoLinux", ArcoLinux },
  { "Precise_Puppy", Precise_Puppy },
  { "SliTaz", SliTaz },
  { "Antergos", Antergos },
  { "TrueOS", TrueOS },
  { "DarkOs", DarkOs },
  { "arch_old", arch_old },
  { "Rosa", Rosa },
  { "DesaOS", DesaOS },
  { "ubuntu_old", ubuntu_old },
  { "artix_small", artix_small },
  { "Chrom", Chrom },
  { "linuxlite_small", linuxlite_small },
  { "Deepin", Deepin },
  { "ubuntu_small", ubuntu_small },
  { "PureOS", PureOS },
  { "Regolith", Regolith },
  { "Mandriva", Mandriva },
  { "mx_small", mx_small },
  { "GalliumOS", GalliumOS },
  { "Windows", Windows },
  { "BunsenLabs", BunsenLabs },
  { "Redcore", Redcore },
  { "Mer", Mer },
  { "DragonFly", DragonFly },
  { "Oracle", Oracle },
  { "Redstar", Redstar },
  { "ArchMerge", ArchMerge },
  { "Ubuntu_MATE", Ubuntu_MATE },
  { "hyperbola_small", hyperbola_small },
  { "MagpieOS", MagpieOS },
  { "Garuda", Garuda },
  { "Ubuntu_Budgie", Ubuntu_Budgie },
  { "ArchBox", ArchBox },
  { "alpine_small", alpine_small },
  { "Haiku", Haiku },
  { "Hyperbola", Hyperbola },
  { "openSUSE_Leap", openSUSE_Leap },
  { "Parabola", Parabola },
  { "KDE", KDE },
  { "Star", Star },
  { "i3buntu", i3buntu },
  { "Linux", Linux },
  { "centos_small", centos_small },
  { "Solus", Solus },
  { "Arch", Arch },
  { "manjaro_small", manjaro_small },
  { "Zorin", Zorin },
  { "Endless", Endless },
  { "Void", Void },
  { "Exherbo", Exherbo },
  { "EuroLinux", EuroLinux },
  { "Funtoo", Funtoo },
  { "BlankOn", BlankOn },
  { "Artix", Artix },
  { "rhel", rhel },
  { "Lubuntu", Lubuntu },
  { "Pardus", Pardus },
  { "dragonfly_small", dragonfly_small },
  { "SwagArch", SwagArch },
  { "Refracted_Devuan", Refracted_Devuan },
  { "debian_small", debian_small },
  { "Sailfish", Sailfish },
  { "Proxmox", Proxmox },
  { "Clear_Linux", Clear_Linux },
  { "Manjaro", Manjaro },
  { "_small", _small },
  { "bonsai", bonsai },
  { "Kali", Kali },
  { "Drauger", Drauger },
  { "linuxmint_small", linuxmint_small },
  { "SharkLinux", SharkLinux },
  { "Reborn", Reborn },
  { "Ubuntu_GNOME", Ubuntu_GNOME },
  { "Venom", Venom },
  { "Ataraxia", Ataraxia },
  { "windows8", windows8 },
  { "Alpine", Alpine },
  { "NixOS", NixOS },
  { "Alter", Alter },
  { "Grombyang", Grombyang },
  { "pureos_small", pureos_small },
  { "OpenMandriva", OpenMandriva },
  { "Huayra", Huayra },
  { "Minix", Minix },
  { "Amazon", Amazon },
  { "Chapeau", Chapeau },
  { "Source_Mage", Source_Mage },
  { "postmarketos_small", postmarketos_small },
  { "DracOS", DracOS },
  { "Septor", Septor },
  { "antiX", antiX },
  { "Obarun", Obarun },
  { "arch_small", arch_small },
  { "parabola_small", parabola_small },
  { "Nurunner", Nurunner },
  { "Kibojoe", Kibojoe },
  { "SUSE", SUSE },
  { "Cleanjaro", Cleanjaro },
  { "Solaris", Solaris },
  { "GoboLinux", GoboLinux },
  { "GNU", GNU },
  { "OS_Elbrus", OS_Elbrus },
  { "Namib", Namib },
  { "BlackArch", BlackArch },
  { "Carbs", Carbs },
  { "sabotage", sabotage },
  { "mint", mint },
  { "Kubuntu", Kubuntu },
  { "guix_small", guix_small },
  { "dahlia", dahlia },
  { "Parsix", Parsix },
  { "Regata", Regata },
  { "BlueLight", BlueLight },
  { "slackware_small", slackware_small },
  { "pop_os", pop_os },
  { "OpenWrt", OpenWrt },
  { "osmc", osmc },
  { "Darwin", Darwin },
  { "Ubuntu_Studio", Ubuntu_Studio },
  { "Maui", Maui },
  { "ChaletOS", ChaletOS },
  { "nixos_small", nixos_small },
  { "Guix", Guix },
  { "Sabayon", Sabayon },
  { "Bitrig", Bitrig },
  { "android_small", android_small },
  { "Korora", Korora },
  { "PacBSD", PacBSD },
  { "gNewSense", gNewSense },
  { "Siduction", Siduction },
  { "LMDE", LMDE },
  { "ClearOS", ClearOS },
  { "Serene", Serene },
  { "t2", t2 },
  { "EndeavourOS", EndeavourOS },
  { "Raspbian_small", Raspbian_small },
  { "SalentOS", SalentOS },
  { "ArchStrike", ArchStrike },
  { "SteamOS", SteamOS },
  { "cleanjaro_small", cleanjaro_small },
  { "haiku_small", haiku_small },
  { "Debian", Debian },
  { "elementary_small", elementary_small },
  { "Lunar", Lunar },
  { "AIX", AIX },
  { "BLAG", BLAG },
  { "NuTyX", NuTyX },
  { "Gentoo", Gentoo },
  { "XFerience", XFerience },
  { "Nitrux", Nitrux },
  { "mint_old", mint_old },
  { "Mageia", Mageia },
  { "Raspbian", Raspbian },
  { "KSLinux", KSLinux },
  { "Cucumber", Cucumber },
  { "KaOS", KaOS },
  { "CRUX", CRUX },
  { "PCLinuxOS", PCLinuxOS },
  { "Neptune", Neptune },
  { "CentOS", CentOS },
  { "Bedrock", Bedrock },
  { "Qubes", Qubes },
  { "Xubuntu", Xubuntu },
  { "Linux_Lite", Linux_Lite },
  { "OpenIndiana", OpenIndiana },
  { "openEuler", openEuler },
  { "GNOME", GNOME },
  { "SmartOS", SmartOS },
  { "ubuntu", ubuntu_old },
  { NULL, NULL }
};

const char* get_neofetch_art(const char* oskey){
  for(const struct neofetch_art* nfa = ncarts ; nfa->oskey ; ++nfa){
    if(strcasecmp(nfa->oskey, oskey) == 0){
      return nfa->ncart;
    }
  }
  return NULL;
}
