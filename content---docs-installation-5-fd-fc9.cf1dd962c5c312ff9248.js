(window.webpackJsonp=window.webpackJsonp||[]).push([[9],{56:function(e,t,n){"use strict";n.r(t),n.d(t,"frontMatter",(function(){return r})),n.d(t,"rightToc",(function(){return o})),n.d(t,"default",(function(){return s}));n(0);var a=n(64);function i(){return(i=Object.assign||function(e){for(var t=1;t<arguments.length;t++){var n=arguments[t];for(var a in n)Object.prototype.hasOwnProperty.call(n,a)&&(e[a]=n[a])}return e}).apply(this,arguments)}function l(e,t){if(null==e)return{};var n,a,i=function(e,t){if(null==e)return{};var n,a,i={},l=Object.keys(e);for(a=0;a<l.length;a++)n=l[a],t.indexOf(n)>=0||(i[n]=e[n]);return i}(e,t);if(Object.getOwnPropertySymbols){var l=Object.getOwnPropertySymbols(e);for(a=0;a<l.length;a++)n=l[a],t.indexOf(n)>=0||Object.prototype.propertyIsEnumerable.call(e,n)&&(i[n]=e[n])}return i}var r={id:"installation",title:"Installation"},o=[{value:"Dependencies",id:"dependencies",children:[{value:"macOS",id:"macos",children:[]},{value:"Ubuntu (64-bit)",id:"ubuntu-64-bit",children:[]},{value:"Experimental: Windows 10 (64-bit)",id:"experimental-windows-10-64-bit",children:[]}]},{value:"Download, Build and Install",id:"download-build-and-install",children:[{value:"Experimental: CMake for Mac, Linux, and Windows",id:"experimental-cmake-for-mac-linux-and-windows",children:[]}]},{value:"Test",id:"test",children:[]}],c={rightToc:o},b="wrapper";function s(e){var t=e.components,n=l(e,["components"]);return Object(a.b)(b,i({},c,n,{components:t,mdxType:"MDXLayout"}),Object(a.b)("h2",{id:"dependencies"},"Dependencies"),Object(a.b)("p",null,"We use package managers to resolve third-party library dependencies."),Object(a.b)("h3",{id:"macos"},"macOS"),Object(a.b)("p",null,"You will need Xcode with command line tools installed.  To get the command line tools, use:"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"xcode-select --install\n")),Object(a.b)("p",null,"Install dependencies using homebrew:"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"brew install autoconf automake libtool python3\nbrew install boost jsoncpp\n")),Object(a.b)("h3",{id:"ubuntu-64-bit"},"Ubuntu (64-bit)"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"sudo apt-get install \\\n    g++ \\\n    automake \\\n    autoconf \\\n    autoconf-archive \\\n    libtool \\\n    liblz4-dev \\\n    liblzma-dev \\\n    make \\\n    zlib1g-dev \\\n    binutils-dev \\\n    libjemalloc-dev \\\n    libiberty-dev \\\n    libjsoncpp-dev\n")),Object(a.b)("p",null,"Redex requires boost version >= 1.58. The versions in the Ubuntu 14.04 and\n14.10 repositories are too old. This script will install boost for you instead:"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"sudo ./get_boost.sh\n")),Object(a.b)("p",null,"If you're on ubuntu 16.04 or newer, the version in the repository is fine:"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"sudo apt-get install libboost-all-dev\n")),Object(a.b)("h3",{id:"experimental-windows-10-64-bit"},"Experimental: Windows 10 (64-bit)"),Object(a.b)("p",null,"You need Visual Studio 2017. Visual Studio 2015 is also possible, but a couple of C++ compile errors need to be fixed. We use ",Object(a.b)("a",i({parentName:"p"},{href:"https://github.com/Microsoft/vcpkg"}),"vcpkg")," for dependencies. Install vcpkg from their ",Object(a.b)("a",i({parentName:"p"},{href:"https://github.com/Microsoft/vcpkg"}),"document"),":"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"cd c:\\tools\ngit clone https://github.com/Microsoft/vcpkg.git\ncd vcpkg\n.\\bootstrap-vcpkg.bat\n.\\vcpkg integrate install\n")),Object(a.b)("p",null,"Install necessary libraries with ",Object(a.b)("inlineCode",{parentName:"p"},"x64-windows-static"),":"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),".\\vcpkg install boost --triplet x64-windows-static\n.\\vcpkg install zlib --triplet x64-windows-static\n.\\vcpkg install jsoncpp --triplet x64-windows-static\n.\\vcpkg install mman --triplet x64-windows-static\n")),Object(a.b)("h2",{id:"download-build-and-install"},"Download, Build and Install"),Object(a.b)("p",null,"Get ReDex from GitHub:"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"git clone https://github.com/facebook/redex.git\ncd redex\n")),Object(a.b)("p",null,"Now, build ReDex using autoconf and make."),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"# if you're using gcc, please use gcc-5\nautoreconf -ivf && ./configure && make -j4\nsudo make install\n")),Object(a.b)("h3",{id:"experimental-cmake-for-mac-linux-and-windows"},"Experimental: CMake for Mac, Linux, and Windows"),Object(a.b)("p",null,"Alternatively, build using CMake. Note that the current ",Object(a.b)("inlineCode",{parentName:"p"},"CMakeLists.txt")," only implements a rule for ",Object(a.b)("inlineCode",{parentName:"p"},"redex-all")," binary. We will support installation and testing soon."),Object(a.b)("p",null,"Generate build files. By default, it uses Makefile:"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"# Assume you are in redex directory\nmkdir build-cmake\ncd build-cmake\n# .. is the root source directory of Redex\ncmake ..\n")),Object(a.b)("p",null,"If you prefer the ninja build system:"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"cmake .. -G Ninja\n")),Object(a.b)("p",null,"On Windows, first, get ",Object(a.b)("inlineCode",{parentName:"p"},"CMAKE_TOOLCHAIN_FILE")," from the output of ",Object(a.b)("inlineCode",{parentName:"p"},'"vcpkg integrate install"'),", and then:"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),'cmake .. -G "Visual Studio 15 2017 Win64"\n -DVCPKG_TARGET_TRIPLET=x64-windows-static\n -DCMAKE_TOOLCHAIN_FILE="C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake"\n')),Object(a.b)("p",null,"Build ",Object(a.b)("inlineCode",{parentName:"p"},"redex-all"),":"),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"cmake --build .\n")),Object(a.b)("p",null,"On Windows, you may build from Visual Studio. ",Object(a.b)("inlineCode",{parentName:"p"},"Redex.sln")," has been generated."),Object(a.b)("p",null,"You should see a ",Object(a.b)("inlineCode",{parentName:"p"},"redex-all")," executable, and the executable should show about 45 passes."),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"./redex-all --show-passes\n")),Object(a.b)("h2",{id:"test"},"Test"),Object(a.b)("p",null,"Optionally, you can run our unit test suite.  We use gtest, which is downloaded\nvia a setup script."),Object(a.b)("pre",null,Object(a.b)("code",i({parentName:"pre"},{}),"./test/setup.sh\ncd test\nmake check\n")))}s.isMDXComponent=!0},64:function(e,t,n){"use strict";n.d(t,"a",(function(){return o})),n.d(t,"b",(function(){return p}));var a=n(0),i=n.n(a),l=i.a.createContext({}),r=function(e){var t=i.a.useContext(l),n=t;return e&&(n="function"==typeof e?e(t):Object.assign({},t,e)),n},o=function(e){var t=r(e.components);return i.a.createElement(l.Provider,{value:t},e.children)};var c="mdxType",b={inlineCode:"code",wrapper:function(e){var t=e.children;return i.a.createElement(i.a.Fragment,{},t)}},s=Object(a.forwardRef)((function(e,t){var n=e.components,a=e.mdxType,l=e.originalType,o=e.parentName,c=function(e,t){var n={};for(var a in e)Object.prototype.hasOwnProperty.call(e,a)&&-1===t.indexOf(a)&&(n[a]=e[a]);return n}(e,["components","mdxType","originalType","parentName"]),s=r(n),p=a,u=s[o+"."+p]||s[p]||b[p]||l;return n?i.a.createElement(u,Object.assign({},{ref:t},c,{components:n})):i.a.createElement(u,Object.assign({},{ref:t},c))}));function p(e,t){var n=arguments,a=t&&t.mdxType;if("string"==typeof e||a){var l=n.length,r=new Array(l);r[0]=s;var o={};for(var b in t)hasOwnProperty.call(t,b)&&(o[b]=t[b]);o.originalType=e,o[c]="string"==typeof e?e:a,r[1]=o;for(var p=2;p<l;p++)r[p]=n[p];return i.a.createElement.apply(null,r)}return i.a.createElement.apply(null,n)}s.displayName="MDXCreateElement"}}]);