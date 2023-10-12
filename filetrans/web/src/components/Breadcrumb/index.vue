<template>
  <el-breadcrumb class="app-breadcrumb" separator="/">
    <!-- <transition-group name="breadcrumb">
      <el-breadcrumb-item style="border:1px solid red;margin-left:10px" v-for="(item, index) in levelList" :key="item.path">
        <span
          v-if="item.redirect === 'noredirect' || index == levelList.length - 1"
          class="no-redirect"
          >{{ item.meta.title }}</span
        >
        <router-link v-else :to="item.path">{{
          item.meta.title
        }}</router-link>
      </el-breadcrumb-item>
    </transition-group> -->
    <span v-for="(item, index) in levelList" :key="item.path" >
    <span class="span"  @click="takb(item.path)">{{ item.meta.title}} </span>
     <span class="span1" @click="deletes(index)"><img src="http://wisdomlife.oss-cn-beijing.aliyuncs.com/mall/images/1619080052590.png" alt=""></span>
    </span>
    
  </el-breadcrumb>
   
      <!-- <el-breadcrumb-item v-for="(item, index) in levelList" :key="item.path">
        <span
          v-if="item.redirect === 'noredirect' || index == levelList.length - 1"
          class="no-redirect"
          >{{ item.meta.title }}</span
        >
        <router-link v-else :to="item.redirect || item.path">{{
          item.meta.title
        }}</router-link>
      </el-breadcrumb-item> -->
</template>
<script>
export default {
  created() {
    this.getBreadcrumb();
  },
  data() {
    return {
      levelList: null,
      arr2:[],
      result:[],
      arr1:[]
    };
  },
  watch: {
    $route() {
      this.getBreadcrumb();
    },
  },
  methods: {
    getBreadcrumb() {
      let arr1 = this.$route.matched.filter((item) => item.name);
      //  console.log(arr1,'this.arr1')
     this.arr2=this.arr2.concat(arr1)
     var result = [];
            var obj = {};
            for(var i =0; i<this.arr2.length; i++){
                if(!obj[this.arr2[i].meta.title]){
                    result.push(this.arr2[i]);
                    obj[this.arr2[i].meta.title] = true;
                }
            }
   
      //return _arr;
      // const first = _arr[0]
      // if (first && first.name !== 'home') {
      //   _arr = [{ path: '/home', meta: { title: '首页' }}].concat(_arr) 
      // }
      this.levelList = result;
    },
    takb(path){
    // console.log(path,'path')
    this.$router.push({path: path})
    },
    deletes(index){
     this.levelList.splice(index ,1)
     this.listupdate(this.levelList)
    
    //  console.log(index,'index')
    },
    listupdate(levelList){
     console.log(levelList,'levelList')
    this.arr2=levelList
      let result = [];
            var obj = {};
            for(var i =0; i<this.arr2.length; i++){
                if(!obj[this.arr2[i].meta.title]){
                    result.push(this.arr2[i]);
                    obj[this.arr2[i].meta.title] = true;
                }
            }
       this.levelList = result;      
    }
  }
}
</script>

<style rel="stylesheet/scss" lang="scss" scoped>
.app-breadcrumb.el-breadcrumb {
  width: 95%;
 height: 40px;
 overflow: hidden;
  // border: 1px solid red;
  display: inline-block;
  font-size: 14px;
  line-height: 50px;
  margin-left: 10px;
  .no-redirect {
    color: #97a8be;
    cursor: text;
  }
}
.span{
margin-left: 10px;
padding: 10px;
background: rgb(64, 158, 255);
color: #fff;
border-radius: 3px;
}
.span1{
 background: rgb(64, 158, 255);
 padding: 10px;
}
.span1 img{
  width: 10px;
  height: 12px;
}
 </style>
