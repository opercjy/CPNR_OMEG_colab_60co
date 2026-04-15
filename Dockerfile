# 베이스 이미지: 순정 Rocky Linux 9
FROM rockylinux:9

# 1. 필수 업데이트 및 확장 저장소(epel, crb) 활성화
RUN dnf update -y && \
    dnf install -y epel-release && \
    dnf config-manager --set-enabled crb

# 2. C++17 빌드 도구, cmake-gui, Qt6 및 X11 그래픽 의존성 설치
RUN dnf install -y --allowerasing \
    gcc gcc-c++ cmake cmake-gui make git wget tar curl unzip \
    python3-dnf-plugin-versionlock \
    expat-devel xerces-c-devel \
    libX11-devel libXext-devel libXmu-devel libXi-devel \
    libglvnd-opengl libglvnd-glx libglvnd-devel mesa-dri-drivers \
    qt6-qtbase-devel qt6-qt3d-devel && \
    dnf clean all

# 3. Geant4 소스 다운로드 (11.2.1 버전)
WORKDIR /opt
RUN wget https://gitlab.cern.ch/geant4/geant4/-/archive/v11.2.1/geant4-v11.2.1.tar.gz && \
    tar -xzf geant4-v11.2.1.tar.gz && \
    rm geant4-v11.2.1.tar.gz

# 4. Geant4 빌드
WORKDIR /opt/geant4-v11.2.1/build
RUN cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr/local/geant4 \
    -DGEANT4_INSTALL_DATA=ON \
    -DGEANT4_USE_OPENGL_X11=ON \
    -DGEANT4_USE_QT=ON \
    -DGEANT4_USE_QT_QT6=ON \
    -DGEANT4_BUILD_MULTITHREADED=ON \
    -DCMAKE_CXX_STANDARD=17 && \
    make -j$(nproc) && \
    make install

# 5. 빌드 잔해물 제거 (경량화)
WORKDIR /opt
RUN rm -rf geant4-v11.2.1

# 6. 환경 변수 스크립트 등록
RUN echo "source /usr/local/geant4/bin/geant4.sh" >> /root/.bashrc

# 7. ROOT 및 분석 패키지 의존성 설치
RUN dnf install -y \
    gedit \
    python3-pip python3-devel python3-numpy \
    pcre-devel pcre2-devel \
    libXpm-devel libXft-devel libXext-devel \
    gsl-devel openssl-devel patch binutils \
    gcc-gfortran mesa-libGL-devel mesa-libGLU-devel glew-devel ftgl-devel \
    mariadb-devel fftw-devel cfitsio-devel graphviz-devel libuuid-devel \
    avahi-compat-libdns_sd-devel openldap-devel libxml2-devel readline-devel \
    xrootd-client-devel xrootd-libs-devel \
    R-devel R-Rcpp-devel R-RInside-devel && \
    dnf clean all

# 8. Python 데이터 분석 패키지 설치
RUN pip3 install --no-cache-dir \
    uproot awkward mplhep scikit-hep \
    vector particle boost-histogram \
    numpy pandas matplotlib scipy jupyterlab

# ==============================================================================
# 9. CERN ROOT 소스 다운로드 (v6.30.04) 및 빌드 (원복)
# ==============================================================================
WORKDIR /opt
RUN wget https://root.cern/download/root_v6.30.04.source.tar.gz && \
    tar -xzf root_v6.30.04.source.tar.gz && \
    rm root_v6.30.04.source.tar.gz

WORKDIR /opt/root-6.30.04/build
RUN cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr/local/root \
    -DCMAKE_CXX_STANDARD=17 \
    -Dgdml=ON \
    -Dmathmore=ON \
    -Dminuit2=ON \
    -Dpyroot=OFF && \
    make -j$(nproc) && \
    make install

# 10. ROOT 빌드 잔해물 제거
WORKDIR /opt
RUN rm -rf root-6.30.04

# 11. ROOT 및 터미널 환경 변수 등록
RUN echo "source /usr/local/root/bin/thisroot.sh" >> /root/.bashrc && \
    echo "alias ls='ls --color=auto'" >> /root/.bashrc && \
    echo "alias ll='ls -l --color=auto'" >> /root/.bashrc && \
    echo "alias grep='grep --color=auto'" >> /root/.bashrc && \
    echo "export PS1='\[\e[1;32m\][\u@\h \W]\$\[\e[0m\] '" >> /root/.bashrc

# ==============================================================================
# 12. [다양한 편집기(Neovim, Emacs, Nano) 및 편의성 도구 설치]
# ==============================================================================
RUN dnf module enable -y nodejs:20 && \
    # ★ emacs-nox(터미널용 이맥스)와 nano가 여기에 포함되어 있습니다!
    dnf install -y --allowerasing nodejs java-17-openjdk-devel ripgrep fd-find \
    vim-enhanced tmux fzf bat htop tree jq libstdc++-static clang \
    emacs-nox nano && \
    dnf clean all

# Rust & Cargo 설치
RUN curl -4 --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# Tree-sitter 파서 컴파일 시 사용할 C/C++ 컴파일러 고정
ENV CC=gcc
ENV CXX=g++

