Source0: https://github.com/dankamongmen/{name}/archive/v{version}.tar.gz
Source1: signature
Source2: keyring

License: ASL 2.0

BuildRequires: gnupg2 cmake make gcc-c++ ncurses-devel pandoc python3-devel

%prep
%{gpgverify} --keyring='%{SOURCE2}' --signature='%{SOURCE1}' --data='%{SOURCE0}'

%build
%cmake -DUSE_FFMPEG=off -DUSE_TEST=off .
%make_build

%install
%make_install
