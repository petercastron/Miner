/*
 * @Author: fjf fjf@123.com
 * @Date: 2022-06-27 16:11:33
 * @LastEditors: fjf fjf@123.com
 * @LastEditTime: 2022-09-28 16:34:43
 * @FilePath: \wp\src\store\index.js
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
import Vue from 'vue'
import Vuex from 'vuex'
import app from './modules/app'
import user from './modules/user'
import permission from './modules/permission'
import getters from './getters'
Vue.use(Vuex)
const store = new Vuex.Store({
  modules: {
    app,
    user,
    permission
  },
  getters
})
export default store
