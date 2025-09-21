cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat

vcpkg install box2d:x64-windows
vcpkg install box2d:x86-windows

vcpkg install glfw3:x64-windows
vcpkg install glfw3:x86-windows

vcpkg install tinyxml2:x64-windows
vcpkg install tinyxml2:x86-windows

vcpkg install glm:x64-windows
vcpkg install glm:x86-windows

vcpkg install imgui:x64-windows
vcpkg install imgui:x86-windows

vcpkg install curl:x64-windows
vcpkg install curl:x86-windows

vcpkg install assimp:x64-windows
vcpkg install assimp:x86-windows

vcpkg install freetype:x64-windows
vcpkg install freetype:x86-windows

vcpkg install glad:x64-windows
vcpkg install glad:x86-windows

vcpkg install freealut:x64-windows
vcpkg install freealut:x86-windows

vcpkg install cryptopp:x64-windows
vcpkg install cryptopp:x86-windows

vcpkg install rtaudio:x64-windows
vcpkg install rtaudio:x86-windows

vcpkg install mysql-connector-cpp:x64-windows
vcpkg install mysql-connector-cpp:x86-windows

vcpkg integrate install
