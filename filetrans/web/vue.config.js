'use strict'
const path = require('path')
function resolve(dir) {
  return path.join(__dirname, dir)
}
const port = 8088
// All configuration item explanations can be find in https://cli.vuejs.org/config/
module.exports = {
  lintOnSave: false  , //
  runtimeCompiler:true,
  // hash 模式下可使用
  // publicPath: process.env.NODE_ENV === 'development' ? '/' : './',
  publicPath: '',
  outputDir: 'dist',
  assetsDir: 'static',
 //lintOnSave: process.env.NODE_ENV === 'development',
  devServer: {
    port: port,
    open: false,
    disableHostCheck:true,
  
     /* proxy: { //配置跨域
        '/platform': {
          target: 'http://localhost:80', //填写请求的目标地址
          // ws: true,
          changOrigin: true, //允许跨域
          pathRewrite: {
            '^/platform': '' 
          }
        }
      }*/
    
  },
  /*configureWebpack: {
    // provide the app's title in webpack's name field, so that
    // it can be accessed in index.html to inject the correct title.
    resolve: {
      alias: {
        '@': resolve('src')
      }
    }
  },*/
  chainWebpack(config) {
    config.plugins.delete('preload') // TODO: need test
    config.plugins.delete('prefetch') // TODO: need test
    config.plugin('html')
    .tap(args => {
        args[0].ieRend= new Date().getTime();
        return args
    })
    // set svg-sprite-loader
   /* config.module
      .rule('svg')
      .exclude.add(resolve('src/assets/icons'))
      .end()
    config.module
      .rule('icons')
      .test(/\.svg$/)
      .include.add(resolve('src/assets/icons'))
      .end()
      .use('svg-sprite-loader')
      .loader('svg-sprite-loader')
      .options({
        symbolId: 'icon-[name]'
      })
      .end()*/

    // set preserveWhitespace
    config.module
      .rule('vue')
      .use('vue-loader')
      .loader('vue-loader')
      .tap(options => {
        options.compilerOptions.preserveWhitespace = true
        return options
      })
      .end()

   /* config
      // https://webpack.js.org/configuration/devtool/#development
      .when(process.env.NODE_ENV === 'development', config =>
        config.devtool('cheap-source-map')
      )*/
  }
}
