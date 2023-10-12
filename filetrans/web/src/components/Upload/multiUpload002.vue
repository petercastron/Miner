<template>
  <div>
    <div class="img-files flex">
      <div class="img-box" v-for="(item, i) in dataObj" :key="i">
        <img :src="item" alt />
        <!-- 右上角三角icon -->
        <div class="sicon">
          <i class="el-icon-upload-success el-icon-check"></i>
        </div>
        <!-- 预览，删除图标 -->
        <span class="deleteicon">
          <i
            class="el-icon-zoom-in"
            style="color:white;margin-right:20px;font-size:23px"
            @click="tapviewimg(item)"
          ></i>
          <i class="el-icon-delete" style="color:white;font-size:23px" @click="tapdel"></i>
        </span>
      </div>
      <!-- 上传图片事件 -->
      <label class="img-file">
        <input
          class="uploadimg"
          type="file"
          accept="image/*"
          @change="fileChange($event)"
          ref="avatarInput"
          style="display:none"
        />
        <span for="files" @click="shangchuan()">
          <i class="el-icon-plus"></i>
        </span>
      </label>
    </div>
    <div style="color:red" slot="tip" class="el-upload__tip">提醒：选择的资源路径名推荐为英文！！！</div>
    <!-- 预览图片 -->
    <div class="viewimg" v-show="viewbox">
      <div class="viewimg_box">
        <img :src="viewurl" class="imgview" />
        <span class="deleviewbox">
          <i class="el-icon-close" @click="tapcloseview"></i>
        </span>
      </div>
    </div>
  </div>
</template>
<script>
import request from '@/utils/request'
import { getToken } from '@/utils/auth'
  export default {
    name: 'multiUpload',
    props: {
      // //图片属性数组
      value: Array,
      // //最大上传图片数量
      maxCount:{
        type:Number,
        default:5
      }
    },
    data() {
      return {
        dataObj: [],      // 图片预览地址
        size: 5     ,  // 限制上传数量
        viewurl:"",
        viewbox:false,
        files:{},
        dialogVisible: false,
        dialogImageUrl:null,
        url:'',
      };
    },
    computed: {
      // fileList() {
      //   let fileList=[];
      //   for(let i=0;i<this.value.length;i++){
      //     fileList.push({url:this.value[i]});
      //   }
      //   return fileList;
      // }
    },
    methods: {
      // 上传图片 
      fileChange(event) {
          if(this.dataObj.length>=5){
                this.$message({
                  message: '最多只能上传'+this.maxCount+'张图片',
                  type: 'warning',
                  duration:1000
                });
              return false;
          }
          // 上传图片事件
          var _this = this
          let fileLIST=[]
          var event = event || window.event;
          var file = event.target.files
          var leng=file.length;
          for(var i=0;i<leng;i++){
            var reader = new FileReader();    // 使用 FileReader 来获取图片路径及预览效果
              fileLIST.push(file[i])
              reader.readAsDataURL(file[i]); 
              reader.onload =function(e){
          };                          
      }
      // 图片上传给后台部分
        _this.files= fileLIST[0]
        let fromData = new FormData();
        fromData.append("file", _this.files, _this.files.name);
         _this.$axios({
              url: "http://192.168.0.246/admin/aliyun/oss/simpleFileUpload",
              method: "POST",
              data: fromData,
              headers: { 'Authorization':getToken(), "Content-Type": "multipart/form-data"}
          }).then(res => {
              if(res.data.code==200){
                   _this.dataObj.push(res.data.data)
                   
                 
              }
        })        
    },
    // 点击预览图片
    tapviewimg(url){
         this.viewurl=url
         this.viewbox=true
         document.body.style.height='100%';
         document.body.style.overflow='hidden';
    },
    // 关闭预览图片
    tapcloseview(){
      this.viewbox=false
      document.body.style.overflow='scroll';
    },
     
     shangchuan(){
       this.$emit("zifu",this.dataObj,'aaa', true);
     },
  
// 删除图片
    tapdel(){},
    }
  }
</script>
<style>
/* 图片列表 */
.flex {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
}
.img-box {
  width: 146px;
  height: 146px;
  border: 1px solid #c0ccda;
  border-radius: 7px;
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
}
.img-box:hover .deleteicon {
  background-color: rgba(0, 0, 0, 0.5);
  opacity: 1;
  cursor: pointer;
}
.img-box:hover .sicon {
  opacity: 0;
}
.img-box img {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: 0;
}

.uploadimg {
  width: 0;
  height: 0;
  opacity: 0;
}
.img-file {
  width: 146px;
  height: 146px;
  margin-top: 20px;
  border: 1px dashed #c0ccda;
  margin-right: 20px;
  border-radius: 7px;
  display: flex;
  align-items: center;
  justify-content: center;
}
.img-file:hover {
  border: 1px dashed #D1AB7F;
  cursor: pointer;
}
.deleteicon {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  opacity: 0;
  border-radius: 7px;
}

/* 预览图片 */
.viewimg {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background-color: rgba(0, 0, 0, 0.5);
  overflow-y: scroll;
  z-index: 999;
}
.viewimg_box {
  width: 60%;
  margin-left: 20%;
  background-color: white;
  position: relative;
  margin-top: 30px;
}
.imgview {
  width: 100%;
}
.deleviewbox {
  position: absolute;
  top: 10px;
  right: 10px;
  color: #999999;
  font-size: 23px;
}
.deleviewbox:hover {
  color: #D1AB7F;
}
.sicon {
  position: absolute;
  right: -15px;
  top: -6px;
  width: 46px;
  height: 24px;
  line-height: 30px;
  background: #13ce66;
  text-align: center;
  -webkit-transform: rotate(45deg);
  transform: rotate(45deg);
  -webkit-box-shadow: 0 0 1pc 1px rgba(0, 0, 0, 0.2);
  box-shadow: 0 0 1pc 1px rgba(0, 0, 0, 0.2);
}
.sicon i {
  color: white;
  width: 24px;
  height: 24px;
  margin-left: 10px;
  -webkit-transform: rotate(-45deg);
  transform: rotate(-45deg);
}
.el-icon-plus {
  color: #999999;
  font-size: 34px;
}
</style>


