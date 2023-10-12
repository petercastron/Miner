<template> 
   <!-- action="http://192.168.0.242:8086/aliyun/oss/multiFileUpload" -->
  <div>
        <el-upload
          class="upload-demo"
          action="https://floms.nonglianbang.net/admin/aliyun/oss/multiFileUpload"
          :on-preview="handlePreviews"
          :on-remove="handleRemoves"
          :before-remove="beforeRemoves"
          :limit="100"
          :on-exceed="handleExceeds" 
          :headers="token"
          :on-success="handlePictureCardPreviewsed"
          :file-list="fileList"
           list-type="picture-card"
          :drag='true'
        >
        <div style="color:red" slot="tip" class="el-upload__tip">提醒：选择的资源路径名推荐为英文！！！</div>
      <i class="el-icon-plus"></i>
        </el-upload>
   <el-dialog :visible.sync="dialogVisible">
      <!-- <img width="100%" :src="dialogImageUrl" alt=""> -->
    </el-dialog>
  </div>
</template>
<script>
  import {policy} from '@/api/oss'
 import Cookies from 'js-cookie'
  export default {
    name: 'singleUploaded',
    props: {
      value: Array
    },
    computed: {
        fileList() {
        let fileList=[];
        for(let i=0;i<this.value.length;i++){
          fileList.push({url:this.value[i]});
        }
        return fileList;
      }
    },
    data() {
      return {
        dataObj: {
          policy: '',
          signature: '',
          key: '',
          ossaccessKeyId: '',
          dir: '',
          host: '',
          // callback:'',
         dialogImageUrl:''
        },
        dialogVisible: false,
         url:'',
             token:{
        Authorization:Cookies.get("loginToken")
      },
      };
    },
    methods: { 
      emitInput(fileList) {
        let value=[];
        for(let i=0;i<fileList.length;i++){
          value.push(fileList[i].url);
        }
        this.$emit('input', value)
      },
        handleRemoves(file, fileList) {
        // console.log(file, fileList);
          this.emitInput(fileList);
      },
      handlePreviews(file,fileList) {
      //  fileList= this.value.productDetailsPictureList
        this.dialogVisible = true;
        this.dialogImageUrl = file.url;
        // console.log(file);
      },
      handleExceeds(files, fileList) {
        // console.log(fileList)
        this.$message.warning(`当前限制选择 5 个文件，本次选择了 ${files.length} 个文件，共选择了 ${files.length + fileList.length} 个文件`);
      },
      beforeRemoves(file, fileList) {
        // return this.$confirm(`确定移除 ${ file.name }？`);
      },
      handlePictureCardPreviewsed(response, file, fileList) {
        let strings = [];
          // console.log(response,'图片1')
          //  console.log(file,'图片2')
        for (let i = 0; i < fileList.length; i++) {
          // console.log(fileList.response,'图片')
        strings.push({url:response.data[0],})
        //  this.fileList.push(response.data[0]);
        }
         
              this.fileList.push(strings[0])
        this.emitInput(this.fileList);
      },
    }
  }
</script>
<style>

</style>


