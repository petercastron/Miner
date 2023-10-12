<template>
  <div id="app">
    <!-- <router-view/> -->
    <!-- <keep-alive>
    <router-view v-if="$route.meta.keepAlive"></router-view>
</keep-alive> -->
<router-view v-if="!$route.meta.keepAlive"></router-view>
  </div>
  
</template>

<script>
import store from '@/store'  
  export default {
    name: 'App',
  	methods:{
  // 添加判断方法
	  // isMobile() {
	  //  	let flag = navigator.userAgent.match(/(phone|pad|pod|iPhone|iPod|ios|iPad|Android|Mobile|BlackBerry|IEMobile|MQQBrowser|JUC|Fennec|wOSBrowser|BrowserNG|WebOS|Symbian|Windows Phone)/i);
		//   if(flag){
		// 		console.log('移动端打开');
    //      console.log(this.store,'store1111111111')      
		//   }else{
		//   	console.log("PC端打开")
		//   }
	  //  }

  	},
	mounted(){
		// this.isMobile();
	},

    
  }
  
</script>

<style>
</style>
