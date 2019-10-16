(window.webpackJsonp=window.webpackJsonp||[]).push([[5],{44:function(e,n,r){"use strict";r.r(n),r.d(n,"frontMatter",(function(){return i})),r.d(n,"rightToc",(function(){return l})),r.d(n,"default",(function(){return d}));r(0);var t=r(64);function a(){return(a=Object.assign||function(e){for(var n=1;n<arguments.length;n++){var r=arguments[n];for(var t in r)Object.prototype.hasOwnProperty.call(r,t)&&(e[t]=r[t])}return e}).apply(this,arguments)}function o(e,n){if(null==e)return{};var r,t,a=function(e,n){if(null==e)return{};var r,t,a={},o=Object.keys(e);for(t=0;t<o.length;t++)r=o[t],n.indexOf(r)>=0||(a[r]=e[r]);return a}(e,n);if(Object.getOwnPropertySymbols){var o=Object.getOwnPropertySymbols(e);for(t=0;t<o.length;t++)r=o[t],n.indexOf(r)>=0||Object.prototype.propertyIsEnumerable.call(e,r)&&(a[r]=e[r])}return a}var i={id:"buck",title:"Using ReDex with Buck"},l=[],c={rightToc:l},p="wrapper";function d(e){var n=e.components,r=o(e,["components"]);return Object(t.b)(p,a({},c,r,{components:n,mdxType:"MDXLayout"}),Object(t.b)("p",null,"ReDex already has a deep integration with BUCK.\nSteps:"),Object(t.b)("p",null,"1 - In your android_binary target add the following parameters:"),Object(t.b)("ul",null,Object(t.b)("li",{parentName:"ul"},Object(t.b)("inlineCode",{parentName:"li"},"redex = True"),","),Object(t.b)("li",{parentName:"ul"},Object(t.b)("inlineCode",{parentName:"li"},"redex_config = <path redex config file>")),Object(t.b)("li",{parentName:"ul"},Object(t.b)("inlineCode",{parentName:"li"},"redex_extra_args = [ <additional args> ]"))),Object(t.b)("p",null,"Example:"),Object(t.b)("pre",null,Object(t.b)("code",a({parentName:"pre"},{className:"language-python"}),"  android_library(\n    name = 'test',\n    srcs = java_files,\n    deps = test_deps,\n  )\n  android_binary(\n    name = 'test_pre',\n    manifest = 'AndroidManifest.xml',\n    keystore = '//keystores:prod',\n    package_type = 'release',\n    deps = [':test'],\n    android_sdk_proguard_config = 'none',\n    proguard_config = 'test_pre-proguard.pro',\n    redex = True,\n    redex_config = 'redex_config.json',\n    redex_extra_args = [ \n    '-j', \n    '/android-sdk/platforms/android-28/android.jar'\n    ],\n  )\n")),Object(t.b)("p",null,"Note: Redex will use the ",Object(t.b)("inlineCode",{parentName:"p"},"commandline.txt")," file generated by Proguard in previous step as its ",Object(t.b)("inlineCode",{parentName:"p"},"proguard-config"),"file."),Object(t.b)("p",null,"2 - In your ",Object(t.b)("inlineCode",{parentName:"p"},".buckconfig")," file add the redex-binary location in the android section"),Object(t.b)("p",null,"Example:"),Object(t.b)("pre",null,Object(t.b)("code",a({parentName:"pre"},{}),"\n[android]\nredex=/usr/local/bin/redex\n\n")))}d.isMDXComponent=!0},64:function(e,n,r){"use strict";r.d(n,"a",(function(){return l})),r.d(n,"b",(function(){return u}));var t=r(0),a=r.n(t),o=a.a.createContext({}),i=function(e){var n=a.a.useContext(o),r=n;return e&&(r="function"==typeof e?e(n):Object.assign({},n,e)),r},l=function(e){var n=i(e.components);return a.a.createElement(o.Provider,{value:n},e.children)};var c="mdxType",p={inlineCode:"code",wrapper:function(e){var n=e.children;return a.a.createElement(a.a.Fragment,{},n)}},d=Object(t.forwardRef)((function(e,n){var r=e.components,t=e.mdxType,o=e.originalType,l=e.parentName,c=function(e,n){var r={};for(var t in e)Object.prototype.hasOwnProperty.call(e,t)&&-1===n.indexOf(t)&&(r[t]=e[t]);return r}(e,["components","mdxType","originalType","parentName"]),d=i(r),u=t,s=d[l+"."+u]||d[u]||p[u]||o;return r?a.a.createElement(s,Object.assign({},{ref:n},c,{components:r})):a.a.createElement(s,Object.assign({},{ref:n},c))}));function u(e,n){var r=arguments,t=n&&n.mdxType;if("string"==typeof e||t){var o=r.length,i=new Array(o);i[0]=d;var l={};for(var p in n)hasOwnProperty.call(n,p)&&(l[p]=n[p]);l.originalType=e,l[c]="string"==typeof e?e:t,i[1]=l;for(var u=2;u<o;u++)i[u]=r[u];return a.a.createElement.apply(null,i)}return a.a.createElement.apply(null,r)}d.displayName="MDXCreateElement"}}]);