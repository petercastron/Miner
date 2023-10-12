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
    <span style="border:1px solid red" v-for="item in levelList" :key="item.path" @click="takb(item.path)">{{ item.meta.title}}</span>
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
      arr2:[]
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
    //  console.log(this.arr2,'this.arr2')
      let _arr = new Array();
    for(var i=0;i<arr1.length;i++){
       _arr.push(arr1[i]);
    }
    for(var i=0;i<this.arr2.length;i++){
        var flag = true;
        for(var j=0;j<arr1.length;j++){
            if(this.arr2[i]==arr1[j]){
                flag=false;
                break;
            }
        }
        if(flag){
            _arr.push(this.arr2[i]);
        }
    }
    console.log(_arr,'_arr')
    //  return _arr;
      const first = _arr[0]
      if (first && first.name !== 'home') {
        _arr = [{ path: '/home', meta: { title: '首页' }}].concat(_arr) 
      }
      this.levelList = _arr;
    },
    takb(path){
    console.log(path,'path')
      // this.$router.push({path: 'path'})
    }
  },
};
</script>

<style rel="stylesheet/scss" lang="scss" scoped>
.app-breadcrumb.el-breadcrumb {
  display: inline-block;
  font-size: 14px;
  line-height: 50px;
  margin-left: 10px;
  .no-redirect {
    color: #97a8be;
    cursor: text;
  }
}
</style>
