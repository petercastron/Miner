/*
 * @Author: fjf fjf@123.com
 * @Date: 2022-06-27 16:11:33
 * @LastEditors: fjf fjf@123.com
 * @LastEditTime: 2022-11-10 12:48:45
 * @FilePath: \wp\src\router\index.js
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
import Vue from 'vue'
import Router from 'vue-router'

Vue.use(Router)

/* Layout */
import Layout from '../views/layout/Layout'
export const constantRouterMap = [
  {path: '/404', component: () => import('@/views/404'), hidden: true},
  {
    path: '',
    component: Layout,
    redirect: '/home',
    children: [{ 
      path: 'home',
      name: 'home',
      component: () => import('@/views/home/index'),
      // component: () => import('@/views/home/index'),
    },
    { 
      path: 'indexds',
      name: 'indexds',
      component: () => import('@/views/home/indexds'),
    },
  ]
  },
  
]
// let head = document.getElementsByTagName('head');
// let meta = document.createElement('meta');
// meta.name = 'referrer';
// //根据实际情况修改referrer的值，可选值参考上文
// meta.content = 'no-referrer';
// head[0].appendChild(meta);
export default new Router({
  // mode: 'history', //后端支持可开
  scrollBehavior: () => ({y: 0}),
  routes: constantRouterMap
})

