(window.webpackJsonp=window.webpackJsonp||[]).push([[2,4],{42:function(e,t,a){"use strict";a.r(t);var r=a(0),n=a.n(r),o=a(70),l=a(65),i=a.n(l),c=a(68),s=a.n(c),m=a(66),u=a.n(m);function f(e){return(f="function"==typeof Symbol&&"symbol"==typeof Symbol.iterator?function(e){return typeof e}:function(e){return e&&"function"==typeof Symbol&&e.constructor===Symbol&&e!==Symbol.prototype?"symbol":typeof e})(e)}function p(e,t){for(var a=0;a<t.length;a++){var r=t[a];r.enumerable=r.enumerable||!1,r.configurable=!0,"value"in r&&(r.writable=!0),Object.defineProperty(e,r.key,r)}}function d(e,t){return!t||"object"!==f(t)&&"function"!=typeof t?function(e){if(void 0===e)throw new ReferenceError("this hasn't been initialised - super() hasn't been called");return e}(e):t}function g(e){return(g=Object.setPrototypeOf?Object.getPrototypeOf:function(e){return e.__proto__||Object.getPrototypeOf(e)})(e)}function h(e,t){return(h=Object.setPrototypeOf||function(e,t){return e.__proto__=t,e})(e,t)}var y=[{title:"Optimizing",content:"Redex provides a framework for reading, writing, and analyzing .dex files, and a set of optimization passes that use this framework to improve the bytecode. An APK optimized by Redex should be smaller and faster."},{title:"Fast",content:"Fewer bytes also means faster download times, faster install times, and lower data usage for cell users. Lastly, less bytecode also typically translates into faster runtime performance."},{title:"Buck Integration",content:"Redex has deep integration with Buck where your Redex config is passed as a parameter to the Buck android_binary rule when generating the APK."}],b=function(e){function t(){return function(e,t){if(!(e instanceof t))throw new TypeError("Cannot call a class as a function")}(this,t),d(this,g(t).apply(this,arguments))}var a,r,o;return function(e,t){if("function"!=typeof t&&null!==t)throw new TypeError("Super expression must either be null or a function");e.prototype=Object.create(t&&t.prototype,{constructor:{value:e,writable:!0,configurable:!0}}),t&&h(e,t)}(t,e),a=t,(r=[{key:"render",value:function(){return n.a.createElement("div",{className:"pluginWrapper buttonWrapper",style:{padding:"0 2px"}},n.a.createElement("a",{className:"button",href:this.props.href,target:this.props.target},this.props.children))}}])&&p(a.prototype,r),o&&p(a,o),t}(n.a.Component);b.defaultProps={target:"_self"};var E=function(e){return n.a.createElement("div",{className:"projectLogo",style:{padding:"0px 250px 130px 0px"}},n.a.createElement("img",{src:e.img_src}))};t.default=function(e){e.language;var t=i()().siteConfig,a=void 0===t?{}:t;return n.a.createElement(o.a,{title:a.title,description:a.description},n.a.createElement("div",null,n.a.createElement("header",{className:s()("hero hero--primary")},n.a.createElement("div",{className:"container padding-vert--lg"},n.a.createElement("div",{className:"row"},n.a.createElement("div",{className:"col text--center"},n.a.createElement("h1",{className:"hero__title"},a.title),n.a.createElement("p",{className:"hero__subtitle"},a.tagline),n.a.createElement("div",{className:"buttons"},n.a.createElement(b,{href:u()("docs/installation")},"GETTING STARTED"),n.a.createElement(b,{href:"https://github.com/facebook/redex"},"GITHUB"))),n.a.createElement(E,{img_src:u()("img/redex-hero.png")})))),n.a.createElement("div",{className:"mainContainer"},y&&y.length&&n.a.createElement("section",{className:"features"},n.a.createElement("div",{className:"container"},n.a.createElement("div",{className:"row padding-vert--xl"},y.map((function(e,t){var a=e.imageUrl,r=e.title,o=e.content;return n.a.createElement("div",{key:t,className:"col col--4 text--center"},a&&n.a.createElement("div",{className:"text--center margin-bottom--lg"},n.a.createElement("img",{className:"featureImage",src:u()(a),alt:r})),n.a.createElement("h3",null,r),n.a.createElement("p",null,o))}))))))))}},69:function(e,t,a){"use strict";var r=a(0),n=a.n(r),o=a(68),l=a.n(o),i=a(67),c=a(65),s=a.n(c),m=a(66),u=a.n(m);function f(){return(f=Object.assign||function(e){for(var t=1;t<arguments.length;t++){var a=arguments[t];for(var r in a)Object.prototype.hasOwnProperty.call(a,r)&&(e[r]=a[r])}return e}).apply(this,arguments)}function p(e){var t="https://github.com/".concat(e.config.organizationName,"/").concat(e.config.projectName);return n.a.createElement("div",{className:"footerSection",style:{flex:1}},n.a.createElement("h4",{className:"footer__title"},"Social"),n.a.createElement("div",{className:"social"},n.a.createElement("a",{className:"github-button",href:t,"data-count-href":"".concat(t,"/stargazers"),"data-show-count":"true","data-count-aria-label":"# stargazers on GitHub","aria-label":"Star this project on GitHub"},e.config.projectName)))}t.a=function(){var e=s()().siteConfig,t=void 0===e?{}:e,a=t.themeConfig,r=(void 0===a?{}:a).footer;if(!r)return null;var o=r.copyright,c=r.links,m=void 0===c?[]:c,d=r.logo;return n.a.createElement("footer",{className:l()("footer",{"footer--dark":"dark"===r.style})},n.a.createElement("div",{className:"container"},m&&m.length>0&&n.a.createElement("div",{className:"row footer__links"},n.a.createElement("a",{href:"#"},n.a.createElement("img",{src:u()("img/redex.png"),width:"66",height:"58"})),m.map((function(e,t){return n.a.createElement("div",{key:t,className:"col footer__col",style:{flex:1}},null!=e.title?n.a.createElement("h4",{className:"footer__title"},e.title):null,null!=e.items&&Array.isArray(e.items)&&e.items.length>0?n.a.createElement("ul",{className:"footer__items"},e.items.map((function(e){return n.a.createElement("li",{key:e.href||e.to,className:"footer__item"},n.a.createElement(i.a,f({className:"footer__link-item"},e,e.href?{target:"_blank",rel:"noopener noreferrer",href:e.href}:{to:u()(e.to)}),e.label))}))):null)})),n.a.createElement(p,{config:t})),(d||o)&&n.a.createElement("div",{className:"text--center"},d&&d.src&&n.a.createElement("div",{className:"margin-bottom--sm"},n.a.createElement("img",{className:"footer__logo",alt:d.alt,src:d.src})),o)))}}}]);