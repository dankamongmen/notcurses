#include "ncart.h"
#include <stdlib.h>
#include <string.h>
// Output of "neofetch-ripper /usr/bin/neofetch"
// Generated on Sun 10 Jan 2021 09:46:22 PM UTC
// Copyright Dylan Araps under an MIT License
// Found get_distro_ascii at line 5286
// Logo #0: AIX...
static const char AIX[] = "${c1}           `:+ssssossossss+-`\\n        .oys///oyhddddhyo///sy+.\\n      /yo:+hNNNNNNNNNNNNNNNNh+:oy/\\n    :h/:yNNNNNNNNNNNNNNNNNNNNNNy-+h:\\n  `ys.yNNNNNNNNNNNNNNNNNNNNNNNNNNy.ys\\n `h+-mNNNNNNNNNNNNNNNNNNNNNNNNNNNNm-oh\\n h+-NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN.oy\\n/d`mNNNNNNN/::mNNNd::m+:/dNNNo::dNNNd`m:\\nh//NNNNNNN: . .NNNh  mNo  od. -dNNNNN:+y\\nN.sNNNNNN+ -N/ -NNh  mNNd.   sNNNNNNNo-m\\nN.sNNNNNs  +oo  /Nh  mNNs` ` /mNNNNNNo-m\\nh//NNNNh  ossss` +h  md- .hm/ `sNNNNN:+y\\n:d`mNNN+/yNNNNNd//y//h//oNNNNy//sNNNd`m-\\n yo-NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNm.ss\\n `h+-mNNNNNNNNNNNNNNNNNNNNNNNNNNNNm-oy\\n   sy.yNNNNNNNNNNNNNNNNNNNNNNNNNNs.yo\\n    :h+-yNNNNNNNNNNNNNNNNNNNNNNs-oh-\\n      :ys:/yNNNNNNNNNNNNNNNmy/:sy:\\n        .+ys///osyhhhhys+///sy+.\\n            -/osssossossso/-\\n";
// AIX: 21 lines, done at line 5314.
// Logo #1: Hash...
static const char Hash[] = "${c1}\\n\\n      +   ######   +\\n    ###   ######   ###\\n  #####   ######   #####\\n ######   ######   ######\\n\\n####### '\"###### '\"########\\n#######   ######   ########\\n#######   ######   ########\\n\\n ###### '\"###### '\"######\\n  #####   ######   #####\\n    ###   ######   ###\\n      ~   ######   ~\\n\\n";
// Hash: 17 lines, done at line 5336.
// Logo #2: alpine_small...
static const char alpine_small[] = "${c1}   /\\\\ /\\\\\\n  /${c2}/ ${c1}\\\\  \\\\\\n /${c2}/   ${c1}\\\\  \\\\\\n/${c2}//    ${c1}\\\\  \\\\\\n${c2}//      ${c1}\\\\  \\\\\\n         \\\\\\n";
// alpine_small: 7 lines, done at line 5348.
// Logo #3: Alpine...
static const char Alpine[] = "${c1}       .hddddddddddddddddddddddh.\\n      :dddddddddddddddddddddddddd:\\n     /dddddddddddddddddddddddddddd/\\n    +dddddddddddddddddddddddddddddd+\\n  `sdddddddddddddddddddddddddddddddds`\\n `ydddddddddddd++hdddddddddddddddddddy`\\n.hddddddddddd+`  `+ddddh:-sdddddddddddh.\\nhdddddddddd+`      `+y:    .sddddddddddh\\nddddddddh+`   `//`   `.`     -sddddddddd\\nddddddh+`   `/hddh/`   `:s-    -sddddddd\\nddddh+`   `/+/dddddh/`   `+s-    -sddddd\\nddd+`   `/o` :dddddddh/`   `oy-    .yddd\\nhdddyo+ohddyosdddddddddho+oydddy++ohdddh\\n.hddddddddddddddddddddddddddddddddddddh.\\n `yddddddddddddddddddddddddddddddddddy`\\n  `sdddddddddddddddddddddddddddddddds`\\n    +dddddddddddddddddddddddddddddd+\\n     /dddddddddddddddddddddddddddd/\\n      :dddddddddddddddddddddddddd:\\n       .hddddddddddddddddddddddh.\\n";
// Alpine: 21 lines, done at line 5374.
// Logo #4: Alter...
static const char Alter[] = "${c1}                      %,\\n                    ^WWWw\\n                   'wwwwww\\n                  !wwwwwwww\\n                 #`wwwwwwwww\\n                @wwwwwwwwwwww\\n               wwwwwwwwwwwwwww\\n              wwwwwwwwwwwwwwwww\\n             wwwwwwwwwwwwwwwwwww\\n            wwwwwwwwwwwwwwwwwwww,\\n           w~1i.wwwwwwwwwwwwwwwww,\\n         3~:~1lli.wwwwwwwwwwwwwwww.\\n        :~~:~?ttttzwwwwwwwwwwwwwwww\\n       #<~:~~~~?llllltO-.wwwwwwwwwww\\n      #~:~~:~:~~?ltlltlttO-.wwwwwwwww\\n     @~:~~:~:~:~~(zttlltltlOda.wwwwwww\\n    @~:~~: ~:~~:~:(zltlltlO    a,wwwwww\\n   8~~:~~:~~~~:~~~~_1ltltu          ,www\\n  5~~:~~:~~:~~:~~:~~~_1ltq             N,,\\n g~:~~:~~~:~~:~~:~:~~~~1q                N,\\n";
// Alter: 21 lines, done at line 5400.
// Logo #5: Amazon...
static const char Amazon[] = "${c1}             `-/oydNNdyo:.`\\n      `.:+shmMMMMMMMMMMMMMMmhs+:.`\\n    -+hNNMMMMMMMMMMMMMMMMMMMMMMNNho-\\n.``      -/+shmNNMMMMMMNNmhs+/-      ``.\\ndNmhs+:.       `.:/oo/:.`       .:+shmNd\\ndMMMMMMMNdhs+:..        ..:+shdNMMMMMMMd\\ndMMMMMMMMMMMMMMNds    odNMMMMMMMMMMMMMMd\\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\\ndMMMMMMMMMMMMMMMMh    yMMMMMMMMMMMMMMMMd\\n.:+ydNMMMMMMMMMMMh    yMMMMMMMMMMMNdy+:.\\n     `.:+shNMMMMMh    yMMMMMNhs+:``\\n            `-+shy    shs+:`\\n";
// Amazon: 20 lines, done at line 5425.
// Logo #6: Anarchy...
static const char Anarchy[] = "                         ${c2}..${c1}\\n                        ${c2}..${c1}\\n                      ${c2}:..${c1}\\n                    ${c2}:+++.${c1}\\n              .:::++${c2}++++${c1}+::.\\n          .:+######${c2}++++${c1}######+:.\\n       .+#########${c2}+++++${c1}##########:.\\n     .+##########${c2}+++++++${c1}##${c2}+${c1}#########+.\\n    +###########${c2}+++++++++${c1}############:\\n   +##########${c2}++++++${c1}#${c2}++++${c1}#${c2}+${c1}###########+\\n  +###########${c2}+++++${c1}###${c2}++++${c1}#${c2}+${c1}###########+\\n :##########${c2}+${c1}#${c2}++++${c1}####${c2}++++${c1}#${c2}+${c1}############:\\n ###########${c2}+++++${c1}#####${c2}+++++${c1}#${c2}+${c1}###${c2}++${c1}######+\\n.##########${c2}++++++${c1}#####${c2}++++++++++++${c1}#######.\\n.##########${c2}+++++++++++++++++++${c1}###########.\\n #####${c2}++++++++++++++${c1}###${c2}++++++++${c1}#########+\\n :###${c2}++++++++++${c1}#########${c2}+++++++${c1}#########:\\n  +######${c2}+++++${c1}##########${c2}++++++++${c1}#######+\\n   +####${c2}+++++${c1}###########${c2}+++++++++${c1}#####+\\n    :##${c2}++++++${c1}############${c2}++++++++++${c1}##:\\n     .${c2}++++++${c1}#############${c2}++++++++++${c1}+.\\n      :${c2}++++${c1}###############${c2}+++++++${c1}::\\n     .${c2}++. .:+${c1}##############${c2}+++++++${c1}..\\n     ${c2}.:.${c1}      ..::++++++::..:${c2}++++${c1}+.\\n     ${c2}.${c1}                       ${c2}.:+++${c1}.\\n                                ${c2}.:${c1}:\\n                                   ${c2}..${c1}\\n                                    ${c2}..${c1}\\n";
// Anarchy: 29 lines, done at line 5459.
// Logo #7: android_small...
static const char android_small[] = "${c1}  ;,           ,;\\n   ';,.-----.,;'\\n  ,'           ',\\n /    O     O    \\\\\\n|                 |\\n'-----------------'\\n";
// android_small: 7 lines, done at line 5471.
// Logo #8: Android...
static const char Android[] = "${c1}         -o          o-\\n          +hydNNNNdyh+\\n        +mMMMMMMMMMMMMm+\\n      `dMM${c2}m:${c1}NMMMMMMN${c2}:m${c1}MMd`\\n      hMMMMMMMMMMMMMMMMMMh\\n  ..  yyyyyyyyyyyyyyyyyyyy  ..\\n.mMMm`MMMMMMMMMMMMMMMMMMMM`mMMm.\\n:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:\\n:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:\\n:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:\\n:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:\\n-MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM-\\n +yy+ MMMMMMMMMMMMMMMMMMMM +yy+\\n      mMMMMMMMMMMMMMMMMMMm\\n      `/++MMMMh++hMMMM++/`\\n          MMMMo  oMMMM\\n          MMMMo  oMMMM\\n          oNMm-  -mMNs\\n";
// Android: 19 lines, done at line 5495.
// Logo #9: Antergos...
static const char Antergos[] = "${c2}              `.-/::/-``\\n            .-/osssssssso/.\\n           :osyysssssssyyys+-\\n        `.+yyyysssssssssyyyyy+.\\n       `/syyyyyssssssssssyyyyys-`\\n      `/yhyyyyysss${c1}++${c2}ssosyyyyhhy/`\\n     .ohhhyyyys${c1}o++/+o${c2}so${c1}+${c2}syy${c1}+${c2}shhhho.\\n    .shhhhys${c1}oo++//+${c2}sss${c1}+++${c2}yyy${c1}+s${c2}hhhhs.\\n   -yhhhhs${c1}+++++++o${c2}ssso${c1}+++${c2}yyy${c1}s+o${c2}hhddy:\\n  -yddhhy${c1}o+++++o${c2}syyss${c1}++++${c2}yyy${c1}yooy${c2}hdddy-\\n .yddddhs${c1}o++o${c2}syyyyys${c1}+++++${c2}yyhh${c1}sos${c2}hddddy`\\n`odddddhyosyhyyyyyy${c1}++++++${c2}yhhhyosddddddo\\n.dmdddddhhhhhhhyyyo${c1}+++++${c2}shhhhhohddddmmh.\\nddmmdddddhhhhhhhso${c1}++++++${c2}yhhhhhhdddddmmdy\\ndmmmdddddddhhhyso${c1}++++++${c2}shhhhhddddddmmmmh\\n-dmmmdddddddhhys${c1}o++++o${c2}shhhhdddddddmmmmd-\\n.smmmmddddddddhhhhhhhhhdddddddddmmmms.\\n   `+ydmmmdddddddddddddddddddmmmmdy/.\\n      `.:+ooyyddddddddddddyyso+:.`\\n";
// Antergos: 20 lines, done at line 5520.
// Logo #10: antiX...
static const char antiX[] = "${c1}\\n                    \\\\n         , - ~ ^ ~ - \\        /\\n     , '              \\ ' ,  /\\n   ,                   \\   '/\\n  ,                     \\  / ,\\n ,___,                   \\/   ,\\n /   |   _  _  _|_ o     /\\   ,\\n|,   |  / |/ |  |  |    /  \\  ,\\n \\,_/\\_/  |  |_/|_/|_/_/    \\,\\n   ,                  /     ,\\\\n     ,               /  , '   \\\\n      ' - , _ _ _ ,  '\\n";
// antiX: 14 lines, done at line 5539.
// Logo #11: AOSC OS/Retro...
static const char AOSC_OS_Retro[] = "${c2}          .........\\n     ...................\\n   .....................${c1}################${c2}\\n ..............     ....${c1}################${c2}\\n..............       ...${c1}################${c2}\\n.............         ..${c1}****************${c2}\\n............     .     .${c1}****************${c2}\\n...........     ...     ${c1}................${c2}\\n..........     .....     ${c1}...............${c2}\\n.........     .......     ...\\n .${c3}......                   ${c2}.\\n  ${c3}.....      .....${c2}....    ${c4}...........\\n  ${c3}....      ......${c2}.       ${c4}...........\\n  ${c3}...      .......        ${c4}...........\\n  ${c3}................        ${c4}***********\\n  ${c3}................        ${c4}###########\\n  ${c3}****************\\n  ${c3}################\\n";
// AOSC_OS_Retro: 19 lines, done at line 5563.
// Logo #12: AOSC OS...
static const char AOSC_OS[] = "${c2}             .:+syhhhhys+:.\\n         .ohNMMMMMMMMMMMMMMNho.\\n      `+mMMMMMMMMMMmdmNMMMMMMMMm+`\\n     +NMMMMMMMMMMMM/   `./smMMMMMN+\\n   .mMMMMMMMMMMMMMMo        -yMMMMMm.\\n  :NMMMMMMMMMMMMMMMs          .hMMMMN:\\n .NMMMMhmMMMMMMMMMMm+/-         oMMMMN.\\n dMMMMs  ./ymMMMMMMMMMMNy.       sMMMMd\\n-MMMMN`      oMMMMMMMMMMMN:      `NMMMM-\\n/MMMMh       NMMMMMMMMMMMMm       hMMMM/\\n/MMMMh       NMMMMMMMMMMMMm       hMMMM/\\n-MMMMN`      :MMMMMMMMMMMMy.     `NMMMM-\\n dMMMMs       .yNMMMMMMMMMMMNy/. sMMMMd\\n .NMMMMo         -/+sMMMMMMMMMMMmMMMMN.\\n  :NMMMMh.          .MMMMMMMMMMMMMMMN:\\n   .mMMMMMy-         NMMMMMMMMMMMMMm.\\n     +NMMMMMms/.`    mMMMMMMMMMMMN+\\n      `+mMMMMMMMMNmddMMMMMMMMMMm+`\\n         .ohNMMMMMMMMMMMMMMNho.\\n             .:+syhhhhys+:.\\n";
// AOSC_OS: 21 lines, done at line 5589.
// Logo #13: Apricity...
static const char Apricity[] = "${c2}                                    ./o-\\n          ``...``              `:. -/:\\n     `-+ymNMMMMMNmho-`      :sdNNm/\\n   `+dMMMMMMMMMMMMMMMmo` sh:.:::-\\n  /mMMMMMMMMMMMMMMMMMMMm/`sNd/\\n oMMMMMMMMMMMMMMMMMMMMMMMs -`\\n:MMMMMMMMMMMMMMMMMMMMMMMMM/\\nNMMMMMMMMMMMMMMMMMMMMMMMMMd\\nMMMMMMMmdmMMMMMMMMMMMMMMMMd\\nMMMMMMy` .mMMMMMMMMMMMmho:`\\nMMMMMMNo/sMMMMMMMNdy+-.`-/\\nMMMMMMMMMMMMNdy+:.`.:ohmm:\\nMMMMMMMmhs+-.`.:+ymNMMMy.\\nMMMMMM/`.-/ohmNMMMMMMy-\\nMMMMMMNmNNMMMMMMMMmo.\\nMMMMMMMMMMMMMMMms:`\\nMMMMMMMMMMNds/.\\ndhhyys+/-`\\n";
// Apricity: 19 lines, done at line 5613.
// Logo #14: arcolinux_small...
static const char arcolinux_small[] = "${c2}          A\\n         ooo\\n        ooooo\\n       ooooooo\\n      ooooooooo\\n     ooooo ooooo\\n    ooooo   ooooo\\n   ooooo     ooooo\\n  ooooo  ${c1}<oooooooo>${c2}\\n ooooo      ${c1}<oooooo>${c2}\\nooooo          ${c1}<oooo>${c2}\\n";
// arcolinux_small: 12 lines, done at line 5630.
// Logo #15: ArcoLinux...
static const char ArcoLinux[] = "${c2}                    /-\\n                   ooo:\\n                  yoooo/\\n                 yooooooo\\n                yooooooooo\\n               yooooooooooo\\n             .yooooooooooooo\\n            .oooooooooooooooo\\n           .oooooooarcoooooooo\\n          .ooooooooo-oooooooooo\\n         .ooooooooo-  oooooooooo\\n        :ooooooooo.    :ooooooooo\\n       :ooooooooo.      :ooooooooo\\n      :oooarcooo         .oooarcooo\\n     :ooooooooy           .ooooooooo\\n    :ooooooooo   ${c1}/ooooooooooooooooooo${c2}\\n   :ooooooooo      ${c1}.-ooooooooooooooooo.${c2}\\n  ooooooooo-             ${c1}-ooooooooooooo.${c2}\\n ooooooooo-                 ${c1}.-oooooooooo.${c2}\\nooooooooo.                     ${c1}-ooooooooo${c2}\\n";
// ArcoLinux: 21 lines, done at line 5656.
// Logo #16: arch_small...
static const char arch_small[] = "${c1}      /\\\\\\n     /  \\\\\\n    /\\\\   \\\\\\n${c2}   /      \\\\\\n  /   ,,   \\\\\\n /   |  |  -\\\\\\n/_-''    ''-_\\\\\\n";
// arch_small: 8 lines, done at line 5669.
// Logo #17: arch_old...
static const char arch_old[] = "${c1}             __\\n         _=(SDGJT=_\\n       _GTDJHGGFCVS)\\n      ,GTDJGGDTDFBGX0\\n${c1}     JDJDIJHRORVFSBSVL${c2}-=+=,_\\n${c1}    IJFDUFHJNXIXCDXDSV,${c2}  \"DEBL\\n${c1}   [LKDSDJTDU=OUSCSBFLD.${c2}   '?ZWX,\\n${c1}  ,LMDSDSWH'     `DCBOSI${c2}     DRDS],\\n${c1}  SDDFDFH'         !YEWD,${c2}   )HDROD\\n${c1} !KMDOCG            &GSU|${c2}\\_GFHRGO\\'\\n${c1} HKLSGP'${c2}           __${c1}\\TKM0${c2}\\GHRBV)'\\n${c1}JSNRVW'${c2}       __+MNAEC${c1}\\IOI,${c2}\\BN'\\n${c1}HELK['${c2}    __,=OFFXCBGHC${c1}\\FD)\\n${c1}?KGHE ${c2}\\_-#DASDFLSV='${c1}    'EF\\n'EHTI                    !H\\n `0F'                    '!\\n";
// arch_old: 17 lines, done at line 5691.
// Logo #18: ArchBox...
static const char ArchBox[] = "${c1}              ...:+oh/:::..\\n         ..-/oshhhhhh`   `::::-.\\n     .:/ohhhhhhhhhhhh`        `-::::.\\n .+shhhhhhhhhhhhhhhhh`             `.::-.\\n /`-:+shhhhhhhhhhhhhh`            .-/+shh\\n /      .:/ohhhhhhhhh`       .:/ohhhhhhhh\\n /           `-:+shhh`  ..:+shhhhhhhhhhhh\\n /                 .:ohhhhhhhhhhhhhhhhhhh\\n /                  `hhhhhhhhhhhhhhhhhhhh\\n /                  `hhhhhhhhhhhhhhhhhhhh\\n /                  `hhhhhhhhhhhhhhhhhhhh\\n /                  `hhhhhhhhhhhhhhhhhhhh\\n /      .+o+        `hhhhhhhhhhhhhhhhhhhh\\n /     -hhhhh       `hhhhhhhhhhhhhhhhhhhh\\n /     ohhhhho      `hhhhhhhhhhhhhhhhhhhh\\n /:::+`hhhhoos`     `hhhhhhhhhhhhhhhhhs+`\\n    `--/:`   /:     `hhhhhhhhhhhho/-\\n             -/:.   `hhhhhhs+:-`\\n                ::::/ho/-`\\n";
// ArchBox: 20 lines, done at line 5716.
// Logo #19: ARCHlabs...
static const char ARCHlabs[] = "${c1}                     'c'\\n                    'kKk,\\n                   .dKKKx.\\n                  .oKXKXKd.\\n                 .l0XXXXKKo.\\n                 c0KXXXXKX0l.\\n                :0XKKOxxOKX0l.\\n               :OXKOc. .c0XX0l.\\n              :OK0o. ${c4}...${c1}'dKKX0l.\\n             :OX0c  ${c4};xOx'${c1}'dKXX0l.\\n            :0KKo.${c4}.o0XXKd'.${c1}lKXX0l.\\n           c0XKd.${c4}.oKXXXXKd..${c1}oKKX0l.\\n         .c0XKk;${c4}.l0K0OO0XKd..${c1}oKXXKo.\\n        .l0XXXk:${c4},dKx,.'l0XKo.${c1}.kXXXKo.\\n       .o0XXXX0d,${c4}:x;   .oKKx'${c1}.dXKXXKd.\\n      .oKXXXXKK0c.${c4};.    :00c'${c1}cOXXXXXKd.\\n     .dKXXXXXXXXk,${c4}.     cKx'${c1}'xKXXXXXXKx'\\n    'xKXXXXK0kdl:.     ${c4}.ok; ${c1}.cdk0KKXXXKx'\\n   'xKK0koc,..         ${c4}'c, ${c1}    ..,cok0KKk,\\n  ,xko:'.             ${c4}.. ${c1}           .':okx;\\n .,'.                                   .',.\\n";
// ARCHlabs: 22 lines, done at line 5743.
// Logo #20: ArchStrike...
static const char ArchStrike[] = "${c1}                   *   \\n                  **.\\n                 ****\\n                ******\\n                *******\\n              ** *******\\n             **** *******\\n            ${c1}****${c2}_____${c1}***${c2}/${c1}*\\n           ***${c2}/${c1}*******${c2}//${c1}***\\n          **${c2}/${c1}********${c2}///${c1}*${c2}/${c1}**\\n         **${c2}/${c1}*******${c2}////${c1}***${c2}/${c1}**\\n        **${c2}/${c1}****${c2}//////.,${c1}****${c2}/${c1}**\\n       ***${c2}/${c1}*****${c2}/////////${c1}**${c2}/${c1}***\\n      ****${c2}/${c1}****    ${c2}/////${c1}***${c2}/${c1}****\\n     ******${c2}/${c1}***  ${c2}////   ${c1}**${c2}/${c1}******\\n    ********${c2}/${c1}* ${c2}///      ${c1}*${c2}/${c1}********\\n  ,******     ${c2}// ______ /    ${c1}******,\\n";
// ArchStrike: 18 lines, done at line 5766.
// Logo #21: XFerience...
static const char XFerience[] = "${c1}           ``--:::::::-.`\\n        .-/+++ooooooooo+++:-`\\n     `-/+oooooooooooooooooo++:.\\n    -/+oooooo/+ooooooooo+/ooo++:`\\n  `/+oo++oo.   .+oooooo+.-: +:-o+-\\n `/+o/.  -o.    :oooooo+ ```:.+oo+-\\n`:+oo-    -/`   :oooooo+ .`-`+oooo/.\\n.+ooo+.    .`   `://///+-+..oooooo+:`\\n-+ooo:`                ``.-+oooooo+/`\\n-+oo/`                       :+oooo/.\\n.+oo:            ..-/. .      -+oo+/`\\n`/++-         -:::++::/.      -+oo+-\\n ./o:          `:///+-     `./ooo+:`\\n  .++-         `` /-`   -:/+oooo+:`\\n   .:+/:``          `-:ooooooo++-\\n     ./+o+//:...../+oooooooo++:`\\n       `:/++ooooooooooooo++/-`\\n          `.-//++++++//:-.`\\n               ``````\\n";
// XFerience: 20 lines, done at line 5791.
// Logo #22: ArchMerge...
static const char ArchMerge[] = "${c1}                    y:\\n                  sMN-\\n                 +MMMm`\\n                /MMMMMd`\\n               :NMMMMMMy\\n              -NMMMMMMMMs\\n             .NMMMMMMMMMM+\\n            .mMMMMMMMMMMMM+\\n            oNMMMMMMMMMMMMM+\\n          `+:-+NMMMMMMMMMMMM+\\n          .sNMNhNMMMMMMMMMMMM/\\n        `hho/sNMMMMMMMMMMMMMMM/\\n       `.`omMMmMMMMMMMMMMMMMMMM+\\n      .mMNdshMMMMd+::oNMMMMMMMMMo\\n     .mMMMMMMMMM+     `yMMMMMMMMMs\\n    .NMMMMMMMMM/        yMMMMMMMMMy\\n   -NMMMMMMMMMh         `mNMMMMMMMMd`\\n  /NMMMNds+:.`             `-/oymMMMm.\\n +Mmy/.                          `:smN:\\n/+.                                  -o.\\n";
// ArchMerge: 21 lines, done at line 5817.
// Logo #23: Arch...
static const char Arch[] = "${c1}                   -`\\n                  .o+`\\n                 `ooo/\\n                `+oooo:\\n               `+oooooo:\\n               -+oooooo+:\\n             `/:-:++oooo+:\\n            `/++++/+++++++:\\n           `/++++++++++++++:\\n          `/+++o${c2}oooooooo${c1}oooo/`\\n${c2}         ${c1}./${c2}ooosssso++osssssso${c1}+`\\n${c2}        .oossssso-````/ossssss+`\\n       -osssssso.      :ssssssso.\\n      :osssssss/        osssso+++.\\n     /ossssssss/        +ssssooo/-\\n   `/ossssso+/:-        -:/+osssso+-\\n  `+sso+:-`                 `.-/+oso:\\n `++:.                           `-/+/\\n .`                                 `/\\n";
// Arch: 20 lines, done at line 5842.
// Logo #24: artix_small...
static const char artix_small[] = "${c1}      /\\\\\\n     /  \\\\\\n    /`'.,\\\\\\n   /     ',\\n  /      ,`\\\\\\n /   ,.'`.  \\\\\\n/.,'`     `'.\\\\\\n";
// artix_small: 8 lines, done at line 5855.
// Logo #25: Artix...
static const char Artix[] = "${c1}                   '\\n                  'o'\\n                 'ooo'\\n                'ooxoo'\\n               'ooxxxoo'\\n              'oookkxxoo'\\n             'oiioxkkxxoo'\\n            ':;:iiiioxxxoo'\\n               `'.;::ioxxoo'\\n          '-.      `':;jiooo'\\n         'oooio-..     `'i:io'\\n        'ooooxxxxoio:,.   `'-;'\\n       'ooooxxxxxkkxoooIi:-.  `'\\n      'ooooxxxxxkkkkxoiiiiiji'\\n     'ooooxxxxxkxxoiiii:'`     .i'\\n    'ooooxxxxxoi:::'`       .;ioxo'\\n   'ooooxooi::'`         .:iiixkxxo'\\n  'ooooi:'`                `'';ioxxo'\\n 'i:'`                          '':io'\\n'`                                   `'\\n";
// Artix: 21 lines, done at line 5881.
// Logo #26: Arya...
static const char Arya[] = "${c1}                `oyyy/${c2}-yyyyyy+\\n${c1}               -syyyy/${c2}-yyyyyy+\\n${c1}              .syyyyy/${c2}-yyyyyy+\\n${c1}              :yyyyyy/${c2}-yyyyyy+\\n${c1}           `/ :yyyyyy/${c2}-yyyyyy+\\n${c1}          .+s :yyyyyy/${c2}-yyyyyy+\\n${c1}         .oys :yyyyyy/${c2}-yyyyyy+\\n${c1}        -oyys :yyyyyy/${c2}-yyyyyy+\\n${c1}       :syyys :yyyyyy/${c2}-yyyyyy+\\n${c1}      /syyyys :yyyyyy/${c2}-yyyyyy+\\n${c1}     +yyyyyys :yyyyyy/${c2}-yyyyyy+\\n${c1}   .oyyyyyyo. :yyyyyy/${c2}-yyyyyy+ ---------\\n${c1}  .syyyyyy+`  :yyyyyy/${c2}-yyyyy+-+syyyyyyyy\\n${c1} -syyyyyy/    :yyyyyy/${c2}-yyys:.syyyyyyyyyy\\n${c1}:syyyyyy/     :yyyyyy/${c2}-yyo.:syyyyyyyyyyy\\n";
// Arya: 16 lines, done at line 5902.
// Logo #27: Bedrock...
static const char Bedrock[] = "${c1}--------------------------------------\\n--------------------------------------\\n--------------------------------------\\n---${c2}\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\${c1}-----------------------\\n----${c2}\\\\\\\\\\\\      \\\\\\\\\\\\${c1}----------------------\\n-----${c2}\\\\\\\\\\\\      \\\\\\\\\\\\${c1}---------------------\\n------${c2}\\\\\\\\\\\\      \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\${c1}------\\n-------${c2}\\\\\\\\\\\\                    \\\\\\\\\\\\${c1}-----\\n--------${c2}\\\\\\\\\\\\                    \\\\\\\\\\\\${c1}----\\n---------${c2}\\\\\\\\\\\\        ______      \\\\\\\\\\\\${c1}---\\n----------${c2}\\\\\\\\\\\\                   ///${c1}---\\n-----------${c2}\\\\\\\\\\\\                 ///${c1}----\\n------------${c2}\\\\\\\\\\\\               ///${c1}-----\\n-------------${c2}\\\\\\\\\\\\////////////////${c1}------\\n--------------------------------------\\n--------------------------------------\\n--------------------------------------\\n";
// Bedrock: 18 lines, done at line 5925.
// Logo #28: Bitrig...
static const char Bitrig[] = "${c1}   `hMMMMN+\\n   -MMo-dMd`\\n   oMN- oMN`\\n   yMd  /NM:\\n  .mMmyyhMMs\\n  :NMMMhsmMh\\n  +MNhNNoyMm-\\n  hMd.-hMNMN:\\n  mMmsssmMMMo\\n .MMdyyhNMMMd\\n oMN.`/dMddMN`\\n yMm/hNm+./MM/\\n.dMMMmo.``.NMo\\n:NMMMNmmmmmMMh\\n/MN/-------oNN:\\nhMd.       .dMh\\nsm/         /ms\\n";
// Bitrig: 18 lines, done at line 5948.
// Logo #29: BlackArch...
static const char BlackArch[] = "${c3}                   00\\n                   11\\n                  ====${c1}\\n                  .${c3}//${c1}\\n                 `o${c3}//${c1}:\\n                `+o${c3}//${c1}o:\\n               `+oo${c3}//${c1}oo:\\n               -+oo${c3}//${c1}oo+:\\n             `/:-:+${c3}//${c1}ooo+:\\n            `/+++++${c3}//${c1}+++++:\\n           `/++++++${c3}//${c1}++++++:\\n          `/+++o${c2}ooo${c3}//${c2}ooo${c1}oooo/`\\n${c2}         ${c1}./${c2}ooosssso${c3}//${c2}osssssso${c1}+`\\n${c2}        .oossssso-`${c3}//${c1}`/ossssss+`\\n       -osssssso.  ${c3}//${c1}  :ssssssso.\\n      :osssssss/   ${c3}//${c1}   osssso+++.\\n     /ossssssss/   ${c3}//${c1}   +ssssooo/-\\n   `/ossssso+/:-   ${c3}//${c1}   -:/+osssso+-\\n  `+sso+:-`        ${c3}//${c1}       `.-/+oso:\\n `++:.             ${c3}//${c1}            `-/+/\\n .`                ${c3}/${c1}                `/\\n";
// BlackArch: 22 lines, done at line 5975.
// Logo #30: BLAG...
static const char BLAG[] = "${c1}             d\\n            ,MK:\\n            xMMMX:\\n           .NMMMMMX;\\n           lMMMMMMMM0clodkO0KXWW:\\n           KMMMMMMMMMMMMMMMMMMX'\\n      .;d0NMMMMMMMMMMMMMMMMMMK.\\n .;dONMMMMMMMMMMMMMMMMMMMMMMx\\n'dKMMMMMMMMMMMMMMMMMMMMMMMMl\\n   .:xKWMMMMMMMMMMMMMMMMMMM0.\\n       .:xNMMMMMMMMMMMMMMMMMK.\\n          lMMMMMMMMMMMMMMMMMMK.\\n          ,MMMMMMMMWkOXWMMMMMM0\\n          .NMMMMMNd.     `':ldko\\n           OMMMK:\\n           oWk,\\n           ;:\\n";
// BLAG: 18 lines, done at line 5998.
// Logo #31: BlankOn...
static const char BlankOn[] = "${c2}        `./ohdNMMMMNmho+.` ${c1}       .+oo:`\\n${c2}      -smMMMMMMMMMMMMMMMMmy-`    ${c1}`yyyyy+\\n${c2}   `:dMMMMMMMMMMMMMMMMMMMMMMd/`  ${c1}`yyyyys\\n${c2}  .hMMMMMMMNmhso/++symNMMMMMMMh- ${c1}`yyyyys\\n${c2} -mMMMMMMms-`         -omMMMMMMN-${c1}.yyyyys\\n${c2}.mMMMMMMy.              .yMMMMMMm:${c1}yyyyys\\n${c2}sMMMMMMy                 `sMMMMMMh${c1}yyyyys\\n${c2}NMMMMMN:                  .NMMMMMN${c1}yyyyys\\n${c2}MMMMMMm.                   NMMMMMN${c1}yyyyys\\n${c2}hMMMMMM+                  /MMMMMMN${c1}yyyyys\\n${c2}:NMMMMMN:                :mMMMMMM+${c1}yyyyys\\n${c2} oMMMMMMNs-            .sNMMMMMMs.${c1}yyyyys\\n${c2}  +MMMMMMMNho:.`  `.:ohNMMMMMMNo ${c1}`yyyyys\\n${c2}   -hMMMMMMMMNNNmmNNNMMMMMMMMh-  ${c1}`yyyyys\\n${c2}     :yNMMMMMMMMMMMMMMMMMMNy:`   ${c1}`yyyyys\\n${c2}       .:sdNMMMMMMMMMMNds/.      ${c1}`yyyyyo\\n${c2}           `.:/++++/:.`           ${c1}:oys+.\\n";
// BlankOn: 18 lines, done at line 6021.
// Logo #32: BlueLight...
static const char BlueLight[] = "${c1}              oMMNMMMMMMMMMMMMMMMMMMMMMM\\n              oMMMMMMMMMMMMMMMMMMMMMMMMM\\n              oMMMMMMMMMMMMMMMMMMMMMMMMM\\n              oMMMMMMMMMMMMMMMMMMMMMMMMM\\n              -+++++++++++++++++++++++mM${c2}\\n             ```````````````````````..${c1}dM${c2}\\n           ```````````````````````....${c1}dM${c2}\\n         ```````````````````````......${c1}dM${c2}\\n       ```````````````````````........${c1}dM${c2}\\n     ```````````````````````..........${c1}dM${c2}\\n   ```````````````````````............${c1}dM${c2}\\n.::::::::::::::::::::::-..............${c1}dM${c2}\\n `-+yyyyyyyyyyyyyyyyyyyo............${c1}+mMM${c2}\\n     -+yyyyyyyyyyyyyyyyo..........${c1}+mMMMM${c2}\\n        ./syyyyyyyyyyyyo........${c1}+mMMMMMM${c2}\\n           ./oyyyyyyyyyo......${c1}+mMMMMMMMM${c2}\\n              omdyyyyyyo....${c1}+mMMMMMMMMMM${c2}\\n              ${c1}oMMM${c2}mdhyyo..${c1}+mMMMMMMMMMMMM\\n              oNNNNNNm${c2}dso${c1}mMMMMMMMMMMMMMM\\n";
// BlueLight: 20 lines, done at line 6046.
// Logo #33: bonsai...
static const char bonsai[] = "${c2}   ,####,\\n   ${c2}#######,  ${c2},#####,\\n   ${c2}#####',#  ${c2}'######\\n    ${c2}''###'${c3}';,,,'${c2}###'\\n   ${c3}       ,;  ''''\\n   ${c3}      ;;;   ${c2},#####,\\n   ${c3}     ;;;'  ,,;${c2};;###\\n   ${c3}     ';;;;''${c2}'####'\\n   ${c3}      ;;;\\n   ${c3}   ,.;;';'',,,\\n   ${c3}  '     '\\n${c1} #\\n #                        O\\n ##, ,##,',##, ,##  ,#,   ,\\n # # #  # #''# #,,  # #   #\\n '#' '##' #  #  ,,# '##;, #\\n";
// bonsai: 17 lines, done at line 6068.
// Logo #34: BSD...
static const char BSD[] = "${c1}             ,        ,\\n            /(        )`\\n            \\ \\___   / |\\n            /- _  `-/  '\\n           (${c2}/\\/ \\ ${c1}\\   /\\\\n           ${c2}/ /   | `    ${c1}\\\\n           ${c3}O O   ${c2}) ${c1}/    |\\n           ${c2}`-^--'${c1}`<     '\\n          (_.)  _  )   /\\n           `.___/`    /\\n             `-----' /\\n${c4}<----.     __ / __   \\\\n${c4}<----|====${c1}O)))${c4}==${c1}) \\) /${c4}====|\\n<----'    ${c1}`--' `.__,' \\\\n             |        |\\n              \\       /       /\\\\n         ${c5}______${c1}( (_  / \\______/\\n       ${c5},'  ,-----'   |\\n       `--{__________)\\n";
// BSD: 20 lines, done at line 6093.
// Logo #35: BunsenLabs...
static const char BunsenLabs[] = "${c1}        `++\\n      -yMMs\\n    `yMMMMN`\\n   -NMMMMMMm.\\n  :MMMMMMMMMN-\\n .NMMMMMMMMMMM/\\n yMMMMMMMMMMMMM/\\n`MMMMMMNMMMMMMMN.\\n-MMMMN+ /mMMMMMMy\\n-MMMm`   `dMMMMMM\\n`MMN.     .NMMMMM.\\n hMy       yMMMMM`\\n -Mo       +MMMMN\\n  /o       +MMMMs\\n           +MMMN`\\n           hMMM:\\n          `NMM/\\n          +MN:\\n          mh.\\n         -/\\n";
// BunsenLabs: 21 lines, done at line 6119.
// Logo #36: Calculate...
static const char Calculate[] = "${c1}                              ......\\n                           ,,+++++++,.\\n                         .,,,....,,,${c2}+**+,,.${c1}\\n                       ............,${c2}++++,,,${c1}\\n                      ...............\\n                    ......,,,........\\n                  .....+*#####+,,,*+.\\n              .....,*###############,..,,,,,,..\\n           ......,*#################*..,,,,,..,,,..\\n         .,,....*####################+***+,,,,...,++,\\n       .,,..,..*#####################*,\\n     ,+,.+*..*#######################.\\n   ,+,,+*+..,########################*\\n.,++++++.  ..+##**###################+\\n.....      ..+##***#################*.\\n           .,.*#*****##############*.\\n           ..,,*********#####****+.\\n     ${c2}.,++*****+++${c1}*****************${c2}+++++,.${c1}\\n      ${c2},++++++**+++++${c1}***********${c2}+++++++++,${c1}\\n     ${c2}.,,,,++++,..  .,,,,,.....,+++,.,,${c1}\\n";
// Calculate: 21 lines, done at line 6145.
// Logo #37: Carbs...
static const char Carbs[] = "${c2}             ..........\\n          ..,;:ccccccc:;'..\\n       ..,clllc:;;;;;:cllc,.\\n      .,cllc,...     ..';;'.\\n     .;lol;..           ..\\n    .,lol;.\\n    .coo:.\\n   .'lol,.\\n   .,lol,.\\n   .,lol,.\\n    'col;.\\n    .:ooc'.\\n    .'col:.\\n     .'cllc'..          .''.\\n      ..:lolc,'.......',cll,.\\n        ..;cllllccccclllc;'.\\n          ...',;;;;;;,,...\\n                .....\\n";
// Carbs: 19 lines, done at line 6168.
// Logo #38: centos_small...
static const char centos_small[] = "${c2} ____${c1}^${c4}____\\n${c2} |\\\\  ${c1}|${c4}  /|\\n${c2} | \\\\ ${c1}|${c4} / |\\n${c4}<---- ${c3}---->\\n${c3} | / ${c2}|${c1} \\\\ |\\n${c3} |/__${c2}|${c1}__\\\\|\\n${c2}     v\\n";
// centos_small: 8 lines, done at line 6181.
// Logo #39: CentOS...
static const char CentOS[] = "${c1}                 ..\\n               .PLTJ.\\n              <><><><>\\n     ${c2}KKSSV' 4KKK ${c1}LJ${c4} KKKL.'VSSKK\\n     ${c2}KKV' 4KKKKK ${c1}LJ${c4} KKKKAL 'VKK\\n     ${c2}V' ' 'VKKKK ${c1}LJ${c4} KKKKV' ' 'V\\n     ${c2}.4MA.' 'VKK ${c1}LJ${c4} KKV' '.4Mb.\\n${c4}   . ${c2}KKKKKA.' 'V ${c1}LJ${c4} V' '.4KKKKK ${c3}.\\n${c4} .4D ${c2}KKKKKKKA.'' ${c1}LJ${c4} ''.4KKKKKKK ${c3}FA.\\n${c4}<QDD ++++++++++++  ${c3}++++++++++++ GFD>\\n${c4} 'VD ${c3}KKKKKKKK'.. ${c2}LJ ${c1}..'KKKKKKKK ${c3}FV\\n${c4}   ' ${c3}VKKKKK'. .4 ${c2}LJ ${c1}K. .'KKKKKV ${c3}'\\n     ${c3} 'VK'. .4KK ${c2}LJ ${c1}KKA. .'KV'\\n     ${c3}A. . .4KKKK ${c2}LJ ${c1}KKKKA. . .4\\n     ${c3}KKA. 'KKKKK ${c2}LJ ${c1}KKKKK' .4KK\\n     ${c3}KKSSA. VKKK ${c2}LJ ${c1}KKKV .4SSKK\\n${c2}              <><><><>\\n               'MKKM'\\n                 ''\\n";
// CentOS: 20 lines, done at line 6206.
// Logo #40: Chakra...
static const char Chakra[] = "${c1}     _ _ _        \"kkkkkkkk.\\n   ,kkkkkkkk.,    'kkkkkkkkk,\\n   ,kkkkkkkkkkkk., 'kkkkkkkkk.\\n  ,kkkkkkkkkkkkkkkk,'kkkkkkkk,\\n ,kkkkkkkkkkkkkkkkkkk'kkkkkkk.\\n  \"''\"''',;::,,\"''kkk''kkkkk;   __\\n      ,kkkkkkkkkk, \"k''kkkkk' ,kkkk\\n    ,kkkkkkk' ., ' .: 'kkkk',kkkkkk\\n  ,kkkkkkkk'.k'   ,  ,kkkk;kkkkkkkkk\\n ,kkkkkkkk';kk 'k  \"'k',kkkkkkkkkkkk\\n.kkkkkkkkk.kkkk.'kkkkkkkkkkkkkkkkkk'\\n;kkkkkkkk''kkkkkk;'kkkkkkkkkkkkk''\\n'kkkkkkk; 'kkkkkkkk.,\"\"''\"''\"\"\\n  ''kkkk;  'kkkkkkkkkk.,\\n     ';'    'kkkkkkkkkkkk.,\\n             ';kkkkkkkkkk'\\n               ';kkkkkk'\\n                  \"''\"\\n";
// Chakra: 19 lines, done at line 6230.
// Logo #41: ChaletOS...
static const char ChaletOS[] = "${c1}             `.//+osso+/:``\\n         `/sdNNmhyssssydmNNdo:`\\n       :hNmy+-`          .-+hNNs-\\n     /mMh/`       `+:`       `+dMd:\\n   .hMd-        -sNNMNo.  /yyy  /mMs`\\n  -NM+       `/dMd/--omNh::dMM   `yMd`\\n .NN+      .sNNs:/dMNy:/hNmo/s     yMd`\\n hMs    `/hNd+-smMMMMMMd+:omNy-    `dMo\\n:NM.  .omMy:/hNMMMMMMMMMMNy:/hMd+`  :Md`\\n/Md` `sm+.omMMMMMMMMMMMMMMMMd/-sm+  .MN:\\n/Md`      MMMMMMMMMMMMMMMMMMMN      .MN:\\n:NN.      MMMMMMm....--NMMMMMN      -Mm.\\n`dMo      MMMMMMd      mMMMMMN      hMs\\n -MN:     MMMMMMd      mMMMMMN     oMm`\\n  :NM:    MMMMMMd      mMMMMMN    +Mm-\\n   -mMy.  mmmmmmh      dmmmmmh  -hMh.\\n     oNNs-                    :yMm/\\n      .+mMdo:`            `:smMd/`\\n         -ohNNmhsoo++osshmNNh+.\\n            `./+syyhhyys+:``\\n";
// ChaletOS: 21 lines, done at line 6256.
// Logo #42: Chapeau...
static const char Chapeau[] = "${c1}               .-/-.\\n            ////////.\\n          ////////${c2}y+${c1}//.\\n        ////////${c2}mMN${c1}/////.\\n      ////////${c2}mMN+${c1}////////.\\n    ////////////////////////.\\n  /////////+${c2}shhddhyo${c1}+////////.\\n ////////${c2}ymMNmdhhdmNNdo${c1}///////.\\n///////+${c2}mMms${c1}////////${c2}hNMh${c1}///////.\\n///////${c2}NMm+${c1}//////////${c2}sMMh${c1}///////\\n//////${c2}oMMNmmmmmmmmmmmmMMm${c1}///////\\n//////${c2}+MMmssssssssssssss+${c1}///////\\n`//////${c2}yMMy${c1}////////////////////\\n `//////${c2}smMNhso++oydNm${c1}////////\\n  `///////${c2}ohmNMMMNNdy+${c1}///////\\n    `//////////${c2}++${c1}//////////\\n       `////////////////.\\n           -////////-\\n";
// Chapeau: 19 lines, done at line 6280.
// Logo #43: Chrom...
static const char Chrom[] = "${c2}            .,:loool:,.\\n        .,coooooooooooooc,.\\n     .,lllllllllllllllllllll,.\\n    ;ccccccccccccccccccccccccc;\\n${c1}  '${c2}ccccccccccccccccccccccccccccc.\\n${c1} ,oo${c2}c::::::::okO${c5}000${c3}0OOkkkkkkkkkkk:\\n${c1}.ooool${c2};;;;:x${c5}K0${c4}kxxxxxk${c5}0X${c3}K0000000000.\\n${c1}:oooool${c2};,;O${c5}K${c4}ddddddddddd${c5}KX${c3}000000000d\\n${c1}lllllool${c2};l${c5}N${c4}dllllllllllld${c5}N${c3}K000000000\\n${c1}lllllllll${c2}o${c5}M${c4}dccccccccccco${c5}W${c3}K000000000\\n${c1};cllllllllX${c5}X${c4}c:::::::::c${c5}0X${c3}000000000d\\n${c1}.ccccllllllO${c5}Nk${c4}c;,,,;cx${c5}KK${c3}0000000000.\\n${c1} .cccccclllllxOO${c5}OOO${c1}Okx${c3}O0000000000;\\n${c1}  .:ccccccccllllllllo${c3}O0000000OOO,\\n${c1}    ,:ccccccccclllcd${c3}0000OOOOOOl.\\n${c1}      '::ccccccccc${c3}dOOOOOOOkx:.\\n${c1}        ..,::cccc${c3}xOOOkkko;.\\n${c1}            ..,:${c3}dOkxl:.\\n";
// Chrom: 19 lines, done at line 6304.
// Logo #44: cleanjaro_small...
static const char cleanjaro_small[] = "${c1}█████ ██████████\\n█████ ██████████\\n█████\\n█████\\n█████\\n████████████████\\n████████████████\\n";
// cleanjaro_small: 8 lines, done at line 6317.
// Logo #45: Cleanjaro...
static const char Cleanjaro[] = "${c1}███████▌ ████████████████\\n███████▌ ████████████████\\n███████▌ ████████████████\\n███████▌\\n███████▌\\n███████▌\\n███████▌\\n███████▌\\n█████████████████████████\\n█████████████████████████\\n█████████████████████████\\n▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀\\n";
// Cleanjaro: 13 lines, done at line 6335.
// Logo #46: ClearOS...
static const char ClearOS[] = "${c1}             `.--::::::--.`\\n         .-:////////////////:-.\\n      `-////////////////////////-`\\n     -////////////////////////////-\\n   `//////////////-..-//////////////`\\n  ./////////////:      ://///////////.\\n `//////:..-////:      :////-..-//////`\\n ://////`    -///:.``.:///-`    ://///:\\n`///////:.     -////////-`    `:///////`\\n.//:--////:.     -////-`    `:////--://.\\n./:    .////:.     --`    `:////-    :/.\\n`//-`    .////:.        `:////-    `-//`\\n :///-`    .////:.    `:////-    `-///:\\n `/////-`    -///:    :///-    `-/////`\\n  `//////-   `///:    :///`   .//////`\\n   `:////:   `///:    :///`   -////:`\\n     .://:   `///:    :///`   -//:.\\n       .::   `///:    :///`   -:.\\n             `///:    :///`\\n              `...    ...`\\n";
// ClearOS: 21 lines, done at line 6361.
// Logo #47: Clear_Linux...
static const char Clear_Linux[] = "${c1}          BBB\\n       BBBBBBBBB\\n     BBBBBBBBBBBBBBB\\n   BBBBBBBBBBBBBBBBBBBB\\n   BBBBBBBBBBB         BBB\\n  BBBBBBBB${c2}YYYYY\\n${c1}  BBBBBBBB${c2}YYYYYY\\n${c1}  BBBBBBBB${c2}YYYYYYY\\n${c1}  BBBBBBBBB${c2}YYYYY${c3}W\\n${c4} GG${c1}BBBBBBBY${c2}YYYY${c3}WWW\\n${c4} GGG${c1}BBBBBBB${c2}YY${c3}WWWWWWWW\\n${c4} GGGGGG${c1}BBBBBB${c3}WWWWWWWW\\n${c4} GGGGGGGG${c1}BBBB${c3}WWWWWWWW\\n${c4}GGGGGGGGGGG${c1}BBB${c3}WWWWWWW\\n${c4}GGGGGGGGGGGGG${c1}B${c3}WWWWWW\\n${c4}GGGGGGGG${c3}WWWWWWWWWWW\\n${c4}GG${c3}WWWWWWWWWWWWWWWW\\n WWWWWWWWWWWWWWWW\\n      WWWWWWWWWW\\n          WWW\\n";
// Clear_Linux: 21 lines, done at line 6387.
// Logo #48: Clover...
static const char Clover[] = "${c1}               `omo``omo`\\n             `oNMMMNNMMMNo`\\n           `oNMMMMMMMMMMMMNo`\\n          oNMMMMMMMMMMMMMMMMNo\\n          `sNMMMMMMMMMMMMMMNs`\\n     `omo`  `sNMMMMMMMMMMNs`  `omo`\\n   `oNMMMNo`  `sNMMMMMMNs`  `oNMMMNo`\\n `oNMMMMMMMNo`  `oNMMNs`  `oNMMMMMMMNo`\\noNMMMMMMMMMMMNo`  `sy`  `oNMMMMMMMMMMMNo\\n`sNMMMMMMMMMMMMNo.${c2}oNNs${c1}.oNMMMMMMMMMMMMNs`\\n`oNMMMMMMMMMMMMNs.${c2}oNNs${c1}.oNMMMMMMMMMMMMNo`\\noNMMMMMMMMMMMNs`  `sy`  `oNMMMMMMMMMMMNo\\n `oNMMMMMMMNs`  `oNMMNo`  `oNMMMMMMMNs`\\n   `oNMMMNs`  `sNMMMMMMNs`  `oNMMMNs`\\n     `oNs`  `sNMMMMMMMMMMNs`  `oNs`\\n          `sNMMMMMMMMMMMMMMNs`\\n          +NMMMMMMMMMMMMMMMMNo\\n           `oNMMMMMMMMMMMMNo`\\n             `oNMMMNNMMMNs`\\n               `omo``oNs`\\n";
// Clover: 21 lines, done at line 6413.
// Logo #49: Condres...
static const char Condres[] = "${c1}syyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy+${c3}.+.\\n${c1}`oyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy+${c3}:++.\\n${c2}/o${c1}+oyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy/${c3}oo++.\\n${c2}/y+${c1}syyyyyyyyyyyyyyyyyyyyyyyyyyyyy${c3}+ooo++.\\n${c2}/hy+${c1}oyyyhhhhhhhhhhhhhhyyyyyyyyy${c3}+oo+++++.\\n${c2}/hhh+${c1}shhhhhdddddhhhhhhhyyyyyyy${c3}+oo++++++.\\n${c2}/hhdd+${c1}oddddddddddddhhhhhyyyys${c3}+oo+++++++.\\n${c2}/hhddd+${c1}odmmmdddddddhhhhyyyy${c3}+ooo++++++++.\\n${c2}/hhdddmo${c1}odmmmdddddhhhhhyyy${c3}+oooo++++++++.\\n${c2}/hdddmmms${c1}/dmdddddhhhhyyys${c3}+oooo+++++++++.\\n${c2}/hddddmmmy${c1}/hdddhhhhyyyyo${c3}+oooo++++++++++:\\n${c2}/hhdddmmmmy${c1}:yhhhhyyyyy+${c3}+oooo+++++++++++:\\n${c2}/hhddddddddy${c1}-syyyyyys+${c3}ooooo++++++++++++:\\n${c2}/hhhddddddddy${c1}-+yyyy+${c3}/ooooo+++++++++++++:\\n${c2}/hhhhhdddddhhy${c1}./yo:${c3}+oooooo+++++++++++++/\\n${c2}/hhhhhhhhhhhhhy${c1}:-.${c3}+sooooo+++++++++++///:\\n${c2}:sssssssssssso++${c1}${c3}`:/:--------.````````\\n";
// Condres: 18 lines, done at line 6436.
// Logo #50: Container_Linux...
static const char Container_Linux[] = "${c1}                .....\\n          .';:cccccccc:;'.\\n        ':ccccclc${c3}lllllllll${c1}cc:.\\n     .;cccccccc${c3}lllllllllllllll${c1}c,\\n    ;clllccccc${c3}llllllllllllllllll${c1}c,\\n  .cllclccccc${c3}lllll${c2}lll${c3}llllllllllll${c1}c:\\n  ccclclcccc${c3}cllll${c2}kWMMNKk${c3}llllllllll${c1}c:\\n :ccclclcccc${c3}llll${c2}oWMMMMMMWO${c3}lllllllll${c1}c,\\n.ccllllllccc${c3}clll${c2}OMMMMMMMMM0${c3}lllllllll${c1}c\\n.lllllclcccc${c3}llll${c2}KMMMMMMMMMMo${c3}llllllll${c1}c.\\n.lllllllcccc${c3}clll${c2}KMMMMMMMMN0${c3}lllllllll${c1}c.\\n.cclllllcccc${c3}lllld${c2}xkkxxdo${c3}llllllllllc${c1}lc\\n :cccllllllcccc${c3}lllccllllcclccc${c1}cccccc;\\n .ccclllllllcccccccc${c3}lll${c1}ccccclccccccc\\n  .cllllllllllclcccclccclccllllcllc\\n    :cllllllllccclcllllllllllllcc;\\n     .cccccccccccccclcccccccccc:.\\n       .;cccclccccccllllllccc,.\\n          .';ccccclllccc:;..\\n                .....\\n";
// Container_Linux: 21 lines, done at line 6462.
// Logo #51: CRUX...
static const char CRUX[] = "${c1}         odddd\\n      oddxkkkxxdoo\\n     ddcoddxxxdoool\\n     xdclodod  olol\\n     xoc  xdd  olol\\n     xdc  ${c2}k00${c1}Okdlol\\n     xxd${c2}kOKKKOkd${c1}ldd\\n     xdco${c2}xOkdlo${c1}dldd\\n     ddc:cl${c2}lll${c1}oooodo\\n   odxxdd${c3}xkO000kx${c1}ooxdo\\n  oxdd${c3}x0NMMMMMMWW0od${c1}kkxo\\n oooxd${c3}0WMMMMMMMMMW0o${c1}dxkx\\ndocldkXW${c3}MMMMMMMWWN${c1}Odolco\\nxx${c2}dx${c1}kxxOKN${c3}WMMWN${c1}0xdoxo::c\\n${c2}xOkkO${c1}0oo${c3}odOW${c2}WW${c1}XkdodOxc:l\\n${c2}dkkkxkkk${c3}OKX${c2}NNNX0Oxx${c1}xc:cd\\n${c2} odxxdx${c3}xllod${c2}ddooxx${c1}dc:ldo\\n${c2}   lodd${c1}dolccc${c2}ccox${c1}xoloo\\n";
// CRUX: 19 lines, done at line 6499.
// Logo #52: Cucumber...
static const char Cucumber[] = "${c1}           `.-://++++++//:-.`\\n        `:/+//${c2}::--------${c1}:://+/:`\\n      -++/:${c2}----..........----${c1}:/++-\\n    .++:${c2}---...........-......---${c1}:++.\\n   /+:${c2}---....-::/:/--//:::-....---${c1}:+/\\n `++:${c2}--.....:---::/--/::---:.....--${c1}:++`\\n /+:${c2}--.....--.--::::-/::--.--.....--${c1}:+/\\n-o:${c2}--.......-:::://--/:::::-.......--${c1}:o-\\n/+:${c2}--...-:-::---:::..:::---:--:-...--${c1}:+/\\no/:${c2}-...-:.:.-/:::......::/:.--.:-...-${c1}:/o\\no/${c2}--...::-:/::/:-......-::::::-/-...-${c1}:/o\\n/+:${c2}--..-/:/:::--:::..:::--::////-..--${c1}:+/\\n-o:${c2}--...----::/:::/--/:::::-----...--${c1}:o-\\n /+:${c2}--....://:::.:/--/:.::://:....--${c1}:+/\\n `++:${c2}--...-:::.--.:..:.--.:/:-...--${c1}:++`\\n   /+:${c2}---....----:-..-:----....---${c1}:+/\\n    .++:${c2}---..................---${c1}:++.\\n      -/+/:${c2}----..........----${c1}:/+/-\\n        `:/+//${c2}::--------:::${c1}/+/:`\\n           `.-://++++++//:-.`\\n";
// Cucumber: 21 lines, done at line 6525.
// Logo #53: dahlia...
static const char dahlia[] = "${c1}\\n                  .#.\\n                *%@@@%*\\n        .,,,,,(&@@@@@@@&/,,,,,.\\n       ,#@@@@@@@@@@@@@@@@@@@@@#.\\n       ,#@@@@@@@&#///#&@@@@@@@#.\\n     ,/%&@@@@@%/,    .,(%@@@@@&#/.\\n   *#&@@@@@@#,.         .*#@@@@@@&#,\\n .&@@@@@@@@@(            .(@@@@@@@@@&&.\\n#@@@@@@@@@@(               )@@@@@@@@@@@#\\n °@@@@@@@@@@(            .(@@@@@@@@@@@°\\n   *%@@@@@@@(.           ,#@@@@@@@%*\\n     ,(&@@@@@@%*.     ./%@@@@@@%(,\\n       ,#@@@@@@@&(***(&@@@@@@@#.\\n       ,#@@@@@@@@@@@@@@@@@@@@@#.\\n        ,*****#&@@@@@@@&(*****,\\n               ,/%@@@%/.\\n                  ,#,\\n";
// dahlia: 19 lines, done at line 6549.
// Logo #54: debian_small...
static const char debian_small[] = "${c1}  _____\\n /  __ \\\\\\n|  /    |\\n|  \\\\___-\\n-_\\n  --_\\n";
// debian_small: 7 lines, done at line 6561.
// Logo #55: Debian...
static const char Debian[] = "${c2}       _,met$$$$$gg.\\n    ,g$$$$$$$$$$$$$$$P.\\n  ,g$$P\"     \"\"\"Y$$.\".\\n ,$$P'              `$$$.\\n',$$P       ,ggs.     `$$b:\\n`d$$'     ,$P\"'   ${c1}.${c2}    $$$\\n $$P      d$'     ${c1},${c2}    $$P\\n $$:      $$.   ${c1}-${c2}    ,d$$'\\n $$;      Y$b._   _,d$P'\\n Y$$.    ${c1}`.${c2}`\"Y$$$$P\"'\\n${c2} `$$b      ${c1}\"-.__\\n${c2}  `Y$$\\n   `Y$$.\\n     `$$b.\\n       `Y$$b.\\n          `\"Y$b._\\n              `\"\"\"\\n";
// Debian: 18 lines, done at line 6584.
// Logo #56: Deepin...
static const char Deepin[] = "${c1}             ............\\n         .';;;;;.       .,;,.\\n      .,;;;;;;;.       ';;;;;;;.\\n    .;::::::::'     .,::;;,''''',.\\n   ,'.::::::::    .;;'.          ';\\n  ;'  'cccccc,   ,' :: '..        .:\\n ,,    :ccccc.  ;: .c, '' :.       ,;\\n.l.     cllll' ., .lc  :; .l'       l.\\n.c       :lllc  ;cl:  .l' .ll.      :'\\n.l        'looc. .   ,o:  'oo'      c,\\n.o.         .:ool::coc'  .ooo'      o.\\n ::            .....   .;dddo      ;c\\n  l:...            .';lddddo.     ,o\\n   lxxxxxdoolllodxxxxxxxxxc      :l\\n    ,dxxxxxxxxxxxxxxxxxxl.     'o,\\n      ,dkkkkkkkkkkkkko;.    .;o;\\n        .;okkkkkdl;.    .,cl:.\\n            .,:cccccccc:,.\\n";
// Deepin: 19 lines, done at line 6608.
// Logo #57: DesaOS...
static const char DesaOS[] = "${c1}███████████████████████\\n███████████████████████\\n███████████████████████\\n███████████████████████\\n████████               ███████\\n████████               ███████\\n████████               ███████\\n████████               ███████\\n████████               ███████\\n████████               ███████\\n████████               ███████\\n██████████████████████████████\\n██████████████████████████████\\n████████████████████████\\n████████████████████████\\n████████████████████████\\n";
// DesaOS: 17 lines, done at line 6630.
// Logo #58: Devuan...
static const char Devuan[] = "${c1}   ..,,;;;::;,..\\n           `':ddd;:,.\\n                 `'dPPd:,.\\n                     `:b$$b`.\\n                        'P$$$d`\\n                         .$$$$$`\\n                         ;$$$$$P\\n                      .:P$$$$$$`\\n                  .,:b$$$$$$$;'\\n             .,:dP$$$$$$$$b:'\\n      .,:;db$$$$$$$$$$Pd'`\\n ,db$$$$$$$$$$$$$$b:'`\\n:$$$$$$$$$$$$b:'`\\n `$$$$$bd:''`\\n   `'''`\\n";
// Devuan: 16 lines, done at line 6651.
// Logo #59: DracOS...
static const char DracOS[] = "${c1}       `-:/-\\n          -os:\\n            -os/`\\n              :sy+-`\\n               `/yyyy+.\\n                 `+yyyyo-\\n                   `/yyyys:\\n`:osssoooo++-        +yyyyyy/`\\n   ./yyyyyyo         yo`:syyyy+.\\n      -oyyy+         +-   :yyyyyo-\\n        `:sy:        `.    `/yyyyys:\\n           ./o/.`           .oyyso+oo:`\\n              :+oo+//::::///:-.`     `.`\\n";
// DracOS: 14 lines, done at line 6670.
// Logo #60: DarkOs...
static const char DarkOs[] = "\\n${c3}⠀⠀⠀⠀  ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⠢⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c1}⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣶⠋⡆⢹⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c5}⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡆⢀⣤⢛⠛⣠⣿⠀⡏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c6}⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣶⣿⠟⣡⠊⣠⣾⣿⠃⣠⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c2}⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣴⣯⣿⠀⠊⣤⣿⣿⣿⠃⣴⣧⣄⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c1}⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣤⣶⣿⣿⡟⣠⣶⣿⣿⣿⢋⣤⠿⠛⠉⢁⣭⣽⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c4}  ⠀⠀⠀⠀⠀⠀ ⠀⣠⠖⡭⢉⣿⣯⣿⣯⣿⣿⣿⣟⣧⠛⢉⣤⣶⣾⣿⣿⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c5}⠀⠀⠀⠀⠀⠀⠀⠀⣴⣫⠓⢱⣯⣿⢿⠋⠛⢛⠟⠯⠶⢟⣿⣯⣿⣿⣿⣿⣿⣿⣦⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c2}⠀⠀⠀⠀⠀⠀⢀⡮⢁⣴⣿⣿⣿⠖⣠⠐⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠉⠉⠛⠛⠛⢿⣶⣄⠀⠀⠀⠀⠀⠀⠀\\n${c3}⠀⠀⠀⠀⢀⣤⣷⣿⣿⠿⢛⣭⠒⠉⠀⠀⠀⣀⣀⣄⣤⣤⣴⣶⣶⣶⣿⣿⣿⣿⣿⠿⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀\\n${c1}⠀⢀⣶⠏⠟⠝⠉⢀⣤⣿⣿⣶⣾⣿⣿⣿⣿⣿⣿⣟⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c6}⢴⣯⣤⣶⣿⣿⣿⣿⣿⡿⣿⣯⠉⠉⠉⠉⠀⠀⠀⠈⣿⡀⣟⣿⣿⢿⣿⣿⣿⣿⣿⣦⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c5}⠀⠀⠀⠉⠛⣿⣧⠀⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⠃⣿⣿⣯⣿⣦⡀⠀⠉⠻⣿⣦⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c3}⠀⠀⠀⠀⠀⠀⠉⢿⣮⣦⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⣿⠀⣯⠉⠉⠛⢿⣿⣷⣄⠀⠈⢻⣆⠀⠀⠀⠀⠀⠀⠀⠀\\n${c2}⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠢⠀⠀⠀⠀⠀⠀⠀⢀⢡⠃⣾⣿⣿⣦⠀⠀⠀⠙⢿⣿⣤⠀⠙⣄⠀⠀⠀⠀⠀⠀⠀\\n${c6}⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⢋⡟⢠⣿⣿⣿⠋⢿⣄⠀⠀⠀⠈⡄⠙⣶⣈⡄⠀⠀⠀⠀⠀⠀\\n${c1}⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠐⠚⢲⣿⠀⣾⣿⣿⠁⠀⠀⠉⢷⡀⠀⠀⣇⠀⠀⠈⠻⡀⠀⠀⠀⠀⠀\\n${c4}⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢢⣀⣿⡏⠀⣿⡿⠀⠀⠀⠀⠀⠀⠙⣦⠀⢧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c3}⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠿⣧⣾⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⣮⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n${c5}⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠙⠛⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n\\n";
// DarkOs: 23 lines, done at line 6698.
// Logo #61: Itc...
static const char Itc[] = "${c1}....................-==============+...\\n${c1}....................-==============:...\\n${c1}...:===========-....-==============:...\\n${c1}...-===========:....-==============-...\\n${c1}....*==========+........-::********-...\\n${c1}....*===========+.:*====**==*+-.-......\\n${c1}....:============*+-..--:+**====*---...\\n${c1}......::--........................::...\\n${c1}..+-:+-.+::*:+::+:-++::++-.:-.*.:++:++.\\n${c1}..:-:-++++:-::--:+::-::.:++-++:++--:-:.    ⠀⠀⠀⠀⠀\\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\\n";
// Itc: 12 lines, done at line 6715.
// Logo #62: dragonfly_old...
static const char dragonfly_old[] = "     ${c1}                   .-.\\n                 ${c3} ()${c1}I${c3}()\\n            ${c1} \"==.__:-:__.==\"\\n            \"==.__/~|~\\__.==\"\\n            \"==._(  Y  )_.==\"\\n ${c2}.-'~~\"\"~=--...,__${c1}\\/|\\/${c2}__,...--=~\"\"~~'-.\\n(               ..=${c1}\\\\=${c1}/${c2}=..               )\\n `'-.        ,.-\"`;${c1}/=\\\\${c2};\"-.,_        .-'`\\n     `~\"-=-~` .-~` ${c1}|=|${c2} `~-. `~-=-\"~`\\n          .-~`    /${c1}|=|${c2}\\    `~-.\\n       .~`       / ${c1}|=|${c2} \\       `~.\\n   .-~`        .'  ${c1}|=|${c2}  `.        `~-.\\n (`     _,.-=\"`  ${c1}  |=|${c2}    `\"=-.,_     `)\\n  `~\"~\"`        ${c1}   |=|${c2}           `\"~\"~`\\n                 ${c1}  /=\\\\\\n                   \\\\=/\\n                    ^\\n";
// dragonfly_old: 18 lines, done at line 6738.
// Logo #63: dragonfly_small...
static const char dragonfly_small[] = "${c2}   ,${c1}_${c2},\\n('-_${c1}|${c2}_-')\\n >--${c1}|${c2}--<\\n(_-'${c1}|${c2}'-_)\\n    ${c1}|\\n    |\\n    |\\n";
// dragonfly_small: 8 lines, done at line 6751.
// Logo #64: DragonFly...
static const char DragonFly[] = "${c2},--,           ${c1}|           ${c2},--,\\n${c2}|   `-,       ${c1},^,       ${c2},-'   |\\n${c2} `,    `-,   ${c3}(/ \\)   ${c2},-'    ,'\\n${c2}   `-,    `-,${c1}/   \\${c2},-'    ,-'\\n${c2}      `------${c1}(   )${c2}------'\\n${c2}  ,----------${c1}(   )${c2}----------,\\n${c2} |        _,-${c1}(   )${c2}-,_        |\\n${c2}  `-,__,-'   ${c1}\\   /${c2}   `-,__,-'\\n${c1}              | |\\n              | |\\n              | |\\n              | |\\n              | |\\n              | |\\n              `|'\\n";
// DragonFly: 16 lines, done at line 6772.
// Logo #65: Drauger...
static const char Drauger[] = "${c1}                  -``-\\n                `:+``+:`\\n               `/++``++/.\\n              .++/.  ./++.\\n             :++/`    `/++:\\n           `/++:        :++/`\\n          ./+/-          -/+/.\\n         -++/.            ./++-\\n        :++:`              `:++:\\n      `/++-                  -++/`\\n     ./++.                    ./+/.\\n    -++/`                      `/++-\\n   :++:`                        `:++:\\n `/++-                            -++/`\\n.:-.`..............................`.-:.\\n`.-/++++++++++++++++++++++++++++++++/-.`\\n";
// Drauger: 17 lines, done at line 6794.
// Logo #66: elementary_small...
static const char elementary_small[] = "${c2}  _______\\n / ____  \\\\\\n/  |  /  /\\\\\\n|__\\\\ /  / |\\n\\\\   /__/  /\\n \\\\_______/\\n";
// elementary_small: 7 lines, done at line 6806.
// Logo #67: Elementary...
static const char Elementary[] = "${c2}         eeeeeeeeeeeeeeeee\\n      eeeeeeeeeeeeeeeeeeeeeee\\n    eeeee  eeeeeeeeeeee   eeeee\\n  eeee   eeeee       eee     eeee\\n eeee   eeee          eee     eeee\\neee    eee            eee       eee\\neee   eee            eee        eee\\nee    eee           eeee       eeee\\nee    eee         eeeee      eeeeee\\nee    eee       eeeee      eeeee ee\\neee   eeee   eeeeee      eeeee  eee\\neee    eeeeeeeeee     eeeeee    eee\\n eeeeeeeeeeeeeeeeeeeeeeee    eeeee\\n  eeeeeeee eeeeeeeeeeee      eeee\\n    eeeee                 eeeee\\n      eeeeeee         eeeeeee\\n         eeeeeeeeeeeeeeeee\\n";
// Elementary: 18 lines, done at line 6829.
// Logo #68: EndeavourOS...
static const char EndeavourOS[] = "${c1}                     ./${c2}o${c3}.\\n${c1}                   ./${c2}sssso${c3}-\\n${c1}                 `:${c2}osssssss+${c3}-\\n${c1}               `:+${c2}sssssssssso${c3}/.\\n${c1}             `-/o${c2}ssssssssssssso${c3}/.\\n${c1}           `-/+${c2}sssssssssssssssso${c3}+:`\\n${c1}         `-:/+${c2}sssssssssssssssssso${c3}+/.\\n${c1}       `.://o${c2}sssssssssssssssssssso${c3}++-\\n${c1}      .://+${c2}ssssssssssssssssssssssso${c3}++:\\n${c1}    .:///o${c2}ssssssssssssssssssssssssso${c3}++:\\n${c1}  `:////${c2}ssssssssssssssssssssssssssso${c3}+++.\\n${c1}`-////+${c2}ssssssssssssssssssssssssssso${c3}++++-\\n${c1} `..-+${c2}oosssssssssssssssssssssssso${c3}+++++/`\\n   ./++++++++++++++++++++++++++++++/:.\\n  `:::::::::::::::::::::::::------``\\n";
// EndeavourOS: 16 lines, done at line 6850.
// Logo #69: Endless...
static const char Endless[] = "${c1}           `:+yhmNMMMMNmhy+:`\\n        -odMMNhso//////oshNMMdo-\\n      /dMMh+.              .+hMMd/\\n    /mMNo`                    `oNMm:\\n  `yMMo`                        `oMMy`\\n `dMN-                            -NMd`\\n hMN.                              .NMh\\n/MM/                  -os`          /MM/\\ndMm    `smNmmhs/- `:sNMd+   ``       mMd\\nMMy    oMd--:+yMMMMMNo.:ohmMMMNy`    yMM\\nMMy    -NNyyhmMNh+oNMMMMMy:.  dMo    yMM\\ndMm     `/++/-``/yNNh+/sdNMNddMm-    mMd\\n/MM/          `dNy:       `-::-     /MM/\\n hMN.                              .NMh\\n `dMN-                            -NMd`\\n  `yMMo`                        `oMMy`\\n    /mMNo`                    `oNMm/\\n      /dMMh+.              .+hMMd/\\n        -odMMNhso//////oshNMMdo-\\n           `:+yhmNMMMMNmhy+:`\\n";
// Endless: 21 lines, done at line 6876.
// Logo #70: EuroLinux...
static const char EuroLinux[] = "${c1}                __\\n         -wwwWWWWWWWWWwww-\\n        -WWWWWWWWWWWWWWWWWWw-\\n          \\WWWWWWWWWWWWWWWWWWW-\\n  _Ww      `WWWWWWWWWWWWWWWWWWWw\\n -W${c2}E${c1}Www                -WWWWWWWWW-\\n_WW${c2}U${c1}WWWW-                _WWWWWWWW\\n_WW${c2}R${c1}WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW-\\nwWW${c2}O${c1}WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW\\nWWW${c2}L${c1}WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWw\\nWWW${c2}I${c1}WWWWWWWWWWWWWWWWWWWWWWWWWWWWww-\\nwWW${c2}N${c1}WWWWw\\n WW${c2}U${c1}WWWWWWw\\n wW${c2}X${c1}WWWWWWWWww\\n   wWWWWWWWWWWWWWWWw\\n    wWWWWWWWWWWWWWWWw\\n       WWWWWWWWWWWWWw\\n           wWWWWWWWw\\n";
// EuroLinux: 19 lines, done at line 6900.
// Logo #71: Exherbo...
static const char Exherbo[] = "${c2} ,\\nOXo.\\nNXdX0:    .cok0KXNNXXK0ko:.\\nKX  '0XdKMMK;.xMMMk, .0MMMMMXx;  ...\\n'NO..xWkMMx   kMMM    cMMMMMX,NMWOxOXd.\\n  cNMk  NK    .oXM.   OMMMMO. 0MMNo  kW.\\n  lMc   o:       .,   .oKNk;   ;NMMWlxW'\\n ;Mc    ..   .,,'    .0M${c1}g;${c2}WMN'dWMMMMMMO\\n XX        ,WMMMMW.  cM${c1}cfli${c2}WMKlo.   .kMk\\n.Mo        .WM${c1}GD${c2}MW.   XM${c1}WO0${c2}MMk        oMl\\n,M:         ,XMMWx::,''oOK0x;          NM.\\n'Ml      ,kNKOxxxxxkkO0XXKOd:.         oMk\\n NK    .0Nxc${c3}:::::::::::::::${c2}fkKNk,      .MW\\n ,Mo  .NXc${c3}::${c2}qXWXb${c3}::::::::::${c2}oo${c3}::${c2}lNK.    .MW\\n  ;Wo oMd${c3}:::${c2}oNMNP${c3}::::::::${c2}oWMMMx${c3}:${c2}c0M;   lMO\\n   'NO;W0c${c3}:::::::::::::::${c2}dMMMMO${c3}::${c2}lMk  .WM'\\n     xWONXdc${c3}::::::::::::::${c2}oOOo${c3}::${c2}lXN. ,WMd\\n      'KWWNXXK0Okxxo,${c3}:::::::${c2},lkKNo  xMMO\\n        :XMNxl,';:lodxkOO000Oxc. .oWMMo\\n          'dXMMXkl;,.        .,o0MMNo'\\n             ':d0XWMMMMWNNNNMMMNOl'\\n                   ':okKXWNKkl'\\n";
// Exherbo: 23 lines, done at line 6928.
// Logo #72: fedora_small...
static const char fedora_small[] = "${c2}      _____\\n     /   __)${c1}\\\\${c2}\\n     |  /  ${c1}\\\\ \\\\${c2}\\n  ${c1}__${c2}_|  |_${c1}_/ /${c2}\\n ${c1}/ ${c2}(_    _)${c1}_/${c2}\\n${c1}/ /${c2}  |  |\\n${c1}\\\\ \\\\${c2}__/  |\\n ${c1}\\\\${c2}(_____/\\n";
// fedora_small: 9 lines, done at line 6942.
// Logo #73: RFRemix...
static const char RFRemix[] = "${c1}          /:-------------:\\\\\\n       :-------------------::\\n     :-----------${c2}/shhOHbmp${c1}---:\\\\\\n   /-----------${c2}omMMMNNNMMD  ${c1}---:\\n  :-----------${c2}sMMMMNMNMP${c1}.    ---:\\n :-----------${c2}:MMMdP${c1}-------    ---\\\\\\n,------------${c2}:MMMd${c1}--------    ---:\\n:------------${c2}:MMMd${c1}-------    .---:\\n:----    ${c2}oNMMMMMMMMMNho${c1}     .----:\\n:--     .${c2}+shhhMMMmhhy++${c1}   .------/\\n:-    -------${c2}:MMMd${c1}--------------:\\n:-   --------${c2}/MMMd${c1}-------------;\\n:-    ------${c2}/hMMMy${c1}------------:\\n:--${c2} :dMNdhhdNMMNo${c1}------------;\\n:---${c2}:sdNMMMMNds:${c1}------------:\\n:------${c2}:://:${c1}-------------::\\n:---------------------://\\n";
// RFRemix: 18 lines, done at line 6965.
// Logo #74: Feren...
static const char Feren[] = "${c1} `----------`\\n :+ooooooooo+.\\n-o+oooooooooo+-\\n..`/+++++++++++/...`````````````````\\n   .++++++++++++++++++++++++++/////-\\n    ++++++++++++++++++++++++++++++++//:`\\n    -++++++++++++++++++++++++++++++/-`\\n     ++++++++++++++++++++++++++++:.\\n     -++++++++++++++++++++++++/.\\n      +++++++++++++++++++++/-`\\n      -++++++++++++++++++//-`\\n        .:+++++++++++++//////-\\n           .:++++++++//////////-\\n             `-++++++---:::://///.\\n           `.:///+++.             `\\n          `.........\\n";
// Feren: 17 lines, done at line 6987.
// Logo #75: freebsd_small...
static const char freebsd_small[] = "${c1}/\\\\,-'''''-,/\\\\\\n\\\\_)       (_/\\n|           |\\n|           |\\n ;         ;\\n  '-_____-'\\n";
// freebsd_small: 7 lines, done at line 6999.
// Logo #76: FreeMiNT...
static const char FreeMiNT[] = "${c1}          ##\\n          ##         #########\\n                    ####      ##\\n            ####  ####        ##\\n####        ####  ##        ##\\n        ####    ####      ##  ##\\n        ####  ####  ##  ##  ##\\n            ####  ######\\n        ######  ##  ##  ####\\n      ####    ################\\n    ####        ##  ####\\n    ##            ####  ######\\n    ##      ##    ####  ####\\n    ##    ##  ##    ##  ##  ####\\n      ####  ##          ##  ##\\n";
// FreeMiNT: 16 lines, done at line 7045.
// Logo #77: Frugalware...
static const char Frugalware[] = "${c1}          `++/::-.`\\n         /o+++++++++/::-.`\\n        `o+++++++++++++++o++/::-.`\\n        /+++++++++++++++++++++++oo++/:-.``\\n       .o+ooooooooooooooooooosssssssso++oo++/:-`\\n       ++osoooooooooooosssssssssssssyyo+++++++o:\\n      -o+ssoooooooooooosssssssssssssyyo+++++++s`\\n      o++ssoooooo++++++++++++++sssyyyyo++++++o:\\n     :o++ssoooooo${c2}/-------------${c1}+syyyyyo+++++oo\\n    `o+++ssoooooo${c2}/-----${c1}+++++ooosyyyyyyo++++os:\\n    /o+++ssoooooo${c2}/-----${c1}ooooooosyyyyyyyo+oooss\\n   .o++++ssooooos${c2}/------------${c1}syyyyyyhsosssy-\\n   ++++++ssooooss${c2}/-----${c1}+++++ooyyhhhhhdssssso\\n  -s+++++syssssss${c2}/-----${c1}yyhhhhhhhhhhhddssssy.\\n  sooooooyhyyyyyh${c2}/-----${c1}hhhhhhhhhhhddddyssy+\\n :yooooooyhyyyhhhyyyyyyhhhhhhhhhhdddddyssy`\\n yoooooooyhyyhhhhhhhhhhhhhhhhhhhddddddysy/\\n-ysooooooydhhhhhhhhhhhddddddddddddddddssy\\n .-:/+osssyyyysyyyyyyyyyyyyyyyyyyyyyyssy:\\n       ``.-/+oosysssssssssssssssssssssss\\n               ``.:/+osyysssssssssssssh.\\n                        `-:/+osyyssssyo\\n                                .-:+++`\\n";
// Frugalware: 24 lines, done at line 7074.
// Logo #78: Funtoo...
static const char Funtoo[] = "${c1}   .dKXXd                         .\\n  :XXl;:.                      .OXo\\n.'OXO''  .''''''''''''''''''''':XNd..'oco.lco,\\nxXXXXXX, cXXXNNNXXXXNNXXXXXXXXNNNNKOOK; d0O .k\\n  kXX  xXo  KNNN0  KNN.       'xXNo   :c; 'cc.\\n  kXX  xNo  KNNN0  KNN. :xxxx. 'NNo\\n  kXX  xNo  loooc  KNN. oNNNN. 'NNo\\n  kXX  xN0:.       KNN' oNNNX' ,XNk\\n  kXX  xNNXNNNNNNNNXNNNNNNNNXNNOxXNX0Xl\\n  ...  ......................... .;cc;.\\n";
// Funtoo: 11 lines, done at line 7090.
// Logo #79: GalliumOS...
static const char GalliumOS[] = "${c1}sooooooooooooooooooooooooooooooooooooo+:\\nyyooooooooooooooooooooooooooooooooo+/:::\\nyyysoooooooooooooooooooooooooooo+/::::::\\nyyyyyoooooooooooooooooooooooo+/:::::::::\\nyyyyyysoooooooooooooooooo++/::::::::::::\\nyyyyyyysoooooooooooooo++/:::::::::::::::\\nyyyyyyyyysoooooo${c2}sydddys${c1}+/:::::::::::::::\\nyyyyyyyyyysooo${c2}smMMMMMMMNd${c1}+::::::::::::::\\nyyyyyyyyyyyyo${c2}sMMMMMMMMMMMN${c1}/:::::::::::::\\nyyyyyyyyyyyyy${c2}dMMMMMMMMMMMM${c1}o//:::::::::::\\nyyyyyyyyyyyyy${c2}hMMMMMMMMMMMm${c1}--//::::::::::\\nyyyyyyyyyyyyyy${c2}hmMMMMMMMNy${c1}:..-://::::::::\\nyyyyyyyyyyyyyyy${c2}yyhhyys+:${c1}......://:::::::\\nyyyyyyyyyyyyyyys+:--...........-///:::::\\nyyyyyyyyyyyys+:--................://::::\\nyyyyyyyyyo+:-.....................-//:::\\nyyyyyyo+:-..........................://:\\nyyyo+:-..............................-//\\no/:-...................................:\\n";
// GalliumOS: 20 lines, done at line 7115.
// Logo #80: Garuda...
static const char Garuda[] = "${c1}                  __,,,,,,,_\\n            _╓╗╣╫╠╠╠╠╠╠╠╠╠╠╠╠╠╕╗╗┐_\\n         ╥╢╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╥,\\n       ╗╠╠╠╠╠╠╠╝╜╜╜╜╝╢╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠┐\\n      ╣╠╠╠╠╠╠╠╠╢╣╢╗╕ , `\"╘╠╠╠╠╠╠╠╠╠╠╠╠╠╠╔╥_\\n    ╒╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╕╙╥╥╜   `\"╜╠╬╠╠╠╠╠╠╠╠╠╠╠╥,\\n    ╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╗╥╥╥╥╗╗╬╠╠╠╠╠╠╠╝╙╠╠╣╠╠╠╠╢┐\\n   ╣╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╥╬╣╠╠╠╠╠╠╠╠╗\\n  ╒╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╗\\n  ╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠\\n  ╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╬     ```\"╜╝╢╠╠╡\\n ╒╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╣,         ╘╠╪\\n ╞╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╢┐        ╜\\n `╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╗\\n ,╬╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠\"╕\\n ╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╗\\n ╝^╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╝╣╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╡\\n  ╔╜`╞┘╢╛╜ ╡╢╠\"╚╠╠╜╝┌╞╞\"╢╠╠╠╠╠╠╠╠╠╠╣╩╢╪\\n     ╜╒\"   `╜    `      ╜╙╕  └╣╠╠╠╠╕ ╞╙╖\\n                                ╠╠╠\\n                                 ╜\\n";
// Garuda: 22 lines, done at line 7142.
// Logo #81: gentoo_small...
static const char gentoo_small[] = "${c1} _-----_\\n(       \\\\\\n\\    0   \\\\\\n${c2} \\        )\\n /      _/\\n(     _-\\n\\____-\\n";
// gentoo_small: 8 lines, done at line 7155.
// Logo #82: Gentoo...
static const char Gentoo[] = "${c1}         -/oyddmdhs+:.\\n     -o${c2}dNMMMMMMMMNNmhy+${c1}-`\\n   -y${c2}NMMMMMMMMMMMNNNmmdhy${c1}+-\\n `o${c2}mMMMMMMMMMMMMNmdmmmmddhhy${c1}/`\\n om${c2}MMMMMMMMMMMN${c1}hhyyyo${c2}hmdddhhhd${c1}o`\\n.y${c2}dMMMMMMMMMMd${c1}hs++so/s${c2}mdddhhhhdm${c1}+`\\n oy${c2}hdmNMMMMMMMN${c1}dyooy${c2}dmddddhhhhyhN${c1}d.\\n  :o${c2}yhhdNNMMMMMMMNNNmmdddhhhhhyym${c1}Mh\\n    .:${c2}+sydNMMMMMNNNmmmdddhhhhhhmM${c1}my\\n       /m${c2}MMMMMMNNNmmmdddhhhhhmMNh${c1}s:\\n    `o${c2}NMMMMMMMNNNmmmddddhhdmMNhs${c1}+`\\n  `s${c2}NMMMMMMMMNNNmmmdddddmNMmhs${c1}/.\\n /N${c2}MMMMMMMMNNNNmmmdddmNMNdso${c1}:`\\n+M${c2}MMMMMMNNNNNmmmmdmNMNdso${c1}/-\\nyM${c2}MNNNNNNNmmmmmNNMmhs+/${c1}-`\\n/h${c2}MMNNNNNNNNMNdhs++/${c1}-`\\n`/${c2}ohdmmddhys+++/:${c1}.`\\n  `-//////:--.\\n";
// Gentoo: 19 lines, done at line 7179.
// Logo #83: Pentoo...
static const char Pentoo[] = "${c2}           `:oydNNMMMMNNdyo:`\\n        :yNMMMMMMMMMMMMMMMMNy:\\n      :dMMMMMMMMMMMMMMMMMMMMMMd:\\n     oMMMMMMMho/-....-/ohMMMMMMMo\\n    oMMMMMMy.            .yMMMMMMo\\n   .MMMMMMo                oMMMMMM.\\n   +MMMMMm                  mMMMMM+\\n   oMMMMMh                  hMMMMMo\\n //hMMMMMm//${c1}`${c2}          ${c1}`${c2}////mMMMMMh//\\nMMMMMMMMMMM${c1}/${c2}      ${c1}/o/`${c2}  ${c1}.${c2}smMMMMMMMMMMM\\nMMMMMMMMMMm      ${c1}`NMN:${c2}    ${c1}.${c2}yMMMMMMMMMM\\nMMMMMMMMMMMh${c1}:.${c2}              dMMMMMMMMM\\nMMMMMMMMMMMMMy${c1}.${c2}            ${c1}-${c2}NMMMMMMMMM\\nMMMMMMMMMMMd:${c1}`${c2}           ${c1}-${c2}yNMMMMMMMMMM\\nMMMMMMMMMMh${c1}`${c2}          ${c1}./${c2}hNMMMMMMMMMMMM\\nMMMMMMMMMM${c1}s${c2}        ${c1}.:${c2}ymMMMMMMMMMMMMMMM\\nMMMMMMMMMMN${c1}s:..-/${c2}ohNMMMMMMMMMMMMMMMMMM\\nMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\\nMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\\n MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\\n\\n";
// Pentoo: 22 lines, done at line 7206.
// Logo #84: gNewSense...
static const char gNewSense[] = "${c1}                     ..,,,,..\\n               .oocchhhhhhhhhhccoo.\\n        .ochhlllllllc hhhhhh ollllllhhco.\\n    ochlllllllllll hhhllllllhhh lllllllllllhco\\n .cllllllllllllll hlllllo  +hllh llllllllllllllc.\\nollllllllllhco''  hlllllo  +hllh  ``ochllllllllllo\\nhllllllllc'       hllllllllllllh       `cllllllllh\\nollllllh          +llllllllllll+          hllllllo\\n `cllllh.           ohllllllho           .hllllc'\\n    ochllc.            ++++            .cllhco\\n       `+occooo+.                .+ooocco+'\\n              `+oo++++      ++++oo+'\\n";
// gNewSense: 13 lines, done at line 7224.
// Logo #85: GNOME...
static const char GNOME[] = "${c1}                               ,@@@@@@@@,\\n                 @@@@@@      @@@@@@@@@@@@\\n        ,@@.    @@@@@@@    *@@@@@@@@@@@@\\n       @@@@@%   @@@@@@(    @@@@@@@@@@@&\\n       @@@@@@    @@@@*     @@@@@@@@@#\\n@@@@*   @@@@,              *@@@@@%\\n@@@@@.\\n @@@@#         @@@@@@@@@@@@@@@@\\n         ,@@@@@@@@@@@@@@@@@@@@@@@,\\n      ,@@@@@@@@@@@@@@@@@@@@@@@@@@&\\n    .@@@@@@@@@@@@@@@@@@@@@@@@@@@@\\n    @@@@@@@@@@@@@@@@@@@@@@@@@@@\\n   @@@@@@@@@@@@@@@@@@@@@@@@(\\n   @@@@@@@@@@@@@@@@@@@@%\\n    @@@@@@@@@@@@@@@@\\n     @@@@@@@@@@@@*        @@@@@@@@/\\n      &@@@@@@@@@@        @@@@@@@@@*\\n        @@@@@@@@@@@,    @@@@@@@@@*\\n          ,@@@@@@@@@@@@@@@@@@@@&\\n              &@@@@@@@@@@@@@@\\n                     ...\\n";
// GNOME: 22 lines, done at line 7251.
// Logo #86: GNU...
static const char GNU[] = "${c1}    _-`````-,           ,- '- .\\n  .'   .- - |          | - -.  `.\\n /.'  /                     `.   \\\\n:/   :      _...   ..._      ``   :\\n::   :     /._ .`:'_.._\\.    ||   :\\n::    `._ ./  ,`  :    \\ . _.''   .\\n`:.      /   |  -.  \\-. \\\\_      /\\n  \\:._ _/  .'   .@)  \\@) ` `\\ ,.'\\n     _/,--'       .- .\\,-.`--`.\\n       ,'/''     (( \\ `  )\\n        /'/'  \\    `-'  (\\n         '/''  `._,-----'\\n          ''/'    .,---'\\n           ''/'      ;:\\n             ''/''  ''/\\n               ''/''/''\\n                 '/'/'\\n                  `;\\n";
// GNU: 19 lines, done at line 7275.
// Logo #87: GoboLinux...
static const char GoboLinux[] = "${c1}  _____       _\\n / ____|     | |\\n| |  __  ___ | |__   ___\\n| | |_ |/ _ \\| '_ \\ / _ \\\\n| |__| | (_) | |_) | (_) |\\n \\_____|\\___/|_.__/ \\___/\\n";
// GoboLinux: 7 lines, done at line 7287.
// Logo #88: Grombyang...
static const char Grombyang[] = "${c1}            eeeeeeeeeeee\\n         eeeeeeeeeeeeeeeee\\n      eeeeeeeeeeeeeeeeeeeeeee\\n    eeeee       ${c2}.o+       ${c1}eeee\\n  eeee         ${c2}`ooo/         ${c1}eeee\\n eeee         ${c2}`+oooo:         ${c1}eeee\\neee          ${c2}`+oooooo:          ${c1}eee\\neee          ${c2}-+oooooo+:         ${c1}eee\\nee         ${c2}`/:oooooooo+:         ${c1}ee\\nee        ${c2}`/+   +++    +:        ${c1}ee\\nee              ${c2}+o+\\             ${c1}ee\\neee             ${c2}+o+\\            ${c1}eee\\neee        ${c2}//  \\\\ooo/  \\\\\\        ${c1}eee\\n eee      ${c2}//++++oooo++++\\\\\\     ${c1}eee\\n  eeee    ${c2}::::++oooo+:::::   ${c1}eeee\\n    eeeee   ${c3}Grombyang OS ${c1}  eeee\\n      eeeeeeeeeeeeeeeeeeeeeee\\n         eeeeeeeeeeeeeeeee\\n";
// Grombyang: 19 lines, done at line 7311.
// Logo #89: guix_small...
static const char guix_small[] = "${c1}|.__          __.|\\n|__ \\\\        / __|\\n   \\\\ \\\\      / /\\n    \\\\ \\\\    / /\\n     \\\\ \\\\  / /\\n      \\\\ \\\\/ /\\n       \\\\__/\\n";
// guix_small: 8 lines, done at line 7324.
// Logo #90: Guix...
static const char Guix[] = "${c1} ..                             `.\\n `--..```..`           `..```..--`\\n   .-:///-:::.       `-:::///:-.\\n      ````.:::`     `:::.````\\n           -//:`    -::-\\n            ://:   -::-\\n            `///- .:::`\\n             -+++-:::.\\n              :+/:::-\\n              `-....`\\n";
// Guix: 11 lines, done at line 7340.
// Logo #91: haiku_small...
static const char haiku_small[] = "${c1}       ,^,\\n      /   \\\\\\n*--_ ;     ; _--*\\n\\\\   '\"     \"'   /\\n '.           .'\\n.-'\"         \"'-.\\n '-.__.   .__.-'\\n       |_|\\n";
// haiku_small: 9 lines, done at line 7354.
// Logo #92: Haiku...
static const char Haiku[] = "${c2}          :dc'\\n       'l:;'${c1},${c2}'ck.    .;dc:.\\n       co    ${c1}..${c2}k.  .;;   ':o.\\n       co    ${c1}..${c2}k. ol      ${c1}.${c2}0.\\n       co    ${c1}..${c2}k. oc     ${c1}..${c2}0.\\n       co    ${c1}..${c2}k. oc     ${c1}..${c2}0.\\n.Ol,.  co ${c1}...''${c2}Oc;kkodxOdddOoc,.\\n ';lxxlxOdxkxk0kd${c1}oooll${c2}dl${c1}ccc:${c2}clxd;\\n     ..${c1}oOolllllccccccc:::::${c2}od;\\n       cx:ooc${c1}:::::::;${c2}cooolcX.\\n       cd${c1}.${c2}''cloxdoollc' ${c1}...${c2}0.\\n       cd${c1}......${c2}k;${c1}.${c2}xl${c1}....  .${c2}0.\\n       .::c${c1};..${c2}cx;${c1}.${c2}xo${c1}..... .${c2}0.\\n          '::c'${c1}...${c2}do${c1}..... .${c2}K,\\n                  cd,.${c1}....:${c2}O,${c1}\\n                    ':clod:'${c1}\\n                        ${c1}\\n";
// Haiku: 18 lines, done at line 7377.
// Logo #93: Huayra...
static const char Huayra[] = "${c2}                     `\\n            .       .       `\\n       ``    -      .      .\\n        `.`   -` `. -  `` .`\\n          ..`-`-` + -  / .`     ```\\n          .--.+--`+:- :/.` .-``.`\\n            -+/so::h:.d-`./:`.`\\n              :hNhyMomy:os-...-.  ````\\n               .dhsshNmNhoo+:-``.```\\n                ${c1}`ohy:-${c2}NMds+::-.``\\n            ````${c1}.hNN+`${c2}mMNho/:-....````\\n       `````     `../dmNhoo+/:..``\\n    ````            .dh++o/:....`\\n.+s/`                `/s-.-.:.`` ````\\n::`                    `::`..`\\n                          .` `..\\n                                ``\\n";
// Huayra: 18 lines, done at line 7400.
// Logo #94: hyperbola_small...
static const char hyperbola_small[] = "${c1}    |`__.`/\\n    \\____/\\n    .--.\\n   /    \\\\\\n  /  ___ \\\\\\n / .`   `.\\\\\\n/.`      `.\\\\\\n";
// hyperbola_small: 8 lines, done at line 7413.
// Logo #95: Hyperbola...
static const char Hyperbola[] = "${c1}                     WW\\n                     KX              W\\n                    WO0W          NX0O\\n                    NOO0NW  WNXK0OOKW\\n                    W0OOOOOOOOOOOOKN\\n                     N0OOOOOOO0KXW\\n                       WNXXXNW\\n                 NXK00000KN\\n             WNK0OOOOOOOOOO0W\\n           NK0OOOOOOOOOOOOOO0W\\n         X0OOOOOOO00KK00OOOOOK\\n       X0OOOO0KNWW      WX0OO0W\\n     X0OO0XNW              KOOW\\n   N00KNW                   KOW\\n NKXN                       W0W\\nWW                           W\\n";
// Hyperbola: 17 lines, done at line 7435.
// Logo #96: Ataraxia...
static const char Ataraxia[] = "${c1}               'l:\\n        loooooo\\n          loooo coooool\\n looooooooooooooooooool\\n  looooooooooooooooo\\n         lool   cooo\\n        coooooooloooooooo\\n     clooooo  ;lood  cloooo\\n  :loooocooo cloo      loooo\\n loooo  :ooooool       loooo\\nlooo    cooooo        cooooo\\nlooooooooooooo      ;loooooo ${c2}looooooc\\n${c1}looooooooo loo   cloooooool    ${c2}looooc\\n${c1} cooo       cooooooooooo       ${c2}looolooooool\\n${c1}            cooo:     ${c2}coooooooooooooooooool\\n                       loooooooooooolc:   loooc;\\n                             cooo:    loooooooooooc\\n                            ;oool         looooooo:\\n                           coool          olc,\\n                          looooc   ,,\\n                        coooooc    loc\\n                       :oooool,    coool:, looool:,\\n                       looool:      ooooooooooooooo:\\n                       cooolc        .ooooooooooool\\n";
// Ataraxia: 25 lines, done at line 7465.
// Logo #97: Kali...
static const char Kali[] = "${c1}..............\\n            ..,;:ccc,.\\n          ......''';lxO.\\n.....''''..........,:ld;\\n           .';;;:::;,,.x,\\n      ..'''.            0Xxoc:,.  ...\\n  ....                ,ONkc;,;cokOdc',.\\n .                   OMo           ':${c2}dd${c1}o.\\n                    dMc               :OO;\\n                    0M.                 .:o.\\n                    ;Wd\\n                     ;XO,\\n                       ,d0Odlc;,..\\n                           ..',;:cdOOd::,.\\n                                    .:d;.':;.\\n                                       'd,  .'\\n                                         ;l   ..\\n                                          .o\\n                                            c\\n                                            .'\\n                                             .\\n";
// Kali: 22 lines, done at line 7492.
// Logo #98: KaOS...
static const char KaOS[] = "${c1}                     ..\\n  .....         ..OSSAAAAAAA..\\n .KKKKSS.     .SSAAAAAAAAAAA.\\n.KKKKKSO.    .SAAAAAAAAAA...\\nKKKKKKS.   .OAAAAAAAA.\\nKKKKKKS.  .OAAAAAA.\\nKKKKKKS. .SSAA..\\n.KKKKKS..OAAAAAAAAAAAA........\\n DKKKKO.=AA=========A===AASSSO..\\n  AKKKS.==========AASSSSAAAAAASS.\\n  .=KKO..========ASS.....SSSSASSSS.\\n    .KK.       .ASS..O.. =SSSSAOSS:\\n     .OK.      .ASSSSSSSO...=A.SSA.\\n       .K      ..SSSASSSS.. ..SSA.\\n                 .SSS.AAKAKSSKA.\\n                    .SSS....S..\\n";
// KaOS: 17 lines, done at line 7514.
// Logo #99: KDE...
static const char KDE[] = "${c1}             `..---+/---..`\\n         `---.``   ``   `.---.`\\n      .--.`        ``        `-:-.\\n    `:/:     `.----//----.`     :/-\\n   .:.    `---`          `--.`    .:`\\n  .:`   `--`                .:-    `:.\\n `/    `:.      `.-::-.`      -:`   `/`\\n /.    /.     `:++++++++:`     .:    .:\\n`/    .:     `+++++++++++/      /`   `+`\\n/+`   --     .++++++++++++`     :.   .+:\\n`/    .:     `+++++++++++/      /`   `+`\\n /`    /.     `:++++++++:`     .:    .:\\n ./    `:.      `.:::-.`      -:`   `/`\\n  .:`   `--`                .:-    `:.\\n   .:.    `---`          `--.`    .:`\\n    `:/:     `.----//----.`     :/-\\n      .-:.`        ``        `-:-.\\n         `---.``   ``   `.---.`\\n             `..---+/---..`\\n";
// KDE: 20 lines, done at line 7539.
// Logo #100: Kibojoe...
static const char Kibojoe[] = "            ${c3}           ./+oooooo+/.\\n           -/+ooooo+/:.`\\n          ${c1}`${c3}yyyo${c2}+++/++${c3}osss${c1}.\\n         ${c1}+NMN${c3}yssssssssssss${c1}.\\n       ${c1}.dMMMMN${c3}sssssssssssy${c1}Ns`\\n      +MMMMMMMm${c3}sssssssssssh${c1}MNo`\\n    `hMMMMMNNNMd${c3}sssssssssssd${c1}MMN/\\n   .${c3}syyyssssssy${c1}NNmmmmd${c3}sssss${c1}hMMMMd:\\n  -NMmh${c3}yssssssssyhhhhyssyh${c1}mMMMMMMMy`\\n -NMMMMMNN${c3}mdhyyyyyyyhdm${c1}NMMMMMMMMMMMN+\\n`NMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMd.\\nods+/:-----://+oyydmNMMMMMMMMMMMMMMMMMN-\\n`                     .-:+osyhhdmmNNNmdo\\n";
// Kibojoe: 14 lines, done at line 7558.
// Logo #101: Kogaion...
static const char Kogaion[] = "${c1}            ;;      ,;\\n           ;;;     ,;;\\n         ,;;;;     ;;;;\\n      ,;;;;;;;;    ;;;;\\n     ;;;;;;;;;;;   ;;;;;\\n    ,;;;;;;;;;;;;  ';;;;;,\\n    ;;;;;;;;;;;;;;, ';;;;;;;\\n    ;;;;;;;;;;;;;;;;;, ';;;;;\\n;    ';;;;;;;;;;;;;;;;;;, ;;;\\n;;;,  ';;;;;;;;;;;;;;;;;;;,;;\\n;;;;;,  ';;;;;;;;;;;;;;;;;;,\\n;;;;;;;;,  ';;;;;;;;;;;;;;;;,\\n;;;;;;;;;;;;, ';;;;;;;;;;;;;;\\n';;;;;;;;;;;;; ';;;;;;;;;;;;;\\n ';;;;;;;;;;;;;, ';;;;;;;;;;;\\n  ';;;;;;;;;;;;;  ;;;;;;;;;;\\n    ';;;;;;;;;;;; ;;;;;;;;\\n        ';;;;;;;; ;;;;;;\\n           ';;;;; ;;;;\\n             ';;; ;;\\n";
// Kogaion: 21 lines, done at line 7584.
// Logo #102: Korora...
static const char Korora[] = "${c2}                ____________\\n             _add55555555554${c1}:\\n           _w?'${c1}``````````'${c2})k${c1}:\\n          _Z'${c1}`${c2}            ]k${c1}:\\n          m(${c1}`${c2}             )k${c1}:\\n     _.ss${c1}`${c2}m[${c1}`${c2},            ]e${c1}:\\n   .uY\"^`${c1}`${c2}Xc${c1}`${c2}?Ss.         d(${c1}`\\n  jF'${c1}`${c2}    `@.  ${c1}`${c2}Sc      .jr${c1}`\\n jr${c1}`${c2}       `?n_ ${c1}`${c2}$;   _a2\"${c1}`\\n.m${c1}:${c2}          `~M${c1}`${c2}1k${c1}`${c2}5?!`${c1}`\\n:#${c1}:${c2}             ${c1}`${c2})e${c1}```\\n:m${c1}:${c2}             ,#'${c1}`\\n:#${c1}:${c2}           .s2'${c1}`\\n:m,________.aa7^${c1}`\\n:#baaaaaaas!J'${c1}`\\n ```````````\\n";
// Korora: 17 lines, done at line 7606.
// Logo #103: KSLinux...
static const char KSLinux[] = "${c1} K   K U   U RRRR   ooo\\n K  K  U   U R   R o   o\\n KKK   U   U RRRR  o   o\\n K  K  U   U R  R  o   o\\n K   K  UUU  R   R  ooo\\n\\n${c2}  SSS   AAA  W   W  AAA\\n S     A   A W   W A   A\\n  SSS  AAAAA W W W AAAAA\\n     S A   A WW WW A   A\\n  SSS  A   A W   W A   A\\n";
// KSLinux: 12 lines, done at line 7623.
// Logo #104: Kubuntu...
static const char Kubuntu[] = "${c1}           `.:/ossyyyysso/:.\\n        .:oyyyyyyyyyyyyyyyyyyo:`\\n      -oyyyyyyyo${c2}dMMy${c1}yyyyyyysyyyyo-\\n    -syyyyyyyyyy${c2}dMMy${c1}oyyyy${c2}dmMMy${c1}yyyys-\\n   oyyys${c2}dMy${c1}syyyy${c2}dMMMMMMMMMMMMMy${c1}yyyyyyo\\n `oyyyy${c2}dMMMMy${c1}syysoooooo${c2}dMMMMy${c1}yyyyyyyyo`\\n oyyyyyy${c2}dMMMMy${c1}yyyyyyyyyyys${c2}dMMy${c1}sssssyyyo\\n-yyyyyyyy${c2}dMy${c1}syyyyyyyyyyyyyys${c2}dMMMMMy${c1}syyy-\\noyyyysoo${c2}dMy${c1}yyyyyyyyyyyyyyyyyy${c2}dMMMMy${c1}syyyo\\nyyys${c2}dMMMMMy${c1}yyyyyyyyyyyyyyyyyysosyyyyyyyy\\nyyys${c2}dMMMMMy${c1}yyyyyyyyyyyyyyyyyyyyyyyyyyyyy\\noyyyyysos${c2}dy${c1}yyyyyyyyyyyyyyyyyy${c2}dMMMMy${c1}syyyo\\n-yyyyyyyy${c2}dMy${c1}syyyyyyyyyyyyyys${c2}dMMMMMy${c1}syyy-\\n oyyyyyy${c2}dMMMy${c1}syyyyyyyyyyys${c2}dMMy${c1}oyyyoyyyo\\n `oyyyy${c2}dMMMy${c1}syyyoooooo${c2}dMMMMy${c1}oyyyyyyyyo\\n   oyyysyyoyyyys${c2}dMMMMMMMMMMMy${c1}yyyyyyyo\\n    -syyyyyyyyy${c2}dMMMy${c1}syyy${c2}dMMMy${c1}syyyys-\\n      -oyyyyyyy${c2}dMMy${c1}yyyyyysosyyyyo-\\n        ./oyyyyyyyyyyyyyyyyyyo/.\\n           `.:/oosyyyysso/:.`\\n";
// Kubuntu: 21 lines, done at line 7649.
// Logo #105: LEDE...
static const char LEDE[] = "    ${c1} _________\\n    /        /\\\\n   /  LE    /  \\\\n  /    DE  /    \\\\n /________/  LE  \\\\n \\        \\   DE /\\n  \\    LE  \\    /\\n   \\  DE    \\  /\\n    \\________\\/\\n";
// LEDE: 10 lines, done at line 7664.
// Logo #106: Linux...
static const char Linux[] = "${c2}        #####\\n${c2}       #######\\n${c2}       ##${c1}O${c2}#${c1}O${c2}##\\n${c2}       #${c3}#####${c2}#\\n${c2}     ##${c1}##${c3}###${c1}##${c2}##\\n${c2}    #${c1}##########${c2}##\\n${c2}   #${c1}############${c2}##\\n${c2}   #${c1}############${c2}###\\n${c3}  ##${c2}#${c1}###########${c2}##${c3}#\\n${c3}######${c2}#${c1}#######${c2}#${c3}######\\n${c3}#######${c2}#${c1}#####${c2}#${c3}#######\\n${c3}  #####${c2}#######${c3}#####\\n";
// Linux: 13 lines, done at line 7682.
// Logo #107: linuxlite_small...
static const char linuxlite_small[] = "${c1}   /\\\\\\n  /  \\\\\\n / ${c2}/ ${c1}/\\n> ${c2}/ ${c1}/\\n\\\\ ${c2}\\\\ ${c1}\\\\\\n \\\\_${c2}\\\\${c1}_\\\\\\n${c2}    \\\\\\n";
// linuxlite_small: 8 lines, done at line 7695.
// Logo #108: Linux_Lite...
static const char Linux_Lite[] = "${c1}          ,xXc\\n      .l0MMMMMO\\n   .kNMMMMMWMMMN,\\n   KMMMMMMKMMMMMMo\\n  'MMMMMMNKMMMMMM:\\n  kMMMMMMOMMMMMMO\\n .MMMMMMX0MMMMMW.\\n oMMMMMMxWMMMMM:\\n WMMMMMNkMMMMMO\\n:MMMMMMOXMMMMW\\n.0MMMMMxMMMMM;\\n:;cKMMWxMMMMO\\n'MMWMMXOMMMMl\\n kMMMMKOMMMMMX:\\n .WMMMMKOWMMM0c\\n  lMMMMMWO0MNd:'\\n   oollXMKXoxl;.\\n     ':. .: .'\\n              ..\\n                .\\n";
// Linux_Lite: 21 lines, done at line 7721.
// Logo #109: LMDE...
static const char LMDE[] = "         ${c2}`.-::---..\\n${c1}      .:++++ooooosssoo:.\\n    .+o++::.      `.:oos+.\\n${c1}   :oo:.`             -+oo${c2}:\\n${c1} ${c2}`${c1}+o/`    .${c2}::::::${c1}-.    .++-${c2}`\\n${c1}${c2}`${c1}/s/    .yyyyyyyyyyo:   +o-${c2}`\\n${c1}${c2}`${c1}so     .ss       ohyo` :s-${c2}:\\n${c1}${c2}`${c1}s/     .ss  h  m  myy/ /s`${c2}`\\n${c1}`s:     `oo  s  m  Myy+-o:`\\n`oo      :+sdoohyoydyso/.\\n :o.      .:////////++:\\n${c1} `/++        ${c2}-:::::-\\n${c1}  ${c2}`${c1}++-\\n${c1}   ${c2}`${c1}/+-\\n${c1}     ${c2}.${c1}+/.\\n${c1}       ${c2}.${c1}:+-.\\n          `--.``\\n";
// LMDE: 18 lines, done at line 7744.
// Logo #110: Lubuntu...
static const char Lubuntu[] = "${c1}           `-mddhhhhhhhhhddmss`\\n        ./mdhhhhhhhhhhhhhhhhhhhhhh.\\n     :mdhhhhhhhhhhhhhhhhhhhhhhhhhhhm`\\n   :ymhhhhhhhhhhhhhhhyyyyyyhhhhhhhhhy:\\n  `odhyyyhhhhhhhhhy+-````./syhhhhhhhho`\\n `hhy..:oyhhhhhhhy-`:osso/..:/++oosyyyh`\\n dhhs   .-/syhhhhs`shhhhhhyyyyyyyyyyyyhs\\n:hhhy`  yso/:+syhy/yhhhhhshhhhhhhhhhhhhh:\\nhhhhho. +hhhys++oyyyhhhhh-yhhhhhhhhhhhhhs\\nhhhhhhs-`/syhhhhyssyyhhhh:-yhhhhhhhhhhhhh\\nhhhhhhs  `:/+ossyyhyyhhhhs -yhhhhhhhhhhhh\\nhhhhhhy/ `syyyssyyyyhhhhhh: :yhhhhhhhhhhs\\n:hhhhhhyo:-/osyhhhhhhhhhhho  ohhhhhhhhhh:\\n sdhhhhhhhyyssyyhhhhhhhhhhh+  +hhhhhhhhs\\n `shhhhhhhhhhhhhhhhhhhhhhy+` .yhhhhhhhh`\\n  +sdhhhhhhhhhhhhhhhhhyo/. `/yhhhhhhhd`\\n   `:shhhhhhhhhh+---..``.:+yyhhhhhhh:\\n     `:mdhhhhhh/.syssyyyyhhhhhhhd:`\\n        `+smdhhh+shhhhhhhhhhhhdm`\\n           `sNmdddhhhhhhhddm-`\\n";
// Lubuntu: 21 lines, done at line 7770.
// Logo #111: Lunar...
static const char Lunar[] = "${c1}`-.                                 `-.\\n  -ohys/-`                    `:+shy/`\\n     -omNNdyo/`          :+shmNNy/`\\n             ${c3}      -\\n                 /mMmo\\n                 hMMMN`\\n                 .NMMs\\n    ${c1}  -:+oooo+//: ${c3}/MN${c1}. -///oooo+/-`\\n     /:.`          ${c3}/${c1}           `.:/`\\n${c3}          __\\n         |  |   _ _ ___ ___ ___\\n         |  |__| | |   | .'|  _|\\n         |_____|___|_|_|__,|_|\\n";
// Lunar: 14 lines, done at line 7789.
// Logo #112: _small...
static const char _small[] = "${c1}       .:'\\n    _ :'_\\n${c2} .'`_`-'_``.\\n:________.-'\\n${c3}:_______:\\n:_______:\\n${c4} :_______`-;\\n${c5}  `._.-._.'\\n";
// _small: 9 lines, done at line 7803.
// Logo #113: Darwin...
static const char Darwin[] = "${c1}                    'c.\\n                 ,xNMM.\\n               .OMMMMo\\n               OMMM0,\\n     .;loddo:' loolloddol;.\\n   cKMMMMMMMMMMNWMMMMMMMMMM0:\\n${c2} .KMMMMMMMMMMMMMMMMMMMMMMMWd.\\n XMMMMMMMMMMMMMMMMMMMMMMMX.\\n${c3};MMMMMMMMMMMMMMMMMMMMMMMM:\\n:MMMMMMMMMMMMMMMMMMMMMMMM:\\n${c4}.MMMMMMMMMMMMMMMMMMMMMMMMX.\\n kMMMMMMMMMMMMMMMMMMMMMMMMWd.\\n ${c5}.XMMMMMMMMMMMMMMMMMMMMMMMMMMk\\n  .XMMMMMMMMMMMMMMMMMMMMMMMMK.\\n    ${c6}kMMMMMMMMMMMMMMMMMMMMMMd\\n     ;KMMMMMMMWXXWMMMMMMMk.\\n       .cooc,.    .,coo:.\\n";
// Darwin: 18 lines, done at line 7826.
// Logo #114: mageia_small...
static const char mageia_small[] = "${c1}   *\\n    *\\n   **\\n${c2} /\\\\__/\\\\\\n/      \\\\\\n\\\\      /\\n \\\\____/\\n";
// mageia_small: 8 lines, done at line 7839.
// Logo #115: Mageia...
static const char Mageia[] = "${c1}        .°°.\\n         °°   .°°.\\n         .°°°. °°\\n         .   .\\n          °°° .°°°.\\n      .°°°.   '___'\\n${c2}     .${c1}'___'     ${c2}   .\\n   :dkxc;'.  ..,cxkd;\\n .dkk. kkkkkkkkkk .kkd.\\n.dkk.  ';cloolc;.  .kkd\\nckk.                .kk;\\nxO:                  cOd\\nxO:                  lOd\\nlOO.                .OO:\\n.k00.              .00x\\n .k00;            ;00O.\\n  .lO0Kc;,,,,,,;c0KOc.\\n     ;d00KKKKKK00d;\\n        .,KKKK,.\\n";
// Mageia: 20 lines, done at line 7864.
// Logo #116: MagpieOS...
static const char MagpieOS[] = "${c1}        ;00000     :000Ol\\n     .x00kk00:    O0kk00k;\\n    l00:   :00.  o0k   :O0k.\\n  .k0k.     x${c2}d$dddd${c1}k'    .d00;\\n  k0k.      ${c2}.dddddl       ${c1}o00,\\n o00.        ${c2}':cc:.        ${c1}d0O\\n.00l                       ,00.\\nl00.                       d0x\\nk0O                     .:k0o\\nO0k                 ;dO0000d.\\nk0O               .O0O${c2}xxxxk${c1}00:\\no00.              k0O${c2}dddddd${c1}occ\\n'00l              x0O${c2}dddddo${c3};..${c1}\\n x00.             .x00${c2}kxxd${c3}:..${c1}\\n .O0x               .:oxxx${c4}Okl.${c1}\\n  .x0d                     ${c4},xx,${c1}\\n    .:o.          ${c4}.xd       ckd${c1}\\n       ..          ${c4}dxl     .xx;\\n                    :xxolldxd'\\n                      ;oxdl.\\n";
// MagpieOS: 21 lines, done at line 7890.
// Logo #117: Mandriva...
static const char Mandriva[] = "${c2}                        ``\\n                       `-.\\n${c1}      `               ${c2}.---\\n${c1}    -/               ${c2}-::--`\\n${c1}  `++    ${c2}`----...```-:::::.\\n${c1} `os.      ${c2}.::::::::::::::-```     `  `\\n${c1} +s+         ${c2}.::::::::::::::::---...--`\\n${c1}-ss:          ${c2}`-::::::::::::::::-.``.``\\n${c1}/ss-           ${c2}.::::::::::::-.``   `\\n${c1}+ss:          ${c2}.::::::::::::-\\n${c1}/sso         ${c2}.::::::-::::::-\\n${c1}.sss/       ${c2}-:::-.`   .:::::\\n${c1} /sss+.    ${c2}..`${c1}  `--`    ${c2}.:::\\n${c1}  -ossso+/:://+/-`        ${c2}.:`\\n${c1}    -/+ooo+/-.              ${c2}`\\n";
// Mandriva: 16 lines, done at line 7911.
// Logo #118: manjaro_small...
static const char manjaro_small[] = "${c1}||||||||| ||||\\n||||||||| ||||\\n||||      ||||\\n|||| |||| ||||\\n|||| |||| ||||\\n|||| |||| ||||\\n|||| |||| ||||\\n";
// manjaro_small: 8 lines, done at line 7924.
// Logo #119: Manjaro...
static const char Manjaro[] = "${c1}██████████████████  ████████\\n██████████████████  ████████\\n██████████████████  ████████\\n██████████████████  ████████\\n████████            ████████\\n████████  ████████  ████████\\n████████  ████████  ████████\\n████████  ████████  ████████\\n████████  ████████  ████████\\n████████  ████████  ████████\\n████████  ████████  ████████\\n████████  ████████  ████████\\n████████  ████████  ████████\\n████████  ████████  ████████\\n";
// Manjaro: 15 lines, done at line 7944.
// Logo #120: Maui...
static const char Maui[] = "${c1}             `.-://////:--`\\n         .:/oooooooooooooooo+:.\\n      `:+ooooooooooooooooooooooo:`\\n    `:oooooooooooooooooooooooooooo/`\\n    ..```-oooooo/-`` `:oooooo+:.` `--\\n  :.      +oo+-`       /ooo/`       -/\\n -o.     `o+-          +o/`         -o:\\n`oo`     ::`  :o/     `+.  .+o`     /oo.\\n/o+      .  -+oo-     `   /oo/     `ooo/\\n+o-        /ooo+`       .+ooo.     :ooo+\\n++       .+oooo:       -oooo+     `oooo+\\n:.      .oooooo`      :ooooo-     :oooo:\\n`      .oooooo:      :ooooo+     `ooo+-`\\n      .+oooooo`     -oooooo:     `o/-\\n      +oooooo:     .ooooooo.\\n     /ooooooo`     /ooooooo/       ..\\n    `:oooooooo/:::/ooooooooo+:--:/:`\\n      `:+oooooooooooooooooooooo+:`\\n         .:+oooooooooooooooo+:.\\n             `.-://////:-.`\\n";
// Maui: 21 lines, done at line 7970.
// Logo #121: Mer...
static const char Mer[] = "${c1}                         dMs\\n                         .-`\\n                       `y`-o+`\\n                        ``NMMy\\n                      .--`:++.\\n                    .hNNNNs\\n                    /MMMMMN\\n                    `ommmd/ +/\\n                      ````  +/\\n                     `:+sssso/-`\\n  .-::. `-::-`     `smNMNmdmNMNd/      .://-`\\n.ymNMNNdmNMMNm+`  -dMMh:.....+dMMs   `sNNMMNo\\ndMN+::NMMy::hMM+  mMMo `ohhy/ `dMM+  yMMy::-\\nMMm   yMM-  :MMs  NMN` `:::::--sMMh  dMM`\\nMMm   yMM-  -MMs  mMM+ `ymmdsymMMMs  dMM`\\nNNd   sNN-  -NNs  -mMNs-.--..:dMMh`  dNN\\n---   .--`  `--.   .smMMmdddmMNdo`   .--\\n                     ./ohddds+:`\\n                     +h- `.:-.\\n                     ./`.dMMMN+\\n                        +MMMMMd\\n                        `+dmmy-\\n                      ``` .+`\\n                     .dMNo-y.\\n                     `hmm/\\n                         .:`\\n                         dMs\\n";
// Mer: 28 lines, done at line 8003.
// Logo #122: Minix...
static const char Minix[] = "${c2}   -sdhyo+:-`                -/syymm:\\n   sdyooymmNNy.     ``    .smNmmdysNd\\n   odyoso+syNNmysoyhhdhsoomNmm+/osdm/\\n    :hhy+-/syNNmddhddddddmNMNo:sdNd:\\n     `smNNdNmmNmddddddddddmmmmmmmy`\\n   `ohhhhdddddmmNNdmddNmNNmdddddmdh-\\n   odNNNmdyo/:/-/hNddNy-`..-+ydNNNmd:\\n `+mNho:`   smmd/ sNNh :dmms`   -+ymmo.\\n-od/       -m${c1}mm${c2}mo -NN+ +m${c1}mm${c2}m-       yms:\\n+sms -.`    :so:  .NN+  :os/     .-`mNh:\\n.-hyh+:////-     -sNNd:`    .--://ohNs-\\n `:hNNNNNNNMMd/sNMmhsdMMh/ymmNNNmmNNy/\\n  -+sNNNNMMNNNsmNMo: :NNmymNNNNMMMms:\\n    //oydNMMMMydMMNysNMMmsMMMMMNyo/`\\n       ../-yNMMy--/::/-.sMMmos+.`\\n           -+oyhNsooo+omy/```\\n              `::ohdmds-`\\n";
// Minix: 18 lines, done at line 8026.
// Logo #123: linuxmint_small...
static const char linuxmint_small[] = "${c1} ___________\\n|_          \\\\\\n  | ${c2}| _____ ${c1}|\\n  | ${c2}| | | | ${c1}|\\n  | ${c2}| | | | ${c1}|\\n  | ${c2}\\\\__${c2}___/ ${c1}|\\n  \\\\_________/\\n";
// linuxmint_small: 8 lines, done at line 8039.
// Logo #124: mint_old...
static const char mint_old[] = "${c1}MMMMMMMMMMMMMMMMMMMMMMMMMmds+.\\nMMm----::-://////////////oymNMd+`\\nMMd      ${c2}/++                ${c1}-sNMd:\\nMMNso/`  ${c2}dMM    `.::-. .-::.` ${c1}.hMN:\\nddddMMh  ${c2}dMM   :hNMNMNhNMNMNh: ${c1}`NMm\\n    NMm  ${c2}dMM  .NMN/-+MMM+-/NMN` ${c1}dMM\\n    NMm  ${c2}dMM  -MMm  `MMM   dMM. ${c1}dMM\\n    NMm  ${c2}dMM  -MMm  `MMM   dMM. ${c1}dMM\\n    NMm  ${c2}dMM  .mmd  `mmm   yMM. ${c1}dMM\\n    NMm  ${c2}dMM`  ..`   ...   ydm. ${c1}dMM\\n    hMM- ${c2}+MMd/-------...-:sdds  ${c1}dMM\\n    -NMm- ${c2}:hNMNNNmdddddddddy/`  ${c1}dMM\\n     -dMNs-${c2}``-::::-------.``    ${c1}dMM\\n      `/dMNmy+/:-------------:/yMMM\\n         ./ydNMMMMMMMMMMMMMMMMMMMMM\\n            .MMMMMMMMMMMMMMMMMMM\\n";
// mint_old: 17 lines, done at line 8061.
// Logo #125: mint...
static const char mint[] = "${c2}             ...-:::::-...\\n${c2}          .-MMMMMMMMMMMMMMM-.\\n      .-MMMM${c1}`..-:::::::-..`${c2}MMMM-.\\n    .:MMMM${c1}.:MMMMMMMMMMMMMMM:.${c2}MMMM:.\\n   -MMM${c1}-M---MMMMMMMMMMMMMMMMMMM.${c2}MMM-\\n `:MMM${c1}:MM`  :MMMM:....::-...-MMMM:${c2}MMM:`\\n :MMM${c1}:MMM`  :MM:`  ``    ``  `:MMM:${c2}MMM:\\n.MMM${c1}.MMMM`  :MM.  -MM.  .MM-  `MMMM.${c2}MMM.\\n:MMM${c1}:MMMM`  :MM.  -MM-  .MM:  `MMMM-${c2}MMM:\\n:MMM${c1}:MMMM`  :MM.  -MM-  .MM:  `MMMM:${c2}MMM:\\n:MMM${c1}:MMMM`  :MM.  -MM-  .MM:  `MMMM-${c2}MMM:\\n.MMM${c1}.MMMM`  :MM:--:MM:--:MM:  `MMMM.${c2}MMM.\\n :MMM${c1}:MMM-  `-MMMMMMMMMMMM-`  -MMM-${c2}MMM:\\n  :MMM${c1}:MMM:`                `:MMM:${c2}MMM:\\n   .MMM${c1}.MMMM:--------------:MMMM.${c2}MMM.\\n     '-MMMM${c1}.-MMMMMMMMMMMMMMM-.${c2}MMMM-'\\n       '.-MMMM${c1}``--:::::--``${c2}MMMM-.'\\n${c2}            '-MMMMMMMMMMMMM-'\\n${c2}               ``-:::::-``\\n";
// mint: 20 lines, done at line 8086.
// Logo #126: mx_small...
static const char mx_small[] = "${c3}    \\\\\\\\  /\\n     \\\\\\\\/\\n      \\\\\\\\\\n   /\\\\/ \\\\\\\\\\n  /  \\\\  /\\\\\\n /    \\\\/  \\\\\\n/__________\\\\\\n";
// mx_small: 8 lines, done at line 8100.
// Logo #127: MX...
static const char MX[] = "${c3}MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNMMMMMMMMM\\nMMMMMMMMMMNs..yMMMMMMMMMMMMMm: +NMMMMMMM\\nMMMMMMMMMN+    :mMMMMMMMMMNo` -dMMMMMMMM\\nMMMMMMMMMMMs.   `oNMMMMMMh- `sNMMMMMMMMM\\nMMMMMMMMMMMMN/    -hMMMN+  :dMMMMMMMMMMM\\nMMMMMMMMMMMMMMh-    +ms. .sMMMMMMMMMMMMM\\nMMMMMMMMMMMMMMMN+`   `  +NMMMMMMMMMMMMMM\\nMMMMMMMMMMMMMMNMMd:    .dMMMMMMMMMMMMMMM\\nMMMMMMMMMMMMm/-hMd-     `sNMMMMMMMMMMMMM\\nMMMMMMMMMMNo`   -` :h/    -dMMMMMMMMMMMM\\nMMMMMMMMMd:       /NMMh-   `+NMMMMMMMMMM\\nMMMMMMMNo`         :mMMN+`   `-hMMMMMMMM\\nMMMMMMh.            `oNMMd:    `/mMMMMMM\\nMMMMm/                -hMd-      `sNMMMM\\nMMNs`                   -          :dMMM\\nMm:                                 `oMM\\nMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\\n";
// MX: 18 lines, done at line 8123.
// Logo #128: Namib...
static const char Namib[] = "${c1}          .:+shysyhhhhysyhs+:.\\n       -/yyys              syyy/-\\n     -shy                      yhs-\\n   -yhs                          shy-\\n  +hy                              yh+\\n +ds                                sd+\\n/ys                  so              sy/\\nsh                 smMMNdyo           hs\\nyo               ymMMMMNNMMNho        oy\\nN             ydMMMNNMMMMMMMMMmy       N\\nN         shmMMMMNNMMMMMMMMMMMMMNy     N\\nyo  ooshmNMMMNNNNMMMMMMMMMMMMMMMMMms  oy\\nsd yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy ds\\n/ys                                  sy/\\n +ds                                sd+\\n  +hy                              yh+\\n   -yhs                          shy-\\n     -shy                      yhs-\\n       -/yyys              syyy/-\\n          .:+shysyhyhhysyhs+:.\\n";
// Namib: 21 lines, done at line 8149.
// Logo #129: Neptune...
static const char Neptune[] = "${c1}            ./+sydddddddys/-.\\n        .+ymNNdyooo/:+oooymNNmy/`\\n     `/hNNh/.`             `-+dNNy:`\\n    /mMd/.          .++.:oy/   .+mMd-\\n  `sMN/             oMMmdy+.     `oNNo\\n `hMd.           `/ymy/.           :NMo\\n oMN-          `/dMd:               /MM-\\n`mMy          -dMN+`                 mMs\\n.MMo         -NMM/                   yMs\\n dMh         mMMMo:`                `NMo\\n /MM/        /ymMMMm-               sMN.\\n  +Mm:         .hMMd`              oMN/\\n   +mNs.      `yNd/`             -dMm-\\n    .yMNs:    `/.`            `/yNNo`\\n      .odNNy+-`           .:ohNNd/.\\n         -+ymNNmdyyyyyyydmNNmy+.\\n             `-//sssssss//.\\n";
// Neptune: 18 lines, done at line 8172.
// Logo #130: netbsd_small...
static const char netbsd_small[] = "${c2}\\\\\\\\${c1}\\`-______,----__\\n${c2} \\\\\\\\        ${c1}__,---\\`_\\n${c2}  \\\\\\\\       ${c1}\\`.____\\n${c2}   \\\\\\\\${c1}-______,----\\`-\\n${c2}    \\\\\\\\\\n     \\\\\\\\\\n      \\\\\\\\\\n";
// netbsd_small: 8 lines, done at line 8185.
// Logo #131: NetBSD...
static const char NetBSD[] = "${c1}                     `-/oshdmNMNdhyo+:-`\\n${c2}y${c1}/s+:-``    `.-:+oydNMMMMNhs/-``\\n${c2}-m+${c1}NMMMMMMMMMMMMMMMMMMMNdhmNMMMmdhs+/-`\\n ${c2}-m+${c1}NMMMMMMMMMMMMMMMMMMMMmy+:`\\n  ${c2}-N/${c1}dMMMMMMMMMMMMMMMds:`\\n   ${c2}-N/${c1}hMMMMMMMMMmho:`\\n    ${c2}-N/${c1}-:/++/:.`\\n${c2}     :M+\\n      :Mo\\n       :Ms\\n        :Ms\\n         :Ms\\n          :Ms\\n           :Ms\\n            :Ms\\n             :Ms\\n              :Ms\\n";
// NetBSD: 18 lines, done at line 8208.
// Logo #132: Netrunner...
static const char Netrunner[] = "${c1}           .:oydmMMMMMMmdyo:`\\n        -smMMMMMMMMMMMMMMMMMMds-\\n      +mMMMMMMMMMMMMMMMMMMMMMMMMd+\\n    /mMMMMMMMMMMMMMMMMMMMMMMMMMMMMm/\\n  `hMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMy`\\n .mMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMd`\\n dMMMMMMMMMMMMMMMMMMMMMMNdhmMMMMMMMMMMh\\n+MMMMMMMMMMMMMNmhyo+/-.   -MMMMMMMMMMMM/\\nmMMMMMMMMd+:.`           `mMMMMMMMMMMMMd\\nMMMMMMMMMMMdy/.          yMMMMMMMMMMMMMM\\nMMMMMMMMMMMMMMMNh+`     +MMMMMMMMMMMMMMM\\nmMMMMMMMMMMMMMMMMMs    -NMMMMMMMMMMMMMMd\\n+MMMMMMMMMMMMMMMMMN.  `mMMMMMMMMMMMMMMM/\\n dMMMMMMMMMMMMMMMMMy  hMMMMMMMMMMMMMMMh\\n `dMMMMMMMMMMMMMMMMM-+MMMMMMMMMMMMMMMd`\\n  `hMMMMMMMMMMMMMMMMmMMMMMMMMMMMMMMMy\\n    /mMMMMMMMMMMMMMMMMMMMMMMMMMMMMm:\\n      +dMMMMMMMMMMMMMMMMMMMMMMMMd/\\n        -odMMMMMMMMMMMMMMMMMMdo-\\n           `:+ydmNMMMMNmhy+-`\\n";
// Netrunner: 21 lines, done at line 8234.
// Logo #133: Nitrux...
static const char Nitrux[] = "${c1}`:/.\\n`/yo\\n`/yo\\n`/yo      .+:.\\n`/yo      .sys+:.`\\n`/yo       `-/sys+:.`\\n`/yo           ./sss+:.`\\n`/yo              .:oss+:-`\\n`/yo                 ./o///:-`\\n`/yo              `.-:///////:`\\n`/yo           `.://///++//-``\\n`/yo       `.-:////++++/-`\\n`/yo    `-://///++o+/-`\\n`/yo `-/+o+++ooo+/-`\\n`/s+:+oooossso/.`\\n`//+sssssso:.\\n`+syyyy+:`\\n:+s+-\\n";
// Nitrux: 19 lines, done at line 8258.
// Logo #134: nixos_small...
static const char nixos_small[] = "  ${c1}  \\\\\\\\  \\\\\\\\ //\\n ==\\\\\\\\__\\\\\\\\/ //\\n   //   \\\\\\\\//\\n==//     //==\\n //\\\\\\\\___//\\n// /\\\\\\\\  \\\\\\\\==\\n  // \\\\\\\\  \\\\\\\\\\n";
// nixos_small: 8 lines, done at line 8271.
// Logo #135: NixOS...
static const char NixOS[] = "${c1}          ::::.    ${c2}':::::     ::::'\\n${c1}          ':::::    ${c2}':::::.  ::::'\\n${c1}            :::::     ${c2}'::::.:::::\\n${c1}      .......:::::..... ${c2}::::::::\\n${c1}     ::::::::::::::::::. ${c2}::::::    ${c1}::::.\\n    ::::::::::::::::::::: ${c2}:::::.  ${c1}.::::'\\n${c2}           .....           ::::' ${c1}:::::'\\n${c2}          :::::            '::' ${c1}:::::'\\n${c2} ........:::::               ' ${c1}:::::::::::.\\n${c2}:::::::::::::                 ${c1}:::::::::::::\\n${c2} ::::::::::: ${c1}..              ${c1}:::::\\n${c2}     .::::: ${c1}.:::            ${c1}:::::\\n${c2}    .:::::  ${c1}:::::          ${c1}'''''    ${c2}.....\\n    :::::   ${c1}':::::.  ${c2}......:::::::::::::'\\n     :::     ${c1}::::::. ${c2}':::::::::::::::::'\\n${c1}            .:::::::: ${c2}'::::::::::\\n${c1}           .::::''::::.     ${c2}'::::.\\n${c1}          .::::'   ::::.     ${c2}'::::.\\n${c1}         .::::      ::::      ${c2}'::::.\\n";
// NixOS: 20 lines, done at line 8296.
// Logo #136: Nurunner...
static const char Nurunner[] = "${c1}                  ,xc\\n                ;00cxXl\\n              ;K0,   .xNo.\\n            :KO'       .lXx.\\n          cXk.    ;xl     cXk.\\n        cXk.    ;k:.,xo.    cXk.\\n     .lXx.    :x::0MNl,dd.    :KO,\\n   .xNx.    cx;:KMMMMMNo'dx.    ;KK;\\n .dNl.    cd,cXMMMMMMMMMWd,ox'    'OK:\\n;WK.    'K,.KMMMMMMMMMMMMMWc.Kx     lMO\\n 'OK:    'dl'xWMMMMMMMMMM0::x:    'OK:\\n   .kNo    .xo'xWMMMMMM0;:O:    ;KK;\\n     .dXd.   .do,oNMMO;ck:    ;00,\\n        oNd.   .dx,;'cO;    ;K0,\\n          oNx.    okk;    ;K0,\\n            lXx.        :KO'\\n              cKk'    cXk.\\n                ;00:lXx.\\n                  ,kd.\\n";
// Nurunner: 20 lines, done at line 8321.
// Logo #137: NuTyX...
static const char NuTyX[] = "${c1}                                      .\\n                                    .\\n                                 ...\\n                               ...\\n            ....     .........--.\\n       ..-++-----....--++++++---.\\n    .-++++++-.   .-++++++++++++-----..\\n  .--...  .++..-+++--.....-++++++++++--..\\n .     .-+-. .**-            ....  ..-+----..\\n     .+++.  .*+.         +            -++-----.\\n   .+++++-  ++.         .*+.     .....-+++-----.\\n  -+++-++. .+.          .-+***++***++--++++.  .\\n -+-. --   -.          -*- ......        ..--.\\n.-. .+-    .          -+.\\n.  .+-                +.\\n   --                 --\\n  -+----.              .-\\n  -++-.+.                .\\n .++. --\\n  +.  ----.\\n  .  .+. ..\\n      -  .\\n      .\\n";
// NuTyX: 24 lines, done at line 8350.
// Logo #138: OBRevenge...
static const char OBRevenge[] = "${c1}   __   __\\n     _@@@@   @@@g_\\n   _@@@@@@   @@@@@@\\n  _@@@@@@M   W@@@@@@_\\n j@@@@P        ^W@@@@\\n @@@@L____  _____Q@@@@\\nQ@@@@@@@@@@j@@@@@@@@@@\\n@@@@@    T@j@    T@@@@@\\n@@@@@ ___Q@J@    _@@@@@\\n@@@@@fMMM@@j@jggg@@@@@@\\n@@@@@    j@j@^MW@P @@@@\\nQ@@@@@ggg@@f@   @@@@@@L\\n^@@@@WWMMP  ^    Q@@@@\\n @@@@@_         _@@@@l\\n  W@@@@@g_____g@@@@@P\\n   @@@@@@@@@@@@@@@@l\\n    ^W@@@@@@@@@@@P\\n       ^TMMMMTll\\n";
// OBRevenge: 19 lines, done at line 8374.
// Logo #139: openbsd_small...
static const char openbsd_small[] = "${c1}      _____\\n    \\\\-     -/\\n \\\\_/         \\\\\\n |        ${c2}O O${c1} |\\n |_  <   )  3 )\\n / \\\\         /\\n    /-_____-\\\\\\n";
// openbsd_small: 8 lines, done at line 8387.
// Logo #140: OpenBSD...
static const char OpenBSD[] = "${c3}                                     _\\n                                    (_)\\n${c1}              |    .\\n${c1}          .   |L  /|   .         ${c3} _\\n${c1}      _ . |\\ _| \\--+._/| .       ${c3}(_)\\n${c1}     / ||\\| Y J  )   / |/| ./\\n    J  |)'( |        ` F`.'/       ${c3} _\\n${c1}  -<|  F         __     .-<        ${c3}(_)\\n${c1}    | /       .-'${c3}. ${c1}`.  /${c3}-. ${c1}L___\\n    J \\\\      <    ${c3}\\ ${c1} | | ${c5}O${c3}\\\\${c1}|.-' ${c3} _\\n${c1}  _J \\\\  .-    \\\\${c3}/ ${c5}O ${c3}| ${c1}| \\\\  |${c1}F    ${c3}(_)\\n${c1} '-F  -<_.     \\\\   .-'  `-' L__\\n__J  _   _.     >-'  ${c1})${c4}._.   ${c1}|-'\\n${c1} `-|.'   /_.          ${c4}\\_|  ${c1} F\\n  /.-   .                _.<\\n /'    /.'             .'  `\\\\\\n  /L  /'   |/      _.-'-\\\\\\n /'J       ___.---'\\|\\n   |\\  .--' V  | `. `\\n   |/`. `-.     `._)\\n      / .-.\\\\\\n      \\\\ (  `\\\\\\n       `.\\\\\\n";
// OpenBSD: 24 lines, done at line 8416.
// Logo #141: openEuler...
static const char openEuler[] = "${c1}\\n                       (#####\\n                     (((########  #####\\n                    (((        ##########    __...__\\n             ((((((((           #######    /((((((###\\\\n           (((((((((((   .......           \\(((((####/\\n          ((((((    ((((#########            *******\\n    %((((((#          ((########\\n /////(((((              ###\\n/////(((((((#   (((&\\n         (((((((((((((\\n          ((((((((((((\\n           (((((((((     ((((((###\\n                       /((((((######\\n                      //((((((######\\n                       /((((((#####\\n                        *********/\\n";
// openEuler: 18 lines, done at line 8439.
// Logo #142: OpenIndiana...
static const char OpenIndiana[] = "${c2}                         .sy/\\n                         .yh+\\n\\n           ${c1}-+syyyo+-     ${c2} /+.\\n         ${c1}+ddo/---/sdh/   ${c2} ym-\\n       ${c1}`hm+        `sms${c2}   ym-```````.-.\\n       ${c1}sm+           sm/ ${c2} ym-         +s\\n       ${c1}hm.           /mo ${c2} ym-         /h\\n       ${c1}omo           ym: ${c2} ym-       `os`\\n        ${c1}smo`       .ym+ ${c2}  ym-     .os-\\n     ``  ${c1}:ymy+///oyms- ${c2}   ym-  .+s+.\\n   ..`     ${c1}`:+oo+/-`  ${c2}    -//oyo-\\n -:`                   .:oys/.\\n+-               `./oyys/.\\nh+`      `.-:+oyyyo/-`\\n`/ossssysso+/-.`\\n";
// OpenIndiana: 17 lines, done at line 8461.
// Logo #143: openmamba...
static const char openmamba[] = "${c1}                 `````\\n           .-/+ooooooooo+/:-`\\n        ./ooooooooooooooooooo+:.\\n      -+oooooooooooooooooooooooo+-\\n    .+ooooooooo+/:---::/+ooooooooo+.\\n   :oooooooo/-`          `-/oo${c2}s´${c1}oooo.${c2}s´${c1}\\n  :ooooooo/`                `${c2}sNds${c1}ooo${c2}sNds${c1}\\n -ooooooo-                   ${c2}:dmy${c1}ooo${c2}:dmy${c1}\\n +oooooo:                      :oooooo-\\n.ooooooo                        .://:`\\n:oooooo+                        ./+o+:`\\n-ooooooo`                      `oooooo+\\n`ooooooo:                      /oooooo+\\n -ooooooo:                    :ooooooo.\\n  :ooooooo+.                .+ooooooo:\\n   :oooooooo+-`          `-+oooooooo:\\n    .+ooooooooo+/::::://oooooooooo+.\\n      -+oooooooooooooooooooooooo+-\\n        .:ooooooooooooooooooo+:.\\n           `-:/ooooooooo+/:.`\\n                 ``````\\n";
// openmamba: 22 lines, done at line 8488.
// Logo #144: OpenMandriva...
static const char OpenMandriva[] = "${c1}                  ``````\\n            `-:/+++++++//:-.`\\n         .:+++oooo+/:.``   ``\\n      `:+ooooooo+:.  `-:/++++++/:.`\\n     -+oooooooo:` `-++o+/::::://+o+/-\\n   `/ooooooooo-  -+oo/.`        `-/oo+.\\n  `+ooooooooo.  :os/`              .+so:\\n  +sssssssss/  :ss/                 `+ss-\\n :ssssssssss`  sss`                  .sso\\n ossssssssss  `yyo                    sys\\n`sssssssssss` `yys                   `yys\\n`sssssssssss:  +yy/                  +yy:\\n oyyyyyyyyyys. `oyy/`              `+yy+\\n :yyyyyyyyyyyo. `+yhs:.         `./shy/\\n  oyyyyyyyyyyys:` .oyhys+:----/+syhy+. `\\n  `syyyyyyyyyyyyo-` .:osyhhhhhyys+:``.:`\\n   `oyyyyyyyyyyyyys+-`` `.----.```./oo.\\n     /yhhhhhhhhhhhhhhyso+//://+osyhy/`\\n      `/yhhhhhhhhhhhhhhhhhhhhhhhhy/`\\n        `:oyhhhhhhhhhhhhhhhhhhyo:`\\n            .:+syhhhhhhhhys+:-`\\n                 ``....``\\n";
// OpenMandriva: 23 lines, done at line 8516.
// Logo #145: OpenStage...
static const char OpenStage[] = "${c1}                 /(/\\n              .(((((((,\\n             /(((((((((/\\n           .(((((/,/(((((,\\n          *(((((*   ,(((((/\\n          (((((*      .*/((\\n         *((((/  (//(/*\\n         /((((*  ((((((((((,\\n      .  /((((*  (((((((((((((.\\n     ((. *((((/        ,((((((((\\n   ,(((/  (((((/     **   ,((((((*\\n  /(((((. .(((((/   //(((*  *(((((/\\n .(((((,    ((/   .(((((/.   .(((((,\\n /((((*        ,(((((((/      ,(((((\\n /(((((((((((((((((((/.  /(((((((((/\\n /(((((((((((((((((,   /(((((((((((/\\n     */(((((//*.      */((/(/(/*\\n";
// OpenStage: 18 lines, done at line 8539.
// Logo #146: OpenWrt...
static const char OpenWrt[] = "${c1} _______\\n|       |.-----.-----.-----.\\n|   -   ||  _  |  -__|     |\\n|_______||   __|_____|__|__|\\n         |__|\\n ________        __\\n|  |  |  |.----.|  |_\\n|  |  |  ||   _||   _|\\n|________||__|  |____|\\n";
// OpenWrt: 10 lines, done at line 8554.
// Logo #147: osmc...
static const char osmc[] = "${c1}            -+shdmNNNNmdhs+-\\n        .+hMNho/:..``..:/ohNMh+.\\n      :hMdo.                .odMh:\\n    -dMy-                      -yMd-\\n   sMd-                          -dMs\\n  hMy       +.            .+       yMh\\n yMy        dMs.        .sMd        yMy\\n:Mm         dMNMs`    `sMNMd        `mM:\\nyM+         dM//mNs``sNm//Md         +My\\nmM-         dM:  +NNNN+  :Md         -Mm\\nmM-         dM: `oNN+    :Md         -Mm\\nyM+         dM/+NNo`     :Md         +My\\n:Mm`        dMMNs`       :Md        `mM:\\n yMy        dMs`         -ms        yMy\\n  hMy       +.                     yMh\\n   sMd-                          -dMs\\n    -dMy-                      -yMd-\\n      :hMdo.                .odMh:\\n        .+hMNho/:..``..:/ohNMh+.\\n            -+shdmNNNNmdhs+-\\n";
// osmc: 21 lines, done at line 8580.
// Logo #148: Oracle...
static const char Oracle[] = "${c1}\\n      `-/+++++++++++++++++/-.`\\n   `/syyyyyyyyyyyyyyyyyyyyyyys/.\\n  :yyyyo/-...............-/oyyyy/\\n /yyys-                     .oyyy+\\n.yyyy`                       `syyy-\\n:yyyo                         /yyy/\\n.yyyy`                       `syyy-\\n /yyys.                     .oyyyo\\n  /yyyyo:-...............-:oyyyy/`\\n   `/syyyyyyyyyyyyyyyyyyyyyyys+.\\n     `.:/+ooooooooooooooo+/:.`\\n";
// Oracle: 13 lines, done at line 8598.
// Logo #149: OS Elbrus...
static const char OS_Elbrus[] = "${c1}   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄\\n   ██▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀██\\n   ██                       ██\\n   ██   ███████   ███████   ██\\n   ██   ██   ██   ██   ██   ██\\n   ██   ██   ██   ██   ██   ██\\n   ██   ██   ██   ██   ██   ██\\n   ██   ██   ██   ██   ██   ██\\n   ██   ██   ███████   ███████\\n   ██   ██                  ██\\n   ██   ██▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄██\\n   ██   ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀██\\n   ██                       ██\\n   ███████████████████████████\\n";
// OS_Elbrus: 15 lines, done at line 8618.
// Logo #150: PacBSD...
static const char PacBSD[] = "${c1}      :+sMs.\\n  `:ddNMd-                         -o--`\\n -sMMMMh:                          `+N+``\\n yMMMMMs`     .....-/-...           `mNh/\\n yMMMMMmh+-`:sdmmmmmmMmmmmddy+-``./ddNMMm\\n yNMMNMMMMNdyyNNMMMMMMMMMMMMMMMhyshNmMMMm\\n :yMMMMMMMMMNdooNMMMMMMMMMMMMMMMMNmy:mMMd\\n  +MMMMMMMMMmy:sNMMMMMMMMMMMMMMMMMMMmshs-\\n  :hNMMMMMMN+-+MMMMMMMMMMMMMMMMMMMMMMMs.\\n .omysmNNhy/+yNMMMMMMMMMMNMMMMMMMMMNdNNy-\\n /hMM:::::/hNMMMMMMMMMMMm/-yNMMMMMMN.mMNh`\\n.hMMMMdhdMMMMMMMMMMMMMMmo  `sMMMMMMN mMMm-\\n:dMMMMMMMMMMMMMMMMMMMMMdo+  oMMMMMMN`smMNo`\\n/dMMMMMMMMMMMMMMMMMMMMMNd/` :yMMMMMN:-hMMM.\\n:dMMMMMMMMMMMMMMMMMMMMMNh`  oMMMMMMNo/dMNN`\\n:hMMMMMMMMMMMMMMMMMMMMMMNs--sMMMMMMMNNmy++`\\n sNMMMMMMMMMMMMMMMMMMMMMMMmmNMMMMMMNho::o.\\n :yMMMMMMMMMMMMMNho+sydNNNNNNNmysso/` -//\\n  /dMMMMMMMMMMMMMs-  ````````..``\\n   .oMMMMMMMMMMMMNs`               ./y:`\\n     +dNMMNMMMMMMMmy`          ``./ys.\\n      `/hMMMMMMMMMMMNo-``    `.+yy+-`\\n        `-/hmNMNMMMMMMmmddddhhy/-`\\n            `-+oooyMMMdsoo+/:.\\n";
// PacBSD: 25 lines, done at line 8648.
// Logo #151: parabola_small...
static const char parabola_small[] = "${c1}  __ __ __  _\\n.`_//_//_/ / `.\\n          /  .`\\n         / .`\\n        /.`\\n       /`\\n";
// parabola_small: 7 lines, done at line 8660.
// Logo #152: Parabola...
static const char Parabola[] = "${c1}                          `.-.    `.\\n                   `.`  `:++.   `-+o+.\\n             `` `:+/. `:+/.   `-+oooo+\\n        ``-::-.:+/. `:+/.   `-+oooooo+\\n    `.-:///-  ..`   .-.   `-+oooooooo-\\n `..-..`                 `+ooooooooo:\\n``                        :oooooooo/\\n                          `ooooooo:\\n                          `oooooo:\\n                          -oooo+.\\n                          +ooo/`\\n                         -ooo-\\n                        `+o/.\\n                        /+-\\n                       //`\\n                      -.\\n";
// Parabola: 17 lines, done at line 8682.
// Logo #153: Pardus...
static const char Pardus[] = "${c1} .smNdy+-    `.:/osyyso+:.`    -+ydmNs.\\n/Md- -/ymMdmNNdhso/::/oshdNNmdMmy/. :dM/\\nmN.     oMdyy- -y          `-dMo     .Nm\\n.mN+`  sMy hN+ -:             yMs  `+Nm.\\n `yMMddMs.dy `+`               sMddMMy`\\n   +MMMo  .`  .                 oMMM+\\n   `NM/    `````.`    `.`````    +MN`\\n   yM+   `.-:yhomy    ymohy:-.`   +My\\n   yM:          yo    oy          :My\\n   +Ms         .N`    `N.      +h sM+\\n   `MN      -   -::::::-   : :o:+`NM`\\n    yM/    sh   -dMMMMd-   ho  +y+My\\n    .dNhsohMh-//: /mm/ ://-yMyoshNd`\\n      `-ommNMm+:/. oo ./:+mMNmmo:`\\n     `/o+.-somNh- :yy: -hNmos-.+o/`\\n    ./` .s/`s+sMdd+``+ddMs+s`/s. `/.\\n        : -y.  -hNmddmNy.  .y- :\\n         -+       `..`       +-\\n";
// Pardus: 19 lines, done at line 8706.
// Logo #154: Parrot...
static const char Parrot[] = "${c1}  `:oho/-`\\n`mMMMMMMMMMMMNmmdhy-\\n dMMMMMMMMMMMMMMMMMMs`\\n +MMsohNMMMMMMMMMMMMMm/\\n .My   .+dMMMMMMMMMMMMMh.\\n  +       :NMMMMMMMMMMMMNo\\n           `yMMMMMMMMMMMMMm:\\n             /NMMMMMMMMMMMMMy`\\n              .hMMMMMMMMMMMMMN+\\n                  ``-NMMMMMMMMMd-\\n                     /MMMMMMMMMMMs`\\n                      mMMMMMMMsyNMN/\\n                      +MMMMMMMo  :sNh.\\n                      `NMMMMMMm     -o/\\n                       oMMMMMMM.\\n                       `NMMMMMM+\\n                        +MMd/NMh\\n                         mMm -mN`\\n                         /MM  `h:\\n                          dM`   .\\n                          :M-\\n                           d:\\n                           -+\\n                            -\\n";
// Parrot: 25 lines, done at line 8736.
// Logo #155: Parsix...
static const char Parsix[] = "                 ${c2}-/+/:.\\n               ${c2}.syssssys.\\n       ${c1}.--.    ${c2}ssssssssso${c1}   ..--.\\n     :++++++:  ${c2}+ssssssss+${c1} ./++/+++:\\n    /+++++++++.${c2}.yssooooy`${c1}-+///////o-\\n    /++++++++++.${c2}+soooos:${c1}:+////////+-\\n     :+++++////o-${c2}oooooo-${c1}+/////////-\\n      `-/++//++-${c4}.-----.-${c1}:+/////:-\\n  ${c3}-://::--${c1}-:/:${c4}.--.````.--.${c1}:::-${c3}--::::::.\\n${c3}-/:::::::://:${c4}.:-`      `-:${c3}`:/:::::::--/-\\n${c3}/::::::::::/-${c4}--.        .-.${c3}-/://///::::/\\n${c3}-/:::::::::/:${c4}`:-.      .-:${c3}`:///////////-\\n `${c3}-::::--${c1}.-://.${c4}---....---${c1}`:+/:-${c3}--::::-`\\n       ${c1}-/+///+o/-${c4}.----.${c1}.:oo+++o+.\\n     ${c1}-+/////+++o:${c2}syyyyy.${c1}o+++++++++:\\n    ${c1}.+////+++++-${c2}+sssssy+${c1}.++++++++++\\\\n    ${c1}.+:/++++++.${c2}.yssssssy-${c1}`+++++++++:\\n     ${c1}:/+++++-  ${c2}+sssssssss  ${c1}-++++++-\\n       ${c1}`--`    ${c2}+sssssssso    ${c1}`--`\\n                ${c2}+sssssy+`\\n                 ${c2}`.::-`\\n";
// Parsix: 22 lines, done at line 8763.
// Logo #156: TrueOS...
static const char TrueOS[] = "${c1}                       ..\\n                        s.\\n                        +y\\n                        yN\\n                       -MN  `.\\n                      :NMs `m\\n                    .yMMm` `No\\n            `-/+++sdMMMNs+-`+Ms\\n        `:oo+-` .yMMMMy` `-+oNMh\\n      -oo-     +NMMMM/       oMMh-\\n    .s+` `    oMMMMM/     -  oMMMhy.\\n   +s`- ::   :MMMMMd     -o `mMMMy`s+\\n  y+  h .Ny+oNMMMMMN/    sh+NMMMMo  +y\\n s+ .ds  -NMMMMMMMMMMNdhdNMMMMMMh`   +s\\n-h .NM`   `hMMMMMMMMMMMMMMNMMNy:      h-\\ny- hMN`     hMMmMMMMMMMMMNsdMNs.      -y\\nm` mMMy`    oMMNoNMMMMMMo`  sMMMo     `m\\nm` :NMMMdyydMMMMo+MdMMMs     sMMMd`   `m\\nh-  `+ymMMMMMMMM--M+hMMN/    +MMMMy   -h\\n:y     `.sMMMMM/ oMM+.yMMNddNMMMMMm   y:\\n y:   `s  dMMN- .MMMM/ :MMMMMMMMMMh  :y\\n `h:  `mdmMMM/  yMMMMs  sMMMMMMMMN- :h`\\n   so  -NMMMN   /mmd+  `dMMMMMMMm- os\\n    :y: `yMMM`       `+NMMMMMMNo`:y:\\n      /s+`.omy      /NMMMMMNh/.+s:\\n        .+oo:-.     /mdhs+::oo+.\\n            -/o+++++++++++/-\\n";
// TrueOS: 28 lines, done at line 8796.
// Logo #157: PCLinuxOS...
static const char PCLinuxOS[] = "            ${c1}mhhhyyyyhhhdN\\n        dyssyhhhhhhhhhhhssyhN\\n     Nysyhhyo/:-.....-/oyhhhssd\\n   Nsshhy+.              `/shhysm\\n  dohhy/                    -shhsy\\n dohhs`                       /hhys\\nN+hho   ${c2}+ssssss+-   .+syhys+   ${c1}/hhsy\\nohhh`   ${c2}ymmo++hmm+`smmy/::+y`   ${c1}shh+\\n+hho    ${c2}ymm-  /mmy+mms          ${c1}:hhod\\n/hh+    ${c2}ymmhhdmmh.smm/          ${c1}.hhsh\\n+hhs    ${c2}ymm+::-`  /mmy`    `    ${c1}/hh+m\\nyyhh-   ${c2}ymm-       /dmdyosyd`  ${c1}`yhh+\\n ohhy`  ${c2}://`         -/+++/-   ${c1}ohhom\\n N+hhy-                      `shhoh\\n   sshho.                  `+hhyom\\n    dsyhhs/.            `:ohhhoy\\n      dysyhhhso///://+syhhhssh\\n         dhyssyhhhhhhyssyyhN\\n              mddhdhdmN\\n";
// PCLinuxOS: 20 lines, done at line 8821.
// Logo #158: Peppermint...
static const char Peppermint[] = "${c1}               PPPPPPPPPPPPPP\\n${c1}           PPPP${c2}MMMMMMM${c1}PPPPPPPPPPP\\n${c1}         PPPP${c2}MMMMMMMMMM${c1}PPPPPPPP${c2}MM${c1}PP\\n${c1}       PPPPPPPP${c2}MMMMMMM${c1}PPPPPPPP${c2}MMMMM${c1}PP\\n${c1}     PPPPPPPPPPPP${c2}MMMMMM${c1}PPPPPPP${c2}MMMMMMM${c1}PP\\n${c1}    PPPPPPPPPPPP${c2}MMMMMMM${c1}PPPP${c2}M${c1}P${c2}MMMMMMMMM${c1}PP\\n${c1}   PP${c2}MMMM${c1}PPPPPPPPPP${c2}MMM${c1}PPPPP${c2}MMMMMMM${c1}P${c2}MM${c1}PPPP\\n${c1}   P${c2}MMMMMMMMMM${c1}PPPPPP${c2}MM${c1}PPPPP${c2}MMMMMM${c1}PPPPPPPP\\n${c1}  P${c2}MMMMMMMMMMMM${c1}PPPPP${c2}MM${c1}PP${c2}M${c1}P${c2}MM${c1}P${c2}MM${c1}PPPPPPPPPPP\\n${c1}  P${c2}MMMMMMMMMMMMMMMM${c1}PP${c2}M${c1}P${c2}MMM${c1}PPPPPPPPPPPPPPPP\\n${c1}  P${c2}MMM${c1}PPPPPPPPPPPPPPPPPPPPPPPPPPPPPP${c2}MMMMM${c1}P\\n${c1}  PPPPPPPPPPPPPPPP${c2}MMM${c1}P${c2}M${c1}P${c2}MMMMMMMMMMMMMMMM${c1}PP\\n${c1}  PPPPPPPPPPP${c2}MM${c1}P${c2}MM${c1}PPPP${c2}MM${c1}PPPPP${c2}MMMMMMMMMMM${c1}PP\\n${c1}   PPPPPPPP${c2}MMMMMM${c1}PPPPP${c2}MM${c1}PPPPPP${c2}MMMMMMMMM${c1}PP\\n${c1}   PPPP${c2}MM${c1}P${c2}MMMMMMM${c1}PPPPPP${c2}MM${c1}PPPPPPPPPP${c2}MMMM${c1}PP\\n${c1}    PP${c2}MMMMMMMMM${c1}P${c2}M${c1}PPPP${c2}MMMMMM${c1}PPPPPPPPPPPPP\\n${c1}     PP${c2}MMMMMMM${c1}PPPPPPP${c2}MMMMMM${c1}PPPPPPPPPPPP\\n${c1}       PP${c2}MMMM${c1}PPPPPPPPP${c2}MMMMMMM${c1}PPPPPPPP\\n${c1}         PP${c2}MM${c1}PPPPPPPP${c2}MMMMMMMMMM${c1}PPPP\\n${c1}           PPPPPPPPPP${c2}MMMMMMMM${c1}PPPP\\n${c1}               PPPPPPPPPPPPPP\\n";
// Peppermint: 22 lines, done at line 8848.
// Logo #159: pop_os_small...
static const char pop_os_small[] = "${c1}______\\n\\\\   _ \\\\        __\\n \\\\ \\\\ \\\\ \\\\      / /\\n  \\\\ \\\\_\\\\ \\\\    / /\\n   \\\\  ___\\\\  /_/\\n    \\\\ \\\\    _\\n   __\\\\_\\\\__(_)_\\n  (___________)`\\n";
// pop_os_small: 9 lines, done at line 8862.
// Logo #160: pop_os...
static const char pop_os[] = "${c1}             /////////////\\n         /////////////////////\\n      ///////${c2}*767${c1}////////////////\\n    //////${c2}7676767676*${c1}//////////////\\n   /////${c2}76767${c1}//${c2}7676767${c1}//////////////\\n  /////${c2}767676${c1}///${c2}*76767${c1}///////////////\\n ///////${c2}767676${c1}///${c2}76767${c1}.///${c2}7676*${c1}///////\\n/////////${c2}767676${c1}//${c2}76767${c1}///${c2}767676${c1}////////\\n//////////${c2}76767676767${c1}////${c2}76767${c1}/////////\\n///////////${c2}76767676${c1}//////${c2}7676${c1}//////////\\n////////////,${c2}7676${c1},///////${c2}767${c1}///////////\\n/////////////*${c2}7676${c1}///////${c2}76${c1}////////////\\n///////////////${c2}7676${c1}////////////////////\\n ///////////////${c2}7676${c1}///${c2}767${c1}////////////\\n  //////////////////////${c2}'${c1}////////////\\n   //////${c2}.7676767676767676767,${c1}//////\\n    /////${c2}767676767676767676767${c1}/////\\n      ///////////////////////////\\n         /////////////////////\\n             /////////////\\n";
// pop_os: 21 lines, done at line 8888.
// Logo #161: Porteus...
static const char Porteus[] = "${c1}             `.-:::-.`\\n         -+ydmNNNNNNNmdy+-\\n      .+dNmdhs+//////+shdmdo.\\n    .smmy+-`             ./sdy:\\n  `omdo.    `.-/+osssso+/-` `+dy.\\n `yms.   `:shmNmdhsoo++osyyo-``oh.\\n hm/   .odNmds/.`    ``.....:::-+s\\n/m:  `+dNmy:`   `./oyhhhhyyooo++so\\nys  `yNmy-    .+hmmho:-.`     ```\\ns:  yNm+`   .smNd+.\\n`` /Nm:    +dNd+`\\n   yN+   `smNy.\\n   dm    oNNy`\\n   hy   -mNm.\\n   +y   oNNo\\n   `y`  sNN:\\n    `:  +NN:\\n     `  .mNo\\n         /mm`\\n          /my`\\n           .sy`\\n             .+:\\n                `\\n";
// Porteus: 24 lines, done at line 8917.
// Logo #162: postmarketos_small...
static const char postmarketos_small[] = "${c1}        /\\\\\\n       /  \\\\\\n      /    \\\\\\n      \\\\__   \\\\\\n    /\\\\__ \\\\  _\\\\\\n   /   /  \\\\/ __\\n  /   / ____/  \\\\\\n /    \\\\ \\\\       \\\\\\n/_____/ /________\\\\\\n";
// postmarketos_small: 10 lines, done at line 8932.
// Logo #163: PostMarketOS...
static const char PostMarketOS[] = "${c1}                 /\\\\\\n                /  \\\\\\n               /    \\\\\\n              /      \\\\\\n             /        \\\\\\n            /          \\\\\\n            \\\\           \\\\\\n          /\\\\ \\\\____       \\\\\\n         /  \\\\____ \\\\       \\\\\\n        /       /  \\\\       \\\\\\n       /       /    \\\\    ___\\\\\\n      /       /      \\\\  / ____\\n     /       /        \\\\/ /    \\\\\\n    /       / __________/      \\\\\\n   /        \\\\ \\\\                 \\\\\\n  /          \\\\ \\\\                 \\\\\\n /           / /                  \\\\\\n/___________/ /____________________\\\\\\n";
// PostMarketOS: 19 lines, done at line 8956.
// Logo #164: Proxmox...
static const char Proxmox[] = "${c1}         .://:`              `://:.\\n       `hMMMMMMd/          /dMMMMMMh`\\n        `sMMMMMMMd:      :mMMMMMMMs`\\n${c2}`-/+oo+/:${c1}`.yMMMMMMMh-  -hMMMMMMMy.`${c2}:/+oo+/-`\\n`:oooooooo/${c1}`-hMMMMMMMyyMMMMMMMh-`${c2}/oooooooo:`\\n  `/oooooooo:${c1}`:mMMMMMMMMMMMMm:`${c2}:oooooooo/`\\n    ./ooooooo+-${c1} +NMMMMMMMMN+ ${c2}-+ooooooo/.\\n      .+ooooooo+-${c1}`oNMMMMNo`${c2}-+ooooooo+.\\n        -+ooooooo/.${c1}`sMMs`${c2}./ooooooo+-\\n          :oooooooo/${c1}`..`${c2}/oooooooo:\\n          :oooooooo/`${c1}..${c2}`/oooooooo:\\n        -+ooooooo/.`${c1}sMMs${c2}`./ooooooo+-\\n      .+ooooooo+-`${c1}oNMMMMNo${c2}`-+ooooooo+.\\n    ./ooooooo+-${c1} +NMMMMMMMMN+ ${c2}-+ooooooo/.\\n  `/oooooooo:`${c1}:mMMMMMMMMMMMMm:${c2}`:oooooooo/`\\n`:oooooooo/`${c1}-hMMMMMMMyyMMMMMMMh-${c2}`/oooooooo:`\\n`-/+oo+/:`${c1}.yMMMMMMMh-  -hMMMMMMMy.${c2}`:/+oo+/-`\\n${c1}        `sMMMMMMMm:      :dMMMMMMMs`\\n       `hMMMMMMd/          /dMMMMMMh`\\n         `://:`              `://:`\\n";
// Proxmox: 21 lines, done at line 8982.
// Logo #165: Precise Puppy...
static const char Precise_Puppy[] = "${c1}           `-/osyyyysosyhhhhhyys+-\\n  -ohmNNmh+/hMMMMMMMMNNNNd+dMMMMNM+\\n yMMMMNNmmddo/NMMMNNNNNNNNNo+NNNNNy\\n.NNNNNNmmmddds:MMNNNNNNNNNNNh:mNNN/\\n-NNNdyyyhdmmmd`dNNNNNmmmmNNmdd/os/\\n.Nm+shddyooo+/smNNNNmmmmNh.   :mmd.\\n NNNNy:`   ./hmmmmmmmNNNN:     hNMh\\n NMN-    -++- +NNNNNNNNNNm+..-sMMMM-\\n.MMo    oNNNNo hNNNNNNNNmhdNNNMMMMM+\\n.MMs    /NNNN/ dNmhs+:-`  yMMMMMMMM+\\n mMM+     .. `sNN+.      hMMMMhhMMM-\\n +MMMmo:...:sNMMMMMms:` hMMMMm.hMMy\\n  yMMMMMMMMMMMNdMMMMMM::/+o+//dMMd`\\n   sMMMMMMMMMMN+:oyyo:sMMMNNMMMNy`\\n    :mMMMMMMMMMMMmddNMMMMMMMMmh/\\n      /dMMMMMMMMMMMMMMMMMMNdy/`\\n        .+hNMMMMMMMMMNmdhs/.\\n            .:/+ooo+/:-.\\n";
// Precise_Puppy: 19 lines, done at line 9006.
// Logo #166: pureos_small...
static const char pureos_small[] = "${c1} _____________\\n|  _________  |\\n| |         | |\\n| |         | |\\n| |_________| |\\n|_____________|\\n";
// pureos_small: 7 lines, done at line 9018.
// Logo #167: PureOS...
static const char PureOS[] = "${c1}dmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmd\\ndNm//////////////////////////////////mNd\\ndNd                                  dNd\\ndNd                                  dNd\\ndNd                                  dNd\\ndNd                                  dNd\\ndNd                                  dNd\\ndNd                                  dNd\\ndNd                                  dNd\\ndNd                                  dNd\\ndNm//////////////////////////////////mNd\\ndmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmd\\n";
// PureOS: 13 lines, done at line 9036.
// Logo #168: Qubes...
static const char Qubes[] = "${c1}               `..--..`\\n            `.----------.`\\n        `..----------------..`\\n     `.------------------------.``\\n `..-------------....-------------..`\\n.::----------..``    ``..----------:+:\\n:////:----..`            `..---:/ossso\\n:///////:`                  `/osssssso\\n:///////:                    /ssssssso\\n:///////:                    /ssssssso\\n:///////:                    /ssssssso\\n:///////:                    /ssssssso\\n:///////:                    /ssssssso\\n:////////-`                .:sssssssso\\n:///////////-.`        `-/osssssssssso\\n`//////////////:-```.:+ssssssssssssso-\\n  .-://////////////sssssssssssssso/-`\\n     `.:///////////sssssssssssssso:.\\n         .-:///////ssssssssssssssssss/`\\n            `.:////ssss+/+ssssssssssss.\\n                `--//-    `-/osssso/.\\n";
// Qubes: 22 lines, done at line 9063.
// Logo #169: Radix...
static const char Radix[] = "${c2}                .:oyhdmNo\\n             `/yhyoosdms`\\n            -o+/ohmmho-\\n           ..`.:/:-`\\n     `.--:::-.``${c1}\\n  .+ydNMMMMMMNmhs:`\\n`omMMMMMMMMMMMMMMNh-\\noNMMMNmddhhyyhhhddmy.\\nmMMMMNmmddhhysoo+/:-`\\nyMMMMMMMMMMMMMMMMNNh.\\n-dmmmmmNNMMMMMMMMMMs`\\n -+oossyhmMMMMMMMMd-\\n `sNMMMMMMMMMMMMMm:\\n  `yMMMMMMNmdhhhh:\\n   `sNMMMMMNmmho.\\n    `+mMMMMMMMy.\\n      .yNMMMm+`\\n       `:yd+.\\n";
// Radix: 19 lines, done at line 9087.
// Logo #170: Raspbian_small...
static const char Raspbian_small[] = "${c1}   .~~.   .~~.\\n  '. \\\\ ' ' / .'\\n${c2}   .~ .~~~..~.\\n  : .~.'~'.~. :\\n ~ (   ) (   ) ~\\n( : '~'.~.'~' : )\\n ~ .~ (   ) ~. ~\\n  (  : '~' :  )\\n   '~ .~~~. ~'\\n       '~'\\n";
// Raspbian_small: 11 lines, done at line 9103.
// Logo #171: Raspbian...
static const char Raspbian[] = "${c1}  `.::///+:/-.        --///+//-:``\\n `+oooooooooooo:   `+oooooooooooo:\\n  /oooo++//ooooo:  ooooo+//+ooooo.\\n  `+ooooooo:-:oo-  +o+::/ooooooo:\\n   `:oooooooo+``    `.oooooooo+-\\n     `:++ooo/.        :+ooo+/.`\\n        ${c2}...`  `.----.` ``..\\n     .::::-``:::::::::.`-:::-`\\n    -:::-`   .:::::::-`  `-:::-\\n   `::.  `.--.`  `` `.---.``.::`\\n       .::::::::`  -::::::::` `\\n .::` .:::::::::- `::::::::::``::.\\n-:::` ::::::::::.  ::::::::::.`:::-\\n::::  -::::::::.   `-::::::::  ::::\\n-::-   .-:::-.``....``.-::-.   -::-\\n .. ``       .::::::::.     `..`..\\n   -:::-`   -::::::::::`  .:::::`\\n   :::::::` -::::::::::` :::::::.\\n   .:::::::  -::::::::. ::::::::\\n    `-:::::`   ..--.`   ::::::.\\n      `...`  `...--..`  `...`\\n            .::::::::::\\n             `.-::::-`\\n";
// Raspbian: 24 lines, done at line 9132.
// Logo #172: Reborn...
static const char Reborn[] = "${c3}\\n        mMMMMMMMMM  MMMMMMMMMm\\n       NM                    MN\\n      MM  ${c1}dddddddd  dddddddd  ${c3}MN\\n     mM  ${c1}dd                dd  ${c3}MM\\n        ${c1}dd  hhhhhh   hhhhh  dd\\n   ${c3}mM      ${c1}hh            hh      ${c3}Mm\\n  NM  ${c1}hd       ${c3}mMMMMMMd       ${c1}dh  ${c3}MN\\n NM  ${c1}dd  hh   ${c3}mMMMMMMMMm   ${c1}hh  dd  ${c3}MN\\nNM  ${c1}dd  hh   ${c3}mMMMMMMMMMMm   ${c1}hh  dd  ${c3}MN\\n NM  ${c1}dd  hh   ${c3}mMMMMMMMMm   ${c1}hh  dd  ${c3}MN\\n  NM  ${c1}hd       ${c3}mMMMMMMm       ${c1}dh  ${c3}MN\\n   mM      ${c1}hh            hh      ${c3}Mm\\n        ${c1}dd  hhhhhh  hhhhhh  dd\\n     ${c3}MM  ${c1}dd                dd  ${c3}MM\\n      MM  ${c1}dddddddd  dddddddd  ${c3}MN\\n       NM                    MN\\n        mMMMMMMMMM  MMMMMMMMMm\\n";
// Reborn: 19 lines, done at line 9156.
// Logo #173: Redstar...
static const char Redstar[] = "${c1}                    ..\\n                  .oK0l\\n                 :0KKKKd.\\n               .xKO0KKKKd\\n              ,Od' .d0000l\\n             .c;.   .'''...           ..'.\\n.,:cloddxxxkkkkOOOOkkkkkkkkxxxxxxxxxkkkx:\\n;kOOOOOOOkxOkc'...',;;;;,,,'',;;:cllc:,.\\n .okkkkd,.lko  .......',;:cllc:;,,'''''.\\n   .cdo. :xd' cd:.  ..';'',,,'',,;;;,'.\\n      . .ddl.;doooc'..;oc;'..';::;,'.\\n        coo;.oooolllllllcccc:'.  .\\n       .ool''lllllccccccc:::::;.\\n       ;lll. .':cccc:::::::;;;;'\\n       :lcc:'',..';::::;;;;;;;,,.\\n       :cccc::::;...';;;;;,,,,,,.\\n       ,::::::;;;,'.  ..',,,,'''.\\n        ........          ......\\n";
// Redstar: 19 lines, done at line 9180.
// Logo #174: Redcore...
static const char Redcore[] = "${c1}                 RRRRRRRRR\\n               RRRRRRRRRRRRR\\n        RRRRRRRRRR      RRRRR\\n   RRRRRRRRRRRRRRRRRRRRRRRRRRR\\n RRRRRRR  RRR         RRR RRRRRRRR\\nRRRRR    RR                 RRRRRRRRR\\nRRRR    RR     RRRRRRRR      RR RRRRRR\\nRRRR   R    RRRRRRRRRRRRRR   RR   RRRRR\\nRRRR   R  RRRRRRRRRRRRRRRRRR  R   RRRRR\\nRRRR     RRRRRRRRRRRRRRRRRRR  R   RRRR\\n RRR     RRRRRRRRRRRRRRRRRRRR R   RRRR\\n  RRR    RRRRRRRRRRRRRRRRRRRR    RRRR\\n    RR   RRRRRRRRRRRRRRRRRRR    RRR\\n     RR   RRRRRRRRRRRRRRRRR    RRR\\n       RR   RRRRRRRRRRRRRR   RR\\n         R       RRRR      RR\\n";
// Redcore: 17 lines, done at line 9202.
// Logo #175: rhel_old...
static const char rhel_old[] = "${c1}             `.-..........`\\n            `////////::.`-/.\\n            -: ....-////////.\\n            //:-::///////////`\\n     `--::: `-://////////////:\\n     //////-    ``.-:///////// .`\\n     `://////:-.`    :///////::///:`\\n       .-/////////:---/////////////:\\n          .-://////////////////////.\\n${c2}         yMN+`.-${c1}::///////////////-`\\n${c2}      .-`:NMMNMs`  `..-------..`\\n       MN+/mMMMMMhoooyysshsss\\nMMM    MMMMMMMMMMMMMMyyddMMM+\\n MMMM   MMMMMMMMMMMMMNdyNMMh`     hyhMMM\\n  MMMMMMMMMMMMMMMMyoNNNMMM+.   MMMMMMMM\\n   MMNMMMNNMMMMMNM+ mhsMNyyyyMNMMMMsMM\\n";
// rhel_old: 17 lines, done at line 9224.
// Logo #176: rhel...
static const char rhel[] = "${c1}           .MMM..:MMMMMMM\\n          MMMMMMMMMMMMMMMMMM\\n          MMMMMMMMMMMMMMMMMMMM.\\n         MMMMMMMMMMMMMMMMMMMMMM\\n        ,MMMMMMMMMMMMMMMMMMMMMM:\\n        MMMMMMMMMMMMMMMMMMMMMMMM\\n  .MMMM'  MMMMMMMMMMMMMMMMMMMMMM\\n MMMMMM    `MMMMMMMMMMMMMMMMMMMM.\\nMMMMMMMM      MMMMMMMMMMMMMMMMMM .\\nMMMMMMMMM.       `MMMMMMMMMMMMM' MM.\\nMMMMMMMMMMM.                     MMMM\\n`MMMMMMMMMMMMM.                 ,MMMMM.\\n `MMMMMMMMMMMMMMMMM.          ,MMMMMMMM.\\n    MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\\n      MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM:\\n         MMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\\n            `MMMMMMMMMMMMMMMMMMMMMMMM:\\n                ``MMMMMMMMMMMMMMMMM'\\n";
// rhel: 19 lines, done at line 9248.
// Logo #177: Refracted_Devuan...
static const char Refracted_Devuan[] = "${c2}                             A\\n                            VW\\n                           VVW\\\\\\n                         .yWWW\\\\\\n ,;,,u,;yy;;v;uyyyyyyy  ,WWWWW^\\n    *WWWWWWWWWWWWWWWW/  $VWWWWw      ,\\n        ^*%WWWWWWVWWX  $WWWW**    ,yy\\n        ,    \"**WWW/' **'   ,yy/WWW*`\\n       &WWWWwy    `*`  <,ywWW%VWWW*\\n     yWWWWWWWWWW*    .,   \"**WW%W\\n   ,&WWWWWM*\"`  ,y/  &WWWww   ^*\\n  XWWX*^   ,yWWWW09 .WWWWWWWWwy,\\n *`        &WWWWWM  WWWWWWWWWWWWWww,\\n           (WWWWW` /#####WWW***********\\n           ^WWWW\\n            VWW\\n            Wh.\\n            V/\\n";
// Refracted_Devuan: 19 lines, done at line 9272.
// Logo #178: Regata...
static const char Regata[] = "${c1}            ddhso+++++osydd\\n        dho/.`hh${c2}.:/+/:.${c1}hhh`:+yd\\n      do-hhhhhh${c2}/sssssss+`${c1}hhhhh./yd\\n    h/`hhhhhhh${c2}-sssssssss:${c1}hhhhhhhh-yd\\n  do`hhhhhhhhh${c2}`ossssssso.${c1}hhhhhhhhhh/d\\n d/hhhhhhhhhhhh${c2}`/ossso/.${c1}hhhhhhhhhhhh.h\\n /hhhhhhhhhhhh${c3}`-/osyso/-`${c1}hhhhhhhhhhhh.h\\nshh${c4}-/ooo+-${c1}hhh${c3}:syyso+osyys/`${c1}hhh${c5}`+oo`${c1}hhh/\\nh${c4}`ohhhhhhho`${c3}+yyo.${c1}hhhhh${c3}.+yyo`${c5}.sssssss.${c1}h`h\\ns${c4}:hhhhhhhhho${c3}yys`${c1}hhhhhhh${c3}.oyy/${c5}ossssssso-${c1}hs\\ns${c4}.yhhhhhhhy/${c3}yys`${c1}hhhhhhh${c3}.oyy/${c5}ossssssso-${c1}hs\\nhh${c4}./syyys+.${c1} ${c3}+yy+.${c1}hhhhh${c3}.+yyo`${c5}.ossssso/${c1}h`h\\nshhh${c4}``.`${c1}hhh${c3}`/syyso++oyys/`${c1}hhh${c5}`+++-`${c1}hh:h\\nd/hhhhhhhhhhhh${c3}`-/osyso+-`${c1}hhhhhhhhhhhh.h\\n d/hhhhhhhhhhhh${c6}`/ossso/.${c1}hhhhhhhhhhhh.h\\n  do`hhhhhhhhh${c6}`ossssssso.${c1}hhhhhhhhhh:h\\n    h/`hhhhhhh${c6}-sssssssss:${c1}hhhhhhhh-yd\\n      h+.hhhhhh${c6}+sssssss+${c1}hhhhhh`/yd\\n        dho:.hhh${c6}.:+++/.${c1}hhh`-+yd\\n            ddhso+++++osyhd\\n";
// Regata: 21 lines, done at line 9298.
// Logo #179: Regolith...
static const char Regolith[] = "${c1}\\n                 ``....```\\n            `.:/++++++/::-.`\\n          -/+++++++:.`\\n        -++++++++:`\\n      `/++++++++-\\n     `/++++++++.                    -/+/\\n     /++++++++/             ``   .:+++:.\\n    -+++++++++/          ./++++:+++/-`\\n    :+++++++++/         `+++++++/-`\\n    :++++++++++`      .-/+++++++`\\n   `:++++++++++/``.-/++++:-:::-`      `\\n `:+++++++++++++++++/:.`            ./`\\n:++/-:+++++++++/:-..              -/+.\\n+++++++++/::-...:/+++/-..````..-/+++.\\n`......``.::/+++++++++++++++++++++/.\\n         -/+++++++++++++++++++++/.\\n           .:/+++++++++++++++/-`\\n              `.-:://////:-.\\n";
// Regolith: 20 lines, done at line 9323.
// Logo #180: Rosa...
static const char Rosa[] = "${c1}           ROSAROSAROSAROSAR\\n        ROSA               AROS\\n      ROS   SAROSAROSAROSAR   AROS\\n    RO   ROSAROSAROSAROSAROSAR   RO\\n  ARO  AROSAROSAROSARO      AROS  ROS\\n ARO  ROSAROS         OSAR   ROSA  ROS\\n RO  AROSA   ROSAROSAROSA    ROSAR  RO\\nRO  ROSAR  ROSAROSAROSAR  R  ROSARO  RO\\nRO  ROSA  AROSAROSAROSA  AR  ROSARO  AR\\nRO AROS  ROSAROSAROSA   ROS  AROSARO AR\\nRO AROS  ROSAROSARO   ROSARO  ROSARO AR\\nRO  ROS  AROSAROS   ROSAROSA AROSAR  AR\\nRO  ROSA  ROS     ROSAROSAR  ROSARO  RO\\n RO  ROS     AROSAROSAROSA  ROSARO  AR\\n ARO  ROSA   ROSAROSAROS   AROSAR  ARO\\n  ARO  OROSA      R      ROSAROS  ROS\\n    RO   AROSAROS   AROSAROSAR   RO\\n     AROS   AROSAROSAROSARO   AROS\\n        ROSA               SARO\\n           ROSAROSAROSAROSAR\\n";
// Rosa: 21 lines, done at line 9349.
// Logo #181: sabotage...
static const char sabotage[] = "${c2} .|'''.|      |     '||''|.    ..|''||\\n ||..  '     |||     ||   ||  .|'    ||\\n  ''|||.    |  ||    ||'''|.  ||      ||\\n.     '||  .''''|.   ||    || '|.     ||\\n|'....|'  .|.  .||. .||...|'   ''|...|'\\n\\n|''||''|     |      ..|'''.|  '||''''|\\n   ||       |||    .|'     '   ||  .\\n   ||      |  ||   ||    ....  ||''|\\n   ||     .''''|.  '|.    ||   ||\\n  .||.   .|.  .||.  ''|...'|  .||.....|\\n";
// sabotage: 12 lines, done at line 9366.
// Logo #182: Sabayon...
static const char Sabayon[] = "${c1}            ...........\\n         ..             ..\\n      ..                   ..\\n    ..           ${c2}o           ${c1}..\\n  ..            ${c2}:W'            ${c1}..\\n ..             ${c2}.d.             ${c1}..\\n:.             ${c2}.KNO              ${c1}.:\\n:.             ${c2}cNNN.             ${c1}.:\\n:              ${c2}dXXX,              ${c1}:\\n:   ${c2}.          dXXX,       .cd,   ${c1}:\\n:   ${c2}'kc ..     dKKK.    ,ll;:'    ${c1}:\\n:     ${c2}.xkkxc;..dkkkc',cxkkl       ${c1}:\\n:.     ${c2}.,cdddddddddddddo:.       ${c1}.:\\n ..         ${c2}:lllllll:           ${c1}..\\n   ..         ${c2}',,,,,          ${c1}..\\n     ..                     ..\\n        ..               ..\\n          ...............\\n";
// Sabayon: 19 lines, done at line 9390.
// Logo #183: Sailfish...
static const char Sailfish[] = "${c1}                 _a@b\\n              _#b (b\\n            _@@   @_         _,\\n          _#^@ _#*^^*gg,aa@^^\\n          #- @@^  _a@^^\\n          @_  *g#b\\n          ^@_   ^@_\\n            ^@_   @\\n             @(b (b\\n            #b(b#^\\n          _@_#@^\\n       _a@a*^\\n   ,a@*^\\n";
// Sailfish: 14 lines, done at line 9409.
// Logo #184: SalentOS...
static const char SalentOS[] = "${c1}                 ``..``\\n        .-:+oshdNMMMMMMNdhyo+:-.`\\n  -oydmMMMMMMMMMMMMMMMMMMMMMMMMMMNdhs/\\n${c4} +hdddm${c1}NMMMMMMMMMMMMMMMMMMMMMMMMN${c4}mdddh+`\\n${c2}`MMMMMN${c4}mdddddm${c1}MMMMMMMMMMMM${c4}mdddddm${c3}NMMMMM-\\n${c2} mMMMMMMMMMMMN${c4}ddddhyyhhddd${c3}NMMMMMMMMMMMM`\\n${c2} dMMMMMMMMMMMMMMMMM${c4}oo${c3}MMMMMMMMMMMMMMMMMN`\\n${c2} yMMMMMMMMMMMMMMMMM${c4}hh${c3}MMMMMMMMMMMMMMMMMd\\n${c2} +MMMMMMMMMMMMMMMMM${c4}hh${c3}MMMMMMMMMMMMMMMMMy\\n${c2} :MMMMMMMMMMMMMMMMM${c4}hh${c3}MMMMMMMMMMMMMMMMMo\\n${c2} .MMMMMMMMMMMMMMMMM${c4}hh${c3}MMMMMMMMMMMMMMMMM/\\n${c2} `NMMMMMMMMMMMMMMMM${c4}hh${c3}MMMMMMMMMMMMMMMMM-\\n${c2}  mMMMMMMMMMMMMMMMM${c4}hh${c3}MMMMMMMMMMMMMMMMN`\\n${c2}  hMMMMMMMMMMMMMMMM${c4}hh${c3}MMMMMMMMMMMMMMMMm\\n${c2}  /MMMMMMMMMMMMMMMM${c4}hh${c3}MMMMMMMMMMMMMMMMy\\n${c2}   .+hMMMMMMMMMMMMM${c4}hh${c3}MMMMMMMMMMMMMms:\\n${c2}      `:smMMMMMMMMM${c4}hh${c3}MMMMMMMMMNh+.\\n${c2}          .+hMMMMMM${c4}hh${c3}MMMMMMdo:\\n${c2}             `:smMM${c4}yy${c3}MMNy/`\\n                 ${c2}.- ${c4}`${c3}:.\\n";
// SalentOS: 21 lines, done at line 9435.
// Logo #185: Scientific...
static const char Scientific[] = "${c1}                 =/;;/-\\n                +:    //\\n               /;      /;\\n              -X        H.\\n.//;;;:;;-,   X=        :+   .-;:=;:;#;.\\nM-       ,=;;;#:,      ,:#;;:=,       ,@\\n:#           :#.=/++++/=.$=           #=\\n ,#;         #/:+/;,,/++:+/         ;+.\\n   ,+/.    ,;@+,        ,#H;,    ,/+,\\n      ;+;;/= @.  ${c3}.H${c2}#${c3}#X   ${c1}-X :///+;\\n      ;+=;;;.@,  ${c2}.X${c3}M${c2}@$.  ${c1}=X.//;=#/.\\n   ,;:      :@#=        =$H:     .+#-\\n ,#=         #;-///==///-//         =#,\\n;+           :#-;;;:;;;;-X-           +:\\n@-      .-;;;;M-        =M/;;;-.      -X\\n :;;::;;-.    #-        :+    ,-;;-;:==\\n              ,X        H.\\n               ;/      #=\\n                //    +;\\n                 '////'\\n";
// Scientific: 21 lines, done at line 9461.
// Logo #186: Septor...
static const char Septor[] = "${c1}ssssssssssssssssssssssssssssssssssssssss\\nssssssssssssssssssssssssssssssssssssssss\\nssssssssssssssssssssssssssssssssssssssss\\nssssssssssssssssssssssssssssssssssssssss\\nssssssssss${c2};okOOOOOOOOOOOOOOko;${c1}ssssssssss\\nsssssssss${c2}oNWWWWWWWWWWWWWWWWWWNo${c1}sssssssss\\nssssssss${c2}:WWWWWWWWWWWWWWWWWWWWWW:${c1}ssssssss\\nssssssss${c2}lWWWWWk${c1}ssssssssss${c2}lddddd:${c1}ssssssss\\nssssssss${c2}cWWWWWNKKKKKKKKKKKKOx:${c1}ssssssssss\\n${c3}yy${c1}sssssss${c2}OWWWWWWWWWWWWWWWWWWWWx${c1}sssssss${c3}yy\\nyyyyyyyyyy${c2}:kKNNNNNNNNNNNNWWWWWW:${c3}yyyyyyyy\\nyyyyyyyy${c2}sccccc;${c3}yyyyyyyyyy${c2}kWWWWW:${c3}yyyyyyyy\\nyyyyyyyy${c2}:WWWWWWNNNNNNNNNNWWWWWW;${c3}yyyyyyyy\\nyyyyyyyy${c2}.dWWWWWWWWWWWWWWWWWWWNd${c3}yyyyyyyyy\\nyyyyyyyyyy${c2}sdO0KKKKKKKKKKKK0Od;${c3}yyyyyyyyyy\\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\\n";
// Septor: 21 lines, done at line 9487.
// Logo #187: Serene...
static const char Serene[] = "${c1}              __---''''''---__\\n          .                      .\\n        :                          :\\n      -                       _______----_-\\n     s               __----'''     __----\\n __h_            _-'           _-'     h\\n '-._''--.._    ;           _-'         y\\n  :  ''-._  '-._/        _-'             :\\n  y       ':_       _--''                y\\n  m    .--'' '-._.;'                     m\\n  m   :        :                         m\\n  y    '.._     '-__                     y\\n  :        '--._    '''----___           :\\n   y            '--._         ''-- _    y\\n    h                '--._          :  h\\n     s                  __';         vs\\n      -         __..--''             -\\n        :_..--''                   :\\n          .                     _ .\\n            `''---______---''-``\\n";
// Serene: 21 lines, done at line 9513.
// Logo #188: SharkLinux...
static const char SharkLinux[] = "${c1}                              `:shd/\\n                          `:yNMMMMs\\n                       `-smMMMMMMN.\\n                     .+dNMMMMMMMMs\\n                   .smNNMMMMMMMMm`\\n                 .sNNNNNNNMMMMMM/\\n               `omNNNNNNNMMMMMMm\\n              /dNNNNNNNNMMMMMMM+\\n            .yNNNNNNNNNMMMMMMMN`\\n           +mNNNNNNNNNMMMMMMMMh\\n         .hNNNNNNNNNNMMMMMMMMMs\\n        +mMNNNNNNNNMMMMMMMMMMMs\\n      .hNMMNNNNMMMMMMMMMMMMMMMd\\n    .oNNNNNNNNNNMMMMMMMMMMMMMMMo\\n `:+syyssoo++++ooooossssssssssso:\\n";
// SharkLinux: 16 lines, done at line 9534.
// Logo #189: Siduction...
static const char Siduction[] = "${c1}                _aass,\\n               jQh: =$w\\n               QWmwawQW\\n               )$QQQQ@(   ..\\n         _a_a.   ~??^  syDY?Sa,\\n       _mW>-<$c       jWmi  imm.\\n       ]QQwayQE       4QQmgwmQQ`\\n        ?WWQWP'       -9QQQQQ@'._aas,\\n _a%is.        .adYYs,. -\"?!` aQB*~^3$c\\n_Qh;.nm       .QWc. {QL      ]QQp;..vmQ/\\n\"QQmmQ@       -QQQggmQP      ]QQWmggmQQ(\\n -???\"         \"$WQQQY`  __,  ?QQQQQQW!\\n        _yZ!?q,   -   .yWY!!Sw, \"???^\\n       .QQa_=qQ       mQm>..vmm\\n        $QQWQQP       $QQQgmQQ@\\n         \"???\"   _aa, -9WWQQWY`\\n               _mB>~)$a  -~~\\n               mQms_vmQ.\\n               ]WQQQQQP\\n                -?T??\"\\n";
// Siduction: 21 lines, done at line 9560.
// Logo #190: slackware_small...
static const char slackware_small[] = "${c1}   ________\\n  /  ______|\\n  | |______\\n  \\\\______  \\\\\\n   ______| |\\n| |________/\\n|____________\\n";
// slackware_small: 8 lines, done at line 9573.
// Logo #191: Slackware...
static const char Slackware[] = "${c1}                  :::::::\\n            :::::::::::::::::::\\n         :::::::::::::::::::::::::\\n       ::::::::${c2}cllcccccllllllll${c1}::::::\\n    :::::::::${c2}lc               dc${c1}:::::::\\n   ::::::::${c2}cl   clllccllll    oc${c1}:::::::::\\n  :::::::::${c2}o   lc${c1}::::::::${c2}co   oc${c1}::::::::::\\n ::::::::::${c2}o    cccclc${c1}:::::${c2}clcc${c1}::::::::::::\\n :::::::::::${c2}lc        cclccclc${c1}:::::::::::::\\n::::::::::::::${c2}lcclcc          lc${c1}::::::::::::\\n::::::::::${c2}cclcc${c1}:::::${c2}lccclc     oc${c1}:::::::::::\\n::::::::::${c2}o    l${c1}::::::::::${c2}l    lc${c1}:::::::::::\\n :::::${c2}cll${c1}:${c2}o     clcllcccll     o${c1}:::::::::::\\n :::::${c2}occ${c1}:${c2}o                  clc${c1}:::::::::::\\n  ::::${c2}ocl${c1}:${c2}ccslclccclclccclclc${c1}:::::::::::::\\n   :::${c2}oclcccccccccccccllllllllllllll${c1}:::::\\n    ::${c2}lcc1lcccccccccccccccccccccccco${c1}::::\\n      ::::::::::::::::::::::::::::::::\\n        ::::::::::::::::::::::::::::\\n           ::::::::::::::::::::::\\n                ::::::::::::\\n";
// Slackware: 22 lines, done at line 9600.
// Logo #192: SliTaz...
static const char SliTaz[] = "${c1}        @    @(               @\\n      @@   @@                  @    @/\\n     @@   @@                   @@   @@\\n    @@  %@@                     @@   @@\\n   @@  %@@@       @@@@@.       @@@@  @@\\n  @@@    @@@@    @@@@@@@    &@@@    @@@\\n   @@@@@@@ %@@@@@@@@@@@@ &@@@% @@@@@@@/\\n       ,@@@@@@@@@@@@@@@@@@@@@@@@@\\n  .@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@/\\n@@@@@@.  @@@@@@@@@@@@@@@@@@@@@  /@@@@@@\\n@@    @@@@@  @@@@@@@@@@@@,  @@@@@   @@@\\n@@ @@@@.    @@@@@@@@@@@@@%    #@@@@ @@.\\n@@ ,@@      @@@@@@@@@@@@@      @@@  @@\\n@   @@.     @@@@@@@@@@@@@     @@@  *@\\n@    @@     @@@@@@@@@@@@      @@   @\\n      @      @@@@@@@@@.     #@\\n       @      ,@@@@@       @\\n";
// SliTaz: 18 lines, done at line 9623.
// Logo #193: SmartOS...
static const char SmartOS[] = "${c1}yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\\nyyyys             oyyyyyyyyyyyyyyyy\\nyyyys  yyyyyyyyy  oyyyyyyyyyyyyyyyy\\nyyyys  yyyyyyyyy  oyyyyyyyyyyyyyyyy\\nyyyys  yyyyyyyyy  oyyyyyyyyyyyyyyyy\\nyyyys  yyyyyyyyy  oyyyyyyyyyyyyyyyy\\nyyyys  yyyyyyyyyyyyyyyyyyyyyyyyyyyy\\nyyyyy                         syyyy\\nyyyyyyyyyyyyyyyyyyyyyyyyyyyy  syyyy\\nyyyyyyyyyyyyyyyy  syyyyyyyyy  syyyy\\nyyyyyyyyyyyyyyyy  oyyyyyyyyy  syyyy\\nyyyyyyyyyyyyyyyy  oyyyyyyyyy  syyyy\\nyyyyyyyyyyyyyyyy  syyyyyyyyy  syyyy\\nyyyyyyyyyyyyyyyy              yyyyy\\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\\n";
// SmartOS: 18 lines, done at line 9646.
// Logo #194: Solus...
static const char Solus[] = "${c2}            -```````````\\n          `-+/------------.`\\n       .---:mNo---------------.\\n     .-----yMMMy:---------------.\\n   `------oMMMMMm/----------------`\\n  .------/MMMMMMMN+----------------.\\n .------/NMMMMMMMMm-+/--------------.\\n`------/NMMMMMMMMMN-:mh/-------------`\\n.-----/NMMMMMMMMMMM:-+MMd//oso/:-----.\\n-----/NMMMMMMMMMMMM+--mMMMh::smMmyo:--\\n----+NMMMMMMMMMMMMMo--yMMMMNo-:yMMMMd/.\\n.--oMMMMMMMMMMMMMMMy--yMMMMMMh:-yMMMy-`\\n`-sMMMMMMMMMMMMMMMMh--dMMMMMMMd:/Ny+y.\\n`-/+osyhhdmmNNMMMMMm-/MMMMMMMmh+/ohm+\\n  .------------:://+-/++++++${c1}oshddys:\\n   -hhhhyyyyyyyyyyyhhhhddddhysssso-\\n    `:ossssssyysssssssssssssssso:`\\n      `:+ssssssssssssssssssss+-\\n         `-/+ssssssssssso+/-`\\n              `.-----..`\\n";
// Solus: 21 lines, done at line 9672.
// Logo #195: Source_Mage...
static const char Source_Mage[] = "${c2}       :ymNMNho.\\n.+sdmNMMMMMMMMMMy`\\n.-::/yMMMMMMMMMMMm-\\n      sMMMMMMMMMMMm/\\n     /NMMMMMMMMMMMMMm:\\n    .MMMMMMMMMMMMMMMMM:\\n    `MMMMMMMMMMMMMMMMMN.\\n     NMMMMMMMMMMMMMMMMMd\\n     mMMMMMMMMMMMMMMMMMMo\\n     hhMMMMMMMMMMMMMMMMMM.\\n     .`/MMMMMMMMMMMMMMMMMs\\n        :mMMMMMMMMMMMMMMMN`\\n         `sMMMMMMMMMMMMMMM+\\n           /NMMMMMMMMMMMMMN`\\n             oMMMMMMMMMMMMM+\\n          ./sd.-hMMMMMMMMmmN`\\n      ./+oyyyh- `MMMMMMMMMmNh\\n                 sMMMMMMMMMmmo\\n                 `NMMMMMMMMMd:\\n                  -dMMMMMMMMMo\\n                    -shmNMMms.\\n";
// Source_Mage: 22 lines, done at line 9699.
// Logo #196: Sparky...
static const char Sparky[] = "${c1}\\n           .            `-:-`\\n          .o`       .-///-`\\n         `oo`    .:/++:.\\n         os+`  -/+++:` ``.........```\\n        /ys+`./+++/-.-::::::----......``\\n       `syyo`++o+--::::-::/+++/-``\\n       -yyy+.+o+`:/:-:sdmmmmmmmmdy+-`\\n::-`   :yyy/-oo.-+/`ymho++++++oyhdmdy/`\\n`/yy+-`.syyo`+o..o--h..osyhhddhs+//osyy/`\\n  -ydhs+-oyy/.+o.-: ` `  :/::+ydhy+```-os-\\n   .sdddy::syo--/:.     `.:dy+-ohhho    ./:\\n     :yddds/:+oo+//:-`- /+ +hy+.shhy:     ``\\n      `:ydmmdysooooooo-.ss`/yss--oyyo\\n        `./ossyyyyo+:-/oo:.osso- .oys\\n       ``..-------::////.-oooo/   :so\\n    `...----::::::::--.`/oooo:    .o:\\n           ```````     ++o+:`     `:`\\n                     ./+/-`        `\\n                   `-:-.\\n                   ``\\n";
// Sparky: 22 lines, done at line 9726.
// Logo #197: Star...
static const char Star[] = "${c1}                   ./\\n                  `yy-\\n                 `y.`y`\\n    ``           s-  .y            `\\n    +h//:..`    +/    /o    ``..:/so\\n     /o``.-::/:/+      o/://::-.`+o`\\n      :s`     `.        .`     `s/\\n       .y.                    .s-\\n        `y-                  :s`\\n      .-//.                  /+:.\\n   .:/:.                       .:/:.\\n-+o:.                             .:+:.\\n-///++///:::`              .-::::///+so-\\n       ``..o/              d-....```\\n           s.     `/.      d\\n           h    .+o-+o-    h.\\n           h  -o/`   `/o:  s:\\n          -s/o:`       `:o/+/\\n          /s-             -yo\\n";
// Star: 20 lines, done at line 9751.
// Logo #198: SteamOS...
static const char SteamOS[] = "${c1}              .,,,,.\\n        .,'onNMMMMMNNnn',.\\n     .'oNMANKMMMMMMMMMMMNNn'.\\n   .'ANMMMMMMMXKNNWWWPFFWNNMNn.\\n  ;NNMMMMMMMMMMNWW'' ,.., 'WMMM,\\n ;NMMMMV+##+VNWWW' .+;'':+, 'WMW,\\n,VNNWP+${c2}######${c1}+WW,  ${c2}+:    ${c1}:+, +MMM,\\n'${c2}+#############,   +.    ,+' ${c1}+NMMM\\n${c2}  '*#########*'     '*,,*' ${c1}.+NMMMM.\\n${c2}     `'*###*'          ,.,;###${c1}+WNM,\\n${c2}         .,;;,      .;##########${c1}+W\\n${c2},',.         ';  ,+##############'\\n '###+. :,. .,; ,###############'\\n  '####.. `'' .,###############'\\n    '#####+++################'\\n      '*##################*'\\n         ''*##########*''\\n              ''''''\\n";
// SteamOS: 19 lines, done at line 9775.
// Logo #199: solaris_small...
static const char solaris_small[] = "${c1}       .   .;   .\\n   .   :;  ::  ;:   .\\n   .;. ..      .. .;.\\n..  ..             ..  ..\\n .;,                 ,;.\\n";
// solaris_small: 6 lines, done at line 9786.
// Logo #200: Solaris...
static const char Solaris[] = "${c1}                 `-     `\\n          `--    `+-    .:\\n           .+:  `++:  -/+-     .\\n    `.::`  -++/``:::`./+/  `.-/.\\n      `++/-`.`          ` /++:`\\n  ``   ./:`                .: `..`.-\\n``./+/:-                     -+++:-\\n    -/+`                      :.\\n";
// Solaris: 9 lines, done at line 9800.
// Logo #201: openSUSE_Leap...
static const char openSUSE_Leap[] = "${c2}                 `-++:`\\n               ./oooooo/-\\n            `:oooooooooooo:.\\n          -+oooooooooooooooo+-`\\n       ./oooooooooooooooooooooo/-\\n      :oooooooooooooooooooooooooo:\\n    `  `-+oooooooooooooooooooo/-   `\\n `:oo/-   .:ooooooooooooooo+:`  `-+oo/.\\n`/oooooo:.   -/oooooooooo/.   ./oooooo/.\\n  `:+ooooo+-`  `:+oooo+-   `:oooooo+:`\\n     .:oooooo/.   .::`   -+oooooo/.\\n        -/oooooo:.    ./oooooo+-\\n          `:+ooooo+-:+oooooo:`\\n             ./oooooooooo/.\\n                -/oooo+:`\\n                  `:/.\\n";
// openSUSE_Leap: 17 lines, done at line 9822.
// Logo #202: t2...
static const char t2[] = "${c2}\\nTTTTTTTTTT\\n    tt   ${c1}222${c2}\\n    tt  ${c1}2   2${c2}\\n    tt     ${c1}2${c2}\\n    tt    ${c1}2${c2}\\n    tt  ${c1}22222${c2}\\n";
// t2: 8 lines, done at line 9835.
// Logo #203: openSUSE_Tumbleweed...
static const char openSUSE_Tumbleweed[] = "${c2}                                     ......\\n     .,cdxxxoc,.               .:kKMMMNWMMMNk:.\\n    cKMMN0OOOKWMMXo. ;        ;0MWk:.      .:OMMk.\\n  ;WMK;.       .lKMMNM,     :NMK,             .OMW;\\n cMW;            'WMMMN   ,XMK,                 oMM'\\n.MMc               ..;l. xMN:                    KM0\\n'MM.                   'NMO                      oMM\\n.MM,                 .kMMl                       xMN\\n KM0               .kMM0. .dl:,..               .WMd\\n .XM0.           ,OMMK,    OMMMK.              .XMK\\n   oWMO:.    .;xNMMk,       NNNMKl.          .xWMx\\n     :ONMMNXMMMKx;          .  ,xNMWKkxllox0NMWk,\\n         .....                    .:dOOXXKOxl,\\n";
// openSUSE_Tumbleweed: 14 lines, done at line 9854.
// Logo #204: suse_small...
static const char suse_small[] = "${c1}  _______\\n__|   __ \\\\\\n     / .\\\\ \\\\\\n     \\\\__/ |\\n   _______|\\n   \\\\_______\\n__________/\\n";
// suse_small: 8 lines, done at line 9867.
// Logo #205: SUSE...
static const char SUSE[] = "${c2}           .;ldkO0000Okdl;.\\n       .;d00xl:^''''''^:ok00d;.\\n     .d00l'                'o00d.\\n   .d0Kd'${c1}  Okxol:;,.          ${c2}:O0d.\\n  .OK${c1}KKK0kOKKKKKKKKKKOxo:,      ${c2}lKO.\\n ,0K${c1}KKKKKKKKKKKKKKK0P^${c2},,,${c1}^dx:${c2}    ;00,\\n.OK${c1}KKKKKKKKKKKKKKKk'${c2}.oOPPb.${c1}'0k.${c2}   cKO.\\n:KK${c1}KKKKKKKKKKKKKKK: ${c2}kKx..dd ${c1}lKd${c2}   'OK:\\ndKK${c1}KKKKKKKKKOx0KKKd ${c2}^0KKKO' ${c1}kKKc${c2}   dKd\\ndKK${c1}KKKKKKKKKK;.;oOKx,..${c2}^${c1}..;kKKK0.${c2}  dKd\\n:KK${c1}KKKKKKKKKK0o;...^cdxxOK0O/^^'  ${c2}.0K:\\n kKK${c1}KKKKKKKKKKKKK0x;,,......,;od  ${c2}lKk\\n '0K${c1}KKKKKKKKKKKKKKKKKKKK00KKOo^  ${c2}c00'\\n  'kK${c1}KKOxddxkOO00000Okxoc;''   ${c2}.dKk'\\n    l0Ko.                    .c00l'\\n     'l0Kk:.              .;xK0l'\\n        'lkK0xl:;,,,,;:ldO0kl'\\n            '^:ldxkkkkxdl:^'\\n";
// SUSE: 19 lines, done at line 9891.
// Logo #206: SwagArch...
static const char SwagArch[] = "${c2}        .;ldkOKXXNNNNXXK0Oxoc,.\\n   ,lkXMMNK0OkkxkkOKWMMMMMMMMMM;\\n 'K0xo  ..,;:c:.     `'lKMMMMM0\\n     .lONMMMMMM'         `lNMk'\\n${c2}    ;WMMMMMMMMMO.              ${c1}....::...\\n${c2}    OMMMMMMMMMMMMKl.       ${c1}.,;;;;;ccccccc,\\n${c2}    `0MMMMMMMMMMMMMM0:         ${c1}.. .ccccccc.\\n${c2}      'kWMMMMMMMMMMMMMNo.   ${c1}.,:'  .ccccccc.\\n${c2}        `c0MMMMMMMMMMMMMN,${c1},:c;    :cccccc:\\n${c2} ckl.      `lXMMMMMMMMMX${c1}occcc:.. ;ccccccc.\\n${c2}dMMMMXd,     `OMMMMMMWk${c1}ccc;:''` ,ccccccc:\\n${c2}XMMMMMMMWKkxxOWMMMMMNo${c1}ccc;     .cccccccc.\\n${c2} `':ldxO0KXXXXXK0Okdo${c1}cccc.     :cccccccc.\\n                    :ccc:'     `cccccccc:,\\n                                   ''\\n";
// SwagArch: 16 lines, done at line 9912.
// Logo #207: Tails...
static const char Tails[] = "${c1}      ``\\n  ./yhNh\\nsyy/Nshh         `:o/\\nN:dsNshh  █   `ohNMMd\\nN-/+Nshh      `yMMMMd\\nN-yhMshh       yMMMMd\\nN-s:hshh  █    yMMMMd so//.\\nN-oyNsyh       yMMMMd d  Mms.\\nN:hohhhd:.     yMMMMd  syMMM+\\nNsyh+-..+y+-   yMMMMd   :mMM+\\n+hy-      -ss/`yMMMM     `+d+\\n  :sy/.     ./yNMMMMm      ``\\n    .+ys- `:+hNMMMMMMy/`\\n      `hNmmMMMMMMMMMMMMdo.\\n       dMMMMMMMMMMMMMMMMMNh:\\n       +hMMMMMMMMMMMMMMMMMmy.\\n         -oNMMMMMMMMMMmy+.`\\n           `:yNMMMds/.`\\n              .//`\\n";
// Tails: 20 lines, done at line 9937.
// Logo #208: Trisquel...
static const char Trisquel[] = "${c1}                         ▄▄▄▄▄▄\\n                      ▄█████████▄\\n      ▄▄▄▄▄▄         ████▀   ▀████\\n   ▄██████████▄     ████▀   ▄▄ ▀███\\n ▄███▀▀   ▀▀████     ███▄   ▄█   ███\\n▄███   ▄▄▄   ████▄    ▀██████   ▄███\\n███   █▀▀██▄  █████▄     ▀▀   ▄████\\n▀███      ███  ███████▄▄  ▄▄██████\\n${c1} ▀███▄   ▄███  █████████████${c2}████▀\\n${c1}  ▀█████████    ███████${c2}███▀▀▀\\n    ▀▀███▀▀     ██████▀▀\\n               ██████▀   ▄▄▄▄\\n              █████▀   ████████\\n              █████   ███▀  ▀███\\n               ████▄   ██▄▄▄  ███\\n                █████▄   ▀▀  ▄██\\n                  ██████▄▄▄████\\n                     ▀▀█████▀▀\\n";
// Trisquel: 19 lines, done at line 9961.
// Logo #209: Ubuntu-Cinnamon...
static const char Ubuntu_Cinnamon[] = "${c1}            .-:/++oooo++/:-.\\n        `:/oooooooooooooooooo/-`\\n      -/oooooooooooooooooooo+ooo/-\\n    .+oooooooooooooooooo+/-`.ooooo+.\\n   :oooooooooooo+//:://++:. .ooooooo:\\n  /oooooooooo+o:`.----.``./+/oooooooo/\\n /ooooooooo+. +ooooooooo+:``/ooooooooo/\\n.ooooooooo: .+ooooooooooooo- -ooooooooo.\\n/oooooo/o+ .ooooooo:`+oo+ooo- :oooooooo/\\nooo+:. .o: :ooooo:` .+/. ./o+:/ooooooooo\\noooo/-`.o: :ooo/` `/+.     ./.:ooooooooo\\n/oooooo+o+``++. `:+-          /oooooooo/\\n.ooooooooo/``  -+:`          :ooooooooo.\\n /ooooooooo+--+/`          .+ooooooooo/\\n  /ooooooooooo+.`      `.:++:oooooooo/\\n   :oooooooooooooo++++oo+-` .ooooooo:\\n    .+ooooooooooooooooooo+:..ooooo+.\\n      -/oooooooooooooooooooooooo/-\\n        `-/oooooooooooooooooo/:`\\n            .-:/++oooo++/:-.\\n";
// Ubuntu_Cinnamon: 21 lines, done at line 9987.
// Logo #210: Ubuntu-Budgie...
static const char Ubuntu_Budgie[] = "${c2}           ./oydmMMMMMMmdyo/.\\n        :smMMMMMMMMMMMhs+:++yhs:\\n     `omMMMMMMMMMMMN+`        `odo`\\n    /NMMMMMMMMMMMMN-            `sN/\\n  `hMMMMmhhmMMMMMMh               sMh`\\n .mMmo-     /yMMMMm`              `MMm.\\n mN/       yMMMMMMMd-              MMMm\\noN-        oMMMMMMMMMms+//+o+:    :MMMMo\\nm/          +NMMMMMMMMMMMMMMMMm. :NMMMMm\\nM`           .NMMMMMMMMMMMMMMMNodMMMMMMM\\nM-            sMMMMMMMMMMMMMMMMMMMMMMMMM\\nmm`           mMMMMMMMMMNdhhdNMMMMMMMMMm\\noMm/        .dMMMMMMMMh:      :dMMMMMMMo\\n mMMNyo/:/sdMMMMMMMMM+          sMMMMMm\\n .mMMMMMMMMMMMMMMMMMs           `NMMMm.\\n  `hMMMMMMMMMMM.oo+.            `MMMh`\\n    /NMMMMMMMMMo                sMN/\\n     `omMMMMMMMMy.            :dmo`\\n        :smMMMMMMMh+-`   `.:ohs:\\n           ./oydmMMMMMMdhyo/.\\n";
// Ubuntu_Budgie: 21 lines, done at line 10013.
// Logo #211: Ubuntu-GNOME...
static const char Ubuntu_GNOME[] = "${c3}          ./o.\\n        .oooooooo\\n      .oooo```soooo\\n    .oooo`     `soooo\\n   .ooo`   ${c4}.o.${c3}   `\\/ooo.\\n   :ooo   ${c4}:oooo.${c3}   `\\/ooo.\\n    sooo    ${c4}`ooooo${c3}    \\/oooo\\n     \\/ooo    ${c4}`soooo${c3}    `ooooo\\n      `soooo    ${c4}`\\/ooo${c3}    `soooo\\n${c4}./oo    ${c3}`\\/ooo    ${c4}`/oooo.${c3}   `/ooo\\n${c4}`\\/ooo.   ${c3}`/oooo.   ${c4}`/oooo.${c3}   ``\\n${c4}  `\\/ooo.    ${c3}/oooo     ${c4}/ooo`\\n${c4}     `ooooo    ${c3}``    ${c4}.oooo\\n${c4}       `soooo.     .oooo`\\n         `\\/oooooooooo`\\n            ``\\/oo``\\n";
// Ubuntu_GNOME: 17 lines, done at line 10035.
// Logo #212: Ubuntu-MATE...
static const char Ubuntu_MATE[] = "${c1}           `:+shmNNMMNNmhs+:`\\n        .odMMMMMMMMMMMMMMMMMMdo.\\n      /dMMMMMMMMMMMMMMMmMMMMMMMMd/\\n    :mMMMMMMMMMMMMNNNNM/`/yNMMMMMMm:\\n  `yMMMMMMMMMms:..-::oM:    -omMMMMMy`\\n `dMMMMMMMMy-.odNMMMMMM:    -odMMMMMMd`\\n hMMMMMMMm-.hMMy/....+M:`/yNm+mMMMMMMMh\\n/MMMMNmMN-:NMy`-yNMMMMMmNyyMN:`dMMMMMMM/\\nhMMMMm -odMMh`sMMMMMMMMMMs sMN..MMMMMMMh\\nNMMMMm    `/yNMMMMMMMMMMMM: MM+ mMMMMMMN\\nNMMMMm    `/yNMMMMMMMMMMMM: MM+ mMMMMMMN\\nhMMMMm -odMMh sMMMMMMMMMMs oMN..MMMMMMMh\\n/MMMMNNMN-:NMy`-yNMMMMMNNsyMN:`dMMMMMMM/\\n hMMMMMMMm-.hMMy/....+M:.+hNd+mMMMMMMMh\\n `dMMMMMMMMy-.odNMMMMMM:    :smMMMMMMd`\\n   yMMMMMMMMMms/..-::oM:    .+dMMMMMy\\n    :mMMMMMMMMMMMMNNNNM: :smMMMMMMm:\\n      /dMMMMMMMMMMMMMMMdNMMMMMMMd/\\n        .odMMMMMMMMMMMMMMMMMMdo.\\n           `:+shmNNMMNNmhs+:`\\n";
// Ubuntu_MATE: 21 lines, done at line 10061.
// Logo #213: ubuntu_old...
static const char ubuntu_old[] = "${c1}                         ./+o+-\\n${c2}                 yyyyy- ${c1}-yyyyyy+\\n${c2}              ${c2}://+//////${c1}-yyyyyyo\\n${c3}          .++ ${c2}.:/++++++/-${c1}.+sss/`\\n${c3}        .:++o:  ${c2}/++++++++/:--:/-\\n${c3}       o:+o+:++.${c2}`..```.-/oo+++++/\\n${c3}      .:+o:+o/.${c2}          `+sssoo+/\\n${c2} .++/+:${c3}+oo+o:`${c2}             /sssooo.\\n${c2}/+++//+:${c3}`oo+o${c2}               /::--:.\\n${c2}+/+o+++${c3}`o++o${c1}               ++////.\\n${c2} .++.o+${c3}++oo+:`${c1}             /dddhhh.\\n${c3}      .+.o+oo:.${c1}          `oddhhhh+\\n${c3}       +.++o+o`${c1}`-````.:ohdhhhhh+\\n${c3}        `:o+++ ${c1}`ohhhhhhhhyo++os:\\n${c3}          .o:${c1}`.syhhhhhhh/${c3}.oo++o`\\n${c1}              /osyyyyyyo${c3}++ooo+++/\\n${c1}                  ````` ${c3}+oo+++o:\\n${c3}                         `oo++.\\n";
// ubuntu_old: 19 lines, done at line 10085.
// Logo #214: Ubuntu-Studio...
static const char Ubuntu_Studio[] = "${c1}              ..-::::::-.`\\n         `.:+++++++++++${c2}ooo${c1}++:.`\\n       ./+++++++++++++${c2}sMMMNdyo${c1}+/.\\n     .++++++++++++++++${c2}oyhmMMMMms${c1}++.\\n   `/+++++++++${c2}osyhddddhys${c1}+${c2}osdMMMh${c1}++/`\\n  `+++++++++${c2}ydMMMMNNNMMMMNds${c1}+${c2}oyyo${c1}++++`\\n  +++++++++${c2}dMMNhso${c1}++++${c2}oydNMMmo${c1}++++++++`\\n :+${c2}odmy${c1}+++${c2}ooysoohmNMMNmyoohMMNs${c1}+++++++:\\n ++${c2}dMMm${c1}+${c2}oNMd${c1}++${c2}yMMMmhhmMMNs+yMMNo${c1}+++++++\\n`++${c2}NMMy${c1}+${c2}hMMd${c1}+${c2}oMMMs${c1}++++${c2}sMMN${c1}++${c2}NMMs${c1}+++++++.\\n`++${c2}NMMy${c1}+${c2}hMMd${c1}+${c2}oMMMo${c1}++++${c2}sMMN${c1}++${c2}mMMs${c1}+++++++.\\n ++${c2}dMMd${c1}+${c2}oNMm${c1}++${c2}yMMNdhhdMMMs${c1}+y${c2}MMNo${c1}+++++++\\n :+${c2}odmy${c1}++${c2}oo${c1}+${c2}ss${c1}+${c2}ohNMMMMmho${c1}+${c2}yMMMs${c1}+++++++:\\n  +++++++++${c2}hMMmhs+ooo+oshNMMms${c1}++++++++\\n  `++++++++${c2}oymMMMMNmmNMMMMmy+oys${c1}+++++`\\n   `/+++++++++${c2}oyhdmmmmdhso+sdMMMs${c1}++/\\n     ./+++++++++++++++${c2}oyhdNMMMms${c1}++.\\n       ./+++++++++++++${c2}hMMMNdyo${c1}+/.\\n         `.:+++++++++++${c2}sso${c1}++:.\\n              ..-::::::-..\\n";
// Ubuntu_Studio: 21 lines, done at line 10111.
// Logo #215: ubuntu_small...
static const char ubuntu_small[] = "${c1}         _\\n     ---(_)\\n _/  ---  \\\\\\n(_) |   |\\n  \\\\  --- _/\\n     ---(_)\\n";
// ubuntu_small: 7 lines, done at line 10123.
// Logo #216: i3buntu...
static const char i3buntu[] = "${c1}            .-/+oossssoo+/-.\\n        `:+ssssssssssssssssss+:`\\n      -+ssssssssssssssssssyyssss+-\\n    .ossssssssssssssssss${c2}dMMMNy${c1}sssso.\\n   /sssssssssss${c2}hdmmNNmmyNMMMMh${c1}ssssss/\\n  +sssssssss${c2}hm${c1}yd${c2}MMMMMMMNddddy${c1}ssssssss+\\n /ssssssss${c2}hNMMM${c1}yh${c2}hyyyyhmNMMMNh${c1}ssssssss/\\n.ssssssss${c2}dMMMNh${c1}ssssssssss${c2}hNMMMd${c1}ssssssss.\\n+ssss${c2}hhhyNMMNy${c1}ssssssssssss${c2}yNMMMy${c1}sssssss+\\noss${c2}yNMMMNyMMh${c1}ssssssssssssss${c2}hmmmh${c1}ssssssso\\noss${c2}yNMMMNyMMh${c1}sssssssssssssshmmmh${c1}ssssssso\\n+ssss${c2}hhhyNMMNy${c1}ssssssssssss${c2}yNMMMy${c1}sssssss+\\n.ssssssss${c2}dMMMNh${c1}ssssssssss${c2}hNMMMd${c1}ssssssss.\\n /ssssssss${c2}hNMMM${c1}yh${c2}hyyyyhdNMMMNh${c1}ssssssss/\\n  +sssssssss${c2}dm${c1}yd${c2}MMMMMMMMddddy${c1}ssssssss+\\n   /sssssssssss${c2}hdmNNNNmyNMMMMh${c1}ssssss/\\n    .ossssssssssssssssss${c2}dMMMNy${c1}sssso.\\n      -+sssssssssssssssss${c2}yyy${c1}ssss+-\\n        `:+ssssssssssssssssss+:`\\n            .-/+oossssoo+/-.\\n";
// i3buntu: 21 lines, done at line 10149.
// Logo #217: Venom...
static const char Venom[] = "${c1}   :::::::          :::::::\\n   mMMMMMMm        dMMMMMMm\\n   /MMMMMMMo      +MMMMMMM/\\n    yMMMMMMN      mMMMMMMy\\n     NMMMMMMs    oMMMMMMm\\n     +MMMMMMN:   NMMMMMM+\\n      hMMMMMMy  sMMMMMMy\\n      :NMMMMMM::NMMMMMN:\\n       oMMMMMMyyMMMMMM+\\n        dMMMMMMMMMMMMh\\n        /MMMMMMMMMMMN:\\n         sMMMMMMMMMMo\\n          mMMMMMMMMd\\n          +MMMMMMMN:\\n            ::::::\\n";
// Venom: 16 lines, done at line 10170.
// Logo #218: void_small...
static const char void_small[] = "${c1}    _______\\n _ \\\\______ -\\n| \\\\  ___  \\\\ |\\n| | /   \\ | |\\n| | \\___/ | |\\n| \\\\______ \\\\_|\\n -_______\\\\\\n";
// void_small: 8 lines, done at line 10183.
// Logo #219: Void...
static const char Void[] = "${c1}                __.;=====;.__\\n            _.=+==++=++=+=+===;.\\n             -=+++=+===+=+=+++++=_\\n        .     -=:``     `--==+=++==.\\n       _vi,    `            --+=++++:\\n      .uvnvi.       _._       -==+==+.\\n     .vvnvnI`    .;==|==;.     :|=||=|.\\n${c2}+QmQQm${c1}pvvnv; ${c2}_yYsyQQWUUQQQm #QmQ#${c1}:${c2}QQQWUV$QQm.\\n${c2} -QQWQW${c1}pvvo${c2}wZ?.wQQQE${c1}==<${c2}QWWQ/QWQW.QQWW${c1}(: ${c2}jQWQE\\n${c2}  -$QQQQmmU'  jQQQ@${c1}+=<${c2}QWQQ)mQQQ.mQQQC${c1}+;${c2}jWQQ@'\\n${c2}   -$WQ8Y${c1}nI:   ${c2}QWQQwgQQWV${c1}`${c2}mWQQ.jQWQQgyyWW@!\\n${c1}     -1vvnvv.     `~+++`        ++|+++\\n      +vnvnnv,                 `-|===\\n       +vnvnvns.           .      :=-\\n        -Invnvvnsi..___..=sv=.     `\\n          +Invnvnvnnnnnnnnvvnn;.\\n            ~|Invnvnvvnvvvnnv}+`\\n               -~|{*l}*|~\\n";
// Void: 19 lines, done at line 10207.
// Logo #220: Obarun...
static const char Obarun[] = "${c1}                    ,;::::;\\n                ;cooolc;,\\n             ,coool;\\n           ,loool,\\n          loooo;\\n        :ooool\\n       cooooc            ,:ccc;\\n      looooc           :oooooool\\n     cooooo          ;oooooooooo,\\n    :ooooo;         :ooooooooooo\\n    oooooo          oooooooooooc\\n   :oooooo         :ooooooooool\\n   loooooo         ;oooooooool\\n   looooooc        .coooooooc\\n   cooooooo:           ,;co;\\n   ,ooooooool;       ,:loc\\n    cooooooooooooloooooc\\n     ;ooooooooooooool;\\n       ;looooooolc;\\n";
// Obarun: 20 lines, done at line 10232.
// Logo #221: windows8...
static const char windows8[] = "${c1}                                ..,\\n                    ....,,:;+ccllll\\n      ...,,+:;  cllllllllllllllllll\\n,cclllllllllll  lllllllllllllllllll\\nllllllllllllll  lllllllllllllllllll\\nllllllllllllll  lllllllllllllllllll\\nllllllllllllll  lllllllllllllllllll\\nllllllllllllll  lllllllllllllllllll\\nllllllllllllll  lllllllllllllllllll\\n\\nllllllllllllll  lllllllllllllllllll\\nllllllllllllll  lllllllllllllllllll\\nllllllllllllll  lllllllllllllllllll\\nllllllllllllll  lllllllllllllllllll\\nllllllllllllll  lllllllllllllllllll\\n`'ccllllllllll  lllllllllllllllllll\\n       `' \\\\*::  :ccllllllllllllllll\\n                       ````''*::cll\\n                                 ``\\n";
// windows8: 20 lines, done at line 10258.
// Logo #222: Windows...
static const char Windows[] = "${c1}        ,.=:!!t3Z3z.,\\n       :tt:::tt333EE3\\n${c1}       Et:::ztt33EEEL${c2} @Ee.,      ..,\\n${c1}      ;tt:::tt333EE7${c2} ;EEEEEEttttt33#\\n${c1}     :Et:::zt333EEQ.${c2} $EEEEEttttt33QL\\n${c1}     it::::tt333EEF${c2} @EEEEEEttttt33F\\n${c1}    ;3=*^```\"*4EEV${c2} :EEEEEEttttt33@.\\n${c3}    ,.=::::!t=., ${c1}`${c2} @EEEEEEtttz33QF\\n${c3}   ;::::::::zt33)${c2}   \"4EEEtttji3P*\\n${c3}  :t::::::::tt33.${c4}:Z3z..${c2}  ``${c4} ,..g.\\n${c3}  i::::::::zt33F${c4} AEEEtttt::::ztF\\n${c3} ;:::::::::t33V${c4} ;EEEttttt::::t3\\n${c3} E::::::::zt33L${c4} @EEEtttt::::z3F\\n${c3}{3=*^```\"*4E3)${c4} ;EEEtttt:::::tZ`\\n${c3}             `${c4} :EEEEtttt::::z7\\n                 \"VEzjt:;;z>*`\\n";
// Windows: 17 lines, done at line 10280.
// Logo #223: Xubuntu...
static const char Xubuntu[] = "${c1}           `-/osyhddddhyso/-`\\n        .+yddddddddddddddddddy+.\\n      :yddddddddddddddddddddddddy:\\n    -yddddddddddddddddddddhdddddddy-\\n   odddddddddddyshdddddddh`dddd+ydddo\\n `yddddddhshdd-   ydddddd+`ddh.:dddddy`\\n sddddddy   /d.   :dddddd-:dy`-ddddddds\\n:ddddddds    /+   .dddddd`yy`:ddddddddd:\\nsdddddddd`    .    .-:/+ssdyodddddddddds\\nddddddddy                  `:ohddddddddd\\ndddddddd.                      +dddddddd\\nsddddddy                        ydddddds\\n:dddddd+                      .oddddddd:\\n sdddddo                   ./ydddddddds\\n `yddddd.              `:ohddddddddddy`\\n   oddddh/`      `.:+shdddddddddddddo\\n    -ydddddhyssyhdddddddddddddddddy-\\n      :yddddddddddddddddddddddddy:\\n        .+yddddddddddddddddddy+.\\n           `-/osyhddddhyso/-`\\n";
// Xubuntu: 21 lines, done at line 10306.
// Logo #224: IRIX...
static const char IRIX[] = "${c1}           ./ohmNd/  +dNmho/-\\n     `:+ydNMMMMMMMM.-MMMMMMMMMdyo:.\\n   `hMMMMMMNhs/sMMM-:MMM+/shNMMMMMMh`\\n   -NMMMMMmo-` /MMM-/MMM- `-omMMMMMN.\\n `.`-+hNMMMMMNhyMMM-/MMMshmMMMMMmy+...`\\n+mMNds:-:sdNMMMMMMMyyMMMMMMMNdo:.:sdMMm+\\ndMMMMMMmy+.-/ymNMMMMMMMMNmy/-.+hmMMMMMMd\\noMMMMmMMMMNds:.+MMMmmMMN/.-odNMMMMmMMMM+\\n.MMMM-/ymMMMMMmNMMy..hMMNmMMMMMmy/-MMMM.\\n hMMM/ `/dMMMMMMMN////NMMMMMMMd/. /MMMh\\n /MMMdhmMMMmyyMMMMMMMMMMMMhymMMMmhdMMM:\\n `mMMMMNho//sdMMMMM//NMMMMms//ohNMMMMd\\n  `/so/:+ymMMMNMMMM` mMMMMMMMmh+::+o/`\\n     `yNMMNho-yMMMM` NMMMm.+hNMMNh`\\n     -MMMMd:  oMMMM. NMMMh  :hMMMM-\\n      -yNMMMmooMMMM- NMMMyomMMMNy-\\n        .omMMMMMMMM-`NMMMMMMMmo.\\n          `:hMMMMMM. NMMMMMh/`\\n             .odNm+  /dNms.\\n";
// IRIX: 20 lines, done at line 10330.
// Logo #225: Zorin...
static const char Zorin[] = "${c1}        `osssssssssssssssssssso`\\n       .osssssssssssssssssssssso.\\n      .+oooooooooooooooooooooooo+.\\n\\n\\n  `::::::::::::::::::::::.         .:`\\n `+ssssssssssssssssss+:.`     `.:+ssso`\\n.ossssssssssssssso/.       `-+ossssssso.\\nssssssssssssso/-`      `-/osssssssssssss\\n.ossssssso/-`      .-/ossssssssssssssso.\\n `+sss+:.      `.:+ssssssssssssssssss+`\\n  `:.         .::::::::::::::::::::::`\\n\\n\\n      .+oooooooooooooooooooooooo+.\\n       -osssssssssssssssssssssso-\\n        `osssssssssssssssssssso`\\n";
// Zorin: 18 lines, done at line 10352.
// Logo #226: BSD...
// Logo #227: Darwin...
// Logo #228: GNU...
// Logo #229: Linux...
// Logo #230: SambaBOX...
static const char SambaBOX[] = "${c1}\\n                    #\\n               *////#####\\n           /////////#########(\\n      .((((((/////    ,####(#(((((\\n  /#######(((*             (#(((((((((.\\n//((#(#(#,        ((##(        ,((((((//\\n//////        #(##########(       //////\\n//////    ((#(#(#(#(##########(/////////\\n/////(    (((((((#########(##((((((/////\\n/(((#(                             ((((/\\n####(#                             ((###\\n#########(((/////////(((((((((,    (#(#(\\n########(   /////////(((((((*      #####\\n####///,        *////(((         (((((((\\n.///////////                .//(((((((((\\n     ///////////,       *(/////((((*\\n         ,/(((((((((##########/.\\n             .((((((#######\\n                  ((##*\\n";
// SambaBOX: 21 lines, done at line 10469.
// Logo #231: SunOS...
static const char SunOS[] = "${c1}                 `-     `\\n          `--    `+-    .:\\n           .+:  `++:  -/+-     .\\n    `.::`  -++/``:::`./+/  `.-/.\\n      `++/-`.`          ` /++:`\\n  ``   ./:`                .: `..`.-\\n``./+/:-                     -+++:-\\n    -/+`                      :.\\n";
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
  { NULL, NULL }
};

const char* get_neofetch_art(const char* oskey){
  for(const struct neofetch_art* nfa = ncarts ; nfa->oskey ; ++nfa){
    if(strcmp(nfa->oskey, oskey) == 0){
      return nfa->ncart;
    }
  }
  return NULL;
}
