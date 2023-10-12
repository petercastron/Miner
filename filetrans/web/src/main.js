import Vue from 'vue'

import 'normalize.css/normalize.css'// A modern alternative to CSS resets

import ElementUI from 'element-ui'
import 'element-ui/lib/theme-chalk/index.css'
import locale from 'element-ui/lib/locale/lang/zh-CN' // lang i18n
import VCharts from 'v-charts'
import visibility from 'vue-visibility-change'

Vue.use(visibility );
visibility.change((evt,hidden)=>{
  // console.log('glsdad:'+hidden)
})
// 适配
// import 'lib-flexible'

import '@/styles/index.scss' // global css
import axios from 'axios'
import App from './App'
import router from './router'
import store from './store'
import JsonExcel from 'vue-json-excel'

Vue.component('downloadExcel', JsonExcel)
import '@/icons' // icon
import '@/permission' // permission control
import oss  from "./utils/oss.js"
Vue.prototype.$oss = oss
import echarts from 'echarts'
Vue.prototype.$echarts = echarts
import VueAMap  from 'vue-amap'

import { lazyAMapApiLoaderInstance } from 'vue-amap';
Vue.use(VueAMap )
VueAMap .initAMapApiLoader({

// 高德的key
key: '42b72796c6f3ba276100a8e5e159a638',
// 插件集合 
plugin: ['AMap.Autocomplete', 'AMap.PlaceSearch', 'AMap.Scale', 'AMap.OverView', 'AMap.ToolBar', 'AMap.MapType', 'AMap.PolyEditor', 'AMap.CircleEditor'],
v: '1.4.4', 
  uiVersion: '1.0' // ui库版本，不配置不加载,
});

 /* lazyAMapApiLoaderInstance.load().then(() => {
    // your code ...
    this.map = new AMap.Map('amapContainer', {
      center: new AMap.LngLat(121.59996, 31.197646)
    });
  });
*/
Vue.use(ElementUI, { locale })
Vue.use(VCharts)

Vue.config.productionTip = false
Vue.prototype.$axios = axios
//封装rem适配
// const setHtmlFontSize = () => {
// const htmlDom = document.getElementsByTagName('html')[0];
// let htmlWidth = document.documentElement.clientWidth || document.body.clientWidth;
// if (htmlWidth >= 750) {
// htmlWidth = 750;
// }
// if (htmlWidth <= 320) {
// htmlWidth = 320;
// }
// htmlDom.style.fontSize = `${htmlWidth / 7.5}px`;
// };
// window.onresize = setHtmlFontSize;
// setHtmlFontSize();

new Vue({
  el: '#app',
  router,
  store,
  template: '<App/>',
  components: { App }
})
