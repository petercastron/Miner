<template> 
  <div>
    <el-upload action="http://upload-z2.qiniup.com" ref="upload" :on-success="handleAvatarSuccess" :before-upload="beforeAvatarUpload"
      :auto-upload="true" :limit="1" :data="form" accept='.jpg,.png,.jpeg,.gif,.mp4,'>
      <el-button size="small" type="primary">点击上传</el-button>
      <div style="color:red" slot="tip" class="el-upload__tip">提醒：选择的资源路径名推荐为英文码？！！！</div>
    </el-upload>
    <!-- <el-dialog :visible.sync="dialogVisible">
      <img width="100%" :src="fileList[0].url" alt="">
    </el-dialog> -->
    <video id="upvideo" src=""></video>
   <canvas id="mycanvas" ></canvas>
  </div>
</template>
<script>
  import {
    policy,
    qiniuyun
  } from '@/api/oss'

  export default {
    name: 'singleUploads',
    props: {
      value: String
    },
    computed: {
      imageUrl() {
        return this.value;
      },
      imageName() {
        if (this.value != null && this.value !== '') {
          return this.value.substr(this.value.lastIndexOf("/") + 1);
        } else {
          return null;
        }
      },
      fileList() {
        return [{
          name: this.imageName,
          url: this.imageUrl,
          // url:'http://upload.qiniup.com',
        }]
      },
      showFileList: {
        get: function() {
          return this.value !== null && this.value !== '' && this.value !== undefined;
        },
        set: function(newValue) {}
      }
    },
    data() {
      return {
        form: {},
        dialogVisible: false,
        url: 'http://upload.qiniup.com',
        view: "http://qiniuoss.falianjishu.com/",
        token: null,
        imgsrc:null,
        sfzzm:[],
        ListFileUrlDetail:{}
      };
    },
    created() {
      this.qiniuyun()
    },
    methods: {
      emitInput(val) {
        this.$emit('input', val)
      },
      handleAvatarSuccess(response, file, fileList) {
        //获取的到的视频地址
        let viewurl = this.view + response.key
        // console.log(viewurl,'response')
        this.emitInput(viewurl);
        //截取图片
        if (file.status) {
          var index = file.name.indexOf("."); //（考虑严谨用lastIndexOf(".")得到）得到"."在第几位
          var tv_id = file.name.substring(index); //截断"."之前的，得到后缀
          this.videoUrl = file.url;
          //如果是视频截取第一个作为图片展示出来
          if (tv_id == ".mp4") {
            //根据后缀，判断是否符合视频格式
            this.findvideocover(this.view + response.key, file);
          }

          // this.sfzzm.push(file);
          // this.ListFileUrlDetail.Url = response.Dada[0];
          // this.ListFileUrlDetail.Size = file.size;
          // this.ListFileUrlDetail.uid = file.uid;

          // this.form.FileUrls.push(Object.assign({}, this.ListFileUrlDetail));
        }
        console.log("sfzzm" + JSON.stringify(this.sfzzm));
      },
      findvideocover(url, file) {
        const video = document.getElementById("upvideo"); // 获取视频对象
        // const video = document.createElement("video") // 也可以自己创建video
        video.src = url // url地址 url跟 视频流是一样的
        var canvas = document.getElementById('mycanvas') // 获取 canvas 对象
        const ctx = canvas.getContext('2d'); // 绘制2d
        video.crossOrigin = 'anonymous' // 解决跨域问题，也就是提示污染资源无法转换视频
        video.currentTime = 1 // 第一帧
        video.oncanplay = () => {
          canvas.width = video.clientWidth; // 获取视频宽度
          canvas.height = video.clientHeight; //获取视频高度
          // console.log(canvas.height,canvas.width,'this.imgsrc')
          // 利用canvas对象方法绘图
          ctx.drawImage(video, 0, 0, video.clientWidth, video.clientHeight)
           // console.log(ctx.drawImage(video, 0, 0, video.clientWidth, video.clientHeight),'this.imgsrc')
          // 转换成base64形式
          this.imgsrc = canvas.toDataURL("image/png") // 截取后的视频封面

          this.$emit('imgsrc', this.imgsrc )
          file.url = this.imgsrc;
          console.log(file,'file')
        }
      },
      handlePreview(file) {
        this.dialogVisible = true;
      },
      beforeAvatarUpload(file) {
        // let _self = this;
        // return new Promise((resolve, reject) => {
        //   policy().then(response => {
        //     _self.dataObj.policy = response.data.policy;
        //     _self.dataObj.signature = response.data.signature;
        //     _self.dataObj.ossaccessKeyId = response.data.accessKeyId;
        //     _self.dataObj.key = response.data.dir + '/${filename}';
        //     _self.dataObj.dir = response.data.dir;
        //     _self.dataObj.host = response.data.host;
        //      _self.url=response.data.host;
        //     // _self.dataObj.callback = response.data.callback;
        //       console.log(_self.dataObj.host)
        //     resolve(true)
        //   }).catch(err => {
        //     console.log(err)
        //     reject(false)
        //   })
        // })
        const fileType = file.type
        const current = new Date().getTime()
        const key = `video_${current}` // key为上传后文件名 必填
        const isLt20M = file.size / 1024 / 1024 < 200 // 算出文件大小
        this.fileSize = file.size // 存储文件大小
        if (!isLt20M) { // 这里我们限制文件大小为20M
          this.$message.error('最大只能上传200M!')
          this.$ref.upload.abort()
          return isLt20M
        }
        // if (fileType !== 'video/mp4') { // 限制文件类型
        //   this.$ref.upload.abort()
        //   this.$message.error('只能上传MP4格式视频!')
        //   return false
        // }
        try {
          const token = this.token
          // console.log(this.token,'token')
          // this.form = {
          //     key,
          //     token,
          // }
          this.form.key = key
          this.form.token = token
          return true
        } catch (error) {
          return error
        }

      },
      qiniuyun() {
        qiniuyun().then(response => {
          this.token = response.data.token
        })
      },
    }
  }
</script>
<style>

</style>
