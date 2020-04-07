Source0: file
Source1: signature
Source2: keyring

BuildRequires: gnupg2 cmake xz make gcc-c++ ncurses-devel pandoc python3-devel

%prep
%{gpgverify} --keyring='%{SOURCE2}' --signature='%{SOURCE1}' --data='%{SOURCE0}'
