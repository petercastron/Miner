/*
 * @Author: fjf fjf@123.com
 * @Date: 2023-03-27 09:47:11
 * @LastEditors: fjf fjf@123.com
 * @LastEditTime: 2023-03-28 15:18:31
 * @FilePath: \fileRun\src\views\home\cs.js
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
// Obtain file from user input or similar
// var file = ...

function startOrResumeUpload(upload) {
    // Check if there are any previous uploads to continue.
    upload.findPreviousUploads().then(function (previousUploads) {
        // Found previous uploads so we select the first one.
        if (previousUploads.length) {
            upload.resumeFromPreviousUpload(previousUploads[0])
        }
        // Start the upload
        upload.start()
    })
}

// Create the tus upload similar to the example from above
var upload = new tus.Upload(file, {
    endpoint: "http://localhost:1080/files/",
    onError: function(error) {
        console.log("Failed because: " + error)
    },
    onSuccess: function() {
        console.log("Download %s from %s", upload.file.name, upload.url)
    }
})

// Add listeners for the pause and unpause button
var pauseButton   = document.querySelector("#pauseButton")
var unpauseButton = document.querySelector("#unpauseButton")

pauseButton.addEventListener("click", function() {
    upload.abort()
})

unpauseButton.addEventListener("click", function() {
    startOrResumeUpload(upload)
})

// Start the upload by default
startOrResumeUpload(upload)