/*
 * @Author: fjf fjf@123.com
 * @Date: 2022-11-18 16:26:21
 * @LastEditors: fjf fjf@123.com
 * @LastEditTime: 2023-03-23 15:02:31
 * @FilePath: \fileRun\src\api\home.js
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
import request from '@/utils/request'
export function login(data) {
  return request({
    url:'/login',
    method:'POST',
    data:data
  })
}
export function spaces(params) {
  return request({
    url:'/v1/file/spaces',
    method:'get',
    params:params
  })
}
export function hello(params) {
  return request({
    url:'/hello',
    method:'get',
    params:params
  })
}
export function fileList(params) {
  return request({
    url:'/v1/file/list',
    method:'get',
    params:params
  })
}
export function download(params) {
  return request({
    url:'/v1/file/download',
    method:'get',
    params:params
  })
}
export function deletes(data) {
  return request({
    url:'/v1/file/delete',
    method:'POST',
    data:data
  })
}
export function clearCache(data) {
  return request({
    url:'/v1/file/clearCache',
    method:'POST',
    data:data
  })
}
export function refresh_token(data) {
  return request({
    url:'/refresh_token',
    method:'get',
    data:data
  })
}