# Cargo를 이용해 tree-sitter-cli 글로벌 설치
RUN cargo install tree-sitter-cli

# Neovim 0.12 (Nightly) 다운로드 및 LazyVim 클론
RUN curl -LO https://github.com/neovim/neovim/releases/download/nightly/nvim-linux-x86_64.tar.gz && \
    tar -C /opt -xzf nvim-linux-x86_64.tar.gz && \
    ln -sf /opt/nvim-linux-x86_64/bin/nvim /usr/local/bin/nvim && \
    rm -f nvim-linux-x86_64.tar.gz
RUN git clone https://github.com/LazyVim/starter /root/.config/nvim && \
    rm -rf /root/.config/nvim/.git

# ★ 핵심: 박사님 요청사항인 Tree-sitter 언어 파서 리스트를 LazyVim 설정 파일로 미리 주입합니다.
# (c, cpp, python, java, json, fortran 및 추가로 bash, cmake 포함)
RUN mkdir -p /root/.config/nvim/lua/plugins && \
    echo 'return { { "nvim-treesitter/nvim-treesitter", opts = function(_, opts) if type(opts.ensure_installed) == "table" then vim.list_extend(opts.ensure_installed, { "c", "cpp", "python", "java", "json", "fortran", "cmake", "bash" }) end end } }' > /root/.config/nvim/lua/plugins/treesitter.lua

# Headless 모드로 Neovim을 백그라운드 실행하여 플러그인과 파서를 빌드 단계에서 100% 자동 설치 완료!
RUN nvim --headless "+Lazy! sync" +qa

# ==============================================================================
# 13. [Nano 및 Emacs 애드온(Add-on) 최적화 설정]
# ==============================================================================
# Nano: 구문 강조(Syntax Highlighting) 패키지 설치 및 코딩용 설정 적용
RUN git clone https://github.com/scopatz/nanorc.git /root/.nano && \
    cat /root/.nano/nanorc > /root/.nanorc && \
    echo "set linenumbers" >> /root/.nanorc && \
    echo "set tabsize 4" >> /root/.nanorc && \
    echo "set autoindent" >> /root/.nanorc

# Emacs: MELPA 패키지 매니저 활성화 및 불필요한 시작 화면 제거
RUN echo "(require 'package)" >> /root/.emacs && \
    echo "(add-to-list 'package-archives '(\"melpa\" . \"https://melpa.org/packages/\") t)" >> /root/.emacs && \
    echo "(package-initialize)" >> /root/.emacs && \
    echo "(custom-set-variables '(inhibit-startup-screen t))" >> /root/.emacs

# ==============================================================================
# 13.5 GUI 경고 해결, 한글 폰트(언어팩), Emacs GUI, VS Code 풀세트 및 Alias 등록
# ==============================================================================
# 1. GUI 경고 해결 패키지, 한국어 언어팩 및 폰트, Emacs 설치 및 VS Code 공식 저장소 등록 후 설치
RUN dnf install -y PackageKit-gtk3-module langpacks-ko google-noto-sans-cjk-ttc-fonts emacs && \
    rpm --import https://packages.microsoft.com/keys/microsoft.asc && \
    sh -c 'echo -e "[code]\nname=Visual Studio Code\nbaseurl=https://packages.microsoft.com/yumrepos/vscode\nenabled=1\ngpgcheck=1\ngpgkey=https://packages.microsoft.com/keys/microsoft.asc" > /etc/yum.repos.d/vscode.repo' && \
    dnf install -y code && \
    dnf clean all

# 2. VS Code 필수 확장팩(C/C++, Python, Copilot, 한국어 등) 자동 설치 및 bashrc Alias 추가
RUN code --install-extension ms-vscode.cpptools-extension-pack \
         --install-extension ms-python.python \
         --install-extension GitHub.copilot \
         --install-extension GitHub.copilot-chat \
         --install-extension github.github-vscode-theme \
         --install-extension MS-CEINTL.vscode-language-pack-ko \
         --user-data-dir=/root/.config/Code \
         --no-sandbox && \
    echo "alias code='code --no-sandbox --user-data-dir=/root/.config/Code'" >> /root/.bashrc
# ==============================================================================
# 13.6 ★ 우주선 발생기 라이브러리 (CRY v1.7) 영구 설치 및 환경변수 등록
# ==============================================================================
WORKDIR /opt
RUN wget https://nuclear.llnl.gov/simulation/cry_v1.7.tar.gz && \
    tar -xzvf cry_v1.7.tar.gz && \
    cd cry_v1.7 && \
    make && \
    rm -f ../cry_v1.7.tar.gz

# Docker ENV 명령어로 환경변수를 등록하면, 모든 컨테이너 셸에서 자동으로 인식합니다.
ENV CRYHOME=/opt/cry_v1.7
ENV CRYDATAPATH=/opt/cry_v1.7/data
# ==============================================================================
# 14. 최종 작업 디렉토리 및 컨테이너 진입 명령
# ==============================================================================
WORKDIR /work
CMD ["/bin/bash"]
