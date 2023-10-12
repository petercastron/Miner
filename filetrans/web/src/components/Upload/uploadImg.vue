<template>
    <el-upload
        :class="'w-' + size + ' h-' + size"
        class="upload-card hover:border-blue-300 hover:shadow"
        action=""
        name="file"
        accept="images/*"
        :show-file-list="false"
        :before-upload="beforeUpload"
        :http-request="onUpload"
        :on-progress="onProgress"
        :on-success="onSuccess"
        :on-error="onError"
    >
        <el-progress type="circle" :percentage="percent" v-if="uploading"></el-progress>
        <el-image fit="contain" :src="img ? oss + img : image" v-else-if="image || img"></el-image>
        <i :class="'w-' + size + ' h-' + size" class="upload-plus el-icon-plus" v-else></i>
    </el-upload>
</template>

<script>
export default {
    name: "ImageUpload",
    data() {
        return {
            oss: process.env.MIX_ALIYUN_OSS_URL,
            uploading: false,
            percent: 0,
            object: null,
            image: null,
        };
    },
    props: {
        size: {
            type: Number,
            default: 32
        },
        path: {
            type: String,
            default: 'image'
        },
        img: {
            type: String,
            default: null,
        }
    },
    methods: {
        beforeUpload(file) {
            const isIMG = file.type.substr(0, 6) === "image/";
            const isLt4M = file.size / 1024 / 1024 < 4;
            if (!isIMG) {
                this.$message.error("上传只能是图片格式!");
            }
            if (!isLt4M) {
                this.$message.error("上传大小不能超过 4MB!");
            }
            this.uploading = true;
            return isIMG && isLt4M;
        },
        async onUpload(option) {
            let file = option.file;
            try {
                let object = this.generateObject(file.name);
                await ossClient.multipartUpload(object, file, {
                    progress: (p, checkpoint) => {
                        option.file.percent = Math.floor(p * 100);
                        option.onProgress(option.file);
                    }
                });
                option.file.response = { object: object };
                return option.file.response;
            } catch (err) {
                option.file.err = err;
                return option.file.err;
            }
        },
        generateObject(fileName) {
            let i = fileName.lastIndexOf(".");
            let suffix = fileName.substr(i + 1);
            let name = _.generateKey(16);
            return `/${this.path}/${name}.${suffix}`;
        },
        onProgress(event, file, fileList) {
            this.percent = event.percent;
        },
        onSuccess(response, file, fileList) {
            this.object = response.object;
            this.image = URL.createObjectURL(file.raw);
            this.uploading = false;
            this.$message.success(`${file.name} 上传成功`);
            this.$emit("uploadSuccess", this.object);
        },
        onError(err, file, fileList) {
            this.image = null;
            this.object = null;
            this.uploading = false;
            this.$message.error(`${file.name} 上传失败`);
            this.$emit("uploadError");
        }
    }
};
</script>

<style lang="scss">
// .upload-card {
//     @apply flex items-center justify-center overflow-hidden rounded-lg bg-white cursor-pointer border border-dashed border-gray-300;
// }
// .upload-plus {
//     @apply flex items-center justify-center text-4xl text-gray-500;
// }
// .upload-card .el-upload {
//     display: flex !important;
//     align-items: center !important;
//     justify-content: center !important;
// }
</style>