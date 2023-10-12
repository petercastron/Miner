<template>
  <div class="upload-container">
    <el-button icon='el-icon-upload' size="mini" :style="{background:color,borderColor:color}"
               @click=" dialogVisible=true" type="primary">上传图片
    </el-button>
    <el-dialog append-to-body :visible.sync="dialogVisible">
      <div class="img-files flex">
      <div class="img-box" v-for="(item, index) in listObj" :key="index">
        <img :src="item.url" alt />
        <!-- 预览，删除图标 -->
        <span class="deleteicon">
          <i class="el-icon-delete" style="color:white;font-size:23px" @click="tapdel(index)"></i>
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
              <span size="small" type="primary" class="editbtn">点击上传</span>
        </label>
    </div>
      <el-button @click="dialogVisible = false">取 消</el-button>
      <el-button type="primary" @click="handleSubmit">确 定</el-button>
    </el-dialog>
  </div>
</template>

<script>
  import {policy} from '@/api/oss'
import request from '@/utils/request'
import { getToken } from '@/utils/auth'
  export default {
    name: 'editorSlideUpload',
    props: {
      color: {
        type: String,
        default: '#1890ff'
      }
    },
    data() {
      return {
        dialogVisible: false,
        listObj: [],
        fileList: [],
        dataObj:[],
         url:''
      }
    },
    methods: {
      fileChange(event) {
          // if(this.value.productDetailsPictureList.length>=5){
          //       this.$message({
          //         message: '最多只能上传5张图片',
          //         type: 'warning',
          //         duration:1000
          //       });
          //     return false;
          // }
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
        console.log( _this.files)
        fromData.append("file", _this.files, _this.files.name);
        console.log(fromData)
         _this.$axios({
              // url: "http://192.168.0.246:8086/admin/aliyun/oss/simpleFileUpload",
               url: "https://floms.nonglianbang.net/admin/aliyun/oss/simpleFileUpload",
              method: "POST",
              data: fromData,
              headers: { 'Authorization':getToken(), "Content-Type": "multipart/form-data"}
          }).then(res => {
              if(res.data.code==200){
                   _this.listObj.push({url:res.data.data})
              }
        })        
    },
      tapdel(index){
        this.listObj.splice(index,1);
      },
      checkAllSuccess() {
        return this.listObj.every(item => this.listObj[item])
      },
      handleSubmit() {
        const arr = this.listObj
        // if (!this.checkAllSuccess()) {
        //   this.$message('请等待所有图片上传成功 或 出现了网络问题，请刷新页面重新上传！')
        //   return
        // }
        this.$emit('successCBK', arr);
        this.listObj = [];
        this.fileList = [];
        this.dialogVisible = false;
      },
    }
  }
</script>

<style rel="stylesheet/scss" lang="scss" scoped>
  .upload-container .editor-slide-upload{
    margin-bottom: 20px;
  }
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
  margin-top: 18px;
  margin-right: 18px;
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
.editbtn{
  width: 80px;
  height: 34px;
  line-height: 34px;
  background-color: #D1AB7F;
  color: white;
  border-radius: 6px;
  text-align: center;
}
</style>
