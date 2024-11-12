# escape=`

ARG BASE_IMAGE=mcr.microsoft.com/dotnet/framework/runtime:4.8
FROM ${BASE_IMAGE}

SHELL ["powershell"]
ENV VS_VERSION=2019
RUN [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; `
    Switch ($env:VS_VERSION) { `
        "2019" {$url_version = "16"} `
        "2022" {$url_version = "17"} `
        default {echo "Unsupported VS version $env:VS_VERSION"; EXIT 1} `
    }; `
    wget -Uri https://aka.ms/vs/${url_version}/release/vs_buildtools.exe -OutFile vs_buildtools.exe
SHELL ["cmd", "/S", "/C"]
RUN (start /w vs_buildtools.exe --quiet --wait --norestart --nocache `
        --installPath "%ProgramFiles(x86)%\Microsoft Visual Studio\%VS_VERSION%\BuildTools" `
        --add Microsoft.Component.MSBuild `
        --add Microsoft.VisualStudio.Component.VC.CoreBuildTools `
        --add Microsoft.VisualStudio.Component.VC.Redist.14.Latest `
        --add Microsoft.VisualStudio.Component.Windows10SDK `
        --add Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Core `
        --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        --add Microsoft.VisualStudio.Component.Windows10SDK.19041 `
        --add Microsoft.VisualStudio.Component.VC.Runtimes.x86.x64.Spectre `
        || IF "%ERRORLEVEL%"=="3010" EXIT 0) `
    && del /q vs_buildtools.exe `
    && if not exist "C:\Program Files (x86)\Microsoft Visual Studio\%VS_VERSION%\BuildTools\VC\Auxiliary\Build\vcvars64.bat" exit 1
ENTRYPOINT ["C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat", "&&"]

SHELL ["powershell"]
ENV chocolateyUseWindowsCompression false
RUN Set-ExecutionPolicy Bypass -Scope Process -Force; `
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; `
    iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
SHELL ["cmd", "/S", "/C"]
RUN call "%ProgramFiles(x86)%\Microsoft Visual Studio\%VS_VERSION%\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && `
    powershell -command "[Environment]::SetEnvironmentVariable('Path', $env:Path, [System.EnvironmentVariableTarget]::Machine)"
RUN choco feature disable --name showDownloadProgress && `
    choco install -y --fail-on-error-output git -params '"/GitAndUnixToolsOnPath"' && `
    choco install -y --fail-on-error-output 7zip && `
    choco install -y --fail-on-error-output ccache && `
    choco install -y --fail-on-error-output cmake.install --installargs 'ADD_CMAKE_TO_PATH=System' && `
    choco install -y --fail-on-error-output ninja

SHELL ["bash.exe", "-x", "-e", "-c"]

RUN "`
curl -OSL --ssl-no-revoke https://zlib.net/zlib131.zip && `
7z.exe x zlib131.zip && `
rm zlib131.zip && `
cd zlib-1.3.1 && `
mkdir build && `
cd build && `
cmake.exe .. -A x64 -T host=x64 -D BUILD_SHARED_LIBS=YES && `
cmake.exe --build . --target INSTALL --config Release && `
cd ../.. && `
rm -rf zlib-1.3.1"

RUN "`
curl -OSL --ssl-no-revoke https://download.sourceforge.net/libpng/lpng1639.zip && `
7z.exe x lpng1639.zip && `
rm lpng1639.zip && `
mkdir -p lpng1639/build && `
cd lpng1639/build && `
cmake .. -T host=x64 -A x64 && `
cmake --build . --target INSTALL --config Release && `
cd ../.. && `
rm -rf lpng1639"

RUN "`
curl -SL --ssl-no-revoke https://github.com/KhronosGroup/OpenCL-Headers/archive/refs/tags/v2022.09.30.zip -o OpenCL-Headers.zip && `
7z.exe x OpenCL-Headers.zip && `
rm OpenCL-Headers.zip && `
mkdir OpenCL-Headers-2022.09.30/build && `
cd OpenCL-Headers-2022.09.30/build && `
cmake .. && `
cmake --build . --target install && `
cd ../.. && `
rm -rf OpenCL-Headers-2022.09.30"
RUN "`
curl -SL --ssl-no-revoke https://github.com/KhronosGroup/OpenCL-ICD-Loader/archive/refs/tags/v2022.09.30.zip -o OpenCL-ICD-Loader.zip && `
7z.exe x OpenCL-ICD-Loader.zip && `
rm OpenCL-ICD-Loader.zip && `
mkdir OpenCL-ICD-Loader-2022.09.30/build && `
cd OpenCL-ICD-Loader-2022.09.30/build && `
cmake .. -T host=x64 -A x64 && `
cmake --build . --target install && `
cd ../.. && `
rm -rf OpenCL-ICD-Loader-2022.09.30"

RUN "`
curl -OSL --ssl-no-revoke https://boostorg.jfrog.io/artifactory/main/release/1.73.0/source/boost_1_73_0.7z && `
7z.exe x boost_1_73_0.7z && `
cd boost_1_73_0 && `
MSYS_NO_PATHCONV=1 cmd /c bootstrap.bat && `
./b2.exe install -j 16 address-model=64 `
  --with-chrono `
  --with-log `
  --with-program_options `
  --with-serialization `
  --with-system `
  --with-timer && `
cd .. && `
rm -rf boost_1_73_0"
