/*
 * @Author: fjf fjf@123.com
 * @Date: 2022-06-27 16:11:33
 * @LastEditors: fjf fjf@123.com
 * @LastEditTime: 2023-03-31 11:10:30
 * @FilePath: \wp\src\utils\request.js
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
import axios from 'axios'
import { Message, MessageBox } from 'element-ui'
import store from '../store'
import { getToken } from '@/utils/auth'
//获取动态的 请求头
var css =window.location.host
const urls="http://"+css.split(':')[0]+ ":" + window.location.port

const service = axios.create({
  // baseURL:"http://192.168.4.85:8080/", // api的base_url线下环境
  baseURL: urls, // urls正式环境
  timeout: 150000 // 请求超时时间
})

// request拦截器
service.interceptors.request.use(config => {
  config.headers = {
        'Content-Type': 'application/json', //配置请求头
        Authorization:'Bearer'+' '+getToken(),
        // Authorization:'Bearer'+' '+"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE2Njg3NzExMDAsImlkZW50aXR5IjoiMHgyOGY0MkJBZDU2OEVGNjkwMENDYjgzQzZlNjJGYjE0NWFjZjg2YUYzIiwib3JpZ19pYXQiOjE2Njg3NjM5MDB9.GSGlq3RV_Vr-GCirJ44LDrRIuLISWFx9_hTSqXtdBo4",
       
    }
  return config
}, error => {
  // Do something with request error
  // console.log(error) // for debug
  Promise.reject(error)
})
// respone拦截器
service.interceptors.response.use(
  response => {
    if (response.status !== 200) {
      // console.log(response,'出错了')
      // Message({
      //   message: res.msg,
      //   type: 'error',
      //   duration: 3 * 1000
      // })
      return Promise.reject(response.data)
    } else{
      // console.log(response,'response')
      // console.log(response,'正确的')
      return response   
    }
  },
  error => {
    // Message({
    //   message: error.msg,
    //   type: 'error',
    //   duration: 3 * 1000
    // })
    // console.log(error,'出错了')
    return Promise.reject(error)
  }
)
export default service
